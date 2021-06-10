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

#ifndef SERVER_OUTPUT_H
#define SERVER_OUTPUT_H

#include <melissa/server/data.h>
#include <melissa/server/options.h>
#include <melissa/utils.h>
#include <melissa/vector.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void melissa_write_stats_seq(melissa_data_t    **data,
                             melissa_options_t  *options,
                             comm_data_t        *comm_data,
                             const char         *field);

void write_stats_txt(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_bin(melissa_data_t    **data,
                     melissa_options_t  *options,
                     comm_data_t        *comm_data,
                     char               *field);

void write_stats_ensight(melissa_data_t    **data,
                         melissa_options_t  *options,
                         comm_data_t        *comm_data,
                         char               *field);



void write_output_d(const char   *file_name,
                        const char   *field,
                        const char   *statisics_name,
                        const int     t,
                        const size_t  vec_size,
                        const double  vec[]);

void write_output_i(const char   *file_name,
                        const char   *field,
                        const char   *statisics_name,
                        const int     t,
                        const size_t  vec_size,
                        const int     vec[]);

void write_simu_param (vector_t *simulations,
                       int       nb_parameters);

#ifdef __cplusplus
}
#endif

#endif // SERVER_OUTPUT_H
