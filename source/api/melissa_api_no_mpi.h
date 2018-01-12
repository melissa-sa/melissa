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

void melissa_init_no_mpi(const char *field_name,
                         const int  *vect_size,
                         const int  *simu_id);

void melissa_send_no_mpi(const int  *time_step,
                         const char *field_name,
                         double     *send_vect,
                         const int  *simu_id);

void melissa_finalize(void);

#endif // MELISSA_API_NO_MPI_H