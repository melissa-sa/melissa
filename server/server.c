
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

#define ZEROCOPY

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = signo;
}

int main (int argc, char **argv)
{
    melissa_options_t   melissa_options;
    melissa_data_t     *data_ptr = NULL;
    field_ptr           field = NULL;
    comm_data_t         comm_data;
    int                 time_step;
    int                 recv_buff_size, i, client_rank;
    char               *buffer = NULL, *buf_ptr = NULL;
    double            **buff_tab_ptr;
    int                 iteration = 0, nb_iterations = 0;
    int                 port_no;
    char                txt_buffer[MPI_MAX_PROCESSOR_NAME] = {0};
    char               *node_names = NULL;
    int                 sinit_tab[3], rinit_tab[2];
    void               *context = zmq_ctx_new ();
    char                node_name[MPI_MAX_PROCESSOR_NAME];
    void               *connexion_responder = zmq_socket (context, ZMQ_REP);
    void               *init_responder = zmq_socket (context, ZMQ_REP);
    void               *data_puller = zmq_socket (context, ZMQ_PULL);
#ifdef BUILD_WITH_PY_ZMQ
    void               *python_pusher = zmq_socket (context, ZMQ_PUSH);
#else // BUILD_WITH_PY_ZMQ
    void               *python_pusher = NULL;
#endif // BUILD_WITH_PY_ZMQ
    int                 nb_fields = 0;
    int                 first_init = 1;
    int                 first_connect = 1;
    int                 get_next_message = 0;
    int                *client_vect_sizes = NULL, *local_vect_sizes = NULL;
    pull_data_t         pull_data;
    int                 nb_bufferized_messages = 32;
    char               *field_name_ptr = NULL;
    int                 simu_id, group_id;
    int                 nb_converged_fields = 0;
    double              interval1, interval_tot;
    zmq_msg_t           msg;
#ifdef BUILD_WITH_PROBES
    double              start_time = 0;
    double              total_comm_time = 0;
    double              start_comm_time = 0;
    double              end_comm_time = 0;
    double              total_computation_time = 0;
    double              start_computation_time = 0;
    double              end_computation_time = 0;
    double              total_wait_time = 0;
    double              start_wait_time = 0;
    double              end_wait_time = 0;
    double              total_write_time = 0;
    long int            total_mbytes_recv = 0;
#endif // BUILD_WITH_PROBES
    double             *last_message_simu;
    int                *simu_state;
    int                 old_simu_state;
    int                *simu_timeout;
    double              last_timeout_check = 0;
    int                 detected_timeouts;
    int                 nb_finished_simulations = 0;
    double              last_checkpoint_time;

#ifdef BUILD_WITH_MPI
    // === init MPI === //

    MPI_Init (&argc, &argv);
    comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm_data.comm, &comm_data.comm_size);
    MPI_Comm_rank (comm_data.comm, &comm_data.rank);
#else
    comm_data.comm_size       = 1;
    comm_data.rank            = 0;
#endif // BUILD_WITH_MPI

    if (comm_data.rank == 0)
    {
        melissa_logo ();
    }

    // === Get the node adress === //

    // Mettre un REGISTER ici !!!

    melissa_get_node_name (node_name);

#ifdef BUILD_WITH_PROBES
    start_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

    // === Install signal handler === //

    if (signal(SIGINT, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
            printf("\ncan't catch SIGUSR2\n");

    // === Read options from command line === //

    melissa_get_options (argc, argv, &melissa_options);
    if (comm_data.rank == 0)
    {
        melissa_print_options (&melissa_options);
        melissa_write_options (&melissa_options);
    }
    nb_iterations     = melissa_options.sampling_size * melissa_options.nb_time_steps ;
    last_message_simu = melissa_calloc (melissa_options.sampling_size, sizeof(double));
    simu_state        = melissa_calloc (melissa_options.sampling_size, sizeof(int));
    simu_timeout      = melissa_calloc (melissa_options.sampling_size, sizeof(int));
    last_checkpoint_time = melissa_get_time();

    // === Open data puller port === //

    port_no = 100 + comm_data.rank;
    sprintf (txt_buffer, "tcp://*:11%d", port_no);
    zmq_setsockopt (data_puller, ZMQ_RCVHWM, &nb_bufferized_messages, sizeof(int));
    melissa_bind (data_puller, txt_buffer);
#ifdef BUILD_WITH_PY_ZMQ
    sprintf (txt_buffer, "tcp://%s:5555", melissa_options.master_name);
    melissa_connect (python_pusher, txt_buffer);
#endif // BUILD_WITH_PY_ZMQ

    if (comm_data.rank == 0)
    {
        melissa_bind (init_responder, "tcp://*:2002");
        melissa_bind (connexion_responder, "tcp://*:2003");

        node_names = melissa_malloc (MPI_MAX_PROCESSOR_NAME * comm_data.comm_size);

        first_connect = 2;
        first_init = 2;
    }

    // === Restart initialisation === //

    if (melissa_options.restart == 1)
    {
        first_connect = 1;
        first_init = 0;
        fprintf (stdout, "reading data files...");
        if (comm_data.rank == 0)
        {
            read_client_data(&comm_data.client_comm_size, &client_vect_sizes, &melissa_options);
            // if client_data.save doesn't exist, we set restart to 0
        }
    }
    if (melissa_options.restart == 1)
    {
#ifdef BUILD_WITH_MPI
        MPI_Bcast(&comm_data.client_comm_size, 1, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
        if (comm_data.rank != 0)
        {
            client_vect_sizes = melissa_malloc (comm_data.client_comm_size * sizeof(int));
        }
        fprintf (stdout, " ok \n");
        fprintf (stdout, "reading simulation states at checkpoint time...");
        read_simu_states(simu_state, &melissa_options, &comm_data, melissa_options.sampling_size);
        for (i=0; i<melissa_options.sampling_size; i++)
        {
            fprintf(stderr, "  simu_state[%d] = %d (rank %d)\n", i, simu_state[i], comm_data.rank);
            if (simu_state[i] == 2)
            {
                nb_finished_simulations += 1;
            }
        }
#ifdef BUILD_WITH_PY_ZMQ
        if (comm_data.rank == 0)
        {
            for (i=0; i<melissa_options.sampling_size; i++)
            {
                sprintf (txt_buffer, "simu_state %d %d", i, simu_state[i]);
                zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
            }
        }
        for (i=0; i<melissa_options.sampling_size; i++)
        {
            if (simu_state[i] == 1)
            {
                simu_state[i] = 0;
            }
        }
#endif // BUILD_WITH_PY_ZMQ
        fprintf (stdout, " ok \n");
    }

    // === Gather node names on node 0 === //

#ifdef BUILD_WITH_MPI
    MPI_Gather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

    sinit_tab[0] = comm_data.comm_size;
    sinit_tab[1] = melissa_options.sobol_op;
    sinit_tab[2] = melissa_options.nb_parameters;

    // =================== //
    // ===  Main loop  === //
    // =================== //

    while (1)
    {
#ifdef BUILD_WITH_PROBES
        start_wait_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
        zmq_pollitem_t items [] = {
            { connexion_responder, 0, ZMQ_POLLIN, 0 },
            { data_puller, 0, ZMQ_POLLIN, 0 }
        };
        zmq_poll (items, 2, 100);
#ifdef BUILD_WITH_PROBES
        end_wait_time = melissa_get_time();
        total_wait_time += end_wait_time - start_wait_time;
#endif // BUILD_WITH_PROBES

        // === If no message on the connexion port === //

//        if (comm_data.rank == 0)
        {
            if (last_timeout_check + 100 < melissa_get_time())
            {
                detected_timeouts = check_timeouts(simu_state, simu_timeout, last_message_simu, melissa_options.sampling_size);
                last_timeout_check = melissa_get_time();
                if (detected_timeouts > 0)
                {
                    send_timeouts (detected_timeouts, simu_timeout, melissa_options.sampling_size, txt_buffer, python_pusher);
                }
            }
        }

        // === If message on the connexion port === //

        if (items[0].revents & ZMQ_POLLIN)
        {
            if (comm_data.rank == 0)
            {
#ifdef BUILD_WITH_PROBES
                start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
                // new simulation wants to connect
                zmq_recv (connexion_responder, rinit_tab, 2 * sizeof(int), 0);
                zmq_send (connexion_responder, sinit_tab, 3 * sizeof(int), 0);
                get_next_message = 1;
                if (first_init == 2)
                {
                    first_init = 1;
                }
#ifdef BUILD_WITH_PROBES
                end_comm_time = melissa_get_time();
                total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
            }
        }

        // === Only after the first connexion, broadcast client info === //

        if (first_init == 1)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(rinit_tab, 2, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            comm_data.client_comm_size = rinit_tab[0];
            client_vect_sizes = melissa_malloc (comm_data.client_comm_size * sizeof(int));
            first_init = 0;
        }

        // === Second part of the connexion message, only for rank 0 === //

        if (get_next_message == 1)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
            // new simulation wants to connect, step two

            zmq_recv (init_responder, client_vect_sizes, comm_data.client_comm_size * sizeof(int), 0);
            zmq_send (init_responder, node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char), 0);

            get_next_message = 0;
            if (first_connect == 2)
            {
                first_connect = 1;
            }
#ifdef BUILD_WITH_PROBES
            end_comm_time = melissa_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES
            write_client_data (&comm_data.client_comm_size, client_vect_sizes);
        }

        // === Melissa structures initialisation, after the second part of the first connexion === //

        if (first_connect == 1 && first_init == 0)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(client_vect_sizes, comm_data.client_comm_size, MPI_INT, 0, comm_data.comm);
#endif // BUILD_WITH_MPI
            melissa_options.global_vect_size = 0;
            for (i=0; i< comm_data.client_comm_size; i++)
            {
                melissa_options.global_vect_size += client_vect_sizes[i];
            }
            local_vect_sizes = melissa_malloc (comm_data.comm_size * sizeof(int));
            for (i=0; i<comm_data.comm_size; i++)
            {
                local_vect_sizes[i] = melissa_options.global_vect_size / comm_data.comm_size;
                if (i < melissa_options.global_vect_size % comm_data.comm_size)
                    local_vect_sizes[i] += 1;
            }

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
            nb_iterations *= pull_data.local_nb_messages;
            recv_buff_size = pull_data.buff_size * sizeof(double);
            if (melissa_options.sobol_op == 1)
            {
                recv_buff_size *= melissa_options.nb_parameters+2;
            }
            recv_buff_size += MAX_FIELD_NAME * sizeof(char) + 4 * sizeof(int);
            buffer = melissa_malloc (recv_buff_size);

            if (melissa_options.sobol_op == 1)
            {
                buff_tab_ptr = melissa_malloc ((melissa_options.nb_parameters + 2) * sizeof(double*));
            }
            else
            {
                buff_tab_ptr = melissa_malloc (sizeof(double*));
            }
            first_connect = 0;
        }

        // === Data reception and statistics computation === //

        if (items[1].revents & ZMQ_POLLIN)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
#ifdef ZEROCOPY
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, data_puller, 0);
            buf_ptr = zmq_msg_data (&msg);
#else // ZEROCOPY
            zmq_recv (data_puller, buffer, recv_buff_size, 0);
            buf_ptr = buffer;
#endif // ZEROCOPY
#ifdef BUILD_WITH_PROBES
            end_comm_time = melissa_get_time();
            total_comm_time += end_comm_time - start_comm_time;
#endif // BUILD_WITH_PROBES

            memcpy(&time_step, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&simu_id, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&group_id, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&client_rank, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            field_name_ptr = buf_ptr;
            if (field == NULL)
            {
                add_field(&field, field_name_ptr, comm_data.client_comm_size, melissa_options.sampling_size);
                data_ptr = get_data_ptr (field, field_name_ptr);
                nb_fields += 1;
            }
            else
            {
                data_ptr = get_data_ptr (field, field_name_ptr);
                if (data_ptr == NULL)
                {
                    add_field(&field, field_name_ptr, comm_data.client_comm_size, melissa_options.sampling_size);
                    data_ptr = get_data_ptr (field, field_name_ptr);
                    nb_fields += 1;
                }
            }
            if (data_ptr[client_rank].is_valid != 1)
            {
                melissa_init_data (&data_ptr[client_rank], &melissa_options, comm_data.rcounts[client_rank]);
                if (melissa_options.restart == 1)
                {
                    fprintf (stdout, "reading checkpoint files...");
                    read_saved_stats (data_ptr, &comm_data, field_name_ptr, client_rank);
                    fprintf (stdout, " ok\n");
                }
            }
            last_message_simu[group_id] = melissa_get_time();
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
#ifdef BUILD_WITH_PROBES
#ifdef ZEROCOPY
            total_mbytes_recv += zmq_msg_size (&msg) / 1000000;
#else // ZEROCOPY
            total_mbytes_recv += recv_buff_size / 1000000;
#endif // ZEROCOPY
            start_computation_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

            if (melissa_options.sobol_op != 1)
            {
                buff_tab_ptr[0] = (double*)buf_ptr;
                // === Compute classical statistics === //
                compute_stats (&data_ptr[client_rank], time_step-1, 1, buff_tab_ptr, group_id);
                iteration++;
            }
            else
            {
                for (i=0; i<melissa_options.nb_parameters+2; i++)
                {
                    buff_tab_ptr[i] = (double*)buf_ptr;
                    buf_ptr += comm_data.rcounts[client_rank] * sizeof(double);
                }
                // === Compute classical statistics + Sobol indices === //
                compute_stats (&data_ptr[client_rank], time_step-1, melissa_options.nb_parameters+2, buff_tab_ptr, group_id);
                iteration++;
                confidence_sobol_martinez (&(data_ptr[client_rank].sobol_indices[time_step-1]), melissa_options.nb_parameters, data_ptr[client_rank].vect_size);
//                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
//                                                                        0.01,
//                                                                        melissa_options.nb_time_steps,
//                                                                        melissa_options.nb_parameters);
            }
#ifdef BUILD_WITH_PROBES
            end_computation_time = melissa_get_time();
            total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
//            fprintf (stderr, "group %d, rank %d, field %s, state %d\n", group_id, comm_data.rank, field_name_ptr, simu_state[group_id]);
            old_simu_state = simu_state[group_id];
            simu_state[group_id] = check_simu_state(field, 2, group_id, melissa_options.nb_time_steps, comm_data.client_comm_size);

            if (simu_state[group_id] == 2)
            {
                nb_finished_simulations += 1;
                fprintf(stderr, "nb_finished_simulations: %d\n", nb_finished_simulations);
            }
#ifdef BUILD_WITH_PY_ZMQ
            // === Send a message to the Python master in case of simulation status update === //
            if (old_simu_state != simu_state[group_id] && comm_data.rank == 0)
            {
                sprintf (txt_buffer, "simu_state %d %d", group_id, simu_state[group_id]);
                zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
            }
#endif // BUILD_WITH_PY_ZMQ

            if (comm_data.rank==0 && ((iteration % 10) == 0 || iteration < 10) )
            {
                fprintf(stdout, "iteration %d / %d  - field \"%s\"\n", iteration, nb_iterations*nb_fields, field_name_ptr);
            }
#ifdef ZEROCOPY
            for (i=0; i<sizeof(buff_tab_ptr)/sizeof(double*); i++)
            {
                buff_tab_ptr[i] = NULL;
            }
            buf_ptr = NULL;
            zmq_msg_close (&msg);
#endif // ZEROCOPY
        }

#ifdef BUILD_WITH_PY_ZMQ
        // === Send a message to the Python master in case of Sobol indices convergence === //
//        if (nb_converged_fields >= nb_fields * pull_data.local_nb_messages && melissa_options.sobol_op == 1)
//        {
//            sprintf (txt_buffer, "converged %d", comm_data.rank);
//            zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
//            if (strcmp ("stop", txt_buffer))
//            {
//                break;
//            }
//        }
#endif // BUILD_WITH_PY_ZMQ

//        if (iteration % 100 == 0)
        if (last_checkpoint_time  + 5000 < melissa_get_time())
        {
            field_ptr fptr = field;
            while (fptr != NULL)
            {
                save_stats (fptr->stats_data, &comm_data, fptr->name);
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    fprintf(stderr, "statistic field %s saved in %s\n", fptr->name, dir);
                }
                fptr = field->next;
            }
            save_simu_states (simu_state, &comm_data, melissa_options.sampling_size);
            last_checkpoint_time = melissa_get_time();
        }


        // === Signal handling === //

        if (end_signal == SIGINT || end_signal == SIGUSR1 || end_signal == SIGUSR2)
        {
            field_ptr fptr = field;
            if (comm_data.rank == 0)
                fprintf (stderr, "\nINTERUPTED at iteration %d \n", iteration);
            while (fptr != NULL)
            {
                save_stats (fptr->stats_data, &comm_data, fptr->name);
                fptr = field->next;
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    fprintf(stderr, "statistic fields saved in %s\n\n", dir);
                }
            }
            save_simu_states (simu_state, &comm_data, melissa_options.sampling_size);
            if (end_signal == SIGINT)
            {
#ifdef BUILD_WITH_MPI
                MPI_Finalize ();
#endif // BUILD_WITH_MPI
                return 0;
            }
            break;
        }

//#ifdef BUILD_WITH_PY_ZMQ
//        if (nb_fields > 0 && melissa_options.sobol_op == 0)
//#else  // BUILD_WITH_PY_ZMQ
        if (nb_fields > 0)
//#endif // BUILD_WITH_PY_ZMQ
        {
//            if (iteration >= nb_iterations*nb_fields)
//            {
//                break;
//            }
            if (nb_finished_simulations >= melissa_options.sampling_size)
            {
                break;
            }
        }
    }


    if (end_signal == 0)
    {

    melissa_free (buff_tab_ptr);
    melissa_free (last_message_simu);
    }

    if (comm_data.rank == 0)
    {
        melissa_free(node_names);
    }

    interval1 = 0;
    interval_tot = 0;
    if (melissa_options.sobol_op == 1)
    {
        global_confidence_sobol_martinez (field, &comm_data, &interval1, &interval_tot);
    }

    if (end_signal == 0)
    {
        finalize_field_data (field, &comm_data, &pull_data, &melissa_options, local_vect_sizes
#ifdef BUILD_WITH_PROBES
                            , &total_write_time
#endif // BUILD_WITH_PROBES
                            );
    }
    melissa_free (buffer);
    melissa_free (comm_data.rcounts);
    melissa_free (comm_data.rdispls);

#ifdef BUILD_WITH_PROBES
#ifdef BUILD_WITH_MPI
    double temp1;
    long int temp2;
    MPI_Reduce (&interval1, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    interval1 = temp1;
    MPI_Reduce (&interval_tot, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    interval_tot = temp1;
    MPI_Reduce (&total_comm_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    total_comm_time = temp1 / comm_data.comm_size;
    MPI_Reduce (&total_mbytes_recv, &temp2, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    total_mbytes_recv = temp2;
#endif // BUILD_WITH_MPI
    if (comm_data.rank==0)
    {
        fprintf (stdout, " --- Number of simulations:           %d\n", melissa_options.nb_simu);
        fprintf (stdout, " --- Number of simulation cores:      %d\n", comm_data.client_comm_size);
        fprintf (stdout, " --- Number of analysis cores:        %d\n", comm_data.comm_size);
        fprintf (stdout, " --- Average communication time:      %g s\n", total_comm_time);
        fprintf (stdout, " --- Calcul time:                     %g s\n", total_computation_time);
        fprintf (stdout, " --- Waiting time:                    %g s\n", total_wait_time);
        fprintf (stdout, " --- Writing time:                    %g s\n", total_write_time);
        fprintf (stdout, " --- Total time:                      %g s\n", melissa_get_time() - start_time);
        fprintf (stdout, " --- MB recieved:                     %ld MB\n",total_mbytes_recv);
        fprintf (stdout, " --- Stats structures memory:         %ld MB\n", mem_conso(&melissa_options));
        fprintf (stdout, " --- Bytes written:                   %ld MB\n", count_mbytes_written(&melissa_options)*nb_fields);
        if (melissa_options.sobol_op == 1)
        {
            fprintf (stdout, " --- Worst Sobol confidence interval: %g (first order)\n", interval1);
            fprintf (stdout, " --- Worst Sobol confidence interval: %g (total order)\n", interval_tot);
        }
        fprintf (stdout, "\n");
    }
#endif // BUILD_WITH_PROBES


    // === Sockets deconnexion === //

    zmq_close (connexion_responder);
    zmq_close (init_responder);
    zmq_close (data_puller);
#ifdef BUILD_WITH_PY_ZMQ
    if (comm_data.rank == 0 && end_signal == 0)
    {
        sprintf (txt_buffer, "stop");
        zmq_send(python_pusher, txt_buffer, strlen(txt_buffer) * sizeof(char), 0);
    }
    zmq_close (python_pusher);
#endif // BUILD_WITH_PY_ZMQ
    zmq_ctx_term (context);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
