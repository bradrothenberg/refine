# Quick Start: nTop Integration

## TL;DR - What You Need to Implement

### 5 Critical Functions (in priority order):

1. **`ref_ntop_eval_at()`** - Evaluate surface at (u,v) → (x,y,z)
   - Called ~10,000 times per adaptation
   - Input: parametric coordinates
   - Output: 3D position on implicit surface

2. **`ref_ntop_inverse_eval()`** - Project point to surface
   - Called 1,000-5,000 times
   - Input: 3D point (x,y,z)
   - Output: parameters (u,v)
   - Use gradient descent: `xyz -= f(xyz) * ∇f / ||∇f||²`

3. **`ref_ntop_face_curvature_at()`** - Principal curvatures
   - Called ~1,000 times
   - Output: kr, ks (eigenvalues), r, s (eigenvectors)
   - Compute from Hessian of implicit function

4. **`ref_ntop_edge_curvature()`** - 1D curve curvature
   - Called 500-1,000 times
   - Output: k (curvature), normal vector

5. **`ref_ntop_feature_size()`** - Metric tensor per node
   - Called once per node
   - Output: (h0, h1, h2) sizing + directions

---

## File Checklist

### Create These Files:
- ✅ `src/ref_ntop.h` - Interface (see full guide)
- ✅ `src/ref_ntop.c` - Implementation (see full guide)
- ⬜ `src/ref_ntop_test.c` - Unit tests

### Modify These Files:
- ⬜ `src/ref_geom.c` - Add dispatch at line 1555, 979, 1153, 1219, etc.
- ⬜ `src/ref_geom.h` - Add `ref_geom_ntop_loaded()` macro
- ⬜ `src/ref_metric.c` - Add nTop curvature queries at lines 1387, 1452
- ⬜ `CMakeLists.txt` or `Makefile` - Link nTop library

---

## Minimal Working Example

```c
/* ref_ntop.c - Minimal implementation */

REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params,
                                    REF_DBL *xyz, REF_DBL *dxyz_dtuv) {
  if (REF_GEOM_FACE == type) {
    REF_DBL u = params[0];
    REF_DBL v = params[1];

    /* YOUR CODE: Evaluate implicit surface at (u,v) */
    // xyz = ntop_eval_surface(face_id, u, v);

    if (dxyz_dtuv != NULL) {
      /* YOUR CODE: Compute derivatives */
      // dxyz_dtuv = ntop_surface_gradient(face_id, u, v);
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param) {
  if (REF_GEOM_FACE == type) {
    /* Newton-Raphson iteration */
    REF_DBL xyz_surf[3] = {xyz[0], xyz[1], xyz[2]};

    for (int iter = 0; iter < 20; iter++) {
      REF_DBL f = /* YOUR CODE: ntop_implicit_eval(face_id, xyz_surf) */;
      REF_DBL grad[3] = /* YOUR CODE: ntop_gradient(face_id, xyz_surf) */;

      /* Move toward surface */
      REF_DBL grad_norm = sqrt(grad[0]*grad[0] + grad[1]*grad[1] + grad[2]*grad[2]);
      xyz_surf[0] -= f * grad[0] / (grad_norm * grad_norm);
      xyz_surf[1] -= f * grad[1] / (grad_norm * grad_norm);
      xyz_surf[2] -= f * grad[2] / (grad_norm * grad_norm);

      if (fabs(f) < 1e-10) break;
    }

    /* Reparameterize */
    param[0] = xyz_surf[0];  /* u = x */
    param[1] = xyz_surf[1];  /* v = y */
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s) {
  /* YOUR CODE:
   * 1. Compute Hessian H = [∂²f/∂xi∂xj]
   * 2. Compute gradient g = ∇f
   * 3. Shape operator S = -H_projected / ||g||
   * 4. Eigen decomposition: kr, ks = eigenvalues, r, s = eigenvectors
   */

  // REF_DBL hessian[9] = ntop_hessian(faceid, uv);
  // REF_DBL gradient[3] = ntop_gradient(faceid, uv);
  // compute_principal_curvatures(hessian, gradient, kr, r, ks, s);

  /* TEMPORARY: Return small curvature */
  *kr = 0.01; *ks = 0.01;
  r[0] = 1.0; r[1] = 0.0; r[2] = 0.0;
  s[0] = 0.0; s[1] = 1.0; s[2] = 0.0;

  return REF_SUCCESS;
}
```

---

## Integration Points in Existing Code

### 1. Add to `ref_geom.h` (after line 98):
```c
#define ref_geom_ntop_loaded(ref_geom) \
  (NULL != (ref_geom)->context && NULL == (ref_geom)->model && \
   NULL == (ref_geom)->meshlink)
```

### 2. Modify `ref_geom.c` - Function `ref_geom_constrain()` (line 1555):
```c
REF_FCN REF_STATUS ref_geom_constrain(REF_GRID ref_grid, REF_INT node) {
  /* ... existing code ... */

  /* ADD THIS BLOCK AFTER MESHLINK CHECK */
  if (ref_geom_ntop_loaded(ref_geom)) {
    /* Use nTop backend */
    REF_INT item, geom;
    REF_DBL xyz[3];

    each_ref_geom_having_node(ref_geom, node, item, geom) {
      RSS(ref_ntop_eval(ref_geom, geom, xyz, NULL), "ntop eval");
      ref_node_xyz(ref_node, 0, node) = xyz[0];
      ref_node_xyz(ref_node, 1, node) = xyz[1];
      ref_node_xyz(ref_node, 2, node) = xyz[2];
    }
    return REF_SUCCESS;
  }

  /* ... rest of EGADS code ... */
}
```

### 3. Modify `ref_metric.c` - Add dispatch (line ~1387):
```c
if (ref_geom_ntop_loaded(ref_geom)) {
  RSS(ref_ntop_face_curvature(ref_grid, geom, &kr, r, &ks, s), "ntop curve");
} else if (ref_geom_meshlinked(ref_geom)) {
  RSS(ref_meshlink_face_curvature(ref_grid, geom, &kr, r, &ks, s), "ml curve");
} else {
  RSS(ref_egads_face_curvature(ref_geom, geom, &kr, r, &ks, s), "egads curve");
}
```

---

## Testing Strategy

### Phase 1: Unit Test
```bash
# Build test
gcc -o ref_ntop_test src/ref_ntop_test.c src/ref_ntop.c -I./src -lntop_core

# Run
./ref_ntop_test
```

### Phase 2: Simple Geometry
```bash
# Adapt a sphere (curvature should be 1/R everywhere)
./ref adapt sphere_coarse.meshb --ntop sphere.ntop --metric-curvature 1.0 -o sphere_fine.meshb
```

### Phase 3: Validation
```bash
# Check gap between mesh and geometry
./ref examine sphere_fine.meshb sphere.ntop --gap

# Expected: gap < 1e-6
```

---

## Key Equations

### Implicit Surface: f(x,y,z) = 0

**Gradient (normal):**
```
∇f = [∂f/∂x, ∂f/∂y, ∂f/∂z]
n = ∇f / ||∇f||
```

**Hessian:**
```
H = [∂²f/∂x²   ∂²f/∂x∂y  ∂²f/∂x∂z]
    [∂²f/∂y∂x  ∂²f/∂y²   ∂²f/∂y∂z]
    [∂²f/∂z∂x  ∂²f/∂z∂y  ∂²f/∂z² ]
```

**Principal Curvatures:**
```
Shape operator: S = -H_projected / ||∇f||
kr, ks = eigenvalues(S)  ← Controls mesh size!
r, s = eigenvectors(S)   ← Anisotropic directions
```

**Mesh Size:**
```
h_r = segments_per_radian / |kr|
h_s = segments_per_radian / |ks|

Typical: segments_per_radian = 1.0
→ h = 1/k (finer mesh for high curvature)
```

---

## Common Issues & Solutions

### Issue 1: "No curvature detected"
- **Solution**: Check that `ref_ntop_face_curvature_at()` returns non-zero kr, ks
- **Debug**: Print kr, ks values - should be O(1/R) for radius R features

### Issue 2: "Mesh doesn't adapt"
- **Solution**: Ensure curvature → metric conversion in `ref_metric.c`
- **Check**: `ref_geom_segments_per_radian_of_curvature()` should be set

### Issue 3: "Gap too large"
- **Solution**: Improve `ref_ntop_inverse_eval()` convergence
- **Fix**: Increase iterations, use better initial guess

### Issue 4: "Crashes during adaptation"
- **Solution**: Check NULL pointers in `ref_ntop_eval_at()`
- **Validate**: Ensure all face/edge IDs are in valid range [1, nface/nedge]

---

## Quick Commands

```bash
# Build with nTop support
cmake -DHAVE_NTOP=ON -DNTOP_DIR=/path/to/ntop/core ..
make

# Run adaptation with curvature-based metric
./ref adapt input.meshb --ntop geometry.ntop \
    --metric-curvature 1.0 \
    -o output.meshb

# Check results
./ref examine output.meshb geometry.ntop --gap --quality

# Visualize in Tecplot
./ref translate output.meshb output.tec
```

---

## What nTop Core Must Provide

Your nTop Core library needs these capabilities:

1. **Surface Evaluation**: `ntop_eval_implicit(surface, u, v) → (x,y,z)`
2. **Gradient**: `ntop_gradient(surface, xyz) → [∂f/∂x, ∂f/∂y, ∂f/∂z]`
3. **Hessian**: `ntop_hessian(surface, xyz) → 3×3 matrix`
4. **Signed Distance**: `ntop_distance(surface, xyz) → f(xyz)`
5. **Projection**: `ntop_project(surface, xyz) → closest point on surface`

If nTop doesn't provide Hessian directly, you can approximate with finite differences:
```c
∂²f/∂x² ≈ (f(x+h,y,z) - 2f(x,y,z) + f(x-h,y,z)) / h²
```

---

## Reference Files

- **Full Guide**: [NTOP_INTEGRATION_GUIDE.md](NTOP_INTEGRATION_GUIDE.md)
- **EGADS Example**: [src/ref_egads.c](src/ref_egads.c) (4,712 lines)
- **FaceLift Example**: [src/ref_facelift.c](src/ref_facelift.c) (alternative backend)
- **Geometry Interface**: [src/ref_geom.h](src/ref_geom.h)

---

## Getting Help

If you get stuck:
1. Check the full guide for mathematical details
2. Compare with `ref_egads.c` implementation
3. Add debug prints to trace function calls
4. Test with simple analytical surfaces first (sphere, cylinder)

Good luck! 🚀
