# nTop Core Integration Guide for Refine

## Overview

This guide explains how to replace the EGADS (Engineering Sketch Pad) geometry backend with nTop Core implicit surfaces in the Refine mesh adaptation framework. The integration follows the existing pattern used by alternative backends (MeshLink and FaceLift).

---

## Architecture Overview

### Current Geometry Backend System

Refine uses a **pluggable geometry architecture** with three existing backends:

1. **EGADS** (`ref_egads.c`) - Primary CAD kernel (BREP surfaces)
2. **MeshLink** (`ref_meshlink.c`) - Alternative CAD interface
3. **FaceLift** (`ref_facelift.c`) - Mesh-based surrogate surfaces

The dispatch logic is in `ref_geom.c` which checks:
```c
if (ref_geom_model_loaded(ref_geom)) {
    // Use EGADS
    ref_egads_eval_at(...)
} else if (ref_geom_meshlinked(ref_geom)) {
    // Use MeshLink
    ref_meshlink_constrain(...)
}
```

### Key Components

```
┌────────────────────────────────────────────┐
│  nTop Core Library                         │
│  (Implicit Surface Functions)              │
└────────────┬───────────────────────────────┘
             │
             ↓
┌────────────────────────────────────────────┐
│  NEW: ref_ntop.c/h                         │
│  • Wrapper for nTop API                    │
│  • Query dispatch and result formatting    │
│  • Implicit surface evaluation             │
└────────────┬───────────────────────────────┘
             │
             ↓
┌────────────────────────────────────────────┐
│  EXISTING: ref_geom.c                      │
│  • Geometry-node associations              │
│  • Parametric interpolation                │
│  • Constraint application                  │
└────────────┬───────────────────────────────┘
             │
             ↓
┌────────────────────────────────────────────┐
│  EXISTING: ref_metric.c                    │
│  • Curvature → mesh size                   │
│  • Metric tensor construction              │
│  • Gradation                               │
└────────────┬───────────────────────────────┘
             │
             ↓
┌────────────────────────────────────────────┐
│  EXISTING: ref_adapt.c                     │
│  • Edge splitting                          │
│  • Smoothing                               │
│  • Main adaptation loop                    │
└────────────────────────────────────────────┘
```

---

## Implementation Steps

### Step 1: Create the nTop Interface Header

Create `src/ref_ntop.h`:

```c
#ifndef REF_NTOP_H
#define REF_NTOP_H

#include "ref_defs.h"

BEGIN_C_DECLORATION

END_C_DECLORATION

#include "ref_geom.h"

BEGIN_C_DECLORATION

/* ========================================
 * INITIALIZATION & LIFECYCLE
 * ======================================== */

/** Initialize nTop context and load geometry */
REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom);

/** Clean up nTop context */
REF_FCN REF_STATUS ref_ntop_close(REF_GEOM ref_geom);

/** Load implicit surface definition from file */
REF_FCN REF_STATUS ref_ntop_load(REF_GEOM ref_geom, const char *filename);

/** Save implicit surface definition to file */
REF_FCN REF_STATUS ref_ntop_save(REF_GEOM ref_geom, const char *filename);


/* ========================================
 * CORE GEOMETRIC QUERIES (CRITICAL)
 * ======================================== */

/**
 * Evaluate surface/curve at parametric location
 *
 * @param ref_geom - geometry object
 * @param type - REF_GEOM_NODE (0), REF_GEOM_EDGE (1), REF_GEOM_FACE (2)
 * @param id - geometry entity ID (1-based indexing)
 * @param params - [t] for edge, [u,v] for face
 * @param xyz - OUTPUT: [x,y,z] position in 3D
 * @param dxyz_dtuv - OUTPUT: derivatives (can be NULL)
 *                    For EDGE: [dx/dt, dy/dt, dz/dt]
 *                    For FACE: [dx/du, dy/du, dz/du, dx/dv, dy/dv, dz/dv]
 *
 * Called ~10,000+ times per adaptation pass
 */
REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params, REF_DBL *xyz,
                                    REF_DBL *dxyz_dtuv);

/**
 * Evaluate at stored parametric location
 * (convenience wrapper around ref_ntop_eval_at)
 */
REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom,
                                 REF_DBL *xyz, REF_DBL *dxyz_dtuv);

/**
 * Inverse evaluation: project point to surface
 *
 * @param ref_geom - geometry object
 * @param type - REF_GEOM_EDGE (1) or REF_GEOM_FACE (2)
 * @param id - geometry entity ID
 * @param xyz - INPUT: [x,y,z] point to project
 * @param param - OUTPUT: [t] for edge, [u,v] for face
 *
 * Find closest point on implicit surface to given xyz.
 * Called 1,000-5,000 times per adaptation.
 *
 * Algorithm suggestion for implicit surfaces:
 *   1. Compute signed distance: d = f(xyz)
 *   2. Compute gradient: grad_f = [∂f/∂x, ∂f/∂y, ∂f/∂z]
 *   3. Project: xyz_surface = xyz - d * grad_f / ||grad_f||
 *   4. Iterate until converged (Newton-Raphson)
 *   5. Return parametric location (u,v) via reparameterization
 */
REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param);

/**
 * Alternative inverse eval with Newton iteration control
 * (You may implement this as a wrapper to ref_ntop_inverse_eval)
 */
REF_FCN REF_STATUS ref_ntop_invert(REF_GEOM ref_geom, REF_INT type,
                                   REF_INT id, REF_DBL *xyz, REF_DBL *param);


/* ========================================
 * CURVATURE QUERIES (DRIVES MESH SIZING)
 * ======================================== */

/**
 * Compute principal curvatures on a face
 *
 * @param ref_geom - geometry object
 * @param faceid - face ID (1-based)
 * @param degen - degenerate parameter flag (handle singularities)
 * @param uv - INPUT: [u, v] parametric location
 * @param kr - OUTPUT: first principal curvature (1/radius_1)
 * @param r - OUTPUT: first principal direction [rx, ry, rz] (unit vector)
 * @param ks - OUTPUT: second principal curvature (1/radius_2)
 * @param s - OUTPUT: second principal direction [sx, sy, sz] (unit vector)
 *
 * Called ~1,000+ times per adaptation.
 * Directly controls mesh element sizes via:
 *   h_r = delta_radian / |kr|
 *   h_s = delta_radian / |ks|
 *
 * For implicit surface f(x,y,z) = 0:
 *   1. Compute Hessian matrix H = [∂²f/∂xi∂xj]
 *   2. Compute gradient: g = ∇f
 *   3. Shape operator: S = -H / ||g|| (projected to tangent plane)
 *   4. Eigenvalues of S → kr, ks
 *   5. Eigenvectors of S → r, s (orthonormal in tangent plane)
 */
REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s);

/**
 * Compute curvature at stored face geom
 * (convenience wrapper)
 */
REF_FCN REF_STATUS ref_ntop_face_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *kr, REF_DBL *r,
                                           REF_DBL *ks, REF_DBL *s);

/**
 * Compute curvature on an edge (1D curve)
 *
 * @param ref_geom - geometry object
 * @param geom - stored edge geometry index
 * @param k - OUTPUT: curvature magnitude (1/radius)
 * @param normal - OUTPUT: principal normal vector [nx, ny, nz]
 *
 * Called 500-1,000 times per adaptation.
 *
 * For 1D curve in 3D:
 *   k = ||r''(t)|| / ||r'(t)||³
 *   normal = (r'' - (r''·T)T) / ||r'' - (r''·T)T||
 *   where T = r'(t) / ||r'(t)||
 */
REF_FCN REF_STATUS ref_ntop_edge_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *k, REF_DBL *normal);


/* ========================================
 * FEATURE SIZE (NODE-CENTRIC METRICS)
 * ======================================== */

/**
 * Compute local feature size at a node
 *
 * @param ref_grid - grid object
 * @param node - node index
 * @param h0 - OUTPUT: minimum feature size (eigenvalue λ0)
 * @param dir0 - OUTPUT: direction of h0 (eigenvector)
 * @param h1 - OUTPUT: medium feature size (eigenvalue λ1)
 * @param dir1 - OUTPUT: direction of h1
 * @param h2 - OUTPUT: maximum feature size (eigenvalue λ2)
 * @param dir2 - OUTPUT: direction of h2
 *
 * Called once per node during initial metric specification.
 *
 * Returns metric tensor eigenvalues/eigenvectors that control
 * anisotropic mesh sizing around the node. Considers all
 * geometric entities (edges, faces) attached to the node.
 *
 * Implementation:
 *   1. For each face touching node: compute kr, ks from curvature
 *   2. For each edge touching node: compute k from curvature
 *   3. Form metric tensor M = sum of contributions
 *   4. Compute eigendecomposition: M = Q Λ Q^T
 *   5. Return (λ0, λ1, λ2) and corresponding eigenvectors
 */
REF_FCN REF_STATUS ref_ntop_feature_size(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *h0, REF_DBL *dir0,
                                         REF_DBL *h1, REF_DBL *dir1,
                                         REF_DBL *h2, REF_DBL *dir2);


/* ========================================
 * TOPOLOGY & PROPERTIES
 * ======================================== */

/** Get parameter range for an edge */
REF_FCN REF_STATUS ref_ntop_edge_trange(REF_GEOM ref_geom, REF_INT id,
                                        REF_DBL *trange);

/** Get geometric tolerance for entity */
REF_FCN REF_STATUS ref_ntop_tolerance(REF_GEOM ref_geom, REF_INT type,
                                      REF_INT id, REF_DBL *tolerance);

/** Measure gap between surface and node */
REF_FCN REF_STATUS ref_ntop_gap(REF_GEOM ref_geom, REF_INT node,
                                REF_DBL *gap);

/** Get diagonal of bounding box */
REF_FCN REF_STATUS ref_ntop_diagonal(REF_GEOM ref_geom, REF_INT geom,
                                     REF_DBL *diag);

/** Convert edge parameter to face UV (for edges on faces) */
REF_FCN REF_STATUS ref_ntop_edge_face_uv(REF_GEOM ref_geom, REF_INT edgeid,
                                         REF_INT faceid, REF_INT sense,
                                         REF_DBL t, REF_DBL *uv);

END_C_DECLORATION

#endif /* REF_NTOP_H */
```

---

### Step 2: Create the nTop Implementation Skeleton

Create `src/ref_ntop.c`:

```c
#include "ref_ntop.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_node.h"
#include "ref_cell.h"
#include "ref_adj.h"

/* Include your nTop Core headers here */
// #include "ntop_core.h"

/* ========================================
 * DATA STRUCTURES
 * ======================================== */

/* Store nTop context in ref_geom->context */
typedef struct REF_NTOP_CONTEXT_STRUCT {
  void *ntop_model;          /* nTop model handle */
  REF_INT nface;             /* Number of implicit surfaces */
  REF_INT nedge;             /* Number of curves */
  void **face_objects;       /* Array of nTop surface objects */
  void **edge_objects;       /* Array of nTop curve objects */
  REF_DBL *param_ranges;     /* Parameter ranges [tmin,tmax] or [umin,umax,vmin,vmax] */
} REF_NTOP_CONTEXT_STRUCT;

typedef REF_NTOP_CONTEXT_STRUCT *REF_NTOP_CONTEXT;


/* ========================================
 * INITIALIZATION
 * ======================================== */

REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom) {
  REF_NTOP_CONTEXT ntop_context;

  /* Allocate context */
  ref_malloc(ntop_context, 1, REF_NTOP_CONTEXT_STRUCT);

  /* Initialize nTop library */
  // ntop_context->ntop_model = ntop_init();

  ntop_context->nface = 0;
  ntop_context->nedge = 0;
  ntop_context->face_objects = NULL;
  ntop_context->edge_objects = NULL;
  ntop_context->param_ranges = NULL;

  /* Store context in ref_geom */
  ref_geom->context = (void *)ntop_context;
  ref_geom->contex_owned = REF_TRUE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_close(REF_GEOM ref_geom) {
  REF_NTOP_CONTEXT ntop_context;

  if (NULL == ref_geom->context) return REF_SUCCESS;

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  /* Clean up nTop objects */
  // ntop_cleanup(ntop_context->ntop_model);

  /* Free arrays */
  ref_free(ntop_context->face_objects);
  ref_free(ntop_context->edge_objects);
  ref_free(ntop_context->param_ranges);
  ref_free(ntop_context);

  ref_geom->context = NULL;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_load(REF_GEOM ref_geom, const char *filename) {
  REF_NTOP_CONTEXT ntop_context;

  if (NULL == ref_geom->context) {
    RSS(ref_ntop_open(ref_geom), "open ntop");
  }

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  /* Load implicit surface definition from file */
  // ntop_load_model(ntop_context->ntop_model, filename);

  /* Query number of surfaces and curves */
  // ntop_context->nface = ntop_get_num_surfaces(ntop_context->ntop_model);
  // ntop_context->nedge = ntop_get_num_curves(ntop_context->ntop_model);

  /* Store in ref_geom */
  ref_geom->nface = ntop_context->nface;
  ref_geom->nedge = ntop_context->nedge;

  printf("Loaded nTop geometry: %d faces, %d edges\n",
         ntop_context->nface, ntop_context->nedge);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_save(REF_GEOM ref_geom, const char *filename) {
  /* Save current implicit surface state */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(filename);

  // TODO: Implement nTop save

  return REF_SUCCESS;
}


/* ========================================
 * CORE GEOMETRIC QUERIES
 * ======================================== */

REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params, REF_DBL *xyz,
                                    REF_DBL *dxyz_dtuv) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  if (REF_GEOM_NODE == type) {
    /* Nodes are fixed points - retrieve stored XYZ */
    // TODO: Store node positions during load
    return REF_IMPLEMENT;

  } else if (REF_GEOM_EDGE == type) {
    /* Evaluate curve at parameter t */
    REF_DBL t = params[0];

    // EXAMPLE nTop API call:
    // void *edge_obj = ntop_context->edge_objects[id - 1];  /* 1-based → 0-based */
    // ntop_curve_eval(edge_obj, t, xyz);

    if (NULL != dxyz_dtuv) {
      /* Compute derivative dx/dt, dy/dt, dz/dt */
      // ntop_curve_deriv(edge_obj, t, dxyz_dtuv);
    }

    /* TEMPORARY: Return error until implemented */
    return REF_IMPLEMENT;

  } else if (REF_GEOM_FACE == type) {
    /* Evaluate implicit surface at parameters (u,v) */
    REF_DBL u = params[0];
    REF_DBL v = params[1];

    // EXAMPLE nTop API for implicit surfaces:
    // void *face_obj = ntop_context->face_objects[id - 1];
    //
    // Method 1: Direct reparameterization
    //   xyz[0] = u;
    //   xyz[1] = v;
    //   xyz[2] = ntop_implicit_eval(face_obj, u, v);  /* Solve f(u,v,z) = 0 */
    //
    // Method 2: Closest point on isosurface
    //   REF_DBL seed[3] = {u, v, 0.0};  /* Initial guess from (u,v) */
    //   ntop_project_to_isosurface(face_obj, seed, xyz);

    if (NULL != dxyz_dtuv) {
      /* Compute derivatives: [dx/du, dy/du, dz/du, dx/dv, dy/dv, dz/dv] */
      // ntop_implicit_gradient(face_obj, xyz, dxyz_dtuv);
    }

    /* TEMPORARY: Return error until implemented */
    return REF_IMPLEMENT;

  } else {
    printf("ERROR: Unknown geometry type %d\n", type);
    return REF_INVALID;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom,
                                 REF_DBL *xyz, REF_DBL *dxyz_dtuv) {
  REF_INT type, id;
  REF_DBL params[2];

  type = ref_geom_type(ref_geom, geom);
  id = ref_geom_id(ref_geom, geom);

  /* Retrieve stored parametric location */
  params[0] = ref_geom_param(ref_geom, 0, geom);
  params[1] = ref_geom_param(ref_geom, 1, geom);

  return ref_ntop_eval_at(ref_geom, type, id, params, xyz, dxyz_dtuv);
}

REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  if (REF_GEOM_EDGE == type) {
    /* Project point to curve, return parameter t */

    // EXAMPLE Newton-Raphson for curve:
    // REF_DBL t = 0.5;  /* Initial guess */
    // REF_INT iter;
    // for (iter = 0; iter < 20; iter++) {
    //   REF_DBL xyz_curve[3], tangent[3];
    //   ntop_curve_eval(edge_obj, t, xyz_curve);
    //   ntop_curve_deriv(edge_obj, t, tangent);
    //
    //   REF_DBL error[3] = {xyz[0] - xyz_curve[0],
    //                       xyz[1] - xyz_curve[1],
    //                       xyz[2] - xyz_curve[2]};
    //   REF_DBL proj_error = DOT(error, tangent);
    //
    //   t += proj_error / DOT(tangent, tangent);
    //
    //   if (fabs(proj_error) < 1e-10) break;
    // }
    // param[0] = t;

    return REF_IMPLEMENT;

  } else if (REF_GEOM_FACE == type) {
    /* Project point to implicit surface, return (u,v) */

    // EXAMPLE for implicit surface f(x,y,z) = 0:
    // 1. Project to surface using gradient descent:
    //    xyz_surf = xyz - f(xyz) * grad_f / ||grad_f||²
    //    (iterate until f(xyz_surf) ≈ 0)
    //
    // 2. Reparameterize: if using z = f(x,y), then:
    //    param[0] = xyz_surf[0];  /* u = x */
    //    param[1] = xyz_surf[1];  /* v = y */

    return REF_IMPLEMENT;

  } else {
    return REF_INVALID;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_invert(REF_GEOM ref_geom, REF_INT type,
                                   REF_INT id, REF_DBL *xyz, REF_DBL *param) {
  /* Wrapper with more aggressive Newton iteration */
  return ref_ntop_inverse_eval(ref_geom, type, id, xyz, param);
}


/* ========================================
 * CURVATURE QUERIES
 * ======================================== */

REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  SUPRESS_UNUSED_COMPILER_WARNING(degen);  /* Handle singularities if needed */

  /* Compute principal curvatures for implicit surface f(x,y,z) = 0 */

  // ALGORITHM:
  // 1. Evaluate surface at (u,v) to get xyz
  // 2. Compute gradient: g = [∂f/∂x, ∂f/∂y, ∂f/∂z]
  // 3. Compute Hessian: H = [∂²f/∂xi∂xj]  (3x3 matrix)
  // 4. Normal: n = g / ||g||
  // 5. Shape operator: S = -H_projected / ||g||
  //    where H_projected = H - n⊗n (project to tangent plane)
  // 6. Compute eigenvalues of S → kr, ks
  // 7. Compute eigenvectors of S → r, s
  //
  // EXAMPLE:
  // void *face_obj = ntop_context->face_objects[faceid - 1];
  // REF_DBL xyz[3];
  // ref_ntop_eval_at(ref_geom, REF_GEOM_FACE, faceid, uv, xyz, NULL);
  //
  // REF_DBL gradient[3], hessian[9];
  // ntop_implicit_gradient(face_obj, xyz, gradient);
  // ntop_implicit_hessian(face_obj, xyz, hessian);
  //
  // /* Compute shape operator eigenvalues/vectors */
  // compute_principal_curvatures(gradient, hessian, kr, r, ks, s);

  /* TEMPORARY: Return small curvature (large elements) */
  *kr = 0.001;
  *ks = 0.001;
  r[0] = 1.0; r[1] = 0.0; r[2] = 0.0;
  s[0] = 0.0; s[1] = 1.0; s[2] = 0.0;

  return REF_IMPLEMENT;
}

REF_FCN REF_STATUS ref_ntop_face_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *kr, REF_DBL *r,
                                           REF_DBL *ks, REF_DBL *s) {
  REF_INT faceid, degen;
  REF_DBL uv[2];

  faceid = ref_geom_id(ref_geom, geom);
  degen = ref_geom_degen(ref_geom, geom);
  uv[0] = ref_geom_param(ref_geom, 0, geom);
  uv[1] = ref_geom_param(ref_geom, 1, geom);

  return ref_ntop_face_curvature_at(ref_geom, faceid, degen, uv,
                                    kr, r, ks, s);
}

REF_FCN REF_STATUS ref_ntop_edge_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *k, REF_DBL *normal) {
  REF_INT edgeid;
  REF_DBL t, xyz[3], tangent[3], dtangent[3];

  edgeid = ref_geom_id(ref_geom, geom);
  t = ref_geom_param(ref_geom, 0, geom);

  /* Evaluate curve derivatives */
  // ALGORITHM for curve r(t):
  // 1. Compute r'(t) and r''(t)
  // 2. Curvature: k = ||r'(t) × r''(t)|| / ||r'(t)||³
  // 3. Normal: n = (r''(t) - (r''·T)T) / ||...||
  //    where T = r'(t) / ||r'(t)||
  //
  // EXAMPLE:
  // ntop_curve_eval(edge_obj, t, xyz);
  // ntop_curve_first_deriv(edge_obj, t, tangent);    /* r'(t) */
  // ntop_curve_second_deriv(edge_obj, t, dtangent);  /* r''(t) */
  //
  // REF_DBL speed = sqrt(DOT(tangent, tangent));
  // REF_DBL cross[3];
  // CROSS(tangent, dtangent, cross);
  // *k = sqrt(DOT(cross, cross)) / (speed * speed * speed);
  //
  // /* Principal normal */
  // REF_DBL T[3] = {tangent[0]/speed, tangent[1]/speed, tangent[2]/speed};
  // REF_DBL proj = DOT(dtangent, T);
  // normal[0] = dtangent[0] - proj * T[0];
  // normal[1] = dtangent[1] - proj * T[1];
  // normal[2] = dtangent[2] - proj * T[2];
  // normalize(normal);

  /* TEMPORARY: Return small curvature */
  *k = 0.001;
  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 1.0;

  return REF_IMPLEMENT;
}


/* ========================================
 * FEATURE SIZE
 * ======================================== */

REF_FCN REF_STATUS ref_ntop_feature_size(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *h0, REF_DBL *dir0,
                                         REF_DBL *h1, REF_DBL *dir1,
                                         REF_DBL *h2, REF_DBL *dir2) {
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_INT item, geom;
  REF_DBL m[6];  /* Metric tensor (symmetric 3x3) */

  /* Initialize metric to identity */
  m[0] = 1.0; m[1] = 0.0; m[2] = 0.0;
  m[3] = 1.0; m[4] = 0.0; m[5] = 1.0;

  /* Accumulate contributions from all geometry touching this node */
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    REF_INT type = ref_geom_type(ref_geom, geom);

    if (REF_GEOM_FACE == type) {
      REF_DBL kr, r[3], ks, s[3];
      RSS(ref_ntop_face_curvature(ref_geom, geom, &kr, r, &ks, s), "curve");

      /* Add contribution to metric */
      // TODO: Intersect metric tensor with curvature-based metric

    } else if (REF_GEOM_EDGE == type) {
      REF_DBL k, n[3];
      RSS(ref_ntop_edge_curvature(ref_geom, geom, &k, n), "curve");

      /* Add contribution to metric */
      // TODO: Intersect metric tensor with edge curvature
    }
  }

  /* Compute eigendecomposition of metric tensor */
  // REF_DBL eigenvalues[3], eigenvectors[9];
  // compute_eigen(m, eigenvalues, eigenvectors);

  /* TEMPORARY: Return isotropic sizing */
  *h0 = 1.0;
  *h1 = 1.0;
  *h2 = 1.0;
  dir0[0] = 1.0; dir0[1] = 0.0; dir0[2] = 0.0;
  dir1[0] = 0.0; dir1[1] = 1.0; dir1[2] = 0.0;
  dir2[0] = 0.0; dir2[1] = 0.0; dir2[2] = 1.0;

  return REF_IMPLEMENT;
}


/* ========================================
 * TOPOLOGY & PROPERTIES
 * ======================================== */

REF_FCN REF_STATUS ref_ntop_edge_trange(REF_GEOM ref_geom, REF_INT id,
                                        REF_DBL *trange) {
  /* Return parameter range for edge [tmin, tmax] */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(id);

  /* Default: [0, 1] */
  trange[0] = 0.0;
  trange[1] = 1.0;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_tolerance(REF_GEOM ref_geom, REF_INT type,
                                      REF_INT id, REF_DBL *tolerance) {
  /* Return geometric tolerance for entity */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(type);
  SUPRESS_UNUSED_COMPILER_WARNING(id);

  /* Default: 1e-6 */
  *tolerance = 1.0e-6;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_gap(REF_GEOM ref_geom, REF_INT node,
                                REF_DBL *gap) {
  /* Measure distance between mesh node and geometry */
  REF_GRID ref_grid = ref_geom->grid;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_INT item, geom;
  REF_DBL max_gap = 0.0;

  each_ref_geom_having_node(ref_geom, node, item, geom) {
    REF_DBL xyz_geom[3], xyz_node[3];
    RSS(ref_ntop_eval(ref_geom, geom, xyz_geom, NULL), "eval");

    xyz_node[0] = ref_node_xyz(ref_node, 0, node);
    xyz_node[1] = ref_node_xyz(ref_node, 1, node);
    xyz_node[2] = ref_node_xyz(ref_node, 2, node);

    REF_DBL dist = sqrt(pow(xyz_geom[0] - xyz_node[0], 2) +
                       pow(xyz_geom[1] - xyz_node[1], 2) +
                       pow(xyz_geom[2] - xyz_node[2], 2));
    max_gap = MAX(max_gap, dist);
  }

  *gap = max_gap;
  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_diagonal(REF_GEOM ref_geom, REF_INT geom,
                                     REF_DBL *diag) {
  /* Return bounding box diagonal */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(geom);

  /* Default: 1.0 */
  *diag = 1.0;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_edge_face_uv(REF_GEOM ref_geom, REF_INT edgeid,
                                         REF_INT faceid, REF_INT sense,
                                         REF_DBL t, REF_DBL *uv) {
  /* Convert edge parameter to face UV */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(edgeid);
  SUPRESS_UNUSED_COMPILER_WARNING(faceid);
  SUPRESS_UNUSED_COMPILER_WARNING(sense);

  /* Default: use t for both u and v */
  uv[0] = t;
  uv[1] = t;

  return REF_SUCCESS;
}
```

---

### Step 3: Integrate into `ref_geom.c`

Modify the dispatch logic in `ref_geom_constrain()` (around [src/ref_geom.c:1555](src/ref_geom.c#L1555)):

```c
/* Add after checking meshlink */
if (ref_geom_ntop_loaded(ref_geom)) {
    RSS(ref_ntop_constrain(ref_grid, node), "ntop constrain");
    return REF_SUCCESS;
}
```

Add macro to `ref_geom.h`:
```c
#define ref_geom_ntop_loaded(ref_geom) \
  (NULL != (ref_geom)->context && NULL == (ref_geom)->model)
```

**Key files to modify:**

1. **[src/ref_geom.c](src/ref_geom.c)** - Lines 979, 1153, 1219, 1230, 1244, 1380, 1428, 1555, 1571, 1591, 1614, 1714
   - Replace `ref_egads_eval_at()` calls with dispatch:
   ```c
   if (ref_geom_ntop_loaded(ref_geom)) {
       RSS(ref_ntop_eval_at(...), "ntop eval");
   } else {
       RSS(ref_egads_eval_at(...), "egads eval");
   }
   ```

2. **[src/ref_metric.c](src/ref_metric.c)** - Lines 1387, 1452
   - Add nTop dispatch for curvature queries

---

### Step 4: Build System Integration

Modify `CMakeLists.txt` or `Makefile`:

```cmake
# Add nTop source files
set(REFINE_SOURCES
    ${REFINE_SOURCES}
    src/ref_ntop.c
)

# Link nTop Core library
target_link_libraries(refine ntop_core)

# Add include directory
target_include_directories(refine PRIVATE ${NTOP_CORE_INCLUDE_DIR})

# Optional: Compile with nTop flag
add_definitions(-DHAVE_NTOP)
```

Conditionally compile with:
```c
#ifdef HAVE_NTOP
  RSS(ref_ntop_load(ref_geom, filename), "load ntop");
#else
  RSS(ref_egads_load(ref_geom, filename), "load egads");
#endif
```

---

### Step 5: Testing Strategy

#### 5.1 Unit Tests

Create `src/ref_ntop_test.c`:

```c
#include "ref_ntop.h"
#include "ref_test.h"

int main(int argc, char *argv[]) {
  REF_GEOM ref_geom;
  REF_DBL xyz[3], params[2];

  /* Test 1: Initialize */
  REIS(REF_SUCCESS, ref_geom_create(&ref_geom), "create");
  REIS(REF_SUCCESS, ref_ntop_open(ref_geom), "open");

  /* Test 2: Load simple sphere */
  REIS(REF_SUCCESS, ref_ntop_load(ref_geom, "sphere.ntop"), "load");

  /* Test 3: Evaluate surface */
  params[0] = 0.5;  /* u */
  params[1] = 0.5;  /* v */
  REIS(REF_SUCCESS, ref_ntop_eval_at(ref_geom, REF_GEOM_FACE, 1,
                                     params, xyz, NULL), "eval");

  /* Test 4: Inverse projection */
  xyz[0] = 1.0; xyz[1] = 0.0; xyz[2] = 0.0;
  REIS(REF_SUCCESS, ref_ntop_inverse_eval(ref_geom, REF_GEOM_FACE, 1,
                                          xyz, params), "inverse");

  /* Test 5: Curvature */
  REF_DBL kr, r[3], ks, s[3];
  REIS(REF_SUCCESS, ref_ntop_face_curvature_at(ref_geom, 1, 0, params,
                                                &kr, r, &ks, s), "curve");
  printf("Sphere curvature: kr=%f, ks=%f\n", kr, ks);

  /* Clean up */
  REIS(REF_SUCCESS, ref_ntop_close(ref_geom), "close");
  REIS(REF_SUCCESS, ref_geom_free(ref_geom), "free");

  return 0;
}
```

#### 5.2 Integration Tests

Test files in `acceptance/`:

```bash
# Test 1: Sphere adaptation
./ref adapt sphere.meshb --ntop sphere.ntop --metric-curvature 1.0

# Test 2: Compare EGADS vs nTop
./ref adapt model.meshb --egads model.egads -o egads_result.meshb
./ref adapt model.meshb --ntop model.ntop -o ntop_result.meshb
./ref metric compare egads_result.meshb ntop_result.meshb
```

---

## Mathematical Details for Implicit Surfaces

### Implicit Surface Representation

nTop uses implicit functions: **f(x, y, z) = 0**

Example: Sphere of radius R:
```
f(x,y,z) = x² + y² + z² - R²
```

### Surface Evaluation

**Challenge**: Converting (u,v) parameters → (x,y,z) on surface

**Option 1**: Direct parameterization (if surface is z = g(x,y)):
```c
xyz[0] = u;
xyz[1] = v;
xyz[2] = solve f(u, v, z) = 0 for z;
```

**Option 2**: Use UV as seed for projection:
```c
REF_DBL seed[3] = {u, v, initial_guess_z};
project_to_surface(f, seed, xyz);  /* Minimize |f(xyz)| */
```

### Inverse Projection

Project point P to surface using gradient descent:

```c
/* Iterate until f(xyz) ≈ 0 */
for (iter = 0; iter < max_iter; iter++) {
    d = f(xyz);                    /* Signed distance */
    grad = ∇f(xyz);                /* Gradient [∂f/∂x, ∂f/∂y, ∂f/∂z] */
    xyz -= d * grad / ||grad||²;   /* Move toward surface */

    if (|d| < tolerance) break;
}

/* Reparameterize: u = xyz[0], v = xyz[1] */
param[0] = xyz[0];
param[1] = xyz[1];
```

### Principal Curvature Computation

For implicit surface f(x,y,z) = 0:

1. **Gradient** (normal direction):
   ```
   g = ∇f = [∂f/∂x, ∂f/∂y, ∂f/∂z]
   n = g / ||g||
   ```

2. **Hessian** (second derivatives):
   ```
   H = [∂²f/∂x², ∂²f/∂x∂y, ∂²f/∂x∂z]
       [∂²f/∂y∂x, ∂²f/∂y², ∂²f/∂y∂z]
       [∂²f/∂z∂x, ∂²f/∂z∂y, ∂²f/∂z²]
   ```

3. **Shape Operator** (projected Hessian):
   ```
   S = -H_projected / ||g||

   where H_projected = H - (n ⊗ n) H - H (n ⊗ n) + (n^T H n) (n ⊗ n)
   (projects H to tangent plane)
   ```

4. **Principal Curvatures**:
   ```
   Solve eigenvalue problem: S v = λ v

   kr, ks = eigenvalues (sorted: |kr| ≥ |ks|)
   r, s = eigenvectors (orthonormal, in tangent plane)
   ```

**Simplified for explicit form z = g(x,y)**:

```c
REF_DBL gx = ∂g/∂x, gy = ∂g/∂y;
REF_DBL gxx = ∂²g/∂x², gxy = ∂²g/∂x∂y, gyy = ∂²g/∂y²;

REF_DBL E = 1 + gx²;
REF_DBL F = gx * gy;
REF_DBL G = 1 + gy²;
REF_DBL denom = sqrt(E*G - F²);

REF_DBL L = gxx / denom;
REF_DBL M = gxy / denom;
REF_DBL N = gyy / denom;

/* Mean curvature */
H = (E*N - 2*F*M + G*L) / (2*(E*G - F²));

/* Gaussian curvature */
K = (L*N - M²) / (E*G - F²);

/* Principal curvatures */
kr = H + sqrt(H² - K);
ks = H - sqrt(H² - K);
```

---

## Parametrization Strategy for nTop

### Challenge

nTop implicit surfaces **don't have natural UV parameters**. You need to define a consistent parameterization.

### Recommended Approach

**Option A: Canonical Projection**

For each face, define a principal plane (XY, XZ, or YZ):

```c
/* Choose plane based on surface normal */
if (|nz| > |nx| && |nz| > |ny|) {
    u = x;   /* Project to XY plane */
    v = y;
} else if (|ny| > |nx|) {
    u = x;   /* Project to XZ plane */
    v = z;
} else {
    u = y;   /* Project to YZ plane */
    v = z;
}
```

**Option B: Geodesic Parameterization**

Compute UV coordinates by:
1. Start from seed point on surface
2. Define local tangent frame (u_dir, v_dir)
3. Store (u,v) as distance traveled along geodesics

**Option C: Bounding Box Normalization**

```c
/* Normalize to [0,1]² */
u = (x - xmin) / (xmax - xmin);
v = (y - ymin) / (ymax - ymin);
```

Store parameterization method in `REF_NTOP_CONTEXT`.

---

## Call Frequency Summary

| Function | Calls/Adaptation | Priority | Complexity |
|----------|------------------|----------|------------|
| `ref_ntop_eval_at` | 10,000+ | **CRITICAL** | Medium |
| `ref_ntop_inverse_eval` | 1,000-5,000 | **HIGH** | High |
| `ref_ntop_face_curvature_at` | 1,000+ | **HIGH** | High |
| `ref_ntop_edge_curvature` | 500-1,000 | **HIGH** | Medium |
| `ref_ntop_feature_size` | 1× per node | **MEDIUM** | Medium |
| `ref_ntop_gap` | 100-500 | **LOW** | Low |
| `ref_ntop_tolerance` | 10-100 | **LOW** | Low |

**Performance Notes:**
- Cache gradient/Hessian computations when possible
- Precompute parameterizations during load
- Use spatial acceleration (KD-tree, BVH) for inverse projection

---

## Debugging & Validation

### Visualization

Output geometry to Tecplot format:
```c
RSS(ref_geom_tec(ref_grid, "debug_geom.tec"), "output geom");
```

Check:
- Gap between mesh and geometry: `ref_geom_max_gap()`
- Parametric validity: `ref_geom_verify_param()`
- Normal alignment: `ref_geom_tri_norm_deviation()`

### Logging

Add debug output:
```c
#define REF_NTOP_DEBUG 1

#if REF_NTOP_DEBUG
  printf("ref_ntop_eval_at: type=%d id=%d uv=[%f,%f] → xyz=[%f,%f,%f]\n",
         type, id, params[0], params[1], xyz[0], xyz[1], xyz[2]);
#endif
```

### Acceptance Criteria

- **Gap < 1e-6**: Mesh matches geometry
- **Curvature range**: 0.001 < k < 100 (reasonable feature sizes)
- **Convergence**: Inverse projection converges in < 20 iterations
- **Consistency**: Forward + inverse eval returns original parameters

---

## Summary Checklist

- [ ] **Step 1**: Create `ref_ntop.h` header with function signatures
- [ ] **Step 2**: Implement `ref_ntop.c` skeleton with TODO markers
- [ ] **Step 3**: Integrate nTop Core library calls
- [ ] **Step 4**: Implement parameterization strategy
- [ ] **Step 5**: Implement curvature computation (Hessian eigenvalues)
- [ ] **Step 6**: Modify `ref_geom.c` dispatch logic
- [ ] **Step 7**: Modify `ref_metric.c` curvature queries
- [ ] **Step 8**: Update build system (CMake/Makefile)
- [ ] **Step 9**: Write unit tests (`ref_ntop_test.c`)
- [ ] **Step 10**: Run integration tests (simple geometries)
- [ ] **Step 11**: Validate gap, curvature, and convergence
- [ ] **Step 12**: Performance tuning and caching

---

## Next Steps

1. **Review nTop Core API documentation**
   - Identify equivalent functions for:
     - Surface evaluation
     - Gradient/Hessian computation
     - Signed distance queries
     - Projection to isosurface

2. **Start with simple geometry** (sphere, cylinder)
   - Verify curvature matches analytical values
   - Sphere of radius R should have kr = ks = 1/R

3. **Implement parameterization first**
   - Choose canonical projection method
   - Test forward/inverse consistency

4. **Add curvature computation**
   - Start with numerical differentiation
   - Optimize with analytical derivatives later

5. **Run acceptance tests**
   - Compare adapted meshes with EGADS baseline
   - Verify element quality and convergence

---

## Support & References

- **Refine docs**: See `README.md` and `acceptance/` tests
- **EGADS reference**: Study [src/ref_egads.c](src/ref_egads.c) implementation
- **FaceLift example**: See [src/ref_facelift.c](src/ref_facelift.c) for mesh-based geometry
- **Curvature math**: _Differential Geometry of Curves and Surfaces_ by Do Carmo

Good luck with the integration! Let me know if you need clarification on any step.
