"""Example of slicing an `.implicit`"""

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
from typing import Any, List, Tuple

import matplotlib.pyplot as plt
from matplotlib.figure import Figure
import numpy as np


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
class Vec2(Structure):
    """X, Y components representing a point or or vector on a known plane of reference."""
    _fields_ = [("x", c_double), ("y", c_double)]

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

# Declare the callback made by the contour call
CFUNCTYPE_CONTOUR_CALLBACK = CFUNCTYPE(None, c_void_p, POINTER(Vec2), c_uint32, c_bool)
lib.ntop_core_query_contours.restype = None
lib.ntop_core_query_contours.argtypes = [
    c_void_p,
    POINTER(Frame),
    c_double,
    c_void_p,
    CFUNCTYPE_CONTOUR_CALLBACK
]
def callback_query_contours(scope_pass: List[List[Tuple[float, float]]]) -> CFUNCTYPE_CONTOUR_CALLBACK:
    """
    Callback for the ntop_core_query_contours to smuggle out the slice data.
    
    `scope_pass` is populated with the slice data.
    """    
    
    def _smuggle(context: c_void_p, polyline: List[Vec2], count: int, is_closed: bool) -> None:
        curve = []

        for i in range(count):
            curve.append((polyline[i].x, polyline[i].y))

        scope_pass.append(curve)

    return CFUNCTYPE_CONTOUR_CALLBACK(_smuggle)


def plot_slice(slice: List[List[Tuple[float, float]]], bounding_box: BoundingBox) -> Figure:
    """Plots a single slice."""

    # Make the figures consistent in size across slices
    fig = plt.figure(figsize=[
        1000 * (bounding_box.max.x - bounding_box.min.x),
        1000 * (bounding_box.max.y - bounding_box.min.y)
    ])
    
    cur_fig = fig.add_subplot(1, 1, 1)
    # Iterate over the curves in the slice
    for curve in slice:
        # iterate over the curves in the contour
        x_vals = [pt[0] for pt in curve]
        y_vals = [pt[1] for pt in curve]

        # Close the curve
        x_vals.append(curve[0][0])
        y_vals.append(curve[0][1])
        
        cur_fig.plot(
            x_vals,
            y_vals,
            scalex=0.001,
            scaley=0.001
        )

    # Annotate the figure
    cur_fig.set_title(f"Z-Offset: {int(round(z_offset, 3)*1000)} mm")
    cur_fig.set_xlabel("meters")
    cur_fig.set_ylabel("meters")
    x_ticks = np.arange(round(bounding_box.min.x, 3), round(bounding_box.max.x, 3) + 0.001, 0.001)
    y_ticks = np.arange(round(bounding_box.min.y, 3), round(bounding_box.max.y, 3) + 0.001, 0.001)
    cur_fig.set_xticks(x_ticks)
    cur_fig.set_yticks(y_ticks)
    cur_fig.set_xbound(bounding_box.min.x - 0.001, bounding_box.max.x + 0.001)
    cur_fig.set_ybound(bounding_box.min.y - 0.001, bounding_box.max.y + 0.001)

    return fig


# Load up the implicit
gyroid_implicit_file_path = Path(__file__).parent.parent / "assets" / "gyroid_sphere_5mm_radius.implicit"
gyroid_handle = c_void_p()

lib.ntop_core_import_from_file(str(gyroid_implicit_file_path).encode("utf8"), gyroid_handle)

## Setup the slice stack
# Get the bounding box to slice the full body of the implicit
bounding_box = BoundingBox()
lib.ntop_core_query_bounding_box(gyroid_handle, pointer(bounding_box))

# Offset from the bounding box slightly to capture the top and
# bottom surfaces
slice_top_bottom_offset = 0.0001

# Slice at 11 heights within the bounding box of the `.implicit`
slice_z_offsets = np.linspace(
    bounding_box.min.z + slice_top_bottom_offset,
    bounding_box.max.z - slice_top_bottom_offset,
    11
    ).tolist()

# Capture features 50 microns in size
min_feat_size = 50 * (10 ** -6)

# Make a folder to capture the output
output_folder = Path(__file__).parent / "output"
output_folder.mkdir(exist_ok=True)

# Create a slice stack
output_paths: list[Path] = []

for idx, z_offset in enumerate(slice_z_offsets):
    slice_plane = Frame(
        Vec3(0, 0, z_offset),
        Vec3(1, 0, 0),
        Vec3(0, 1, 0)
    )

    # List containing a list of tuples representing closed polyline curves
    slice = []

    # slice the implicit on each frame
    lib.ntop_core_query_contours(gyroid_handle, pointer(slice_plane), min_feat_size, None, callback_query_contours(slice))

    # save a plot of the figure
    fig = plot_slice(slice, bounding_box)
    output_path = output_folder / f"slice_{idx}_{gyroid_implicit_file_path.stem}_{int(round(z_offset, 3)*1000)}mmZ.png"
    fig.savefig(output_path, format="png")
    output_paths.append(output_path)

# Print out the location of the generated files
print("Generated the following slices:")
for f in output_paths:
    print(f"\t{f}")

# Unload the `.implicit`
lib.ntop_core_release(gyroid_handle)
