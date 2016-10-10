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

void connect_to_stats(const int *local_vect_size,
                      const int *comm_size,
                      const int *rank,
                      MPI_Comm  *comm);

void connect_to_stats_sobol(const int *local_vect_size,
                            const int *comm_size,
                            const int *rank,
                            const int *sobol_rank,
                            const int *sobol_group,
                            MPI_Comm  *comm);

void connect_from_fortran(int       *local_vect_size,
                          int       *comm_size,
                          int       *rank,
                          MPI_Fint  *comm);

void send_to_stats(const int  *time_step,
                   const char *field_name,
                   double     *send_vect,
                   const int  *rank);

void send_to_stats_sobol(const int  *time_step,
                         const char *field_name,
                         double     *send_vect,
                         const int  *rank,
                         const int  *sobol_rank,
                         const int  *sobol_group);

void disconnect_from_stats();

void disconnect_from_stats_sobol();

#endif // STATS_API_H
