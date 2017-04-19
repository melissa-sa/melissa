/**
*
* @file melissa_io.h
* @author Terraz Théophile
* @date 2017-15-01
*
**/

#ifndef MELISSA_IO_H
#define MELISSA_IO_H
#include "melissa_data.h"
#include "melissa_options.h"

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

void save_simu_states(int         *simu_states,
                      comm_data_t *comm_data,
                      int          size);

void read_ensight (melissa_options_t  *options,
                   comm_data_t      *comm_data,
                   double           *in_vect,
                   int              *local_vect_sizes,
                   char             *file_name);

#endif // MELISSA_IO_H
