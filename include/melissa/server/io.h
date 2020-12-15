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
* @file melissa/server/io.h
* @author Terraz Théophile
* @date 2017-15-01
*
**/

#ifndef MELISSA_IO_H
#define MELISSA_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <melissa/server/data.h>
#include <melissa/server/options.h>

void write_stats_bin(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_txt(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_ensight(melissa_data_t    **data,
                         melissa_options_t  *options,
                         comm_data_t        *comm_data,
                         char               *field);

void save_stats (melissa_data_t *data,
                 comm_data_t    *comm_data,
                 char           *field_name);

void read_saved_stats (melissa_data_t *data,
                       comm_data_t    *comm_data,
                       char           *field_name,
                       int             client_rank);

void save_simu_states (vector_t    *simu_states,
                       comm_data_t *comm_data);

void read_simu_states (vector_t          *simu_states,
                       melissa_options_t *options,
                       comm_data_t       *comm_data);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_IO_H