/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
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
 * @file melissa/api.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 * @defgroup melissa_api API
 *
 **/

#ifndef MELISSA_API_H
#define MELISSA_API_H

#include <melissa/config.h>

#include <mpi.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

void melissa_init(const char *field_name,
                  const int  local_vect_size,
                  MPI_Comm   comm);

void melissa_init_f(const char *field_name,
                    int        *local_vect_size,
                    MPI_Fint   *comm_fortran);

void melissa_send(const char   *field_name,
                  const double *send_vect);

void melissa_finalize(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif // MELISSA_API_H
