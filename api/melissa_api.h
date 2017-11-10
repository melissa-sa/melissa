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


#ifdef BUILD_WITH_MPI
#include <mpi.h>

void melissa_init(const char *field_name,
                  const int  *local_vect_size,
                  const int  *comm_size,
                  const int  *rank,
                  const int  *sobol_rank,
                  const int  *sample_id,
                  MPI_Comm   *comm,
                  const int  *coupling);

void melissa_init_f(const char *field_name,
                    int        *local_vect_size,
                    int        *comm_size,
                    int        *rank,
                    const int  *sobol_rank,
                    const int  *sample_id,
                    MPI_Fint   *comm_fortran,
                    int        *coupling);

void melissa_send(const int  *time_step,
                  const char *field_name,
                  double     *send_vect,
                  const int  *rank,
                  const int  *sobol_rank,
                  const int  *sample_id);

void melissa_finalize();
#endif // BUILD_WITH_MPI

#endif // MELISSA_API_H
