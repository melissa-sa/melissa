/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

/**
 *
 * @file melissa_api.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 * @defgroup melissa_api API
 *
 **/

#ifndef MELISSA_API_H
#define MELISSA_API_H

#include <mpi.h>

#define MELISSA_COUPLING_NONE 0    /**< No coupling */
#define MELISSA_COUPLING_DEFAULT 0 /**< Default coupling */
#define MELISSA_COUPLING_ZMQ 0     /**< ZeroMQ coupling */
#define MELISSA_COUPLING_MPI 1     /**< MPI coupling */
#define MELISSA_COUPLING_FLOWVR 2  /**< FlowVR coupling */

#ifdef __cplusplus
extern "C" {
#endif

#define MELISSA_MAJOR_NUM @Melissa_VERSION_MAJOR@
#define MELISSA_MINOR_NUM @Melissa_VERSION_MINOR@
#define MELISSA_RELEASE_NUM @Melissa_VERSION_PATCH@

void melissa_init_mpi (const char *field_name,
                       const int  vect_size,
                       MPI_Comm comm,
                       const int  coupling);

void melissa_init(const char *field_name,
                  const int  local_vect_size,
                  const int  comm_size,
                  const int  rank,
                  const int  simu_id,
                  MPI_Comm   comm,
                  const int  coupling);

void melissa_init_f(const char *field_name,
                    int        *local_vect_size,
                    int        *comm_size,
                    int        *rank,
                    const int  *simu_id,
                    MPI_Fint   *comm_fortran,
                    int        *coupling);

void melissa_send(const char   *field_name,
                  const double *send_vect);

void melissa_finalize(void);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_API_H
