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
    void                 *deconnexion_responder = zmq_socket (context, ZMQ_REP);
    void                 *data_puller = zmq_socket (context, ZMQ_PULL);
    void                 *text_puller = zmq_socket (context, ZMQ_SUB);
    void                 *text_pusher = zmq_socket (context, ZMQ_PUSH);
    int                   first_init;
    int                  *first_send;
    int                   local_nb_messages;
    int                   nb_bufferized_messages = 32;
    char                 *field_name_ptr = NULL;
    int                   simu_id, group_id;
    int                   nb_converged_fields = 0;
    double                interval1, interval_tot;
    zmq_msg_t             msg;
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
    int                   old_simu_state;
    double                last_timeout_check = 0;
    int                   detected_timeouts;
    int                   nb_finished_simulations = 0;
    double                last_checkpoint_time = 0.0;
    double                last_msg_launcher;
    const double          timeout_launcher = 50;
    vector_t              simulations;
    melissa_simulation_t *simu_ptr;
    char*                 melissa_output_lib  = INSTALL_PREFIX"/lib/libmelissa_output.so";
    char*                 melissa_output_func = "write_stats_txt";

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

    // === Get the node adress === //

    melissa_get_node_name (node_name);

    start_time = melissa_get_time();

    // === Install signal handler === //

    // === Read options from command line === //

    melissa_get_options (argc, argv, &melissa_options);

    if (signal(SIGINT, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa can't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa an't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa an't catch SIGUSR2\n");

    // === load the output library === //

    melissa_get_output_lib (melissa_output_lib, melissa_output_func);

    if (comm_data.rank == 0)
    {
        melissa_logo ();
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

    // === Open launcher ports === //
    i = 10000; // linger

    sprintf (txt_buffer, "tcp://%s:5555", melissa_options.launcher_name);
    zmq_setsockopt (text_pusher, ZMQ_LINGER, &i, sizeof(int));
    melissa_connect (text_pusher, txt_buffer);

    sprintf (txt_buffer, "tcp://%s:5556", melissa_options.launcher_name);
    zmq_setsockopt (text_puller, ZMQ_LINGER, &i, sizeof(int));
    zmq_setsockopt(text_puller, ZMQ_SUBSCRIBE, "", 0);
    melissa_connect (text_puller, txt_buffer);

    if (comm_data.rank == 0)
    {
        melissa_print (VERBOSE_INFO, "Server connected to launcher\n");
        melissa_bind (deconnexion_responder, "tcp://*:2002");
        melissa_bind (connexion_responder, "tcp://*:2003");

        node_names = melissa_malloc (MPI_MAX_PROCESSOR_NAME * comm_data.comm_size);
        first_init = 2;
    }
    else
    {
        first_init = 1;
    }
    last_msg_launcher = melissa_get_time();

    // === send node 0 name to launcher === //

    if (comm_data.rank == 0)
    {
        sprintf (txt_buffer, "server %s", node_name);
        zmq_send(text_pusher, txt_buffer, strlen(txt_buffer), 0);
        melissa_print (VERBOSE_INFO, "Server node name sent to launcher\n");
    }

    if (melissa_options.restart != 1)
    {
        alloc_vector (&simulations, melissa_options.sampling_size);
        for (i=0; i<melissa_options.sampling_size; i++)
        {
            vector_add (&simulations, add_simulation());
        }
    }
    else
    {
        // === Restart initialisation === //
        if (comm_data.rank == 0)
        {
            melissa_print (VERBOSE_INFO, "Reading simulation states at checkpoint time...\n");
        }
        read_simu_states(&simulations, &melissa_options, &comm_data);
        for (i=0; i<simulations.size; i++)
        {
            simu_ptr = simulations.items[i];
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
                melissa_print (VERBOSE_DEBUG, "Simu_state %d %d\n", i, simu_ptr->status);
                sprintf (txt_buffer, "simu_state %d %d", i, simu_ptr->status);
                zmq_send(text_pusher, txt_buffer, strlen(txt_buffer), 0);
            }
        }
        melissa_options.sampling_size = simulations.size;
        if (comm_data.rank == 0)
        {
            melissa_print (VERBOSE_INFO, "Reading simulation states at checkpoint time ok \n");
        }
    }

    // === Gather node names on node 0 === //

#ifdef BUILD_WITH_MPI
    MPI_Gather(node_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (node_names, node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

    // =================== //
    // ===  Main loop  === //
    // =================== //

    while (1)
    {
        start_wait_time = melissa_get_time();
        zmq_pollitem_t items [] = {
            { text_puller, 0, ZMQ_POLLIN, 0 },
            { connexion_responder, 0, ZMQ_POLLIN, 0 },
            { data_puller, 0, ZMQ_POLLIN, 0 },
            { deconnexion_responder, 0, ZMQ_POLLIN, 0 }
        };
        zmq_poll (items, 4, 100);
        end_wait_time = melissa_get_time();
        total_wait_time += end_wait_time - start_wait_time;

        // === check timeouts === //

        if (comm_data.rank == 0)
        {
            if (last_timeout_check + 20 < melissa_get_time())
            {
                // === check simulations timeouts === //
                detected_timeouts = check_timeouts(&simulations,
                                                   melissa_options.timeout_simu);
                last_timeout_check = melissa_get_time();
                if (detected_timeouts > 0)
                {
                    send_timeouts (detected_timeouts,
                                   &simulations,
                                   text_pusher);
                }
            }
        }
        // === check launcher timeouts === //
        if (last_msg_launcher + timeout_launcher < melissa_get_time())
        {
            if (comm_data.rank == 0)
            {
                melissa_print (VERBOSE_ERROR, " --- Server detected Launcher timeout ---\n Melissa will stop\n");
            }
            // remove unsubmited simulations from the sample
            i = count_job_status(&simulations, -1);
            melissa_options.sampling_size -= i;
            if (comm_data.rank == 0)
            {
                melissa_print (VERBOSE_ERROR, "Remove %d samples\n", i);
                melissa_print (VERBOSE_ERROR, "Waiting for remaining simulations to complete\n");
            }
        }

        // === If message on the text port === //

        if (items[0].revents & ZMQ_POLLIN)
        {
            char text[256];
            zmq_recv (text_puller, text, 255, 0);
            melissa_print (VERBOSE_DEBUG, "Reçu %s (rank %d)\n", text, comm_data.rank);
            last_msg_launcher = melissa_get_time();
            process_txt_message(text, &simulations);
            melissa_options.sampling_size = simulations.size;
        }

        // === If message on the connexion port === //

        if (items[1].revents & ZMQ_POLLIN)
        {
            if (comm_data.rank == 0)
            {
                start_comm_time = melissa_get_time();
                // new simulation wants to connect
                zmq_msg_init (&msg);
                zmq_msg_recv (&msg, connexion_responder, 0);
                memcpy(rinit_tab, zmq_msg_data (&msg), 2 * sizeof(int));
                zmq_msg_close (&msg);
                zmq_msg_init_size (&msg, 4 * sizeof(int) + comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                buf_ptr = zmq_msg_data (&msg);
                memcpy (buf_ptr, &comm_data.comm_size, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &melissa_options.sobol_op, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &melissa_options.nb_parameters, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &melissa_options.verbose_lvl, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, node_names, comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                zmq_msg_send (&msg, connexion_responder, 0);
                if (first_init == 2)
                {
                    first_init = 1;
                }
                end_comm_time = melissa_get_time();
                total_comm_time += end_comm_time - start_comm_time;
            }
        }

        // === Only after the first connexion, broadcast client === //

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

        if (items[2].revents & ZMQ_POLLIN)
        {
            start_comm_time = melissa_get_time();
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, data_puller, 0);
            buf_ptr = zmq_msg_data (&msg);
            end_comm_time = melissa_get_time();
            total_comm_time += end_comm_time - start_comm_time;

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

            melissa_print (VERBOSE_DEBUG, "Server rank %d recieved timestep %d from rank %d of group %d\n", comm_data.rank, time_step, client_rank, group_id);

            if (time_step >= melissa_options.nb_time_steps || time_step < 0)
            {
                melissa_print (VERBOSE_WARNING, "Bad time stamp (field %s)\n", field_name_ptr);
                continue;
            }

            field_id = get_field_id(fields, melissa_options.nb_fields, field_name_ptr);
            if (field_id == -1)
            {
                if (time_step == 0 && client_rank == 0)
                {
                    melissa_print (VERBOSE_WARNING, "Not computing field %s\n", field_name_ptr);
                }
                continue;
            }
            if (first_send[field_id*comm_data.client_comm_size+client_rank] == 0)
            {
                local_nb_messages += 1;
                first_send[field_id*comm_data.client_comm_size+client_rank] = 1;
            }
            if (group_id > simulations.size)
            {
                for (i=simulations.size; i<group_id; i++)
                {
                    vector_add (&simulations, add_simulation());
                }
                melissa_options.sampling_size = simulations.size;
            }

            data_ptr = fields[field_id].stats_data;
            if (data_ptr[client_rank].is_valid != 1)
            {
                melissa_init_data (&data_ptr[client_rank], &melissa_options, recv_vect_size);
                last_checkpoint_time = melissa_get_time();
                if (melissa_options.restart > 0)
                {
                    start_read_time = melissa_get_time();
                    if (comm_data.rank == 0)
                    {
                        melissa_print (VERBOSE_INFO, "reading checkpoint files...");
                    }
                    read_saved_stats (data_ptr, &comm_data, field_name_ptr, client_rank);
                    if (comm_data.rank == 0)
                    {
                        melissa_print (VERBOSE_INFO, "reading checkpoint files ok");
                    }
                    last_checkpoint_time = melissa_get_time();
                    end_read_time = melissa_get_time();
                    total_read_time += end_read_time - start_read_time;
                }
            }
            simu_ptr = simulations.items[group_id];
            simu_ptr->last_message = melissa_get_time();
            buf_ptr += MAX_FIELD_NAME * sizeof(char);
            total_mbytes_recv += zmq_msg_size (&msg) / 1000000;
            start_computation_time = melissa_get_time();

            if (melissa_options.sobol_op != 1)
            {
                buff_tab_ptr[0] = (double*)buf_ptr;
                // === Compute classical statistics === //
                compute_stats (&data_ptr[client_rank],
                               time_step,
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
                               time_step,
                               melissa_options.nb_parameters+2,
                               buff_tab_ptr,
                               group_id);
                confidence_sobol_martinez (&(data_ptr[client_rank].sobol_indices[time_step]),
                                           melissa_options.nb_parameters,
                                           data_ptr[client_rank].vect_size);
                nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                                                                        0.01,
                                                                        melissa_options.nb_time_steps,
                                                                        melissa_options.nb_parameters);
            }
            end_computation_time = melissa_get_time();
            total_computation_time += end_computation_time - start_computation_time;
            old_simu_state = simu_ptr->status;
            simu_ptr->status = check_simu_state(fields, melissa_options.nb_fields, group_id, melissa_options.nb_time_steps, &comm_data);
            melissa_print(VERBOSE_DEBUG, "Group %d, rank %d, field %s, status %d\n", group_id, comm_data.rank, field_name_ptr, simu_ptr->status);

            if (simu_ptr->status == 2)
            {
                if (comm_data.rank != 0)
                {
                    nb_finished_simulations += 1;
                }
//                if (comm_data.rank == 0)
//                {
//                    melissa_print(VERBOSE_DEBUG, "Simulation %d finished\n", group_id);
//                    melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", nb_finished_simulations, simulations.size);
//                }
            }
            // === Send a message to the Python master in case of simulation status update === //
            if (old_simu_state != simu_ptr->status && comm_data.rank == 0 && simu_ptr->status == 1)
            {
                sprintf (txt_buffer, "group_state %d %d", group_id, simu_ptr->status);
                melissa_print(VERBOSE_DEBUG, "Send \"%s\" to launcher\n", txt_buffer);
                zmq_send(text_pusher, txt_buffer, strlen(txt_buffer), 0);
            }

//            if (comm_data.rank==0)
//            {
//                melissa_print(VERBOSE_DEBUG, "time step %d - simulation %d\n", time_step, group_id);
//            }
            for (i=0; i<sizeof(buff_tab_ptr)/sizeof(double*); i++)
            {
                buff_tab_ptr[i] = NULL;
            }
            buf_ptr = NULL;
            zmq_msg_close (&msg);
        }

        if (items[3].revents & ZMQ_POLLIN)
        {
            if (comm_data.rank == 0)
            {
                start_comm_time = melissa_get_time();
                zmq_msg_init (&msg);
                zmq_msg_recv (&msg, deconnexion_responder, 0);
                memcpy(&group_id, zmq_msg_data (&msg), sizeof(int));
                zmq_msg_close (&msg);
                melissa_print (VERBOSE_DEBUG, "Group %d ask to disconnect \n", group_id);
                // simulation wants to disconnect
                simu_ptr = simulations.items[group_id];
                zmq_msg_init_size (&msg, sizeof(int));
                memcpy (zmq_msg_data (&msg), &simu_ptr->status, sizeof(int));
                zmq_msg_send (&msg, deconnexion_responder, 0);
                end_comm_time = melissa_get_time();
                total_comm_time += end_comm_time - start_comm_time;
                if (simu_ptr->status == 2)
                {
                    sprintf (txt_buffer, "group_state %d %d", group_id, simu_ptr->status);
                    melissa_print(VERBOSE_DEBUG, "Send \"%s\" to launcher\n", txt_buffer);
                    zmq_send(text_pusher, txt_buffer, strlen(txt_buffer), 0);
                    simu_ptr->status = 3;
                    nb_finished_simulations += 1;
                    melissa_print(VERBOSE_INFO, "Simulation %d finished\n", group_id);
                    melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", nb_finished_simulations, simulations.size);
                }
            }
        }

//        if (iteration % 100 == 0)
        if (last_checkpoint_time  + melissa_options.check_interval < melissa_get_time() && last_checkpoint_time > 0.1)
        {
            start_save_time = melissa_get_time();
            for (i=0; i<melissa_options.nb_fields; i++)
            {
                save_stats (fields[i].stats_data, &comm_data, fields[i].name);
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    melissa_print(VERBOSE_DEBUG, "Statistic field %s saved in %s\n", fields[i].name, dir);
                }
            }
            save_simu_states (&simulations, &comm_data);
            last_checkpoint_time = melissa_get_time();
            end_save_time = melissa_get_time();
            melissa_print(VERBOSE_DEBUG, "Chekpoint time: %g (proc %d)\n", end_save_time - start_save_time, comm_data.rank);
            total_save_time += end_save_time - start_save_time;
        }

        // === Signal handling === //

        if (end_signal == SIGINT || end_signal == SIGUSR1 || end_signal == SIGUSR2)
        {
            start_save_time = melissa_get_time();
            if (comm_data.rank == 0)
            {
                melissa_print(VERBOSE_WARNING, "\n --- INTERUPTED ---\n");
            }
            for (i=0; i<melissa_options.nb_fields; i++)
            {
                save_stats (fields[i].stats_data, &comm_data, fields[i].name);
                if (comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    melissa_print(VERBOSE_INFO, "Statistic fields saved in %s\n\n", dir);
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
            end_save_time = melissa_get_time();
            melissa_print(VERBOSE_DEBUG, "Chekpoint time: %g (proc %d)\n", end_save_time - start_save_time, comm_data.rank);
            total_save_time += end_save_time - start_save_time;
            break;
        }

        // === Send a message to the Python Launcher in case of Sobol indices convergence === //

        if (first_init == 0 &&
            nb_converged_fields >= melissa_options.nb_fields * local_nb_messages &&
            melissa_options.sobol_op == 1 &&
            nb_finished_simulations > 1)
        {
            sprintf (txt_buffer, "converged %d", comm_data.rank);
            zmq_send(text_pusher, txt_buffer, strlen(txt_buffer), 0);
            //                if (strcmp ("stop", txt_buffer))
            //                {
            //                    break;
            //                }
        }

        if (nb_finished_simulations >= melissa_options.sampling_size)
        {
            break;
        }
    }

    for (i=0; i<melissa_options.nb_fields; i++)
    {
        save_stats (fields[i].stats_data, &comm_data, fields[i].name);
    }
    save_simu_states (&simulations, &comm_data);

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
        finalize_field_data (fields,
                             &comm_data,
                             &melissa_options,
                             &total_write_time);
    }
    melissa_free (first_send);
    free_simu_vector (simulations);

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
        melissa_print (VERBOSE_INFO, " --- Number of simulations:           %d\n", melissa_options.nb_simu);
        melissa_print (VERBOSE_INFO, " --- Number of simulation processes:  %d\n", comm_data.client_comm_size);
        melissa_print (VERBOSE_INFO, " --- Number of analysis processes:    %d\n", comm_data.comm_size);
        melissa_print (VERBOSE_INFO, " --- Average communication time:      %g s\n", total_comm_time);
        melissa_print (VERBOSE_INFO, " --- Calcul time:                     %g s\n", total_computation_time);
        melissa_print (VERBOSE_INFO, " --- Waiting time:                    %g s\n", total_wait_time);
        melissa_print (VERBOSE_INFO, " --- Reading time:                    %g s\n", total_read_time);
        melissa_print (VERBOSE_INFO, " --- Writing time:                    %g s\n", total_write_time);
        melissa_print (VERBOSE_INFO, " --- Chekpointing time:               %g s\n", total_save_time);
        melissa_print (VERBOSE_INFO, " --- Total time:                      %g s\n", melissa_get_time() - start_time);
        melissa_print (VERBOSE_INFO, " --- MB recieved:                     %ld MB\n",total_mbytes_recv);
//        melissa_print (VERBOSE_INFO, " --- Stats structures memory:         %ld MB\n", mem_conso(&melissa_options));
        melissa_print (VERBOSE_INFO, " --- Bytes written:                   %ld MB\n", count_mbytes_written(&melissa_options));
        if (melissa_options.sobol_op == 1)
        {
            melissa_print (VERBOSE_INFO, " --- Worst Sobol confidence interval: %g (first order)\n", interval1);
            melissa_print (VERBOSE_INFO, " --- Worst Sobol confidence interval: %g (total order)\n", interval_tot);
        }
        melissa_print (VERBOSE_INFO, "\n");
    }

    melissa_close_output_lib();

    // === Sockets deconnexion === //

    zmq_close (connexion_responder);
    zmq_close (deconnexion_responder);
    zmq_close (data_puller);

    if (comm_data.rank == 0 && end_signal == 0)
    {
        sprintf (txt_buffer, "stop");
        zmq_send(text_pusher, txt_buffer, strlen(txt_buffer) * sizeof(char), 0);
    }
    zmq_close (text_pusher);
    zmq_close (text_puller);
    zmq_ctx_term (context);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
