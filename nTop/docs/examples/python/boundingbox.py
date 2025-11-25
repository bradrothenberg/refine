"""Example of getting the bounding box of an `.implicit`"""

from ctypes import (
    CDLL,
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

# Load the nTop Core library
NTOP_CORE_LIB_ENVVAR = "NTOP_CORE_LIB"
if NTOP_CORE_LIB_ENVVAR in os.environ:
    env_lib_path = Path(os.environ[NTOP_CORE_LIB_ENVVAR])
    if env_lib_path.exists() and env_lib_path.is_file():
        path_to_lib = env_lib_path
    else:
        raise FileNotFoundError(env_lib_path)

lib = CDLL(str(path_to_lib))
# In order to successfully load the nTop Core Library the dependent libraries of
# the `ntop_core` library must be on the PATH or the current working directory must contain
# those libraries not found on the PATH.
# For simplicity here we are chosing, for the moment, to append to the PATH environment
if str(path_to_lib.parent) not in os.environ["PATH"]:
    os.environ["PATH"] += os.pathsep + str(path_to_lib.parent)

# Setup the data structures and function signatures needed to communicate with the library
class Vec3(Structure):
    """X, Y, Z components representing a point or vector."""
    _fields_ = [("x", c_double), ("y", c_double), ("z", c_double)]

class BoundingBox(Structure):
    """
    Minimal, axis-aligned bounding box of an Implicit.

    The representation is comprised of two points (min_x, min_y, min_z) and (max_x, max_y, max_z).
    """
    _fields_ = [("min", Vec3), ("max", Vec3)]


# Define the data type marshalling between Python and nTop Core

# Load the `.implicit` into the library
lib.ntop_core_import_from_file.restype = c_uint32
lib.ntop_core_import_from_file.argtypes = [
    c_char_p,
    POINTER(c_void_p)
]
def callback_import_from_file_error_check(result: c_uint32, func, arguments: Any) -> bool:
        # Check to see if the 
        if result != 0:
            raise RuntimeError(f"Cannot load implicit: {arguments[1]}.")

        return True
lib.ntop_core_import_from_file.errcheck = callback_import_from_file_error_check

# Release the memory used for the `.implicit`
lib.ntop_core_release.restype = None
lib.ntop_core_release.argtypes = [c_void_p]

# Bounding Box call
lib.ntop_core_query_bounding_box.restype = None
lib.ntop_core_query_bounding_box.argtypes = [
    c_void_p,
    POINTER(BoundingBox)
]

# Load the implicit
gyroid_implicit_file_path = Path(__file__).parent.parent / "assets" / "gyroid_sphere_5mm_radius.implicit"
gyroid_handle = c_void_p()
lib.ntop_core_import_from_file(str(gyroid_implicit_file_path).encode("utf8"), gyroid_handle)

# Get the bounding box of the implicit body
bounding_box = BoundingBox()
lib.ntop_core_query_bounding_box(gyroid_handle, pointer(bounding_box))

# Unload the implicit
lib.ntop_core_release(gyroid_handle)

# Print some information about the bounding box
print(f"Minimum: ({bounding_box.min.x}, {bounding_box.min.y}, {bounding_box.min.z})")
print(f"Maximum: ({bounding_box.max.x}, {bounding_box.max.y}, {bounding_box.max.z})")

print("Corners:")
print("   G-----H  Z ^   ^ Y")
print("  /|    /|    |  /")
print(" / |   / |    | /")
print("E-----F  |    |/")
print("|  C--|--D    *------> X")
print("| /   | /")
print("|/    |/")
print("A-----B")
print(f"\tA: ({bounding_box.min.x}, {bounding_box.min.y}, {bounding_box.min.z})")
print(f"\tB: ({bounding_box.max.x}, {bounding_box.min.y}, {bounding_box.min.z})")
print(f"\tC: ({bounding_box.min.x}, {bounding_box.max.y}, {bounding_box.min.z})")
print(f"\tD: ({bounding_box.max.x}, {bounding_box.max.y}, {bounding_box.min.z})")
print(f"\tE: ({bounding_box.min.x}, {bounding_box.min.y}, {bounding_box.max.z})")
print(f"\tF: ({bounding_box.max.x}, {bounding_box.min.y}, {bounding_box.max.z})")
print(f"\tG: ({bounding_box.min.x}, {bounding_box.max.y}, {bounding_box.max.z})")
print(f"\tH: ({bounding_box.max.x}, {bounding_box.max.y}, {bounding_box.max.x})")

