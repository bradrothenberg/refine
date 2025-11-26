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

### Implicit Surface Parameterization

Since implicit surfaces `f(x,y,z) = 0` don't have natural (u,v) parameters, refine uses a **projection-based parameterization**:

1. **Forward evaluation** `(u,v) → xyz`:
   - Use (u,v) as seed coordinates in XY plane
   - Project seed point onto implicit surface using gradient descent

2. **Inverse evaluation** `xyz → (u,v)`:
   - Project xyz onto surface
   - Extract (u,v) from projected XY coordinates

### Curvature Estimation

Principal curvatures are computed using **finite-difference normal variation**:

```
k ≈ |Δn| / |Δs|
```

Where:
- `n = ∇f / ||∇f||` is the surface normal
- `Δn` is the change in normal over distance `Δs`

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

## References

- [NASA refine documentation](https://github.com/nasa/refine)
- [nTop Core SDK](https://www.ntop.com/software/capabilities/implicit-interop/)
- Original EGADS interface: `src/ref_egads.c`
