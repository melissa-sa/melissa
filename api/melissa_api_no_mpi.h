/**
 *
 * @file melissa_api_no_mpi.h
 * @author Terraz Théophile
 * @date 2016-09-05
 *
 **/

#ifndef MELISSA_API_NO_MPI_H
#define MELISSA_API_NO_MPI_H

void melissa_init_no_mpi(const int *vect_size,
                         const int *sobol_rank,
                         const int *sample_id);

void melissa_send_no_mpi(const int  *time_step,
                         const char *field_name,
                         double     *send_vect,
                         const int  *sobol_rank,
                         const int  *sample_id);

void melissa_finalize();

#endif // MELISSA_API_NO_MPI_H
