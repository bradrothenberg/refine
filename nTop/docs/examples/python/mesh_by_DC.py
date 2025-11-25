"""Example using the nTop Core meshing by DC call."""
# region Imports
from ctypes import (
    CDLL,
    CFUNCTYPE,
    pointer,
    POINTER,
    c_bool,
    c_char_p,
    c_double,
    c_uint32,
    c_void_p,
    Structure
)
import os
from pathlib import Path
from typing import Any

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import numpy as np
import numpy.typing as npt
from skimage import measure
import datetime
# endregion Imports

#########
#########
# Setup #
#########
#########
# region Preamble Setup

########################
# nTop Core Primitives #
########################
# region nTop Core Primitives

class Vec3(Structure):
    """X, Y, Z components representing a point or vector."""
    _fields_ = [("x", c_double), ("y", c_double), ("z", c_double)]

class BoundingBox(Structure):
    """
    Minimal, axis-aligned bounding box of an Implicit.

    The representation is comprised of two points (min_x, min_y, min_z) and (max_x, max_y, max_z).
    """
    _fields_ = [("min", Vec3), ("max", Vec3)]

# endregion

###########################################################
# Setup library functions and data marshaling definitions #
###########################################################
# region Library wrapping setup 

# Load the nTop Core library
NTOP_CORE_LIB_ENVVAR = "NTOP_CORE_LIB"
if NTOP_CORE_LIB_ENVVAR in os.environ:
    env_lib_path = Path(os.environ[NTOP_CORE_LIB_ENVVAR])
    if env_lib_path.exists() and env_lib_path.is_file():
        path_to_lib = env_lib_path
    else:
        raise FileNotFoundError(env_lib_path)

ntop_core = CDLL(str(path_to_lib))
# In order to successfully load the nTop Core Library the dependent libraries of
# the `ntop_core` library must be on the PATH or the current working directory must contain
# those libraries not found on the PATH.
# For simplicity here we are choosing, for the moment, to append to the PATH environment
if str(path_to_lib.parent) not in os.environ["PATH"]:
    os.environ["PATH"] += os.pathsep + str(path_to_lib.parent)

# Define data marshalling specification for loading implicit
ntop_core.ntop_core_import_from_file.restype = c_uint32
ntop_core.ntop_core_import_from_file.argtypes = [
    c_char_p,
    POINTER(c_void_p)
]

def callback_import_from_file_error_check(result: c_uint32, func, arguments: Any) -> bool:
    """
    Callback function which happens after the function is called but before 
    control is returned to the caller enabling error handling.

    :param result: Return value of `ntop_core_import_from_file`
    :param arguments: Tuple of parameters passed to the invocation
        of `ntop_core_import_from_file`
    :returns: True if loading the implicit was successful.
    :raises: RuntimeError if any error state is reported by nTop Core.
    """
    # Check to see if the implicit loaded correctly
    if result != 0:
        raise RuntimeError(f"Cannot load implicit: {arguments[0]}.")

    return True

# Setup error check when loading the file.
ntop_core.ntop_core_import_from_file.errcheck = callback_import_from_file_error_check

# Define data marshalling specification for releasing the memory used for the `.implicit`
ntop_core.ntop_core_release.restype = None
ntop_core.ntop_core_release.argtypes = [c_void_p]

# Define data marshalling specification for getting the bounding box of an implicit
ntop_core.ntop_core_query_bounding_box.restype = None
ntop_core.ntop_core_query_bounding_box.argtypes = [
    c_void_p,
    POINTER(BoundingBox)
]

# Define the signatures for the callbacks in the `ntop_core_generate_mesh_by_DC``
CFUNCTYPE_MESH_VERTEX_CALLBACK = CFUNCTYPE(
    None,
    c_void_p,
    POINTER(Vec3),
    c_uint32
)

CFUNCTYPE_MESH_TRIANGLE_CALLBACK = CFUNCTYPE(
    None,
    c_void_p,
    c_uint32,
    c_uint32,
    c_uint32
)

CFUNCTYPE_MESH_CONTINUATION_CALLBACK = CFUNCTYPE(c_bool, c_void_p, c_uint32)

ntop_core.ntop_core_generate_mesh_by_DC.restype = None
ntop_core.ntop_core_generate_mesh_by_DC.argtypes = [c_void_p, c_double, c_double, c_void_p, c_void_p, c_void_p, CFUNCTYPE_MESH_VERTEX_CALLBACK, CFUNCTYPE_MESH_TRIANGLE_CALLBACK,  CFUNCTYPE_MESH_CONTINUATION_CALLBACK]

# Example callbacks that can be used with `ntop_core_generate_mesh_by_DC` invocations.
def mesh_vertex_callback(scope_pass) -> CFUNCTYPE_MESH_VERTEX_CALLBACK:
    """Callback for the ntop_core_generate_mesh_by_DC to smuggle out the vertex data."""

    def gather_vertex(context: c_void_p, pt:POINTER(Vec3), index:c_uint32):
        scope_pass.append([pt.contents.x, pt.contents.y, pt.contents.z])

    return CFUNCTYPE_MESH_VERTEX_CALLBACK(gather_vertex)

def mesh_triangle_callback(scope_pass) -> CFUNCTYPE_MESH_TRIANGLE_CALLBACK:
    """Callback for the ntop_core_generate_mesh_by_DC to smuggle out the triangle index data."""

    def gather_triangle(context: c_void_p, a:c_uint32, b:c_uint32, c:c_uint32):
        scope_pass.append([a,b,c])

    return CFUNCTYPE_MESH_TRIANGLE_CALLBACK(gather_triangle)

class mesh_continuation:
    completed: bool
    timeout: int
    reporting_frequency: int

def mesh_continuation_callback(scope_pass) -> CFUNCTYPE_MESH_CONTINUATION_CALLBACK:
    """Callback for the ntop_core_generate_mesh_by_DC to report progress and optionally cancel."""
    global last_report
    now = datetime.datetime.now()
    stop_after = now + datetime.timedelta(seconds = scope_pass.timeout)
    last_report = now
    
    def proceed(context: c_void_p, percent: c_uint32):
        global last_report
        now =  datetime.datetime.now() 
        scope_pass.completed = now < stop_after
        if (now > last_report + datetime.timedelta(seconds = scope_pass.reporting_frequency)):
            print(f"Meshing {percent}% complete.")
            last_report = datetime.datetime.now()
        return scope_pass.completed

    return CFUNCTYPE_MESH_CONTINUATION_CALLBACK(proceed)

# endregion Library wrapping setup 

# endregion Preamble Setup

######################
######################
# Utility Functions #
######################
######################
# region Utilities

###################
# STL File Writer #
###################
# region STL writer
def write_stl(triangles: npt.ArrayLike, filename: Path) -> None:
    """
    Write a mesh stored as triangles in STL format.
    
    Note: degenerate faces where vertices are colinear are filtered out.

    :param triangles: numpy Array of dimensions Nx3x3 (N is the number of triangles in
                    the mesh) with each vertex in 3D space. 
    :param filename: File name of the STL to write.
    """
    with open(filename, "w") as f:
        f.write("solid object\n")
        for triangle in triangles:
            normal = np.cross(triangle[1] - triangle[0], triangle[2] - triangle[0])
            #if (np.sum(normal ** 2) < 0): 
            #    print(normal)
            #    print(triangle)
            normal_length = np.sqrt(np.sum(normal ** 2))
            if (normal_length == 0):
                # Skip degenerate faces
                continue

            unit_normal = normal / normal_length

            f.write(f"  facet normal {unit_normal[0]:.6f}  {unit_normal[1]:.6f}  {unit_normal[2]:.6f}\n")
            f.write(f"    outer loop\n")
            for point in triangle:
                f.write(f"      vertex {point[0]:.6f}  {point[1]:.6f}  {point[2]:.6f}\n")
            f.write(f"    endloop\n")
            f.write(f"  endfacet\n")
        f.write("endsolid object\n")

# endregion STL writer

# endregion Utilities

################################################
################################################
# Functional example of meshing with nTop Core #
################################################
#################################################
# region Example meshing code with nTop Core

# Load the implicit
gyroid_implicit_file_path = Path(__file__).parent.parent / "assets" / "heat-sink.implicit"
print(f"Reading {gyroid_implicit_file_path}.")
gyroid_handle = c_void_p()
ntop_core.ntop_core_import_from_file(str(gyroid_implicit_file_path).encode("utf8"), gyroid_handle)


# Setup meshing parameters and callbacks
#
# Lists to store meshing callback values dynamically
vertices = []
triangle_indices = []
# Granularity, in meters, at which the body's field is sampled. 
# Make this smaller to get a finer mesh.
feature_size = 0.00025 
# Controls deimation of the mesh. 
# Make this smaller to get a finer mesh. 0 for no deimation.
adaptivity = 1
continuation = mesh_continuation()
continuation.completed = True        # Set false by callback if computation was cancelled
continuation.timeout = 10            # Seconds until meshing computation is cancelled
continuation.reporting_frequency = 1 # Minimum seconds between progress reports

# Mesh the implicit body
ntop_core.ntop_core_generate_mesh_by_DC(gyroid_handle, 
                                        feature_size, 
                                        adaptivity, 
                                        0, 
                                        0, 
                                        0, 
                                        mesh_vertex_callback(vertices), 
                                        mesh_triangle_callback(triangle_indices), 
                                        mesh_continuation_callback(continuation))

if (not continuation.completed):
    print("Meshing took too long.  Adjust feature_size, timeout, or model to allow meshing to complete.")
    exit(1)
else: 
    print("Meshing completed.")
    print("Gathering vertices and faces.")

# Copy values from lists to numpy arrays.
np_vertices = np.asarray(vertices)
np_triangle_indices = np.asarray(triangle_indices) 

# Use fancy indexing to make the triangles
triangles = np_vertices[np_triangle_indices]

# Get the bounding box to know the extent of the points to render
bounding_box = BoundingBox()
ntop_core.ntop_core_query_bounding_box(gyroid_handle, pointer(bounding_box))

# Free the memory associated with the implicit as no more calls are needed
ntop_core.ntop_core_release(gyroid_handle)

# Make a folder to capture the output
output_folder = Path(__file__).parent / "output"
output_folder.mkdir(exist_ok=True)

# Write the mesh as an STL
generated_mesh_filepath = output_folder / "mesh_by_DC.stl"
print(f"Writing stl to {generated_mesh_filepath}")
write_stl(triangles, generated_mesh_filepath)

# Display resulting mesh using Matplotlib.
print("Generating Matplotlib display.")
fig = plt.figure(figsize=(25, 25))
ax = fig.add_subplot(111, projection='3d')

# Add the mesh visualization
# Note: Matplotlib is not very performant for this rendering task and should
# primarily be used for proof-of-concept using other mesh visualization tools
# for deeper analysis.
mesh = Poly3DCollection(triangles)
mesh.set_edgecolor('k')
ax.add_collection3d(mesh)

# Label the plot
ax.set_title("Mesh visualization of implicit")
ax.set_xlabel("x-axis (meters)")
ax.set_ylabel("y-axis (meters)")
ax.set_zlabel("z-axis (meters)")

# Set reasonable limits to scale the part appropriately for viewing
ax.set_xlim(bounding_box.min.x, bounding_box.max.x)
ax.set_ylim(bounding_box.min.y, bounding_box.max.y)
ax.set_zlim(bounding_box.min.z, bounding_box.max.z)

# Render the mesh and save to a PNG
print("Rendering as png.")
mesh_render_filepath = output_folder / "mesh_by_DC.png"
plt.savefig(mesh_render_filepath)
print(f"Render of mesh saved to {mesh_render_filepath}")

# Uncomment the line below to render the mesh in an interactive window
#plt.show()

# endregion Example meshing code with nTop Core
