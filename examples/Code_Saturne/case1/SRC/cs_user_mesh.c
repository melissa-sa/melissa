/*============================================================================
 * Definition of the calculation mesh.
 *
 * Mesh-related user functions (called in this order):
 *   1) Manage the exchange of data between Code_Saturne and the pre-processor
 *   2) Define (conforming or non-conforming) mesh joinings.
 *   3) Define (conforming or non-conforming) periodicity.
 *   4) Define thin walls.
 *   5) Modify the geometry and mesh.
 *   6) Smooth the mesh.
 *============================================================================*/

/* Code_Saturne version 4.0 */

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2013 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * BFT library headers
 *----------------------------------------------------------------------------*/

#include "bft_error.h"
#include "bft_mem.h"
#include "bft_printf.h"

/*----------------------------------------------------------------------------
 * FVM library headers
 *----------------------------------------------------------------------------*/

#include "fvm_defs.h"
#include "fvm_selector.h"

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_base.h"
#include "cs_join.h"
#include "cs_join_perio.h"
#include "cs_mesh.h"
#include "cs_mesh_quantities.h"
#include "cs_mesh_bad_cells.h"
#include "cs_mesh_smoother.h"
#include "cs_mesh_thinwall.h"
#include "cs_mesh_warping.h"
#include "cs_parall.h"
#include "cs_post.h"
#include "cs_preprocessor_data.h"
#include "cs_selector.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_prototypes.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

static int _n_tubes_x = 3;
static int _n_tubes_y = 2;

/*============================================================================
 * User function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 * Define mesh files to read and optional associated transformations.
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_input(void)
{
  const char *path = "mesh_input";

  int i, j, k;
  const int n = _n_tubes_x, m = _n_tubes_y;
  char _renames[64][8];
  char *renames[8];

  for (i = 0; i < n; i++) {
    for (j = 0; j < m; j++) {

      int n_renames = 0;

      if (i > 0) {
        strcpy(_renames[n_renames*2], "left");
        strcpy(_renames[n_renames*2+1], "join");
        n_renames++;
      }
      if (i < n-1) {
        strcpy(_renames[n_renames*2], "right");
        strcpy(_renames[n_renames*2+1], "join");
        n_renames++;
      }
      if (j > 0) {
        strcpy(_renames[n_renames*2], "bottom");
        strcpy(_renames[n_renames*2+1], "join");
        n_renames++;
      }
      if (j < m-1) {
        strcpy(_renames[n_renames*2], "top");
        strcpy(_renames[n_renames*2+1], "join");
        n_renames++;
      }

      for (k = 0; k < n_renames*2; k++)
        renames[k] = _renames[k];

      const double transf_matrix[3][4] = {{1., 0., 0., -0.0225*(n-1) + 0.0450*i},
                                          {0., 1., 0., -0.0225*(m-1) + 0.0450*j},
                                          {0., 0., 1., 0.}};

      cs_preprocessor_data_add_file(path,
                                    n_renames, renames,
                                    transf_matrix);
    }
  }
}

/*----------------------------------------------------------------------------
 * Define mesh joinings.
 *
 * This is done by calling the cs_join_add() function for each joining
 * operation to add.
 *
 * The arguments to cs_join_add() are:
 *   sel_criteria <-- boundary face selection criteria string
 *   fraction     <-- value of the fraction parameter;
 *                    the initial tolerance radius associated to each vertex
 *                    is equal to the lenght of the shortest incident edge,
 *                    multiplied by this fraction.
 *   plane        <-- value of the plane parameter;
 *                    when subdividing faces, 2 faces are considered
 *                    coplanar and may be joined if angle between their
 *                    normals (in degrees) does not exceed this parameter.
 *   verbosity    <-- level of verbosity required
 *
 * The function returns a number (1 to n) associated with the
 * new joining. This number may optionnally be used to assign advanced
 * parameters to the joining.
 *----------------------------------------------------------------------------*/

void
cs_user_join(void)
{
  int    join_num;

  /* Add a joining operation */
  /* ----------------------- */

  int    verbosity = 1;     /* per-task dump if > 1, debug level if >= 3 */
  int    visualization = 1; /* debug level if >= 3 */
  float  fraction = 0.10, plane = 25.;

  join_num = cs_join_add("join",
                         fraction,
                         plane,
                         verbosity,
                         visualization);
}

/*----------------------------------------------------------------------------
 * Define periodic faces.
 *
 * This is done by calling one of the cs_join_perio_add_*() functions for
 * each periodicity to add.
 *
 * The first arguments to cs_join_perio_add_() are the same as for
 * mesh joining:
 *   sel_criteria <-- boundary face selection criteria string
 *   fraction     <-- value of the fraction parameter;
 *                    the initial tolerance radius associated to each vertex
 *                    is equal to the lenght of the shortest incident edge,
 *                    multiplied by this fraction.
 *   plane        <-- value of the plane parameter;
 *                    when subdividing faces, 2 faces are considered
 *                    coplanar and may be joined if angle between their
 *                    normals (in degrees) does not exceed this parameter.
 *   verbosity    <-- level of verbosity required
 *
 * The last arguments depend on the type of periodicity to define,
 * and are described below.
 *
 * The function returns a number (1 to n) associated with the
 * new joining. This number may optionnally be used to assign advanced
 * parameters to the joining.
 *----------------------------------------------------------------------------*/

void
cs_user_periodicity(void)
{
  int    join_num;

  int    verbosity = 1;     /* per-task dump if > 1, debug level if >= 3 */
  int    visualization = 1; /* debug level if >= 3 */
  float  fraction = 0.10, plane = 25.;

  const double translation[3] = {0.0,
                                 0.0450*_n_tubes_y,
                                 0.0}; /* Translation vector */

  join_num = cs_join_perio_add_translation("bottom or top",
                                           fraction,
                                           plane,
                                           verbosity,
                                           visualization,
                                           translation);
}

/*----------------------------------------------------------------------------
 * Set options for cutting of warped faces
 *
 * parameters:
 *   mesh <-> pointer to mesh structure to smoothe
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_warping(void)
{
}

/*----------------------------------------------------------------------------
 * Insert thin wall into a mesh.
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_thinwall(cs_mesh_t  *mesh)
{
}

/*----------------------------------------------------------------------------
 * Modify geometry and mesh.
 *
 * The mesh structure is described in cs_mesh.h
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_modify(cs_mesh_t  *mesh)
{
}

/*----------------------------------------------------------------------------
 * Mesh smoothing.
 *
 * parameters:
 *   mesh <-> pointer to mesh structure to smoothe
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_smoothe(cs_mesh_t  *mesh)
{
}

/*----------------------------------------------------------------------------
 * Enable or disable mesh saving.
 *
 * By default, mesh is saved when modified.
 *
 * parameters:
 *   mesh <-> pointer to mesh structure
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_save(cs_mesh_t  *mesh)
{
  /* Mark mesh as not modified (0) to disable saving;
     Mark it as modified (> 0) to force saving */

  mesh->modified = 0;
}

/*----------------------------------------------------------------------------
 * Tag bad cells within the mesh based on user-defined geometric criteria.
 *
 * The mesh structure is described in cs_mesh.h
 *----------------------------------------------------------------------------*/

void
cs_user_mesh_bad_cells_tag(cs_mesh_t             *mesh,
                           cs_mesh_quantities_t  *mesh_quantities)
{
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
