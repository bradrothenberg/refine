"""Example of transforming and scaling an `.implicit`"""

from ctypes import (
    CDLL,
    pointer,
    POINTER,
    c_char_p,
    c_double,
    c_int32,
    c_uint32,
    c_void_p,
    Structure
)
import os
from pathlib import Path
from typing import Any
from enum import Enum

########################
# nTop Core Primitives #
########################
# region nTop Core Primitives

class Vec3(Structure):
    """X, Y, Z components representing a point or vector."""
    _fields_ = [("x", c_double), ("y", c_double), ("z", c_double)]

class Frame(Structure):
    """
    A point and two orthogonal axes defining a frame of reference.
    """
    _fields_ = [("origin", Vec3), ("x_axis", Vec3), ("y_axis", Vec3)]

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
# For simplicity here we are chosing, for the moment, to append to the PATH environment
if str(path_to_lib.parent) not in os.environ["PATH"]:
    os.environ["PATH"] += os.pathsep + str(path_to_lib.parent)


class ImplicitLoadMessage(Enum):
    """Enumertaion of the load error messages and their respective codes."""
    # Codes as defined in ntop_core.h
    SUCCESS = 0
    UNKNOWN_ERROR = 1
    FILE_DOESNT_EXIST = 2
    FILE_OPEN_ERROR = 3 
    CORRUPT_FILE = 4
    UNSUPPORTED_VERSION = 5
    OUT_OF_MEMORY = 6

# Define the data type marshalling between Python and nTop Core
ntop_core.ntop_core_import_from_file.restype = c_uint32
ntop_core.ntop_core_import_from_file.argtypes = [
    c_char_p,
    POINTER(c_void_p)
]

ntop_core.ntop_core_library_version.argtypes = [
    POINTER(c_int32),
    POINTER(c_int32),
    POINTER(c_int32)
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
        if result == 5:
            major = c_int32(); minor = c_int32(); patch = c_int32()
            ntop_core.ntop_core_library_version(pointer(major), pointer(minor), pointer(patch))  
            version = f"{major.value}.{minor.value}.{patch.value}"
            raise RuntimeError(f"Implicit file is unsupported by nTop Core version {version}")
        raise RuntimeError(f"Cannot load implicit: {ImplicitLoadMessage(result).name}.")
    return True


# Setup error check when loading the file.
ntop_core.ntop_core_import_from_file.errcheck = callback_import_from_file_error_check


# Define the data type marshalling between Python and nTop Core
ntop_core.ntop_core_export_to_file.restype = c_uint32
ntop_core.ntop_core_export_to_file.argtypes = [
    c_char_p,
    c_void_p
]

def callback_export_to_file_error_check(result: c_uint32, func, arguments: Any) -> bool:
    """
    Callback function which happens after the function is called but before 
    control is returned to the caller enabling error handling.

    :param result: Return value of `ntop_core_export_to_file`
    :param arguments: Tuple of parameters passed to the invocation
        of `ntop_core_export_to_file`
    :returns: True if saving the implicit was successful.
    :raises: RuntimeError if any error state is reported by nTop Core.
    """
    # Check to see if the implicit loaded correctly
    if result != 0:
        raise RuntimeError(f"Cannot save implicit: {arguments[0]}.")

    return True

# Setup error check when saving the file.
ntop_core.ntop_core_export_to_file.errcheck = callback_export_to_file_error_check

# Define data marshalling specification for releasing the memory used for the `.implicit`
ntop_core.ntop_core_release.restype = None
ntop_core.ntop_core_release.argtypes = [c_void_p]

# Define data marshalling specification for getting the implicit field value at a point 
ntop_core.ntop_core_query_field.restype = c_double
ntop_core.ntop_core_query_field.argtypes = [
    c_void_p,
    POINTER(Vec3)
]

# Define data marshalling specification for scaling an implicit body
ntop_core.ntop_core_scaled.restype = None
ntop_core.ntop_core_scaled.argtypes = [
    c_void_p,
     POINTER(Vec3),
     POINTER(Vec3),
     POINTER(c_void_p),
]

# Define data marshalling specification for transforming an implicit body
ntop_core.ntop_core_transformed.restype = None
ntop_core.ntop_core_transformed.argtypes = [
    c_void_p,
     POINTER(Frame),
     POINTER(c_void_p),
]

# endregion Library wrapping setup 

################################################
################################################
# Functional example of transforming and scaling with nTop Core #
################################################
#################################################

# region Example transforming and scaling code with nTop Core

# Load the implicit
cone_implicit_file_path = Path(__file__).parent.parent / "assets" / "cone_5mm_radius_10mm_apex.implicit"
cone_handle = c_void_p()
ntop_core.ntop_core_import_from_file(str(cone_implicit_file_path).encode("utf8"), cone_handle)
print ("Loaded implicit cone from " + str(cone_implicit_file_path))

# Scale the cone uniformly relative to its apex
scaled_handle = c_void_p()
scale = Vec3(2, 2, 2) # Uniform scale
fixed_pt = Vec3(0, 0, 0.010) # Apex of the cone (all units in meters)
ntop_core.ntop_core_scaled(cone_handle, pointer(scale), pointer(fixed_pt), pointer(scaled_handle))

# Transform the body by translating origin to (10mm, 0, 0) and rotating to make the Y axis "up".
transformed_handle = c_void_p()
transform = Frame(Vec3(-0.010, 0, 0), Vec3(1, 0, 0), Vec3(0, 0, 1))
ntop_core.ntop_core_transformed(scaled_handle, pointer(transform), pointer(transformed_handle))

# Make a folder to capture the output
output_folder = Path(__file__).parent / "output"
output_folder.mkdir(exist_ok=True)

# Sample x,y,z points of interest
# We sample the apex, the original cone base, and the rescaled cone base
sample_points = [Vec3(0, 0, 0),          # origin
                 Vec3(0, 0, 0.010),      # apex - invariant to this scaling because it is the fixed point
                 Vec3(0.005, 0, 0),      # original base surface points
                 Vec3(-0.005, 0, 0), 
                 Vec3(0, 0.005, 0), 
                 Vec3(0, -0.005, 0), 
                 Vec3(0.010, 0, -0.010), # scaled base surface point
                 Vec3(-0.010, 0, -0.010),
                 Vec3(0, 0.010, -0.010),
                 Vec3(0, -0.010, -0.010)]

# Sample x,y,z points of interest
# We sample the apex, the original cone base, and the rescaled cone base
transformed_points = [Vec3(0.010, 0, 0),      # transformed origin 
                      Vec3(0.010, 0.010, 0),  # transformed apex
                      Vec3(0.005, 0, 0),      # transformed original base surface points
                      Vec3(0.015, 0, 0), 
                      Vec3(0.010, 0, -0.005), 
                      Vec3(0.010, 0, -0.005), 
                      Vec3(0, -0.010, 0),     # transformed scaled base surface points
                      Vec3(0.020, -0.010, 0),
                      Vec3(0.010, -0.010, 0.010),
                      Vec3(0.010, -0.010, -0.010)]

print ("Original values:")
for sample_point in sample_points:
    value = ntop_core.ntop_core_query_field(cone_handle, sample_point)
    print (f"Value at ({sample_point.x}, {sample_point.y}, {sample_point.z}) = {value:.6f}")

print ("Scaled values:")
for sample_point in sample_points:
    value = ntop_core.ntop_core_query_field(scaled_handle, sample_point)
    print (f"Value at ({sample_point.x}, {sample_point.y}, {sample_point.z}) = {value:.6f}")

# Save the scaled implicit
scaled_output_path = output_folder / "scaled_cone.implicit"
ntop_core.ntop_core_export_to_file(str(scaled_output_path).encode("utf8"), scaled_handle)
print ("Saved scaled cone to " + str(scaled_output_path))

print ("Scaled and transformed values:")
for transform_point in transformed_points:
    value = ntop_core.ntop_core_query_field(transformed_handle, transform_point)
    print (f"Value at ({transform_point.x}, {transform_point.y}, {transform_point.z}) = {value:.6f}")

# Save the transformed implicit
transformed_output_path = output_folder / "transformed_cone.implicit"
ntop_core.ntop_core_export_to_file(str(transformed_output_path).encode("utf8"), transformed_handle)
print ("Saved scaled and transformed cone to " + str(transformed_output_path))

# Unload the implicit bodies
ntop_core.ntop_core_release(cone_handle)
ntop_core.ntop_core_release(scaled_handle)
ntop_core.ntop_core_release(transformed_handle)


