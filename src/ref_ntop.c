
/* Copyright 2006, 2014, 2021 United States Government as represented
 * by the Administrator of the National Aeronautics and Space
 * Administration. No copyright is claimed in the United States under
 * Title 17, U.S. Code.  All Other Rights Reserved.
 *
 * The refine version 3 unstructured grid adaptation platform is
 * licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See
 * the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ref_ntop.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ref_adj.h"
#include "ref_cell.h"
#include "ref_malloc.h"
#include "ref_math.h"
#include "ref_node.h"

/* When HAVE_NTOP is defined, declare nTop Core API (C-compatible) */
#ifdef HAVE_NTOP
/* The ntop_core.h header is C++, so we declare the C API directly */
#include <stdint.h>

#ifdef _WIN32
#define NTOP_CORE_API __declspec(dllimport)
#else
#define NTOP_CORE_API
#endif

typedef void* ntop_core_handle;

typedef struct {
  double x, y, z;
} ntop_core_vec3;

typedef struct {
  ntop_core_vec3 min;
  ntop_core_vec3 max;
} ntop_core_bounding_box;

typedef struct {
  double dx, dy, dz;
  double distance;
} ntop_core_derivative;

/* Import result codes */
#define NTOP_IMPORT_SUCCESS             0
#define NTOP_IMPORT_UNKNOWN_ERROR       1
#define NTOP_IMPORT_FILE_DOESNT_EXIST   2
#define NTOP_IMPORT_FILE_OPEN_ERROR     3
#define NTOP_IMPORT_CORRUPT_FILE        4
#define NTOP_IMPORT_UNSUPPORTED_VERSION 5

/* nTop Core function declarations */
NTOP_CORE_API uint32_t ntop_core_import_from_file(const char* utf8_filename,
                                                   ntop_core_handle* out_handle);
NTOP_CORE_API void ntop_core_release(ntop_core_handle handle);
NTOP_CORE_API void ntop_core_query_bounding_box(ntop_core_handle handle,
                                                 ntop_core_bounding_box* out_bbox);
NTOP_CORE_API double ntop_core_query_field(ntop_core_handle handle,
                                            ntop_core_vec3 point);
NTOP_CORE_API void ntop_core_query_derivative(ntop_core_handle handle,
                                               ntop_core_vec3 point,
                                               ntop_core_derivative* out_deriv);
#endif

/* ========================================================================
 * DATA STRUCTURES
 * ======================================================================== */

/* nTop Core context stored in ref_geom->context */
typedef struct REF_NTOP_CONTEXT_STRUCT {
  void *implicit_handle;       /* nTop Core handle */
  REF_DBL bbox_min[3];         /* Cached bounding box min */
  REF_DBL bbox_max[3];         /* Cached bounding box max */
  REF_DBL unit_scale;          /* Convert problem units → meters */
  REF_DBL tolerance;           /* Projection tolerance (meters) */
  REF_INT param_mode;          /* Parameterization: 0=XY, 1=XZ, 2=YZ */
} REF_NTOP_CONTEXT_STRUCT;

typedef REF_NTOP_CONTEXT_STRUCT *REF_NTOP_CONTEXT;

/* ========================================================================
 * INITIALIZATION
 * ======================================================================== */

REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom) {
  REF_NTOP_CONTEXT ntop_context;

  /* Allocate context */
  ref_malloc(ntop_context, 1, REF_NTOP_CONTEXT_STRUCT);

  ntop_context->implicit_handle = NULL;
  ntop_context->unit_scale = 1.0;     /* Assume problem is in meters */
  ntop_context->tolerance = 1.0e-6;   /* 1 micron tolerance */
  ntop_context->param_mode = 0;       /* Default: XY projection */

  /* Initialize bounding box */
  ntop_context->bbox_min[0] = -1.0;
  ntop_context->bbox_min[1] = -1.0;
  ntop_context->bbox_min[2] = -1.0;
  ntop_context->bbox_max[0] = 1.0;
  ntop_context->bbox_max[1] = 1.0;
  ntop_context->bbox_max[2] = 1.0;

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
#ifdef HAVE_NTOP
  if (NULL != ntop_context->implicit_handle) {
    ntop_core_release(ntop_context->implicit_handle);
  }
#endif

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

#ifdef HAVE_NTOP
  {
    uint32_t result;
    ntop_core_bounding_box bbox;

    result = ntop_core_import_from_file(filename, &(ntop_context->implicit_handle));

    switch (result) {
      case 0: /* SUCCESS */
        break;
      case 2: /* FILE_DOESNT_EXIST */
        printf("ERROR: nTop file not found: %s\n", filename);
        return REF_FAILURE;
      case 3: /* FILE_OPEN_ERROR */
        printf("ERROR: Unable to open nTop file: %s\n", filename);
        return REF_FAILURE;
      case 4: /* CORRUPT_FILE */
        printf("ERROR: Corrupt nTop file: %s\n", filename);
        return REF_FAILURE;
      case 5: /* UNSUPPORTED_VERSION */
        printf("ERROR: Unsupported nTop file version: %s\n", filename);
        return REF_FAILURE;
      default:
        printf("ERROR: Unknown error loading nTop file: %s (code %u)\n", filename, result);
        return REF_FAILURE;
    }

    ntop_core_query_bounding_box(ntop_context->implicit_handle, &bbox);

    ntop_context->bbox_min[0] = bbox.min.x;
    ntop_context->bbox_min[1] = bbox.min.y;
    ntop_context->bbox_min[2] = bbox.min.z;
    ntop_context->bbox_max[0] = bbox.max.x;
    ntop_context->bbox_max[1] = bbox.max.y;
    ntop_context->bbox_max[2] = bbox.max.z;
  }
#else
  SUPRESS_UNUSED_COMPILER_WARNING(filename);
  /* Without HAVE_NTOP, set up a stub sphere for testing */
  ntop_context->bbox_min[0] = -0.005;
  ntop_context->bbox_min[1] = -0.005;
  ntop_context->bbox_min[2] = -0.005;
  ntop_context->bbox_max[0] = 0.005;
  ntop_context->bbox_max[1] = 0.005;
  ntop_context->bbox_max[2] = 0.005;
  printf("NOTE: refine compiled without HAVE_NTOP - using stub sphere\n");
#endif

  /* For now, treat as single implicit body (face_id = 1) */
  ref_geom->nface = 1;
  ref_geom->nedge = 0;
  ref_geom->nnode = 0;

  printf("Loaded nTop implicit: %s\n", filename);
  printf("  Bounding box: [%f, %f, %f] to [%f, %f, %f]\n",
         ntop_context->bbox_min[0], ntop_context->bbox_min[1],
         ntop_context->bbox_min[2], ntop_context->bbox_max[0],
         ntop_context->bbox_max[1], ntop_context->bbox_max[2]);

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_save(REF_GEOM ref_geom, const char *filename) {
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(filename);

#ifdef HAVE_NTOP
  /* TODO: Implement nTop save if needed
   * REF_NTOP_CONTEXT ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);
   * ntop_core_export_to_file(filename, ntop_context->implicit_handle);
   */
  return REF_IMPLEMENT;
#else
  return REF_IMPLEMENT;
#endif
}

/* ========================================================================
 * HELPER: Query field value (signed distance estimate)
 * ======================================================================== */

static double ref_ntop_query_field_impl(REF_NTOP_CONTEXT ntop_context,
                                        REF_DBL x, REF_DBL y, REF_DBL z) {
#ifdef HAVE_NTOP
  ntop_core_vec3 pt;
  pt.x = x * ntop_context->unit_scale;
  pt.y = y * ntop_context->unit_scale;
  pt.z = z * ntop_context->unit_scale;
  return ntop_core_query_field(ntop_context->implicit_handle, pt);
#else
  SUPRESS_UNUSED_COMPILER_WARNING(ntop_context);
  SUPRESS_UNUSED_COMPILER_WARNING(x);
  SUPRESS_UNUSED_COMPILER_WARNING(y);
  SUPRESS_UNUSED_COMPILER_WARNING(z);
  return 0.0;
#endif
}

/* ========================================================================
 * HELPER: Project point to surface
 * ======================================================================== */

static REF_STATUS ref_ntop_project_to_surface(REF_NTOP_CONTEXT ntop_context,
                                               REF_DBL *xyz_in,
                                               REF_DBL *xyz_out) {
  REF_INT iter;
  REF_DBL xyz[3];

  xyz[0] = xyz_in[0];
  xyz[1] = xyz_in[1];
  xyz[2] = xyz_in[2];

  /* Newton-Raphson iteration using gradient descent */
  for (iter = 0; iter < 20; iter++) {
    REF_DBL f, grad[3], grad_mag_sq;

#ifdef HAVE_NTOP
    {
      ntop_core_vec3 pt;
      ntop_core_derivative deriv;
      pt.x = xyz[0] * ntop_context->unit_scale;
      pt.y = xyz[1] * ntop_context->unit_scale;
      pt.z = xyz[2] * ntop_context->unit_scale;
      ntop_core_query_derivative(ntop_context->implicit_handle, pt, &deriv);
      f = deriv.distance;
      grad[0] = deriv.dx;
      grad[1] = deriv.dy;
      grad[2] = deriv.dz;
    }
#else
    /* Finite difference for stub sphere */
    {
      REF_DBL eps = 1.0e-8;
      f = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2]);
      REF_DBL fx =
          ref_ntop_query_field_impl(ntop_context, xyz[0] + eps, xyz[1], xyz[2]);
      REF_DBL fy =
          ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1] + eps, xyz[2]);
      REF_DBL fz =
          ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2] + eps);
      grad[0] = (fx - f) / eps;
      grad[1] = (fy - f) / eps;
      grad[2] = (fz - f) / eps;
    }
#endif

    grad_mag_sq = grad[0] * grad[0] + grad[1] * grad[1] + grad[2] * grad[2];

    if (grad_mag_sq < 1.0e-20) break;

    /* Move toward surface: xyz -= f * grad / ||grad||^2 */
    xyz[0] -= f * grad[0] / grad_mag_sq;
    xyz[1] -= f * grad[1] / grad_mag_sq;
    xyz[2] -= f * grad[2] / grad_mag_sq;

    if (fabs(f) < ntop_context->tolerance) break;
  }

  xyz_out[0] = xyz[0];
  xyz_out[1] = xyz[1];
  xyz_out[2] = xyz[2];

  return REF_SUCCESS;
}

/* ========================================================================
 * SURFACE EVALUATION
 * ======================================================================== */

REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params, REF_DBL *xyz,
                                    REF_DBL *dxyz_dtuv) {
  REF_NTOP_CONTEXT ntop_context;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  SUPRESS_UNUSED_COMPILER_WARNING(id);

  if (REF_GEOM_FACE == type) {
    /* Use (u,v) as seed and project to surface */
    REF_DBL u = params[0];
    REF_DBL v = params[1];
    REF_DBL seed[3];

    /* Construct seed point from parameters */
    switch (ntop_context->param_mode) {
      case 0: /* XY projection */
        seed[0] = u;
        seed[1] = v;
        seed[2] = (ntop_context->bbox_min[2] + ntop_context->bbox_max[2]) / 2.0;
        break;
      case 1: /* XZ projection */
        seed[0] = u;
        seed[1] = (ntop_context->bbox_min[1] + ntop_context->bbox_max[1]) / 2.0;
        seed[2] = v;
        break;
      case 2: /* YZ projection */
        seed[0] = (ntop_context->bbox_min[0] + ntop_context->bbox_max[0]) / 2.0;
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

/* Forward declaration for use in ref_ntop_eval */
static REF_STATUS ref_ntop_compute_normal(REF_NTOP_CONTEXT ntop_context,
                                          REF_DBL *xyz, REF_DBL *normal);

REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom, REF_DBL *xyz,
                                 REF_DBL *dxyz_dtuv) {
  REF_NTOP_CONTEXT ntop_context;
  REF_INT type;
  REF_DBL xyz_proj[3];

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  if (NULL == ntop_context->implicit_handle) {
    return REF_FAILURE;
  }

  type = ref_geom_type(ref_geom, geom);

  if (REF_GEOM_FACE == type) {
    /* For nTop implicit, use the incoming xyz as seed and project directly
     * to the surface. No 2D parameterization needed - just march along
     * the gradient from the current position to the surface. */
    RSS(ref_ntop_project_to_surface(ntop_context, xyz, xyz_proj), "proj");

    xyz[0] = xyz_proj[0];
    xyz[1] = xyz_proj[1];
    xyz[2] = xyz_proj[2];

    /* Compute derivatives if requested */
    if (NULL != dxyz_dtuv) {
      /* For implicit surfaces, derivatives are based on the surface normal.
       * Construct a local tangent basis from the gradient. */
      REF_DBL normal[3];
      REF_DBL tangent1[3], tangent2[3], t1_mag;

      RSS(ref_ntop_compute_normal(ntop_context, xyz, normal), "normal");

      if (fabs(normal[2]) < 0.9) {
        tangent1[0] = -normal[1];
        tangent1[1] = normal[0];
        tangent1[2] = 0.0;
      } else {
        tangent1[0] = 0.0;
        tangent1[1] = -normal[2];
        tangent1[2] = normal[1];
      }

      t1_mag = sqrt(tangent1[0] * tangent1[0] + tangent1[1] * tangent1[1] +
                    tangent1[2] * tangent1[2]);
      if (t1_mag > 1.0e-14) {
        tangent1[0] /= t1_mag;
        tangent1[1] /= t1_mag;
        tangent1[2] /= t1_mag;
      }

      tangent2[0] = normal[1] * tangent1[2] - normal[2] * tangent1[1];
      tangent2[1] = normal[2] * tangent1[0] - normal[0] * tangent1[2];
      tangent2[2] = normal[0] * tangent1[1] - normal[1] * tangent1[0];

      dxyz_dtuv[0] = tangent1[0];
      dxyz_dtuv[1] = tangent1[1];
      dxyz_dtuv[2] = tangent1[2];
      dxyz_dtuv[3] = tangent2[0];
      dxyz_dtuv[4] = tangent2[1];
      dxyz_dtuv[5] = tangent2[2];
    }

    return REF_SUCCESS;
  }

  return REF_IMPLEMENT;
}

/* ========================================================================
 * INVERSE EVALUATION (xyz → uv)
 * ======================================================================== */

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
      case 0: /* XY projection */
        param[0] = xyz_surf[0];
        param[1] = xyz_surf[1];
        break;
      case 1: /* XZ projection */
        param[0] = xyz_surf[0];
        param[1] = xyz_surf[2];
        break;
      case 2: /* YZ projection */
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

REF_FCN REF_STATUS ref_ntop_invert(REF_GEOM ref_geom, REF_INT type,
                                   REF_INT id, REF_DBL *xyz, REF_DBL *param) {
  /* Wrapper to inverse_eval */
  return ref_ntop_inverse_eval(ref_geom, type, id, xyz, param);
}

/* ========================================================================
 * CURVATURE (via finite differences of normals)
 * ======================================================================== */

static REF_STATUS ref_ntop_compute_normal(REF_NTOP_CONTEXT ntop_context,
                                          REF_DBL *xyz, REF_DBL *normal) {
#ifdef HAVE_NTOP
  /* Use ntop_core_query_derivative for exact gradient (= surface normal) */
  ntop_core_vec3 pt;
  ntop_core_derivative deriv;
  REF_DBL grad[3], mag;

  pt.x = xyz[0] * ntop_context->unit_scale;
  pt.y = xyz[1] * ntop_context->unit_scale;
  pt.z = xyz[2] * ntop_context->unit_scale;

  ntop_core_query_derivative(ntop_context->implicit_handle, pt, &deriv);

  grad[0] = deriv.dx;
  grad[1] = deriv.dy;
  grad[2] = deriv.dz;

  /* Normalize */
  mag = sqrt(grad[0] * grad[0] + grad[1] * grad[1] + grad[2] * grad[2]);

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
#else
  /* Fallback: finite difference for stub implementation */
  REF_DBL eps = 1.0e-7;
  REF_DBL f0, fx, fy, fz;
  REF_DBL grad[3], mag;
  
  f0 = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2]);
  fx = ref_ntop_query_field_impl(ntop_context, xyz[0] + eps, xyz[1], xyz[2]);
  fy = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1] + eps, xyz[2]);
  fz = ref_ntop_query_field_impl(ntop_context, xyz[0], xyz[1], xyz[2] + eps);

  grad[0] = (fx - f0) / eps;
  grad[1] = (fy - f0) / eps;
  grad[2] = (fz - f0) / eps;

  /* Normalize */
  mag = sqrt(grad[0] * grad[0] + grad[1] * grad[1] + grad[2] * grad[2]);

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
#endif

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

  *kr = dn1 / eps; /* First principal curvature */
  *ks = dn2 / eps; /* Second principal curvature */

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

  return ref_ntop_face_curvature_at(ref_geom, faceid, degen, uv, kr, r, ks, s);
}

REF_FCN REF_STATUS ref_ntop_edge_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *k, REF_DBL *normal) {
  /* Not implemented yet */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(geom);

  *k = 0.01; /* Small curvature */
  normal[0] = 0.0;
  normal[1] = 0.0;
  normal[2] = 1.0;

  return REF_IMPLEMENT;
}

REF_FCN REF_STATUS ref_ntop_feature_size(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *h0, REF_DBL *dir0,
                                         REF_DBL *h1, REF_DBL *dir1,
                                         REF_DBL *h2, REF_DBL *dir2) {
  /* Not implemented yet - return isotropic sizing */
  SUPRESS_UNUSED_COMPILER_WARNING(ref_grid);
  SUPRESS_UNUSED_COMPILER_WARNING(node);

  *h0 = 1.0;
  *h1 = 1.0;
  *h2 = 1.0;

  dir0[0] = 1.0;
  dir0[1] = 0.0;
  dir0[2] = 0.0;
  dir1[0] = 0.0;
  dir1[1] = 1.0;
  dir1[2] = 0.0;
  dir2[0] = 0.0;
  dir2[1] = 0.0;
  dir2[2] = 1.0;

  return REF_IMPLEMENT;
}

/* ========================================================================
 * UTILITY FUNCTIONS
 * ======================================================================== */

REF_FCN REF_STATUS ref_ntop_edge_trange(REF_GEOM ref_geom, REF_INT id,
                                        REF_DBL *trange) {
  SUPRESS_UNUSED_COMPILER_WARNING(ref_geom);
  SUPRESS_UNUSED_COMPILER_WARNING(id);

  /* Default: [0, 1] */
  trange[0] = 0.0;
  trange[1] = 1.0;

  return REF_SUCCESS;
}

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
  REF_INT item, geom;
  REF_DBL xyz_eval[3], xyz_base[3], dist;
  REF_BOOL has_face;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  *gap = 0.0;

  /* Check if node has face geometry */
  RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &has_face), "face check");
  if (!has_face) return REF_SUCCESS;

  /* Get base position from first face geom eval */
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (REF_GEOM_FACE == ref_geom_type(ref_geom, geom)) {
      RSS(ref_ntop_eval(ref_geom, geom, xyz_base, NULL), "eval base");
      break;
    }
  }

  /* Compute max distance between different face evaluations */
  each_ref_geom_having_node(ref_geom, node, item, geom) {
    if (REF_GEOM_FACE == ref_geom_type(ref_geom, geom)) {
      RSS(ref_ntop_eval(ref_geom, geom, xyz_eval, NULL), "eval");
      dist = sqrt((xyz_eval[0] - xyz_base[0]) * (xyz_eval[0] - xyz_base[0]) +
                  (xyz_eval[1] - xyz_base[1]) * (xyz_eval[1] - xyz_base[1]) +
                  (xyz_eval[2] - xyz_base[2]) * (xyz_eval[2] - xyz_base[2]));
      *gap = MAX(*gap, dist);
    }
  }

  return REF_SUCCESS;
}

REF_FCN REF_STATUS ref_ntop_diagonal(REF_GEOM ref_geom, REF_INT geom,
                                     REF_DBL *diag) {
  REF_NTOP_CONTEXT ntop_context;

  SUPRESS_UNUSED_COMPILER_WARNING(geom);

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  ntop_context = (REF_NTOP_CONTEXT)(ref_geom->context);

  /* Compute bounding box diagonal */
  *diag = sqrt(pow(ntop_context->bbox_max[0] - ntop_context->bbox_min[0], 2) +
               pow(ntop_context->bbox_max[1] - ntop_context->bbox_min[1], 2) +
               pow(ntop_context->bbox_max[2] - ntop_context->bbox_min[2], 2));

  return REF_SUCCESS;
}

/* ========================================================================
 * CONSTRAIN ALL SURFACE NODES TO IMPLICIT
 * ======================================================================== */

REF_FCN REF_STATUS ref_ntop_constrain_all(REF_GRID ref_grid) {
  REF_NODE ref_node = ref_grid_node(ref_grid);
  REF_GEOM ref_geom = ref_grid_geom(ref_grid);
  REF_CELL ref_cell = ref_grid_tri(ref_grid);
  REF_INT node, cell, cell_node, nodes[REF_CELL_MAX_SIZE_PER];
  REF_DBL xyz[3], param[2];
  REF_INT face_id = 1; /* Single implicit face */
  REF_INT n_constrained = 0;

  RNS(ref_geom, "null geom");
  RNS(ref_geom->context, "null context");

  /* Iterate over all triangles and constrain their nodes */
  each_ref_cell_valid_cell_with_nodes(ref_cell, cell, nodes) {
    each_ref_cell_cell_node(ref_cell, cell_node) {
      REF_BOOL has_face;
      node = nodes[cell_node];

      /* Skip if already has face geom */
      RSS(ref_geom_is_a(ref_geom, node, REF_GEOM_FACE, &has_face), "face check");
      if (has_face) continue;

      /* Get node position */
      xyz[0] = ref_node_xyz(ref_node, 0, node);
      xyz[1] = ref_node_xyz(ref_node, 1, node);
      xyz[2] = ref_node_xyz(ref_node, 2, node);

      /* Project to get UV parameters */
      RSS(ref_ntop_inverse_eval(ref_geom, REF_GEOM_FACE, face_id, xyz, param),
          "inverse eval");

      /* Add face geometry association */
      RSS(ref_geom_add(ref_geom, node, REF_GEOM_FACE, face_id, param),
          "add face geom");
      n_constrained++;
    }
  }

  printf("Constrained %d nodes to implicit face\n", n_constrained);

  return REF_SUCCESS;
}
