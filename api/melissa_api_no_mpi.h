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
 * @file melissa_api_no_mpi.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 **/

#ifndef MELISSA_API_NO_MPI_H
#define MELISSA_API_NO_MPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define MELISSA_MAJOR_NUM @Melissa_VERSION_MAJOR@
#define MELISSA_MINOR_NUM @Melissa_VERSION_MINOR@
#define MELISSA_RELEASE_NUM @Melissa_VERSION_PATCH@

void melissa_init_no_mpi(const char *field_name,
                         const int  vect_size);

void melissa_init_no_mpi_f (const char *field_name,
                            const int  *vect_size);

void melissa_send_no_mpi(const char *field_name,
                         const double *send_vect);

void melissa_finalize(void);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_API_NO_MPI_H
