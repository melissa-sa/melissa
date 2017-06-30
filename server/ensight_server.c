
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include "server.h"
#include "melissa_io.h"
#include "compute_stats.h"
#include "melissa_options.h"
#include "melissa_data.h"
#include "melissa_utils.h"

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = 1;
}

int main (int argc, char **argv)
{
    melissa_options_t   melissa_options;
    melissa_data_t     *data_ptr = NULL;
    field_ptr           field = NULL;
    comm_data_t         comm_data;
    int                 time_step;
    int                 i;
    double             *buffer;
    double            **buff_tab_ptr;
    int                 iteration = 0, nb_iterations = 0;
    const int           nb_fields = 1;
    char                field_name[1][255];
    int                *local_vect_sizes;
    pull_data_t         pull_data;
    char               *field_name_ptr;
    int                 rank_id, group_id;
    char                file_name[256];
#ifdef BUILD_WITH_PROBES
    double              start_time;
    double              total_read_time = 0;
    double              start_comm_time;
    double              end_comm_time;
    double              total_computation_time = 0;
    double              start_computation_time;
    double              end_computation_time;
    double              total_write_time = 0;
#endif // BUILD_WITH_PROBES

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm_data.comm, &comm_data.comm_size);
    MPI_Comm_rank (comm_data.comm, &comm_data.rank);
#else
    comm_data.comm_size       = 1;
    comm_data.rank            = 0;
#endif // BUILD_WITH_MPI
    comm_data.client_comm_size = 1;
    comm_data.rcounts = melissa_malloc (sizeof(int));
    comm_data.rcounts[0] = 1;
    comm_data.rdispls = melissa_malloc (sizeof(int));
    comm_data.rdispls[0] = 0;

#ifdef BUILD_WITH_PROBES
    start_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR2\n");

    melissa_get_options (argc, argv, &melissa_options);
    if (comm_data.rank == 0)
    {
        melissa_print_options (&melissa_options);
        melissa_write_options (&melissa_options);
    }

    melissa_options.global_vect_size = 10;

    local_vect_sizes = melissa_malloc (comm_data.comm_size * sizeof(int));
    for (i=0; i<comm_data.comm_size; i++)
    {
        local_vect_sizes[i] = melissa_options.global_vect_size / comm_data.comm_size;
        if (i < melissa_options.global_vect_size % comm_data.comm_size)
            local_vect_sizes[i] += 1;
    }
    buffer = melissa_malloc (local_vect_sizes[comm_data.rank] * sizeof(double));

    if (melissa_options.sobol_op == 1)
    {
        buff_tab_ptr = melissa_malloc ((melissa_options.nb_parameters + 2) * sizeof(double*));
        for (i=0; i<melissa_options.nb_parameters+2; i++)
        {
            buff_tab_ptr[i] = melissa_malloc (local_vect_sizes[comm_data.rank] * sizeof(double));
        }
    }
    else
    {
        buff_tab_ptr = melissa_malloc (sizeof(double*));
    }

    nb_iterations = melissa_options.sampling_size * melissa_options.nb_time_steps ;
    sprintf(field_name[0], "scalar1");
    for (i=0; i<nb_fields; i++)
    {
        fprintf(stdout, "add field %s\n", field_name[i]);
        add_field(&field, field_name[i], 1);
    }

    for (time_step=0; time_step<melissa_options.nb_time_steps; time_step++)
    {
        for (group_id=0; group_id<melissa_options.sampling_size; group_id++)
        {
            for (i=0; i<nb_fields; i++)
            {
                for (rank_id=0; rank_id<melissa_options.nb_parameters+2; rank_id++)
                {
                    field_name_ptr = field_name[i];
                    fprintf(stdout, " field_name_ptr = %s\n", field_name_ptr);
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    if (data_ptr->is_valid != 1)
                    {
                        melissa_init_data (data_ptr, &melissa_options, local_vect_sizes[comm_data.rank]);
                    }

                    sprintf(file_name, "results.%s.%.*d", field_name_ptr, 5, time_step +1);
#ifdef BUILD_WITH_PROBES
//                    fprintf(stdout, " Read results.%s.%.*d\n", field_name_ptr, 5, time_step +1);
                    start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
                    read_ensight (&melissa_options, &comm_data, buffer, local_vect_sizes, file_name);
#ifdef BUILD_WITH_PROBES
                    end_comm_time = melissa_get_time();
                    total_read_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
                    if (melissa_options.sobol_op == 1)
                    {
                        memcpy(buff_tab_ptr[rank_id], buffer, local_vect_sizes[comm_data.rank] * sizeof(double));
                    }

#ifdef BUILD_WITH_PROBES
//                    fprintf(stdout, " compute results...\n");
                    start_computation_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

                    if (melissa_options.sobol_op != 1)
                    {
                        buff_tab_ptr[0] = (double*)buffer;
                        compute_stats (data_ptr, time_step, 1, buff_tab_ptr, group_id);
                        iteration++;
                    }
                    else if (rank_id == melissa_options.nb_parameters+1)
                    {
                        compute_stats (data_ptr, time_step, melissa_options.nb_parameters+2, buff_tab_ptr, group_id);
                        iteration++;
                        confidence_sobol_martinez (&(data_ptr->sobol_indices[time_step]), melissa_options.nb_parameters, local_vect_sizes[comm_data.rank]);
                        //                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                        //                                                                        0.01,
                        //                                                                        melissa_options.nb_time_steps,
                        //                                                                        melissa_options.nb_parameters);

                    }
#ifdef BUILD_WITH_PROBES
                    end_computation_time = melissa_get_time();
                    total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
                    if (comm_data.rank==0 && (iteration % 10) == 0 )
                    {
                        fprintf(stderr, "iteration %d / %d  - field \"%s\"\n", iteration, nb_iterations*nb_fields, field_name_ptr);
                    }

                    if (end_signal != 0)
                    {
                        field_ptr fptr = field;
                        if (comm_data.rank == 0)
                            fprintf (stderr, "\nINTERUPTED\n");
                        while (fptr != NULL)
                        {
                            save_stats (fptr->stats_data, &comm_data, fptr->name);
                            fptr = field->next;
                            if (comm_data.rank == 0)
                            {
                                char dir[256];
                                getcwd(dir, 256*sizeof(char));
                                fprintf(stderr, "\nstatistic fields saved in %s\n", dir);
                            }
                        }
#ifdef BUILD_WITH_MPI
                        MPI_Finalize ();
#endif // BUILD_WITH_MPI
                        return 0;
                        break;
                    }
#ifdef BUILD_WITH_MPI
                    MPI_Barrier(comm_data.comm);
#endif // BUILD_WITH_MPI
                }
            }
        }
    }

    if (melissa_options.sobol_op == 1)
    {
        for (i=0; i<melissa_options.nb_parameters+2; i++)
        {
            melissa_free (buff_tab_ptr[i]);
        }
    }
    melissa_free (buff_tab_ptr);

    if (end_signal == 0)
    {
        finalize_field_data (field, &comm_data, &pull_data, &melissa_options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                            , &total_write_time
#endif // BUILD_WITH_PROBES
                            );
    }
    melissa_free (buffer);

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp1;
    MPI_Reduce (&total_read_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, comm_data.comm);
    total_read_time = temp1 / comm_data.comm_size;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:           %d\n", melissa_options.nb_simu);
        fprintf (stdout, " --- Number of simulation cores:      %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:        %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average reading time:            %g s\n", total_read_time);
        fprintf (stdout, " --- Calcul time:                     %g s\n", total_computation_time);
        fprintf (stdout, " --- Writing time:                    %g s\n", total_write_time);
        fprintf (stdout, " --- Total time:                      %g s\n", melissa_get_time() - start_time);
        fprintf (stdout, " --- Stats structures memory:         %ld bytes\n", mem_conso(&melissa_options));
        fprintf (stdout, " --- Bytes written:                   %ld bytes\n", count_mbytes_written(&melissa_options)*nb_fields);
    }
#endif // BUILD_WITH_PROBES

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
