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
 * @file melissa_api_no_mpi.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 **/

#ifndef MELISSA_API_NO_MPI_H
#define MELISSA_API_NO_MPI_H

#define MELISSA_COUPLING_NONE 0    /**< No coupling */
#define MELISSA_COUPLING_DEFAULT 0 /**< Default coupling */
#define MELISSA_COUPLING_ZMQ 0     /**< ZeroMQ coupling */
#define MELISSA_COUPLING_MPI 1     /**< MPI coupling */
#define MELISSA_COUPLING_FLOWVR 2  /**< FlowVR coupling */

#ifdef __cplusplus
extern "C" {
#endif

void melissa_init_no_mpi(const char *field_name,
                         const int  vect_size,
                         const int  simu_id,
                         const int  coupling);

void melissa_send_no_mpi(const char *field_name,
                         const double *send_vect);

void melissa_finalize(void);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_API_NO_MPI_H
