/**
 *
 * @file stats_api.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 * @defgroup stats_api API
 *
 **/

#include <mpi.h>

#ifndef STATS_API_H
#define STATS_API_H

void connect_to_stats(const int *nb_parameters,
                      int       *local_vect_size,
                      int       *comm_size,
                      int       *rank,
                      MPI_Comm  *comm);

void connect_from_fortran(const int *nb_parameters,
                          int       *local_vect_size,
                          int       *comm_size,
                          int       *rank,
                          MPI_Fint  *comm);

void send_to_stats(const int  *time_step,
                   const int  *parameters,
                   const int  *nb_parameters,
                   const char *field_name,
                   double     *send_vect,
                   const int  *rank);

void disconnect_from_stats();

#endif // STATS_API_H
