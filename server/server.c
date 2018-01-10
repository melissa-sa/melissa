/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

/**
 *
 * @file server.c
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <zmq.h>
#include "server.h"
#include "melissa_io.h"
#include "compute_stats.h"
#include "melissa_options.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#include "fault_tolerance.h"

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = signo;
}

int main (int argc, char **argv)
{
    melissa_options_t     melissa_options;
    melissa_data_t       *data_ptr = NULL;
    melissa_field_t      *fields = NULL;
    comm_data_t           comm_data;
    int                   time_step;
    int                   i, client_rank, field_id;
    int                   recv_vect_size = 0;
    char                 *buf_ptr = NULL;
    double              **buff_tab_ptr;
    int                   port_no;
    char                  txt_buffer[MPI_MAX_PROCESSOR_NAME] = {0};
    char                 *node_names = NULL;
    int                   rinit_tab[2];
    void                 *context = zmq_ctx_new ();
    char                  node_name[MPI_MAX_PROCESSOR_NAME];
    void                 *connexion_responder = zmq_socket (context, ZMQ_REP);
    void                 *init_responder = zmq_socket (context, ZMQ_REP);
    void                 *data_puller = zmq_socket (context, ZMQ_PULL);
    void                 *python_pusher = zmq_socket (context, ZMQ_PUSH);
    int                   first_init;
    int                  *first_send;
    int                   local_nb_messages;
    int                   nb_bufferized_messages = 32;
    char                 *field_name_ptr = NULL;
    int                   simu_id, group_id;
    int                   nb_converged_fields = 0;
    double                interval1, interval_tot;
    zmq_msg_t             msg;
#ifdef BUILD_WITH_PROBES
    double                start_time = 0;
    double                total_comm_time = 0;
    double                start_comm_time = 0;
    double                end_comm_time = 0;
    double                total_computation_time = 0;
    double                start_computation_time = 0;
    double                end_computation_time = 0;
    double                total_wait_time = 0;
    double                start_wait_time = 0;
    double                end_wait_time = 0;
    double                total_save_time = 0;
    double                start_save_time = 0;
    double                end_save_time = 0;
    double                total_read_time = 0;
    double                start_read_time = 0;
    double                end_read_time = 0;
    double                total_write_time = 0;
    long int              total_mbytes_recv = 0;
#endif // BUILD_WITH_PROBES
    int                   old_simu_state;
    double                last_timeout_check = 0;
    int                   detected_timeouts;
    int                   nb_finished_simulations = 0;
    double                last_checkpoint_time = 0.0;
    vector_t              simulations;
    melissa_simulation_t *simu_ptr;

#ifdef BUILD_WITH_MPI

    // === init MPI === //

    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED , &i);
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
//        melissa_write_options (&melissa_options);
    }

    fields = melissa_malloc (melissa_options.nb_fields * sizeof(melissa_field_t));
    melissa_get_fields (argc, argv, fields, melissa_options.nb_fields);

    // === Open data puller port === //

    port_no = 100 + comm_data.rank;
    sprintf (txt_buffer, "tcp://*:11%d", port_no);
    zmq_setsockopt (data_puller, ZMQ_RCVHWM, &nb_bufferized_messages, sizeof(int));
    melissa_bind (data_puller, txt_buffer);

    // === Open launcher port === //

    sprintf (txt_buffer, "tcp://%s:5555", melissa_options.launcher_name);
    i = 10000; // linger
    zmq_setsockopt (python_pusher, ZMQ_LINGER, &i, sizeof(int));
    melissa_connect (python_pusher, txt_buffer);
    if (comm_data.rank == 0)
    {
        melissa_bind (init_responder, "tcp://*:2002");
        melissa_bind (connexion_responder, "tcp://*:2003");

        node_names = melissa_malloc (MPI_MAX_PROCESSOR_NAME * comm_data.comm_size);
        first_init = 2;
    }
    else
    {
        first_init = 1;
    }

    // === send node 0 name to launcher === //

    if (comm_data.rank == 0)
    {
        sprintf (txt_buffer, "server %s", node_name);
        zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
    }

    if (melissa_options.restart == 0)
    {
        alloc_vector (&simulations, melissa_options.sampling_size);
        for (i=0; i<melissa_options.sampling_size; i++)
        {
            vector_set (&simulations, i, add_simulation());
        }
    }
    else
    {
        // === Restart initialisation === //
        fprintf (stdout, "reading simulation states at checkpoint time... ");
        read_simu_states(&simulations, &melissa_options, &comm_data);
        for (i=0; i<simulations.size; i++)
        {
            simu_ptr = simulations.items[i];
            fprintf(stderr, "  simu_state[%d] = %d (rank %d)\n", i, simu_ptr->status, comm_data.rank);
            if (simu_ptr->status == 2)
            {
                nb_finished_simulations += 1;
            }
        }
        for (i=0; i<simulations.size; i++)
        {
            simu_ptr = simulations.items[i];
            if (simu_ptr->status == 1)
            {
                simu_ptr->status = 0;
            }
        }
        if (comm_data.rank == 0)
        {
            for (i=0; i<simulations.size; i++)
            {
                simu_ptr = simulations.items[i];
                sprintf (txt_buffer, "simu_state %d %d", i, simu_ptr->status);
                zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
            }
        }
        melissa_options.sampling_size = simulations.size;
        fprintf (stdout, " ok \n");
    }

    // === Gather node names on node 0 === //

#ifdef BUILD_WITH_MPI
    MPI_Gather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

//    sinit_tab[0] = comm_data.comm_size;
//    sinit_tab[1] = melissa_options.sobol_op;
//    sinit_tab[2] = melissa_options.nb_parameters;

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

        if (comm_data.rank == 0)
        {
            if (last_timeout_check + 20 < melissa_get_time())
            {
                detected_timeouts = check_timeouts(&simulations,
                                                   melissa_options.timeout_simu);
                last_timeout_check = melissa_get_time();
                if (detected_timeouts > 0)
                {
                    send_timeouts (detected_timeouts,
                                   &simulations,
                                   python_pusher);
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
                zmq_msg_init (&msg);
                zmq_msg_recv (&msg, connexion_responder, 0);
                memcpy(rinit_tab, zmq_msg_data (&msg), 2 * sizeof(int));
                zmq_msg_close (&msg);
                zmq_msg_init_size (&msg, 3 * sizeof(int) + comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                buf_ptr = zmq_msg_data (&msg);
                memcpy (buf_ptr, &comm_data.comm_size, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &melissa_options.sobol_op, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &melissa_options.nb_parameters, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                zmq_msg_send (&msg, connexion_responder, 0);
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
            first_send = melissa_calloc(melissa_options.nb_fields*comm_data.client_comm_size, sizeof(int));
            for (i=0; i<melissa_options.nb_fields; i++)
            {
                fields[i].client_vect_sizes = melissa_calloc (comm_data.client_comm_size, sizeof(int));
            }
            if (melissa_options.sobol_op == 1)
            {
                buff_tab_ptr = melissa_malloc ((melissa_options.nb_parameters + 2) * sizeof(double*));
            }
            else
            {
                buff_tab_ptr = melissa_malloc (sizeof(double*));
            }
            local_nb_messages = 0;
            add_fields(fields,
                       comm_data.client_comm_size,
                       melissa_options.nb_fields);

            first_init = 0;
        }

        // === Data reception and statistics computation === //

        if (items[1].revents & ZMQ_POLLIN)
        {
#ifdef BUILD_WITH_PROBES
            start_comm_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, data_puller, 0);
            buf_ptr = zmq_msg_data (&msg);
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
            memcpy(&recv_vect_size, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            field_name_ptr = buf_ptr;

//======================== NEW ========================
            field_id = get_field_id(fields, melissa_options.nb_fields, field_name_ptr);
            if (first_send[field_id*comm_data.client_comm_size+client_rank] == 0)
            {
                local_nb_messages += 1;
                first_send[field_id*comm_data.client_comm_size+client_rank] = 1;
            }
            if (group_id > simulations.size)
            {
                for (i=simulations.size; i<group_id; i++)
                {
                    vector_set (&simulations, i, add_simulation());
                }
                melissa_options.sampling_size = simulations.size;
            }
//====================== END NEW ======================

            data_ptr = fields[field_id].stats_data;
            if (data_ptr[client_rank].is_valid != 1)
            {
                melissa_init_data (&data_ptr[client_rank], &melissa_options, recv_vect_size);
                last_checkpoint_time = melissa_get_time();
                if (melissa_options.restart == 1)
                {
                    start_read_time = melissa_get_time();
                    if (comm_data.rank == 0)
                    {
                        fprintf (stdout, "reading checkpoint files...");
                    }
                    read_saved_stats (data_ptr, &comm_data, field_name_ptr, client_rank);
                    if (comm_data.rank == 0)
                    {
                        fprintf (stdout, " ok\n");
                    }
                    last_checkpoint_time = melissa_get_time();
                    end_read_time = melissa_get_time();
                    total_read_time += end_read_time - start_read_time;
                }
            }
            simu_ptr = simulations.items[group_id];
            simu_ptr->last_message = melissa_get_time();
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
#ifdef BUILD_WITH_PROBES
            total_mbytes_recv += zmq_msg_size (&msg) / 1000000;
            start_computation_time = melissa_get_time();
#endif // BUILD_WITH_PROBES

            if (melissa_options.sobol_op != 1)
            {
                buff_tab_ptr[0] = (double*)buf_ptr;
                // === Compute classical statistics === //
                compute_stats (&data_ptr[client_rank],
                               time_step-1,
                               1,
                               buff_tab_ptr,
                               group_id);
            }
            else
            {
                for (i=0; i<melissa_options.nb_parameters+2; i++)
                {
                    buff_tab_ptr[i] = (double*)buf_ptr;
                    buf_ptr += recv_vect_size * sizeof(double);
                }
                // === Compute classical statistics + Sobol indices === //
                compute_stats (&data_ptr[client_rank],
                               time_step-1,
                               melissa_options.nb_parameters+2,
                               buff_tab_ptr,
                               group_id);
                confidence_sobol_martinez (&(data_ptr[client_rank].sobol_indices[time_step-1]),
                                           melissa_options.nb_parameters,
                                           data_ptr[client_rank].vect_size);
                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                                                                        0.01,
                                                                        melissa_options.nb_time_steps,
                                                                        melissa_options.nb_parameters);
            }
#ifdef BUILD_WITH_PROBES
            end_computation_time = melissa_get_time();
            total_computation_time += end_computation_time - start_computation_time;
#endif // BUILD_WITH_PROBES
            old_simu_state = simu_ptr->status;
            simu_ptr->status = check_simu_state(fields, melissa_options.nb_fields, group_id, melissa_options.nb_time_steps, &comm_data);
//            fprintf (stderr, "group %d, rank %d, field %s, state %d\n", group_id, comm_data.rank, field_name_ptr, simu_state[group_id]);

            if (simu_ptr->status == 2)
            {
                nb_finished_simulations += 1;
                if (comm_data.rank == 0)
                {
                    fprintf(stdout, "  finished simulations: %d/%d\n", nb_finished_simulations, simulations.size);
                }
            }
            // === Send a message to the Python master in case of simulation status update === //
            if (old_simu_state != simu_ptr->status && comm_data.rank == 0)
            {
                sprintf (txt_buffer, "group_state %d %d", group_id, simu_ptr->status);
                zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
            }

//            if (/*comm_data.rank==0 && */((iteration % 10) == 0 || iteration < 10) )
//            {
//                fprintf(stdout, "time step %d - simulation %d\n", time_step, group_id);
//            }
            for (i=0; i<sizeof(buff_tab_ptr)/sizeof(double*); i++)
            {
                buff_tab_ptr[i] = NULL;
            }
            buf_ptr = NULL;
            zmq_msg_close (&msg);
        }

//        if (iteration % 100 == 0)
        if (last_checkpoint_time  + melissa_options.check_interval < melissa_get_time() && last_checkpoint_time > 0.1)
        {
#ifdef BUILD_WITH_PROBES
            start_save_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
            for (i=0; i<melissa_options.nb_fields; i++)
            {
                save_stats (fields[i].stats_data, &comm_data, fields[i].name);
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    fprintf(stderr, "statistic field %s saved in %s\n", fields[i].name, dir);
                }
            }
            save_simu_states (&simulations, &comm_data);
            last_checkpoint_time = melissa_get_time();
#ifdef BUILD_WITH_PROBES
            end_save_time = melissa_get_time();
            fprintf (stdout, "chekpoint time: %g (proc %d)\n", end_save_time - start_save_time, comm_data.rank);
            total_save_time += end_save_time - start_save_time;
#endif // BUILD_WITH_PROBES
        }

        // === Signal handling === //

        if (end_signal == SIGINT || end_signal == SIGUSR1 || end_signal == SIGUSR2)
        {
#ifdef BUILD_WITH_PROBES
            start_save_time = melissa_get_time();
#endif // BUILD_WITH_PROBES
            if (comm_data.rank == 0)
            {
                fprintf (stderr, "\n   INTERUPTED\n");
            }
            for (i=0; i<melissa_options.nb_fields; i++)
            {
                save_stats (fields[i].stats_data, &comm_data, fields[i].name);
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    fprintf(stderr, "statistic fields saved in %s\n\n", dir);
                }
            }
            save_simu_states (&simulations, &comm_data);
            if (end_signal == SIGINT)
            {
#ifdef BUILD_WITH_MPI
                MPI_Finalize ();
#endif // BUILD_WITH_MPI
                return 0;
            }
#ifdef BUILD_WITH_PROBES
            end_save_time = melissa_get_time();
            fprintf (stdout, "chekpoint time: %g (proc %d)\n", end_save_time - start_save_time, comm_data.rank);
#endif // BUILD_WITH_PROBES
            total_save_time += end_save_time - start_save_time;
            break;
        }

        // === Send a message to the Python Launcher in case of Sobol indices convergence === //

        if (first_init == 0 &&
            nb_converged_fields >= melissa_options.nb_fields * local_nb_messages &&
            melissa_options.sobol_op == 1)
        {
            sprintf (txt_buffer, "converged %d", comm_data.rank);
            zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
            //                if (strcmp ("stop", txt_buffer))
            //                {
            //                    break;
            //                }
        }

        if (nb_finished_simulations >= simulations.size)
        {
            break;
        }
    }

    if (end_signal == 0)
    {

    melissa_free (buff_tab_ptr);
    }

    if (comm_data.rank == 0)
    {
        melissa_free(node_names);
    }

    interval1 = 0;
    interval_tot = 0;
    if (melissa_options.sobol_op == 1)
    {
        global_confidence_sobol_martinez (fields,
                                          melissa_options.nb_fields,
                                          &comm_data,
                                          &interval1,
                                          &interval_tot);
    }

    if (end_signal == 0)
    {
        for (i=0; i< melissa_options.nb_fields; i++)
        {
            finalize_field_data (&fields[i],
                                 &comm_data,
                                 &melissa_options
#ifdef BUILD_WITH_PROBES
                                 , &total_write_time
#endif // BUILD_WITH_PROBES
                                 );
        }
    }
//    melissa_free (comm_data.rcounts);
//    melissa_free (comm_data.rdispls);
    melissa_free (first_send);
    free_simu_vector (simulations);

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
        fprintf (stdout, " --- Reading time:                    %g s\n", total_read_time);
        fprintf (stdout, " --- Writing time:                    %g s\n", total_write_time);
        fprintf (stdout, " --- Chekpointing time:               %g s\n", total_save_time);
        fprintf (stdout, " --- Total time:                      %g s\n", melissa_get_time() - start_time);
        fprintf (stdout, " --- MB recieved:                     %ld MB\n",total_mbytes_recv);
//        fprintf (stdout, " --- Stats structures memory:         %ld MB\n", mem_conso(&melissa_options));
        fprintf (stdout, " --- Bytes written:                   %ld MB\n", count_mbytes_written(&melissa_options));
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

    if (comm_data.rank == 0 && end_signal == 0)
    {
        sprintf (txt_buffer, "stop");
        zmq_send(python_pusher, txt_buffer, strlen(txt_buffer) * sizeof(char), 0);
    }
    zmq_close (python_pusher);
    zmq_ctx_term (context);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
