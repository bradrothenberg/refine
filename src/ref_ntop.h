
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef REF_NTOP_H
#define REF_NTOP_H

#include "ref_defs.h"

BEGIN_C_DECLORATION

END_C_DECLORATION

#include "ref_geom.h"
#include "ref_grid.h"

BEGIN_C_DECLORATION

/* Initialize nTop context and load geometry */
REF_FCN REF_STATUS ref_ntop_open(REF_GEOM ref_geom);

/* Clean up nTop context */
REF_FCN REF_STATUS ref_ntop_close(REF_GEOM ref_geom);

/* Load implicit surface definition from file */
REF_FCN REF_STATUS ref_ntop_load(REF_GEOM ref_geom, const char *filename);
/* Load edge curves from STEP file */
REF_FCN REF_STATUS ref_ntop_load_step_edges(REF_GEOM ref_geom, const char *filename);


/* Save implicit surface definition to file */
REF_FCN REF_STATUS ref_ntop_save(REF_GEOM ref_geom, const char *filename);

/* Evaluate surface/curve at parametric location */
REF_FCN REF_STATUS ref_ntop_eval_at(REF_GEOM ref_geom, REF_INT type,
                                    REF_INT id, REF_DBL *params, REF_DBL *xyz,
                                    REF_DBL *dxyz_dtuv);

/* Evaluate at stored parametric location */
REF_FCN REF_STATUS ref_ntop_eval(REF_GEOM ref_geom, REF_INT geom, REF_DBL *xyz,
                                 REF_DBL *dxyz_dtuv);

/* Inverse evaluation: project point to surface */
REF_FCN REF_STATUS ref_ntop_inverse_eval(REF_GEOM ref_geom, REF_INT type,
                                         REF_INT id, REF_DBL *xyz,
                                         REF_DBL *param);

/* Alternative inverse eval (wrapper) */
REF_FCN REF_STATUS ref_ntop_invert(REF_GEOM ref_geom, REF_INT type,
                                   REF_INT id, REF_DBL *xyz, REF_DBL *param);

/* Compute principal curvatures on a face */
REF_FCN REF_STATUS ref_ntop_face_curvature_at(REF_GEOM ref_geom,
                                               REF_INT faceid, REF_INT degen,
                                               REF_DBL *uv, REF_DBL *kr,
                                               REF_DBL *r, REF_DBL *ks,
                                               REF_DBL *s);

/* Compute curvature at stored face geom */
REF_FCN REF_STATUS ref_ntop_face_curvature(REF_GRID ref_grid, REF_INT geom,
                                           REF_DBL *kr, REF_DBL *r,
                                           REF_DBL *ks, REF_DBL *s);

/* Compute curvature on an edge (1D curve) */
REF_FCN REF_STATUS ref_ntop_edge_curvature(REF_GEOM ref_geom, REF_INT geom,
                                           REF_DBL *k, REF_DBL *normal);

/* Compute local feature size at a node */
REF_FCN REF_STATUS ref_ntop_feature_size(REF_GRID ref_grid, REF_INT node,
                                         REF_DBL *h0, REF_DBL *dir0,
                                         REF_DBL *h1, REF_DBL *dir1,
                                         REF_DBL *h2, REF_DBL *dir2);

/* Get parameter range for an edge */
REF_FCN REF_STATUS ref_ntop_edge_trange(REF_GEOM ref_geom, REF_INT id,
                                        REF_DBL *trange);

/* Get geometric tolerance for entity */
REF_FCN REF_STATUS ref_ntop_tolerance(REF_GEOM ref_geom, REF_INT type,
                                      REF_INT id, REF_DBL *tolerance);

/* Measure gap between surface and node */
REF_FCN REF_STATUS ref_ntop_gap(REF_GEOM ref_geom, REF_INT node, REF_DBL *gap);

/* Get diagonal of bounding box */
REF_FCN REF_STATUS ref_ntop_diagonal(REF_GEOM ref_geom, REF_INT geom,
                                     REF_DBL *diag);

/* Associate all surface nodes with the implicit face (for curvature metrics) */
REF_FCN REF_STATUS ref_ntop_constrain_all(REF_GRID ref_grid);

/* Associate boundary edge nodes with edge curves from STEP file */
REF_FCN REF_STATUS ref_ntop_constrain_edges(REF_GRID ref_grid);

END_C_DECLORATION

#endif /* REF_NTOP_H */
