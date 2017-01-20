/**
*
* @file melissa_io.h
* @author Terraz Théophile
* @date 2017-15-01
*
**/

#ifndef SOBOL_H
#define SOBOL_H
#include "melissa_data.h"

void write_stats(melissa_data_t    **data,
                 melissa_options_t  *options,
                 comm_data_t        *comm_data,
                 int                *local_vect_sizes,
                 char               *field);

void write_stats_ensight(melissa_data_t    **data,
                         melissa_options_t  *options,
                         comm_data_t        *comm_data,
                         int                *local_vect_sizes,
                         char               *field);

void write_client_data (int *client_comm_size,
                        int *client_vect_sizes);

int read_client_data (int  *client_comm_size,
                      int **client_vect_sizes);

void save_stats (melissa_data_t *data,
                 comm_data_t    *comm_data,
                 char           *field_name);

void read_saved_stats (melissa_data_t *data,
                       comm_data_t    *comm_data,
                       char           *field_name,
                       int             client_rank);

#endif // SOBOL_H
