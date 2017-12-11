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
 * @file server.h
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 **/

#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H
#include "melissa_fields.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI

int check_simu_state(melissa_field_t *field,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data);

long int count_mbytes_written (melissa_options_t  *options);

int string_recv (void *socket,
                 char *recv_buff);

void global_confidence_sobol_martinez(melissa_field_t *field,
                                      int              nb_fields,
                                      comm_data_t     *comm_data,
                                      double          *interval1,
                                      double          *interval_tot);

#endif // SERVER_HELPER_H
