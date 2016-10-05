/**
 *
 * @file stats_api.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 * @defgroup stats_api API
 *
 **/

#ifndef STATS_API_NO_MPI_H
#define STATS_API_NO_MPI_H

void connect_to_stats_no_mpi(int       *vect_size);

void connect_from_fortran_no_mpi(int       *local_vect_size,
                                 int       *comm_size,
                                 int       *rank);

void send_to_stats_no_mpi(const int  *time_step,
                          const char *field_name,
                          double     *send_vect);

void disconnect_from_stats();

#endif // STATS_API_NO_MPI_H
