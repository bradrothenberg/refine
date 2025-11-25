# nTop Core Concrete Implementation Guide

Based on the actual nTop Core API documentation and examples.

## nTop Core API Summary

The nTop Core library provides these key functions for working with implicit surfaces:

### Data Types

```c
typedef struct {
    double x, y, z;
} ntop_core_vec3;

typedef struct {
    ntop_core_vec3 min;  // Minimum corner
    ntop_core_vec3 max;  // Maximum corner
} ntop_core_bounding_box;

typedef enum {
    SUCCESS = 0,
    FILE_DOESNT_EXIST = 2,
    FILE_OPEN_ERROR = 3,
    CORRUPT_FILE = 4,
    UNSUPPORTED_VERSION = 5,
    OUT_OF_MEMORY = 6
} ntop_core_import_result;
```

### Core Functions

```c
// Import/export
uint32_t ntop_core_import_from_file(const char* path, void** handle_out);
void ntop_core_release(void* handle);
uint32_t ntop_core_export_to_file(const char* path, void* handle);

// Library version
void ntop_core_library_version(int* major, int* minor, int* patch);

// Geometry queries
void ntop_core_query_bounding_box(void* handle, ntop_core_bounding_box* bbox);
double ntop_core_query_field(void* handle, ntop_core_vec3 point);

// Closest point projection (callback-based)
typedef void (*ntop_core_query_closest_point_array_cb)(
    void* context,
    ntop_core_vec3 const* closest_points,
    uint32_t n_points,
    uint32_t const* bad_indices,  // Indices that didn't converge
    uint32_t n_bad_indices
);

void ntop_core_query_closest_point_array(
    void* handle,
    ntop_core_vec3* query_points,
    uint32_t n_query_points,
    float tolerance,  // In meters
    void* callback_context,
    ntop_core_query_closest_point_array_cb callback
);

// Transformation
void ntop_core_transformed(void* input_handle,
                           ntop_core_frame* frame,
                           void** output_handle);

void ntop_core_scaled(void* input_handle,
                      ntop_core_vec3* scale_factors,
                      ntop_core_vec3* fixed_point,
                      void** output_handle);
```

**IMPORTANT**: All units are in **meters**. Convert from your problem units!

### Key Limitations

1. **No gradient/Hessian API exposed** - Must use finite differences
2. **No explicit parameterization** - Implicit surfaces don't have UV coords
3. **Callback-based results** - Closest point returns via callback
4. **Field is approximate distance** - Not exact signed distance

---

## Implementation Strategy

### Approach: Parameterize via Projection

Since implicit surfaces don't have natural (u,v) parameters, we'll use:
- **Parameters (u,v)** = projected coordinates on principal plane
- **Evaluation** = project (u,v,z_guess) onto implicit surface
- **Inverse** = extract (u,v) from projected point

### Example: Parameterization for a sphere

```c
// Forward: (u,v) → (x,y,z) on surface
// Use (u,v) as seed, project to surface
xyz_seed = {u, v, sqrt(R² - u² - v²)};  // Initial guess
xyz_surface = closest_point_on_implicit(xyz_seed);

// Inverse: (x,y,z) → (u,v)
// Extract dominant coordinates
u = x;
v = y;
// (Could also use spherical coords θ,φ for better coverage)
```

---

## Concrete Implementation

### ref_ntop.h (Add to existing header)

```c
#ifndef REF_NTOP_H
#define REF_NTOP_H

#include "ref_defs.h"
#include "ref_geom.h"

// Include nTop Core header
#include <ntop_core/ntop_core.h>

BEGIN_C_DECLORATION

/* nTop Core context structure */
typedef struct REF_NTOP_CONTEXT_STRUCT {
  void *implicit_handle;         /* nTop Core handle */
  ntop_core_bounding_box bbox;   /* Cached bounding box */
  REF_DBL unit_scale;            /* Convert problem units → meters */
  REF_DBL tolerance;             /* Projection tolerance (meters) */

  /* Parameterization strategy */
  REF_INT param_mode;            /* 0=XY, 1=XZ, 2=YZ projection */
} REF_NTOP_CONTEXT_STRUCT;

typedef REF_NTOP_CONTEXT_STRUCT *REF_NTOP_CONTEXT;

/* Initialize nTop context */
REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom);
REF_FCN REF_STATUS ref_ntop_close(REF_GEOM ref_geom);

/* Load implicit surface */
REF_FCN REF_STATUS ref_ntop_load(REF_GEOM ref_geom, const char *filename);

/* Core geometric queries */
REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params,
                                    REF_DBL *xyz, REF_DBL *dxyz_dtuv);

REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom,
                                 REF_DBL *xyz, REF_DBL *dxyz_dtuv);

REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param);

/* Curvature queries */
REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s);

REF_FCN REF_STATUS ref_ntop_face_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *kr, REF_DBL *r,
                                           REF_DBL *ks, REF_DBL *s);

/* Utility */
REF_FCN REF_STATUS ref_ntop_tolerance(REF_GEOM ref_geom, REF_INT type,
                                      REF_INT id, REF_DBL *tolerance);

REF_FCN REF_STATUS ref_ntop_gap(REF_GEOM ref_geom, REF_INT node,
                                REF_DBL *gap);

END_C_DECLORATION

#endif /* REF_NTOP_H */
```

### ref_ntop.c (Concrete implementation)

```c
#include "ref_ntop.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_node.h"

/* =================================================================
 * INITIALIZATION
 * ================================================================= */

REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom) {
  REF_NTOP_CONTEXT ntop_context;

  /* Allocate context */
  ref_malloc(ntop_context, 1, REF_NTOP_CONTEXT_STRUCT);

  ntop_context->implicit_handle = NULL;
  ntop_context->unit_scale = 1.0;  /* Assume problem is in meters */
  ntop_context->tolerance = 1.0e-6;  /* 1 micron tolerance */
  ntop_context->param_mode = 0;  /* Default: XY projection */

  /* Store context in ref_geom */
  ref_geom->context = (void *)ntop_context;
  ref_geom->contex_owned = REF_TRUE;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_close(REF_GEOM ref_geom) {
  REF_NTOP_CONTEXT ntop_context;

  if (NULL == ref_geom->context) return REF_SUCCESS;

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  /* Release nTop Core handle */
  if (NULL != ntop_context->implicit_handle) {
    ntop_core_release(ntop_context->implicit_handle);
  }

  ref_free(ntop_context);
  ref_geom->context = NULL;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_load(REF_GEOM ref_geom, const char *filename) {
  REF_NTOP_CONTEXT ntop_context;
  uint32_t result;

  if (NULL == ref_geom->context) {
    RSS(ref_ntop_open(ref_geom), "open ntop");
  }

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  /* Load implicit file */
  result = ntop_core_import_from_file(filename,
                                      &(ntop_context->implicit_handle));

  /* Handle errors */
  switch (result) {
    case 0:  /* SUCCESS */
      break;
    case 2:  /* FILE_DOESNT_EXIST */
      printf("ERROR: nTop file not found: %s\n", filename);
      return REF_FAILURE;
    case 3:  /* FILE_OPEN_ERROR */
      printf("ERROR: Cannot open nTop file: %s\n", filename);
      return REF_FAILURE;
    case 4:  /* CORRUPT_FILE */
      printf("ERROR: Corrupt nTop file: %s\n", filename);
      return REF_FAILURE;
    case 5:  /* UNSUPPORTED_VERSION */
      printf("ERROR: Unsupported nTop file version: %s\n", filename);
      return REF_FAILURE;
    case 6:  /* OUT_OF_MEMORY */
      printf("ERROR: Out of memory loading: %s\n", filename);
      return REF_FAILURE;
    default:
      printf("ERROR: Unknown error loading: %s\n", filename);
      return REF_FAILURE;
  }

  /* Query bounding box */
  ntop_core_query_bounding_box(ntop_context->implicit_handle,
                               &(ntop_context->bbox));

  /* For now, treat as single implicit body (face_id = 1) */
  ref_geom->nface = 1;
  ref_geom->nedge = 0;
  ref_geom->nnode = 0;

  printf("Loaded nTop implicit: %s\n", filename);
  printf("  Bounding box: [%f, %f, %f] to [%f, %f, %f]\n",
         ntop_context->bbox.min.x, ntop_context->bbox.min.y,
         ntop_context->bbox.min.z,
         ntop_context->bbox.max.x, ntop_context->bbox.max.y,
         ntop_context->bbox.max.z);

  return REF_SUCCESS;
}

/* =================================================================
 * HELPER: Query field value (signed distance estimate)
 * ================================================================= */

static double ref_ntop_query_field_impl(REF_NTOP_CONTEXT ntop_context,
                                        REF_DBL x, REF_DBL y, REF_DBL z) {
  ntop_core_vec3 pt;
  pt.x = x * ntop_context->unit_scale;
  pt.y = y * ntop_context->unit_scale;
  pt.z = z * ntop_context->unit_scale;

  return ntop_core_query_field(ntop_context->implicit_handle, pt);
}

/* =================================================================
 * HELPER: Project point to surface using closest_point query
 * ================================================================= */

typedef struct {
  ntop_core_vec3 result;
  REF_BOOL converged;
} ref_ntop_projection_result;

static void ref_ntop_closest_point_callback(
    void *context,
    ntop_core_vec3 const *closest_points,
    uint32_t n_points,
    uint32_t const *bad_indices,
    uint32_t n_bad_indices) {

  ref_ntop_projection_result *result = (ref_ntop_projection_result *)context;

  if (n_points > 0) {
    result->result = closest_points[0];
    result->converged = REF_TRUE;

    /* Check if this point failed convergence */
    for (uint32_t i = 0; i < n_bad_indices; i++) {
      if (bad_indices[i] == 0) {
        result->converged = REF_FALSE;
        break;
      }
    }
  }
}

static REF_STATUS ref_ntop_project_to_surface(REF_NTOP_CONTEXT ntop_context,
                                               REF_DBL *xyz_in,
                                               REF_DBL *xyz_out) {
  ntop_core_vec3 query_pt;
  ref_ntop_projection_result result;

  /* Scale to meters */
  query_pt.x = xyz_in[0] * ntop_context->unit_scale;
  query_pt.y = xyz_in[1] * ntop_context->unit_scale;
  query_pt.z = xyz_in[2] * ntop_context->unit_scale;

  result.converged = REF_FALSE;

  /* Query closest point */
  ntop_core_query_closest_point_array(
      ntop_context->implicit_handle,
      &query_pt,
      1,  /* One point */
      (float)(ntop_context->tolerance),
      &result,
      ref_ntop_closest_point_callback);

  if (!result.converged) {
    printf("WARNING: Closest point did not converge for (%f, %f, %f)\n",
           xyz_in[0], xyz_in[1], xyz_in[2]);
  }

  /* Scale back from meters */
  xyz_out[0] = result.result.x / ntop_context->unit_scale;
  xyz_out[1] = result.result.y / ntop_context->unit_scale;
  xyz_out[2] = result.result.z / ntop_context->unit_scale;

  return REF_SUCCESS;
}

/* =================================================================
 * SURFACE EVALUATION
 * ================================================================= */

REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params,
                                    REF_DBL *xyz, REF_DBL *dxyz_dtuv) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  SUPRESS_UNUSED_COMPILER_WARNING(id);  /* Only one implicit body for now */

  if (REF_GEOM_FACE == type) {
    /* Use (u,v) as seed and project to surface */
    REF_DBL u = params[0];
    REF_DBL v = params[1];
    REF_DBL seed[3];

    /* Construct seed point from parameters */
    switch (ntop_context->param_mode) {
      case 0:  /* XY projection */
        seed[0] = u;
        seed[1] = v;
        /* Estimate z from bounding box or solve field=0 */
        seed[2] = (ntop_context->bbox.min.z + ntop_context->bbox.max.z) / 2.0;
        break;
      case 1:  /* XZ projection */
        seed[0] = u;
        seed[1] = (ntop_context->bbox.min.y + ntop_context->bbox.max.y) / 2.0;
        seed[2] = v;
        break;
      case 2:  /* YZ projection */
        seed[0] = (ntop_context->bbox.min.x + ntop_context->bbox.max.x) / 2.0;
        seed[1] = u;
        seed[2] = v;
        break;
      default:
        return REF_INVALID;
    }

    /* Project seed to surface */
    RSS(ref_ntop_project_to_surface(ntop_context, seed, xyz), "project");

    /* Compute derivatives if requested */
    if (NULL != dxyz_dtuv) {
      /* Finite difference approximation */
      REF_DBL eps = 1.0e-7;
      REF_DBL params_du[2] = {u + eps, v};
      REF_DBL params_dv[2] = {u, v + eps};
      REF_DBL xyz_du[3], xyz_dv[3];

      RSS(ref_ntop_eval_at(ref_geom, type, id, params_du, xyz_du, NULL),
          "eval du");
      RSS(ref_ntop_eval_at(ref_geom, type, id, params_dv, xyz_dv, NULL),
          "eval dv");

      /* dxyz/du */
      dxyz_dtuv[0] = (xyz_du[0] - xyz[0]) / eps;
      dxyz_dtuv[1] = (xyz_du[1] - xyz[1]) / eps;
      dxyz_dtuv[2] = (xyz_du[2] - xyz[2]) / eps;

      /* dxyz/dv */
      dxyz_dtuv[3] = (xyz_dv[0] - xyz[0]) / eps;
      dxyz_dtuv[4] = (xyz_dv[1] - xyz[1]) / eps;
      dxyz_dtuv[5] = (xyz_dv[2] - xyz[2]) / eps;
    }

    return REF_SUCCESS;

  } else if (REF_GEOM_EDGE == type) {
    /* For edges, not implemented yet */
    return REF_IMPLEMENT;

  } else {
    return REF_INVALID;
  }
}

REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom,
                                 REF_DBL *xyz, REF_DBL *dxyz_dtuv) {
  REF_INT type, id;
  REF_DBL params[2];

  type = ref_geom_type(ref_geom, geom);
  id = ref_geom_id(ref_geom, geom);

  params[0] = ref_geom_param(ref_geom, 0, geom);
  params[1] = ref_geom_param(ref_geom, 1, geom);

  return ref_ntop_eval_at(ref_geom, type, id, params, xyz, dxyz_dtuv);
}

/* =================================================================
 * INVERSE EVALUATION (xyz → uv)
 * ================================================================= */

REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  SUPRESS_UNUSED_COMPILER_WARNING(id);

  if (REF_GEOM_FACE == type) {
    /* Project to surface first */
    REF_DBL xyz_surf[3];
    RSS(ref_ntop_project_to_surface(ntop_context, xyz, xyz_surf), "project");

    /* Extract parameters from projected point */
    switch (ntop_context->param_mode) {
      case 0:  /* XY projection */
        param[0] = xyz_surf[0];
        param[1] = xyz_surf[1];
        break;
      case 1:  /* XZ projection */
        param[0] = xyz_surf[0];
        param[1] = xyz_surf[2];
        break;
      case 2:  /* YZ projection */
        param[0] = xyz_surf[1];
        param[1] = xyz_surf[2];
        break;
      default:
        return REF_INVALID;
    }

    return REF_SUCCESS;

  } else {
    return REF_IMPLEMENT;
  }
}

/* =================================================================
 * CURVATURE (via finite differences of normals)
 * ================================================================= */

static REF_STATUS ref_ntop_compute_normal(REF_NTOP_CONTEXT ntop_context,
                                          REF_DBL *xyz, REF_DBL *normal) {
  /* Compute gradient of field (approximate normal) */
  REF_DBL eps = 1.0e-7;
  REF_DBL f0 = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2]);
  REF_DBL fx = ref_ntop_query_field_impl(ntop_context, xyz[0] + eps, xyz[1], xyz[2]);
  REF_DBL fy = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1] + eps, xyz[2]);
  REF_DBL fz = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2] + eps);

  REF_DBL grad[3];
  grad[0] = (fx - f0) / eps;
  grad[1] = (fy - f0) / eps;
  grad[2] = (fz - f0) / eps;

  /* Normalize */
  REF_DBL mag = sqrt(grad[0] * grad[0] + grad[1] * grad[1] + grad[2] * grad[2]);

  if (mag < 1.0e-14) {
    /* Degenerate - return arbitrary normal */
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = 1.0;
  } else {
    normal[0] = grad[0] / mag;
    normal[1] = grad[1] / mag;
    normal[2] = grad[2] / mag;
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s) {
  REF_NTOP_CONTEXT ntop_context;
  REF_DBL xyz[3], normal[3];
  REF_DBL eps = 1.0e-6;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  SUPRESS_UNUSED_COMPILER_WARNING(faceid);
  SUPRESS_UNUSED_COMPILER_WARNING(degen);

  /* Evaluate surface at (u,v) */
  RSS(ref_ntop_eval_at(ref_geom, REF_GEOM_FACE, 1, uv, xyz, NULL), "eval");

  /* Compute normal at center point */
  RSS(ref_ntop_compute_normal(ntop_context, xyz, normal), "normal");

  /* Estimate curvature via normal variation */
  /* Sample normals in two perpendicular directions */
  REF_DBL tangent1[3], tangent2[3];

  /* Construct tangent basis (perpendicular to normal) */
  if (fabs(normal[2]) < 0.9) {
    tangent1[0] = -normal[1];
    tangent1[1] = normal[0];
    tangent1[2] = 0.0;
  } else {
    tangent1[0] = 0.0;
    tangent1[1] = -normal[2];
    tangent1[2] = normal[1];
  }

  /* Normalize tangent1 */
  REF_DBL t1_mag = sqrt(tangent1[0] * tangent1[0] + tangent1[1] * tangent1[1] +
                        tangent1[2] * tangent1[2]);
  tangent1[0] /= t1_mag;
  tangent1[1] /= t1_mag;
  tangent1[2] /= t1_mag;

  /* tangent2 = normal × tangent1 */
  tangent2[0] = normal[1] * tangent1[2] - normal[2] * tangent1[1];
  tangent2[1] = normal[2] * tangent1[0] - normal[0] * tangent1[2];
  tangent2[2] = normal[0] * tangent1[1] - normal[1] * tangent1[0];

  /* Sample normals at offset points */
  REF_DBL xyz1[3], xyz2[3], n1[3], n2[3];

  xyz1[0] = xyz[0] + eps * tangent1[0];
  xyz1[1] = xyz[1] + eps * tangent1[1];
  xyz1[2] = xyz[2] + eps * tangent1[2];
  RSS(ref_ntop_project_to_surface(ntop_context, xyz1, xyz1), "proj1");
  RSS(ref_ntop_compute_normal(ntop_context, xyz1, n1), "n1");

  xyz2[0] = xyz[0] + eps * tangent2[0];
  xyz2[1] = xyz[1] + eps * tangent2[1];
  xyz2[2] = xyz[2] + eps * tangent2[2];
  RSS(ref_ntop_project_to_surface(ntop_context, xyz2, xyz2), "proj2");
  RSS(ref_ntop_compute_normal(ntop_context, xyz2, n2), "n2");

  /* Curvature ≈ Δnormal / Δposition */
  REF_DBL dn1 = sqrt((n1[0] - normal[0]) * (n1[0] - normal[0]) +
                     (n1[1] - normal[1]) * (n1[1] - normal[1]) +
                     (n1[2] - normal[2]) * (n1[2] - normal[2]));

  REF_DBL dn2 = sqrt((n2[0] - normal[0]) * (n2[0] - normal[0]) +
                     (n2[1] - normal[1]) * (n2[1] - normal[1]) +
                     (n2[2] - normal[2]) * (n2[2] - normal[2]));

  *kr = dn1 / eps;  /* First principal curvature */
  *ks = dn2 / eps;  /* Second principal curvature */

  /* Principal directions */
  r[0] = tangent1[0];
  r[1] = tangent1[1];
  r[2] = tangent1[2];

  s[0] = tangent2[0];
  s[1] = tangent2[1];
  s[2] = tangent2[2];

  return REF_SUCCESS;
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

/* =================================================================
 * UTILITY FUNCTIONS
 * ================================================================= */

REF_FCN REF_STATUS ref_ntop_tolerance(REF_GEOM ref_geom, REF_INT type,
                                      REF_INT id, REF_DBL *tolerance) {
  REF_NTOP_CONTEXT ntop_context;

  SUPRESS_UNUSED_COMPILER_WARNING(type);
  SUPRESS_UNUSED_COMPILER_WARNING(id);

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);
  *tolerance = ntop_context->tolerance;

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_gap(REF_GEOM ref_geom, REF_INT node,
                                REF_DBL *gap) {
  REF_GRID ref_grid = ref_geom->grid;
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_NTOP_CONTEXT ntop_context;
  REF_DBL xyz_node[3], xyz_surf[3], dist;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  xyz_node[0] = ref_node_xyz(ref_node, 0, node);
  xyz_node[1] = ref_node_xyz(ref_node, 1, node);
  xyz_node[2] = ref_node_xyz(ref_node, 2, node);

  RSS(ref_ntop_project_to_surface(ntop_context, xyz_node, xyz_surf),
      "project");

  dist = sqrt((xyz_surf[0] - xyz_node[0]) * (xyz_surf[0] - xyz_node[0]) +
              (xyz_surf[1] - xyz_node[1]) * (xyz_surf[1] - xyz_node[1]) +
              (xyz_surf[2] - xyz_node[2]) * (xyz_surf[2] - xyz_node[2]));

  *gap = dist;
  return REF_SUCCESS;
}
```

---

## Test Sphere File

You have: `c:\cplusplus\ntop\refine\nTop\data\TestSphere.implicit`

This is a 5mm radius sphere (in the assets folder there's also `sphere_5mm_radius.implicit`).

---

## Next Steps

1. **Copy the implementation above** to `src/ref_ntop.h` and `src/ref_ntop.c`

2. **Create CMake integration** (see next section)

3. **Test with the sphere**:
```bash
./ref_ntop_test
```

4. **Run a simple adaptation**:
```bash
./ref adapt sphere_coarse.meshb --ntop ../nTop/data/TestSphere.implicit -o sphere_adapted.meshb
```

Let me know when you're ready and I'll help you with the build system integration and testing!
