"""A simple implementation of the "Marching Cubes" using scikit-image."""
# region Imports
from ctypes import (
    CDLL,
    CFUNCTYPE,
    pointer,
    POINTER,
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

# Define the signature for the callback in the `ntop_core_query_field_array``
CFUNCTYPE_QUERY_FIELD_ARRAY_CALLBACK = CFUNCTYPE(
    None,
    c_void_p,
    POINTER(c_double),
    c_uint32
)

# An example callback that can be used with `ntop_core_query_field_array` invocations.
def query_field_array_callback(scope_pass) -> CFUNCTYPE_QUERY_FIELD_ARRAY_CALLBACK:
    """Callback for the ntop_core_query_field_array to smuggle out the query data."""

    def _smuggle(context: c_void_p, query_vals: POINTER(c_double), count: int) -> None:
        temp = np.ctypeslib.as_array(query_vals, shape = (count, 1)).flatten()
        np.copyto(scope_pass, temp)

    return  CFUNCTYPE_QUERY_FIELD_ARRAY_CALLBACK(_smuggle)

ntop_core.ntop_core_query_field_array.restype = None
ntop_core.ntop_core_query_field_array.argtypes = [
    c_void_p,
    np.ctypeslib.ndpointer(dtype=np.double, ndim=2, flags='CONTIGUOUS'),
    c_uint32,
    c_void_p,
    CFUNCTYPE_QUERY_FIELD_ARRAY_CALLBACK
]
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
gyroid_handle = c_void_p()
ntop_core.ntop_core_import_from_file(str(gyroid_implicit_file_path).encode("utf8"), gyroid_handle)

# Get the bounding box to know the extent of the points to mesh
bounding_box = BoundingBox()
ntop_core.ntop_core_query_bounding_box(gyroid_handle, pointer(bounding_box))

# Offset the bounding box slightly to ensure that we capture points beyond the isosurface
# so the meshing algorithm can close the part.
bounding_box_offset = 0.001

# Set the minimum feature size you want to capture in the part. The values
# selected here will have an O(n^3) impact on mesh file size amd generation
# time as it directly effects the number of sampled points.
# In this example it is isotropic but if dimensional variability is important
# that can be modified here.
min_feature_size = (0.00025, 0.00025, 0.00025)

# Setup the sampling grid
sample_points_numpy_form = np.mgrid[
    bounding_box.min.x - bounding_box_offset:bounding_box.max.x + bounding_box_offset:min_feature_size[0],
    bounding_box.min.y - bounding_box_offset:bounding_box.max.y + bounding_box_offset:min_feature_size[1],
    bounding_box.min.z - bounding_box_offset:bounding_box.max.z + bounding_box_offset:min_feature_size[2],
]

# Turn this into an array of x,y,z points as expected by nTop Core
sample_points_ntop_core_form = np.ascontiguousarray(sample_points_numpy_form.reshape(3, -1, order='C').transpose(), dtype=np.double)

# Setup the variable to capture the output from nTop Core
field_values = np.zeros(sample_points_ntop_core_form.shape[0])

# Use nTop Core's batch query functionality to query the field at all the
# points simultaneously.
# Note: for sufficiently large parts or very fine features you may need to
# chunk these queries depending on your system configuration.
ntop_core.ntop_core_query_field_array(
    gyroid_handle,
    sample_points_ntop_core_form,
    sample_points_ntop_core_form.shape[0],
    None,
    query_field_array_callback(field_values)
)

# Free the memory associated with the implicit as no more calls are needed
ntop_core.ntop_core_release(gyroid_handle)

# Arrange the data back to the original shape as that is the form the meshing
# algorithm expects: a top-down Z-axis slice view of field values.
volume_data = field_values.reshape(sample_points_numpy_form.shape[1:])

# Use skimage to mesh the data
vertices, faces, normals, values = measure.marching_cubes(
    volume_data,
    0,
    allow_degenerate=False,
    spacing=min_feature_size
)

# The implementation of `measure.marching_cubes()` translates samples to its
# own frame in octant 1 so we translate to the original position of the implicit
# for consistency.
vertices += np.array([bounding_box.min.x, bounding_box.min.y, bounding_box.min.z])
vertices -= bounding_box_offset

# Fancy indexing: `vertices[faces]` to generate a collection of triangles.
# This takes the form of an array containing triplets of 3D points.
triangles = vertices[faces]

# Make a folder to capture the output
output_folder = Path(__file__).parent / "output"
output_folder.mkdir(exist_ok=True)

# Write the mesh as an STL
generated_mesh_filepath = output_folder / "mesh.stl"
write_stl(triangles, generated_mesh_filepath)

# Display resulting mesh using Matplotlib.
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
mesh_render_filepath = output_folder / "mesh.png"
plt.savefig(mesh_render_filepath)
print(f"Render of mesh saved to {mesh_render_filepath}")

# Uncomment the line below to render the mesh in an interactive window
# plt.show()

# endregion Example meshing code with nTop Core
