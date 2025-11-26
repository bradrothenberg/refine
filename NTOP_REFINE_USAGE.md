git # Using nTop Core with NASA Refine

This guide explains how to use implicit surfaces from nTop Core to guide mesh adaptation in NASA's refine mesh adaptation framework.

## Overview

The nTop integration allows refine to use implicit surfaces (`.implicit` files) instead of EGADS BREP geometry. This enables:

- Mesh adaptation guided by implicit surface curvature
- Node projection onto implicit surfaces during smoothing
- Curvature-based metric computation for anisotropic refinement

## Building with nTop Core Support

### Prerequisites

1. **nTop Core SDK** - You need the nTop Core library:
   - `ntop_core.lib` (Windows) or `libntop_core.so` (Linux)
   - `ntop_core.dll` (Windows runtime)

2. **CMake 3.10+**
3. **Visual Studio 2022 Build Tools** (Windows) or GCC (Linux)

### Build Steps

```bash
# Clone and enter the repository
cd refine

# Create build directory
mkdir build && cd build

# Configure with nTop Core
cmake -G "Visual Studio 17 2022" \
      -DNTOP_CORE_DIR="C:/path/to/nTopCore" \
      ..

# Build
cmake --build . --config Release

# Copy the DLL to output (Windows only)
cp /path/to/nTopCore/lib/ntop_core.dll Release/
```

### CMake Options

| Option | Description |
|--------|-------------|
| `NTOP_CORE_DIR` | Path to nTop Core SDK directory containing `lib/` |

When `NTOP_CORE_DIR` is set and the library is found, `HAVE_NTOP` is automatically defined.

## Creating Implicit Files

Implicit files (`.implicit`) are created in nTopology software:

1. Open nTopology
2. Create or import your geometry
3. Convert to implicit body if needed
4. Export as `.implicit` file

Test files are available in `nTop/docs/examples/assets/`:
- `sphere_5mm_radius.implicit`
- `cone_5mm_radius_10mm_apex.implicit`
- `gyroid_sphere_5mm_radius.implicit`
- `heat-sink.implicit`

## API Reference

### Loading an Implicit Surface

```c
#include "ref_ntop.h"

REF_GEOM ref_geom;
// ... initialize ref_geom ...

// Load the implicit surface
RSS(ref_ntop_load(ref_geom, "geometry.implicit"), "load ntop");
```

### Surface Evaluation

```c
// Evaluate surface at parametric coordinates
REF_DBL params[2] = {u, v};
REF_DBL xyz[3];
REF_DBL dxyz_dtuv[6];  // Optional derivatives

RSS(ref_ntop_eval_at(ref_geom, REF_GEOM_FACE, face_id, params, xyz, dxyz_dtuv), "eval");
```

### Inverse Evaluation (Projection)

```c
// Project a point onto the surface
REF_DBL xyz[3] = {x, y, z};
REF_DBL param[2];

RSS(ref_ntop_inverse_eval(ref_geom, REF_GEOM_FACE, face_id, xyz, param), "inverse");
```

### Curvature Computation

```c
// Get principal curvatures at a surface point
REF_DBL uv[2] = {u, v};
REF_DBL kr, ks;           // Principal curvatures
REF_DBL r[3], s[3];       // Principal directions

RSS(ref_ntop_face_curvature_at(ref_geom, face_id, degen, uv, &kr, r, &ks, s), "curv");
```

### Cleanup

```c
// Release nTop resources
RSS(ref_ntop_close(ref_geom), "close ntop");
```

## How It Works

### Direct XYZ-Based Geometry Evaluation

Unlike EGADS surfaces, implicit surfaces `f(x,y,z) = 0` don't have natural (u,v) parameters.
The nTop integration **bypasses UV parameterization entirely** and works directly with 3D node positions:

1. **Curvature computation**:
   - Uses the node's XYZ coordinates directly (no UV conversion)
   - Computes surface normal at the node position via gradient: `n = ∇f / ||∇f||`
   - Estimates principal curvatures using finite-difference normal variation in orthogonal tangent directions:
     ```
     k ≈ |Δn| / |Δs|
     ```
   - Constructs orthogonal tangent basis perpendicular to the surface normal
   - Samples normals at offset points along each tangent direction

2. **Node projection**:
   - Projects nodes onto the implicit surface using gradient descent
   - Maintains surface fidelity during mesh smoothing operations
   - No UV parameters are stored or used in the process

The nTop Core API provides `ntop_core_query_derivative()` for efficient gradient computation.

### Mesh Adaptation Loop

The standard refine adaptation loop works with nTop:

```
1. Load mesh and implicit geometry
2. Compute curvature-based metric: h = segments_per_radian / curvature
3. Adapt mesh (split, collapse, swap, smooth)
4. Project nodes to surface during smoothing
5. Repeat until convergence
```

## Dispatch Logic

The nTop backend is selected automatically when:

```c
ref_geom_ntop_loaded(ref_geom)  // Returns TRUE when nTop context is loaded
```

This macro checks:
- `ref_geom->context != NULL` (nTop context exists)
- `ref_geom->model == NULL` (not EGADS)
- `ref_geom->meshlink == NULL` (not MeshLink)

## Units

**Important**: nTop Core uses **meters** for all coordinates.

If your mesh is in different units, set the unit scale:

```c
REF_NTOP_CONTEXT ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);
ntop_context->unit_scale = 0.001;  // Convert mm to meters
```

## Example: Sphere Adaptation

```c
#include "ref_grid.h"
#include "ref_geom.h"
#include "ref_ntop.h"
#include "ref_metric.h"
#include "ref_adapt.h"

int main() {
    REF_GRID ref_grid;
    REF_GEOM ref_geom;

    // Create initial mesh (e.g., load from file)
    RSS(ref_grid_create(&ref_grid), "create grid");
    ref_geom = ref_grid_geom(ref_grid);

    // Load implicit geometry
    RSS(ref_ntop_load(ref_geom, "sphere_5mm_radius.implicit"), "load");

    // Compute curvature-based metric
    REF_DBL segments_per_radian = 2.0;
    RSS(ref_metric_interpolated_curvature(ref_grid), "metric");

    // Adapt
    RSS(ref_adapt_surf(ref_grid), "adapt");

    // Export result
    RSS(ref_export_tec(ref_grid, "adapted_sphere.tec"), "export");

    // Cleanup
    RSS(ref_ntop_close(ref_geom), "close");
    RSS(ref_grid_free(ref_grid), "free");

    return 0;
}
```

## Troubleshooting

### "refine compiled without HAVE_NTOP"

The build didn't find nTop Core. Check:
- `NTOP_CORE_DIR` is set correctly
- `lib/ntop_core.lib` exists in that directory

### "nTop file not found"

Check the file path. Use absolute paths or ensure the working directory is correct.

### "Unsupported nTop file version"

The `.implicit` file was created with a newer version of nTopology than your nTop Core library supports. Update your nTop Core SDK.

### Large gaps between mesh and surface

Possible causes:
- Unit mismatch (check `unit_scale`)
- Tolerance too large (check `ntop_context->tolerance`)
- Not enough adaptation iterations

### Curvature is zero everywhere

- Verify the implicit loads correctly (check bounding box output)
- Ensure you're querying points on or near the surface

## Files Modified for nTop Integration

| File | Changes |
|------|---------|
| `src/ref_ntop.h` | New: nTop API declarations |
| `src/ref_ntop.c` | New: nTop implementation |
| `src/ref_geom.h` | Added `ref_geom_ntop_loaded()` macro |
| `src/ref_geom.c` | Added nTop dispatch in `ref_geom_constrain()` |
| `src/ref_metric.c` | Added curvature dispatch for nTop |
| `src/CMakeLists.txt` | Added nTop Core build options |

## nTop Core API Summary

Functions used from nTop Core:

| Function | Purpose |
|----------|---------|
| `ntop_core_import_from_file()` | Load `.implicit` file |
| `ntop_core_release()` | Free implicit handle |
| `ntop_core_query_bounding_box()` | Get geometry bounds |
| `ntop_core_query_field()` | Signed distance at point |
| `ntop_core_query_derivative()` | Gradient + distance at point |

## Using the Command Line Interface

### Basic Usage with Implicit Surfaces

The `ref` executable can adapt meshes using implicit surfaces with the `--implicit` flag:

```bash
ref adapt input.meshb --implicit geometry.implicit -x output.meshb
```

**IMPORTANT**: You must provide **both**:
1. A **volumetric mesh** (`.meshb`, `.ugrid`, `.b8.ugrid`, etc.) containing tetrahedra
2. An **implicit surface file** (`.implicit`) for geometry evaluation

The volumetric mesh provides:
- Initial tetrahedral elements to adapt
- Surface triangles marking the geometry boundary
- Node positions as starting points

The implicit surface provides:
- Exact geometry for node projection during smoothing
- Curvature information for metric computation
- Surface normals for anisotropic refinement

### Controlling Mesh Density

Without metric control, the curvature-based adaptation can create overly refined (or degenerate) meshes. Use `--implied-complexity` to control target mesh density:

```bash
ref adapt input.meshb --implicit geometry.implicit \
    --implied-complexity 20000 \
    -x output.meshb
```

**Recommended complexity values**:
- Coarse mesh: 5,000 - 10,000
- Medium mesh: 20,000 - 50,000
- Fine mesh: 100,000 - 500,000

The `--implied-complexity` flag:
- Derives a metric from the input mesh structure
- Scales it to achieve the target complexity value
- Prevents excessive refinement in high-curvature regions
- Maintains better element quality throughout adaptation

### Complete Workflow Example

Here's a complete workflow using the SphereCube example:

```bash
# 1. Convert surface mesh (STL) to meshb format
util/stl2mesh.exe SphereCube.stl SphereCube_surf.meshb

# 2. Convert volumetric mesh (BDF) to meshb format
util/bdf2mesh.exe SphereCube.bdf SphereCube_vol.meshb

# 3. Combine surface and volume into one mesh (if needed)
ref translate SphereCube_vol.meshb SphereCube_combined.meshb

# 4. Adapt with implicit surface and controlled complexity
ref adapt SphereCube_combined.meshb \
    --implicit SphereCube.implicit \
    --implied-complexity 20000 \
    -x SphereCube_adapted.meshb

# 5. Convert back to BDF format (optional)
util/mesh2bdf.exe SphereCube_adapted.meshb SphereCube_adapted.bdf
```

### Available Mesh Formats

The refine framework supports multiple mesh formats:

**Input/Output formats**:
- `.meshb` - Gamma Mesh Format (binary, recommended)
- `.mesh` - Gamma Mesh Format (ASCII)
- `.ugrid` - NASA UGRID format (ASCII)
- `.b8.ugrid` - NASA UGRID format (binary)
- `.bdf` - NASTRAN Bulk Data Format (via converters)
- `.stl` - STereoLithography (surface only, via converters)

**Converter utilities** (in `util/`):
- `bdf2mesh` - Convert NASTRAN BDF to meshb
- `mesh2bdf` - Convert meshb to NASTRAN BDF
- `stl2mesh` - Convert STL to meshb (surface mesh)
- `mesh2stl` - Convert meshb to STL (surface mesh)

### Adaptation Options

Common flags for the `adapt` command:

| Flag | Description |
|------|-------------|
| `--implicit <file>` | Use implicit surface for geometry |
| `--implied-complexity <N>` | Target mesh complexity (recommended) |
| `-x <output>` | Output mesh file |
| `--surf-pass <N>` | Number of surface adaptation passes (default: 10) |
| `--vol-pass <N>` | Number of volume adaptation passes (default: 10) |

### Checking Adaptation Quality

Monitor the adaptation output for these indicators:

**Good adaptation**:
```
pass 5 of 10: id ratio 0.7071 quality 0.0448 complexity   19524
```
- Quality ratio > 0.7 (closer to 1.0 is better)
- Complexity near target value
- Smooth convergence across passes

**Poor adaptation** (warning signs):
```
pass 22 of 25: id ratio 0.0010 quality 0.9846 complexity 5127391281
WARNING: termination recommended
```
- Quality ratio < 0.01 indicates degenerate elements
- Complexity orders of magnitude from target
- "Termination recommended" warnings early in adaptation
- Extremely small minimum volumes (< 1e-15)

If you see poor quality, use `--implied-complexity` to control the metric.

## References

- [NASA refine documentation](https://github.com/nasa/refine)
- [nTop Core SDK](https://www.ntop.com/software/capabilities/implicit-interop/)
- Original EGADS interface: `src/ref_egads.c`
