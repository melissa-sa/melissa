
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
    int                 i;
    const int           nb_fields = 1;
    char                field_name[1][255];
    int                *client_vect_sizes = NULL, *local_vect_sizes;
    pull_data_t         pull_data;
    char               *field_name_ptr;
#ifdef BUILD_WITH_PROBES
    double              start_time = 0;
    double              total_read_time = 0;
    double              start_read_time = 0;
    double              end_read_time = 0;
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
    comm_data.client_comm_size = 32;

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

    melissa_options.global_vect_size = 9603840;

    local_vect_sizes = melissa_malloc (comm_data.comm_size * sizeof(int));
    for (i=0; i<comm_data.comm_size; i++)
    {
        local_vect_sizes[i] = melissa_options.global_vect_size / comm_data.comm_size;
        if (i < melissa_options.global_vect_size % comm_data.comm_size)
            local_vect_sizes[i] += 1;
    }

    fprintf (stdout, "reading data files...");
    if (comm_data.rank == 0)
    {
        read_client_data(&comm_data.client_comm_size, &client_vect_sizes, &melissa_options);
    }
#ifdef BUILD_WITH_MPI
    MPI_Bcast(&comm_data.client_comm_size, 1, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
    if (comm_data.rank != 0)
    {
        client_vect_sizes = melissa_malloc (comm_data.client_comm_size * sizeof(int));
    }
    fprintf (stdout, " ok \n");
    sprintf(field_name[0], "scalar1");

    comm_data.rcounts = melissa_calloc (comm_data.client_comm_size, sizeof(int));
    comm_data.rdispls = melissa_calloc (comm_data.client_comm_size, sizeof(int));

    comm_n_to_m_init (comm_data.rcounts,
                      comm_data.rdispls,
                      melissa_options.global_vect_size,
                      local_vect_sizes,
                      client_vect_sizes,
                      comm_data.client_comm_size,
                      comm_data.rank,
                      &pull_data);
    field_name_ptr = field_name[0];
    add_field(&field, field_name_ptr, comm_data.client_comm_size, melissa_options.sampling_size);
    data_ptr = get_data_ptr (field, field_name_ptr);
    for (i=0; i<comm_data.client_comm_size; i++)
    {
        if (comm_data.rcounts[i] > 0)
        {
            melissa_init_data (&data_ptr[i], &melissa_options, comm_data.rcounts[i]);
            fprintf (stdout, "reading checkpoint files (server %d, client %d) ... ", comm_data.rank, i);
#ifdef BUILD_WITH_PROBES
            melissa_get_time(start_read_time);
#endif // BUILD_WITH_PROBES
            read_saved_stats (data_ptr, &comm_data, field_name_ptr, i);
#ifdef BUILD_WITH_PROBES
            melissa_get_time(end_read_time);
            total_read_time = end_read_time - start_read_time;
#endif // BUILD_WITH_PROBES
            fprintf (stdout, " ok\n");
        }
    }


    if (end_signal == 0)
    {
        finalize_field_data (field, &comm_data, &pull_data, &melissa_options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                            , &total_write_time
#endif // BUILD_WITH_PROBES
                            );
    }

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp1;
    MPI_Reduce (&total_read_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    total_read_time = temp1 / comm_data.comm_size;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:           %d\n", melissa_options.nb_simu);
        fprintf (stdout, " --- Number of simulation cores:      %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:        %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average reading time:            %g s\n", total_read_time);
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
