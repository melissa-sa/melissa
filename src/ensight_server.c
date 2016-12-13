
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "stats.h"
#include "server.h"

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = 1;
}

int main (int argc, char **argv)
{
    stats_options_t     stats_options;
    stats_data_t       *data_ptr = NULL;
    field_ptr           field = NULL;
    comm_data_t         comm_data;
    int                 time_step;
    int                 i, j;
    char               *buffer;
    double            **buff_tab_ptr;
    int                 iteration = 0, nb_iterations = 0;
    const int           nb_fields = 2;
    char                field_name[2][255];
    int                *local_vect_sizes;
    pull_data_t         pull_data;
    char               *field_name_ptr;
    int                 rank_id, group_id;
    char                file_name[256];

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm_data.comm, &comm_data.comm_size);
    MPI_Comm_rank (comm_data.comm, &comm_data.rank);
#else
    comm_data.comm_size       = 1;
    comm_data.rank            = 0;
#endif // BUILD_WITH_MPI

#ifdef BUILD_WITH_PROBES
    start_time = stats_get_time();
#endif // BUILD_WITH_PROBES

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR2\n");

    stats_get_options (argc, argv, &stats_options);
    if (comm_data.rank == 0)
    {
        print_options (&stats_options);
        write_options (&stats_options);
    }

    stats_options.global_vect_size = 10;

    local_vect_sizes = malloc (comm_data.comm_size * sizeof(int));
    for (i=0; i<comm_data.comm_size; i++)
    {
        local_vect_sizes[i] = stats_options.global_vect_size / comm_data.comm_size;
        if (i < stats_options.global_vect_size % comm_data.comm_size)
            local_vect_sizes[i] += 1;
    }
    buffer = malloc (local_vect_sizes[comm_data.rank] * sizeof(double));

    nb_iterations = stats_options.nb_groups * stats_options.nb_time_steps ;
    sprintf(field_name[0], "HeatC");
    sprintf(field_name[1], "scalar1");
    for (i=0; i<nb_fields; i++)
    {
        add_field(&field, field_name[i], 1);
    }

    for (time_step=0; time_step<stats_options.nb_time_steps; time_step++)
    {
        for (group_id=0; group_id<stats_options.nb_groups; group_id++)
        {
            for (i=0; i<nb_fields; i++)
            {
                for (rank_id=0; rank_id<stats_options.nb_parameters+2; rank_id++)
                {
                    field_name_ptr = field_name[i];
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    if (data_ptr->is_valid != 1)
                    {
                        init_data (data_ptr, &stats_options, local_vect_sizes[comm_data.rank]);
                    }

                    sprintf(file_name, "results.%s.%.*d", field, 5, time_step +1);
#ifdef BUILD_WITH_PROBES
                    start_comm_time = stats_get_time();
#endif // BUILD_WITH_PROBES
                    read_ensight (stats_options, comm_data, buffer, local_vect_sizes, file_name);
#ifdef BUILD_WITH_PROBES
                    end_comm_time = stats_get_time();
                    total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
                    memcpy(buff_tab_ptr[rank_id], buffer, local_vect_sizes[comm_data.rank] * sizeof(double));

#ifdef BUILD_WITH_PROBES
                    start_computation_time = stats_get_time();
#endif // BUILD_WITH_PROBES

                    if (stats_options.sobol_op != 1)
                    {
                        buff_tab_ptr[0] = (double*)buffer;
                        compute_stats (data_ptr, time_step, 1, buff_tab_ptr);
                        iteration++;
                    }
                    else if (rank_id == stats_options.nb_parameters+1)
                    {
                        compute_stats (data_ptr, time_step, stats_options.nb_parameters+2, buff_tab_ptr);
                        iteration++;
                        confidence_sobol_martinez (&(data_ptr->sobol_indices[time_step]), stats_options.nb_parameters, local_vect_sizes[comm_data.rank]);
                        //                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                        //                                                                        0.01,
                        //                                                                        stats_options.nb_time_steps,
                        //                                                                        stats_options.nb_parameters);

#ifdef BUILD_WITH_PROBES
                        end_computation_time = stats_get_time();
                        total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
                        if (comm_data.rank==0 && (iteration % 10) == 0 )
                        {
                            fprintf(stderr, "iteration %d / %d  - field \"%s\"\n", iteration, nb_iterations*nb_fields, field_name_ptr);
                        }
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
                        MPI_Finalize ();
                        return 0;
                        break;
                    }
                }
            }
        }
    }

    if (stats_options.sobol_op == 1)
    {
        for (i=0; i<stats_options.nb_parameters+2; i++)
        {
            free (buff_tab_ptr[i]);
        }
    }
    free (buff_tab_ptr);

    if (end_signal == 0)
    {
        finalize_field_data (field, &comm_data, &pull_data, &stats_options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                            , &total_write_time
#endif // BUILD_WITH_PROBES
                            );
    }
    free (buffer);

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp1;
    long int temp2;
    MPI_Reduce (&total_comm_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    total_comm_time = temp1 / comm_data.comm_size;
    MPI_Reduce (&total_bytes_recv, &temp2, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    total_bytes_recv = temp2;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:           %d\n", stats_options.nb_simu);
        fprintf (stdout, " --- Number of simulation cores:      %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:        %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average reading time:            %g s\n", total_comm_time);
        fprintf (stdout, " --- Calcul time:                     %g s\n", total_computation_time);
        fprintf (stdout, " --- Writing time:                    %g s\n", total_write_time);
        fprintf (stdout, " --- Total time:                      %g s\n", stats_get_time() - start_time);
        fprintf (stdout, " --- Stats structures memory:         %ld bytes\n", mem_conso(&stats_options));
        fprintf (stdout, " --- Bytes written:                   %ld bytes\n", count_bytes_written(&stats_options)*nb_fields);
    }
#endif // BUILD_WITH_PROBES

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
