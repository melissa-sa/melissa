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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <zmq.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
//#include <openturns/Study.hxx>

extern "C" {
#include "server.h"
#include "melissa_io.h"
#include "compute_stats.h"
#include "melissa_options.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#include "fault_tolerance.h"
#include "melissa_messages.h"
#include "melissa_output.h"
}

static volatile int end_signal = 0;

void sig_handler(int signo) {
    if (signo == SIGUSR1 || signo == SIGINT || signo == SIGUSR2)
        end_signal = signo;
}

void log_confidence_sobol_martinez(sobol_array_t *sobol_array,
                               int            nb_parameters,
                               int            vect_size)
{
    int j;
    // TODO: later we want to know the sobol from different timesteps t...
    for (j=0; j< nb_parameters; j++)
    {
        melissa_print(VERBOSE_INFO, "sobol confidence 1st order - parameter %d: %g\n",
            j, sobol_array->sobol_martinez[j].confidence_interval[0]);
    }
}

void melissa_server_init (int argc, char **argv, void **server_handle)
{
    melissa_server_t     *server_ptr;
//#if MELISSA4PY != 1
//    char*                 melissa_output_lib  = INSTALL_PREFIX"/lib/libmelissa_output.so";
//    char*                 melissa_output_func = "melissa_write_stats_seq";
//#endif // MELISSA4PY
    int                   i;
    melissa_simulation_t *simu_ptr;
    char                  txt_buffer[MPI_MAX_PROCESSOR_NAME + 50];
//    OT::Study             OTStudy;
    zmq_msg_t             msg;

    *server_handle = melissa_malloc (sizeof(melissa_server_t));

    server_ptr = (melissa_server_t*)*server_handle;

    // === init variables === //

    server_ptr->port_names = NULL;
    server_ptr->fields = NULL;
    server_ptr->nb_bufferized_messages = 32;
    server_ptr->nb_converged_fields = 0;
    server_ptr->start_time = 0;
    server_ptr->total_comm_time = 0;
    server_ptr->start_comm_time = 0;
    server_ptr->end_comm_time = 0;
    server_ptr->total_computation_time = 0;
    server_ptr->start_computation_time = 0;
    server_ptr->end_computation_time = 0;
    server_ptr->total_wait_time = 0;
    server_ptr->start_wait_time = 0;
    server_ptr->end_wait_time = 0;
    server_ptr->total_save_time = 0;
    server_ptr->start_save_time = 0;
    server_ptr->end_save_time = 0;
    server_ptr->total_read_time = 0;
    server_ptr->start_read_time = 0;
    server_ptr->end_read_time = 0;
    server_ptr->total_write_time = 0;
    server_ptr->total_mbytes_recv = 0;
    server_ptr->last_timeout_check = 0;
    server_ptr->nb_finished_simulations = 0;
    server_ptr->last_checkpoint_time = 0.0;
    server_ptr->timeout_launcher = 250;

    // === init ZMQ context === //

    server_ptr->context = zmq_ctx_new ();
    server_ptr->connexion_responder = zmq_socket (server_ptr->context, ZMQ_REP);
#ifdef CHECK_SIMU_DECONNECTION
    server_ptr->deconnexion_responder = zmq_socket (server_ptr->context, ZMQ_REP);
#endif // CHECK_SIMU_DECONNECTION
    server_ptr->data_puller = zmq_socket (server_ptr->context, ZMQ_PULL);
    server_ptr->text_puller = zmq_socket (server_ptr->context, ZMQ_SUB);
    server_ptr->text_pusher = zmq_socket (server_ptr->context, ZMQ_PUSH);
    server_ptr->text_requester = zmq_socket (server_ptr->context, ZMQ_REQ);

//    OTStudy.hasObject("toto");

#ifdef BUILD_WITH_MPI

    // === init MPI === //
    MPI_Initialized (&i);
    if (!i)
    {
        MPI_Init_thread (&argc, &argv, MPI_THREAD_FUNNELED , &i);
    }

    server_ptr->comm_data.comm = MPI_COMM_WORLD;
    MPI_Comm_size (server_ptr->comm_data.comm, &server_ptr->comm_data.comm_size);
    MPI_Comm_rank (server_ptr->comm_data.comm, &server_ptr->comm_data.rank);
#else
    server_ptr->comm_data.comm_size       = 1;
    server_ptr->comm_data.rank            = 0;
#endif // BUILD_WITH_MPI

    // === Get the node adress === //

    melissa_get_node_name (server_ptr->node_name);

    server_ptr->start_time = melissa_get_time();

    // === Read options from command line === //

    melissa_get_options (argc, argv, &server_ptr->melissa_options);

    // === Install signal handler === //

    if (signal(SIGINT, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa can't catch SIGINT\n");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa can't catch SIGUSR1\n");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
            melissa_print (VERBOSE_WARNING, "Melissa can't catch SIGUSR2\n");

    if (server_ptr->comm_data.rank == 0)
    {
        melissa_logo ();
        melissa_print_options (&server_ptr->melissa_options);
//        melissa_write_options (&melissa_options);

        server_ptr->port_names = (char*)melissa_malloc (MPI_MAX_PROCESSOR_NAME * server_ptr->comm_data.comm_size);
    }

    // === load the output library === //

//#if MELISSA4PY != 1
//    melissa_get_output_lib (melissa_output_lib, melissa_output_func);
//#endif // MELISSA4PY

    server_ptr->fields = (melissa_field_t*)melissa_malloc (server_ptr->melissa_options.nb_fields * sizeof(melissa_field_t));
    melissa_get_fields (argc, argv, server_ptr->fields, server_ptr->melissa_options.nb_fields);

    // === Open data puller port === //

    server_ptr->port_no = create_port_number(&server_ptr->comm_data,
                                             server_ptr->node_name,
                                             server_ptr->melissa_options.data_port,
                                             server_ptr->melissa_options.txt_push_port,
                                             server_ptr->melissa_options.txt_pull_port,
                                             server_ptr->melissa_options.txt_req_port,
                                             2002,
                                             2003);
    sprintf (txt_buffer, "tcp://*:%d", server_ptr->port_no);
    zmq_setsockopt (server_ptr->data_puller, ZMQ_RCVHWM, &server_ptr->nb_bufferized_messages, sizeof(int));
    melissa_bind (server_ptr->data_puller, txt_buffer);

    // === Gather port names on node 0 === //

    sprintf (txt_buffer, "tcp://%s:%d", server_ptr->node_name, server_ptr->port_no);
#ifdef BUILD_WITH_MPI
    MPI_Gather(txt_buffer, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, server_ptr->port_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, server_ptr->comm_data.comm);
#else // BUILD_WITH_MPI
    memcpy (server_ptr->port_names, txt_buffer, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI

    // === Open launcher ports === //
    i = 10000; // linger

    sprintf (txt_buffer, "tcp://%s:%d", server_ptr->melissa_options.launcher_name, server_ptr->melissa_options.txt_push_port);
    zmq_setsockopt (server_ptr->text_pusher, ZMQ_LINGER, &i, sizeof(int));
    melissa_connect (server_ptr->text_pusher, txt_buffer);

    sprintf (txt_buffer, "tcp://%s:%d", server_ptr->melissa_options.launcher_name, server_ptr->melissa_options.txt_pull_port);
    zmq_setsockopt(server_ptr->text_puller, ZMQ_SUBSCRIBE, "", 0);
    zmq_setsockopt (server_ptr->text_puller, ZMQ_LINGER, &i, sizeof(int));
    melissa_connect (server_ptr->text_puller, txt_buffer);

    // === opent req-rep port  === //
    sprintf (txt_buffer, "tcp://%s:%d", server_ptr->melissa_options.launcher_name, server_ptr->melissa_options.txt_req_port);
    zmq_setsockopt (server_ptr->text_requester, ZMQ_LINGER, &i, sizeof(int));
    i = 100000; // recv timeout
    zmq_setsockopt (server_ptr->text_requester, ZMQ_RCVTIMEO, &i, sizeof(int));
    melissa_connect (server_ptr->text_requester, txt_buffer);

    if (server_ptr->comm_data.rank == 0)
    {
//        sprintf (txt_buffer, "tcp://*:%d", server_ptr->melissa_options.txt_pull_port);
//        zmq_setsockopt (server_ptr->text_puller, ZMQ_LINGER, &i, sizeof(int));
//        melissa_bind (server_ptr->text_puller, txt_buffer);

        melissa_print (VERBOSE_INFO, "Server connected to launcher\n");
#ifdef CHECK_SIMU_DECONNECTION
        melissa_bind (server_ptr->deconnexion_responder, "tcp://*:2002");
#endif // CHECK_SIMU_DECONNECTION
        melissa_bind (server_ptr->connexion_responder, "tcp://*:2003");
        server_ptr->first_init = 2;
    }
    else
    {
        server_ptr->first_init = 1;
    }
    server_ptr->last_msg_launcher = melissa_get_time();

    // === send node 0 name to launcher === //

    send_message_server_name(server_ptr->node_name, server_ptr->comm_data.rank, server_ptr->text_pusher, 0);
    if (server_ptr->comm_data.rank == 0)
    {
        melissa_print (VERBOSE_INFO, "Server node name sent to launcher\n");
    }

    // === get stats options from the launcher === //
    zmq_msg_init_size (&msg, strlen("options") + 1);
    sprintf((char*)zmq_msg_data (&msg), "options");
    zmq_msg_send (&msg, server_ptr->text_requester, 0);
    int size = 0;
    zmq_msg_init(&msg);
    while (size < 1)
    {
        size = zmq_msg_recv (&msg, server_ptr->text_requester, 0);
    }
    server_ptr->last_msg_launcher = melissa_get_time();
    process_launcher_message (zmq_msg_data (&msg), server_ptr);
    zmq_msg_close (&msg);

    if (server_ptr->melissa_options.restart != 1)
    {
        alloc_vector (&server_ptr->simulations, server_ptr->melissa_options.sampling_size);
        for (i=0; i<server_ptr->melissa_options.sampling_size; i++)
        {
            vector_add (&server_ptr->simulations, add_simulation());
        }
    }
    else
    {
        // === Restart initialisation === //
        if (server_ptr->comm_data.rank == 0)
        {
            melissa_print (VERBOSE_INFO, "Reading simulation states at checkpoint time...\n");
        }
        read_simu_states(&server_ptr->simulations, &server_ptr->melissa_options, &server_ptr->comm_data);
        for (i=0; i<server_ptr->simulations.size; i++)
        {
            simu_ptr = (melissa_simulation_t*)server_ptr->simulations.items[i];
            if (simu_ptr->status == 2)
            {
                server_ptr->nb_finished_simulations += 1;
            }
        }
        for (i=0; i<server_ptr->simulations.size; i++)
        {
            simu_ptr = (melissa_simulation_t*)server_ptr->simulations.items[i];
            if (simu_ptr->status == 1)
            {
                simu_ptr->status = 0;
            }
        }
        if (server_ptr->comm_data.rank == 0)
        {
            for (i=0; i<server_ptr->simulations.size; i++)
            {
                simu_ptr = (melissa_simulation_t*)server_ptr->simulations.items[i];
                melissa_print (VERBOSE_DEBUG, "Simu_state %d %d\n", i, simu_ptr->status);
                send_message_simu_status(i, simu_ptr->status, server_ptr->text_pusher, 0);
            }
        }
        server_ptr->melissa_options.sampling_size = server_ptr->simulations.size;
        if (server_ptr->comm_data.rank == 0)
        {
            melissa_print (VERBOSE_INFO, "Reading simulation states at checkpoint time ok \n");
        }
    }
}

void melissa_server_run (void **server_handle, simulation_data_t *simu_data)
{
    melissa_server_t     *server_ptr;
    int                   simu_id, i;
    int                   field_id;
    int                   old_simu_state;
#ifdef CHECK_SIMU_DECONNECTION
    int                   old_last_time_step_state;
#endif // CHECK_SIMU_DECONNECTION
    int                   recv_vect_size = 0;
    int                   client_rank;
    int                   new_data = 0;
    char                 *buf_ptr;
    char                  txt_buffer[MPI_MAX_PROCESSOR_NAME];
    zmq_msg_t             msg;
    int                   wait_launcher_msg = 0;
    melissa_simulation_t *simu_ptr = NULL;
    char                 *field_name_ptr = NULL;
    melissa_data_t       *data_ptr = NULL;

    server_ptr = (melissa_server_t*)*server_handle;

    simu_data->first_init = 0;
    simu_data->status = 0;

    // =================== //
    // ===  Main loop  === //
    // =================== //

    while (1)
    {

        // === check timeouts === //

        if (server_ptr->comm_data.rank == 0)
        {
            if (server_ptr->last_timeout_check + 20 < melissa_get_time())
            {
                // === check simulations timeouts === //
                server_ptr->detected_timeouts = check_timeouts(&server_ptr->simulations,
                                                   server_ptr->melissa_options.timeout_simu);
                server_ptr->last_timeout_check = melissa_get_time();
                if (server_ptr->detected_timeouts > 0)
                {
                    send_timeouts (server_ptr->detected_timeouts,
                                   &server_ptr->simulations,
                                   server_ptr->text_pusher);
                }
            }
        }
        // === check launcher timeouts === //
        if (server_ptr->last_msg_launcher + server_ptr->timeout_launcher < melissa_get_time())
        {
            if (server_ptr->comm_data.rank == 0)
            {
                melissa_print (VERBOSE_ERROR, " --- Server detected Launcher timeout ---\n Melissa will stop\n");
            }
            // remove unsubmited simulations from the sample
            i = count_job_status(&server_ptr->simulations, -1);
            server_ptr->melissa_options.sampling_size -= i;
            if (server_ptr->comm_data.rank == 0)
            {
                melissa_print (VERBOSE_ERROR, "Remove %d samples\n", i);
                melissa_print (VERBOSE_ERROR, "Waiting for remaining simulations to complete\n");
            }
        }

        if (server_ptr->last_checkpoint_time + server_ptr->melissa_options.check_interval < melissa_get_time() && server_ptr->last_checkpoint_time > 0.1)
        {
            server_ptr->start_save_time = melissa_get_time();
            for (i=0; i<server_ptr->melissa_options.nb_fields; i++)
            {
                save_stats(server_ptr->fields[i].stats_data, &server_ptr->comm_data, server_ptr->fields[i].name);
                if (server_ptr->comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    melissa_print(VERBOSE_DEBUG, "Statistic field %s saved in %s\n", server_ptr->fields[i].name, dir);
                }
            }
            save_simu_states (&server_ptr->simulations, &server_ptr->comm_data);
            server_ptr->last_checkpoint_time = melissa_get_time();
            server_ptr->end_save_time = melissa_get_time();
            melissa_print(VERBOSE_DEBUG, "Chekpoint time: %g (proc %d)\n", server_ptr->end_save_time - server_ptr->start_save_time, server_ptr->comm_data.rank);
            server_ptr->total_save_time += server_ptr->end_save_time - server_ptr->start_save_time;
            simu_data->status = 2;
            if (server_ptr->melissa_options.learning > 0)
            {
                break;
            }
        }

        // poll on ZMQ ports

        server_ptr->start_wait_time = melissa_get_time();
        zmq_pollitem_t items [] = {
            { server_ptr->text_puller, 0, ZMQ_POLLIN, 0 },
            { server_ptr->connexion_responder, 0, ZMQ_POLLIN, 0 },
            { server_ptr->data_puller, 0, ZMQ_POLLIN, 0 }
#ifdef CHECK_SIMU_DECONNECTION
            ,{ server_ptr->deconnexion_responder, 0, ZMQ_POLLIN, 0 }
#endif // CHECK_SIMU_DECONNECTION
        };
#ifdef CHECK_SIMU_DECONNECTION
        zmq_poll (items, 4, 100);
#else // CHECK_SIMU_DECONNECTION
        zmq_poll (items, 3, 100);
#endif // CHECK_SIMU_DECONNECTION
        server_ptr->end_wait_time = melissa_get_time();
        server_ptr->total_wait_time += server_ptr->end_wait_time - server_ptr->start_wait_time;

        // === If message on the text port === //

        if (items[0].revents & ZMQ_POLLIN)
        {
//            char text[melissa_get_message_len()];
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, server_ptr->text_puller, 0);
            melissa_print (VERBOSE_DEBUG, "Recieved %s (rank %d)\n", zmq_msg_data (&msg), server_ptr->comm_data.rank);
            server_ptr->last_msg_launcher = melissa_get_time();
            process_launcher_message(zmq_msg_data (&msg), server_ptr);
            server_ptr->melissa_options.sampling_size = server_ptr->simulations.size;
            zmq_msg_close (&msg);
//            zmq_recv (server_ptr->text_puller, text, melissa_get_message_len()-1, 0);
//            melissa_print (VERBOSE_DEBUG, "Recieved %s (rank %d)\n", text, server_ptr->comm_data.rank);
//            server_ptr->last_msg_launcher = melissa_get_time();
//            process_launcher_message(text, server_ptr);
            if (server_ptr->melissa_options.sampling_size < server_ptr->simulations.size)
            {
                server_ptr->melissa_options.sampling_size = server_ptr->simulations.size;
            }
        }

        // === If message on the connexion port === //

        if (items[1].revents & ZMQ_POLLIN)
        {
            if (server_ptr->comm_data.rank == 0)
            {
                server_ptr->start_comm_time = melissa_get_time();
                // new simulation wants to connect
                zmq_msg_init (&msg);
                zmq_msg_recv (&msg, server_ptr->connexion_responder, 0);
                memcpy(server_ptr->rinit_tab, zmq_msg_data (&msg), 2 * sizeof(int));
                zmq_msg_close (&msg);
                zmq_msg_init_size (&msg, 5 * sizeof(int) + server_ptr->comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                buf_ptr = (char*)zmq_msg_data (&msg);
                memcpy (buf_ptr, &server_ptr->comm_data.comm_size, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &server_ptr->melissa_options.sobol_op, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &server_ptr->melissa_options.learning, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &server_ptr->melissa_options.nb_parameters, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, &server_ptr->melissa_options.verbose_lvl, sizeof(int));
                buf_ptr += sizeof(int);
                memcpy (buf_ptr, server_ptr->port_names, server_ptr->comm_data.comm_size * MPI_MAX_PROCESSOR_NAME * sizeof(char));
                zmq_msg_send (&msg, server_ptr->connexion_responder, 0);
                if (server_ptr->first_init == 2)
                {
                    server_ptr->first_init = 1;
                }
                server_ptr->end_comm_time = melissa_get_time();
                server_ptr->total_comm_time += server_ptr->end_comm_time - server_ptr->start_comm_time;
            }
        }

        // === Only after the first connexion, broadcast client === //

        if (server_ptr->first_init == 1)
        {
#ifdef BUILD_WITH_MPI
            MPI_Bcast(server_ptr->rinit_tab, 2, MPI_INT, 0, server_ptr->comm_data.comm);
#endif // BUILD_WITH_MPI
            server_ptr->comm_data.client_comm_size = server_ptr->rinit_tab[0];
            if (server_ptr->melissa_options.learning > 0)
            {
                server_ptr->comm_data.client_comm_size = 1;
            }
            server_ptr->first_send = (int*)melissa_calloc(server_ptr->melissa_options.nb_fields*server_ptr->comm_data.client_comm_size, sizeof(int));
            for (i=0; i<server_ptr->melissa_options.nb_fields; i++)
            {
                server_ptr->fields[i].client_vect_sizes = (int*)melissa_calloc (server_ptr->comm_data.client_comm_size, sizeof(int));
            }
            if (server_ptr->melissa_options.sobol_op == 1)
            {
                server_ptr->buff_tab_ptr = (double**)melissa_malloc ((server_ptr->melissa_options.nb_parameters + 2) * sizeof(double*));
            }
            else
            {
                server_ptr->buff_tab_ptr = (double**)melissa_malloc (sizeof(double*));
            }
            server_ptr->local_nb_messages = 0;
            add_fields(server_ptr->fields,
                       server_ptr->comm_data.client_comm_size,
                       server_ptr->melissa_options.nb_fields);

            server_ptr->first_init = 0;
            simu_data->first_init = 1;
            simu_data->status = 0;
            simu_data->val_size = 0;
            simu_data->max_val_size = 0;
            simu_data->nb_param = server_ptr->melissa_options.nb_parameters;
            simu_data->parameters = (double*)melissa_malloc(server_ptr->melissa_options.nb_parameters * sizeof(double));
//            return;
        }

        // === Data reception and statistics computation === //
        // code where the data for one time step from one simulation and one field arrives
        if (items[2].revents & ZMQ_POLLIN)
        {
            server_ptr->start_comm_time = melissa_get_time();
            zmq_msg_init (&msg);
            zmq_msg_recv (&msg, server_ptr->data_puller, 0);
            buf_ptr = (char*)zmq_msg_data (&msg);
            server_ptr->end_comm_time = melissa_get_time();
            server_ptr->total_comm_time += server_ptr->end_comm_time - server_ptr->start_comm_time;

//            memcpy(&time_step, buf_ptr, sizeof(int));
//          TODO: why not using apacked stuct here??
            memcpy(&simu_data->time_stamp, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&simu_id, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
//            memcpy(&group_id, buf_ptr, sizeof(int));
            memcpy(&simu_data->simu_id, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&client_rank, buf_ptr, sizeof(int));
            buf_ptr += sizeof(int);
            memcpy(&recv_vect_size, buf_ptr, sizeof(int));
            if (recv_vect_size > simu_data->max_val_size && recv_vect_size > 0)
            {
                melissa_print (VERBOSE_DEBUG, "realloc, new size: %d\n", recv_vect_size);
                simu_data->val = (double*)melissa_realloc(simu_data->val, recv_vect_size*sizeof(double));
                simu_data->max_val_size = recv_vect_size;
            }
            simu_data->val_size = recv_vect_size;
            buf_ptr += sizeof(int);
            field_name_ptr = buf_ptr;
            new_data = 1;

            melissa_print (VERBOSE_DEBUG, "Server rank %d recieved timestep %d from rank %d of group %d (vect_size: %d)\n", server_ptr->comm_data.rank, simu_data->time_stamp, client_rank, simu_data->simu_id, recv_vect_size);

            if (simu_data->time_stamp >= server_ptr->melissa_options.nb_time_steps || simu_data->time_stamp < 0)
            {
                melissa_print (VERBOSE_WARNING, "Bad time stamp (field %s)\n", field_name_ptr);
                continue;
            }

            field_id = get_field_id(server_ptr->fields, server_ptr->melissa_options.nb_fields, field_name_ptr);
            if (field_id == -1)
            {
                if (simu_data->time_stamp == 0 && client_rank == 0)
                {
                    melissa_print (VERBOSE_WARNING, "Not computing field %s\n", field_name_ptr);
                }
                continue;
            }
            if (server_ptr->first_send[field_id*server_ptr->comm_data.client_comm_size+client_rank] == 0)
            {
                server_ptr->local_nb_messages += 1;
                server_ptr->first_send[field_id*server_ptr->comm_data.client_comm_size+client_rank] = 1;
            }
            if (simu_data->simu_id > server_ptr->simulations.size)
            {
                for (i=server_ptr->simulations.size; i<simu_data->simu_id; i++)
                {
                    vector_add (&server_ptr->simulations, add_simulation());
                }
                if (server_ptr->melissa_options.sampling_size < server_ptr->simulations.size)
                {
                    server_ptr->melissa_options.sampling_size = server_ptr->simulations.size;
                }
            }

            data_ptr = server_ptr->fields[field_id].stats_data;
            if (data_ptr[client_rank].stats_init != 1 && recv_vect_size > 0)
            {
                melissa_init_data (&data_ptr[client_rank], &server_ptr->melissa_options, recv_vect_size);
                server_ptr->last_checkpoint_time = melissa_get_time();
                if (server_ptr->melissa_options.restart > 0)
                {
                    server_ptr->start_read_time = melissa_get_time();
                    if (server_ptr->comm_data.rank == 0)
                    {
                        melissa_print (VERBOSE_INFO, "reading checkpoint files...\n");
                    }
                    read_saved_stats (data_ptr, &server_ptr->comm_data, field_name_ptr, client_rank);
                    if (server_ptr->comm_data.rank == 0)
                    {
                        melissa_print (VERBOSE_INFO, "reading checkpoint files ok\n");
                    }
                    server_ptr->last_checkpoint_time = melissa_get_time();
                    server_ptr->end_read_time = melissa_get_time();
                    server_ptr->total_read_time += server_ptr->end_read_time - server_ptr->start_read_time;
                    simu_data->status = 3;
                }
            }
            else if (data_ptr[client_rank].steps_init != 1)
            {
                melissa_init_data (&data_ptr[client_rank], &server_ptr->melissa_options, recv_vect_size);
                server_ptr->last_checkpoint_time = melissa_get_time();
            }
            simu_ptr = (melissa_simulation_t*)server_ptr->simulations.items[simu_data->simu_id];

            if (simu_ptr->parameters == NULL && recv_vect_size > 0)
            {
                // ask launcher for the simulation informations
                sprintf (txt_buffer, "simu_info %d", simu_data->simu_id);
                zmq_send(server_ptr->text_requester, txt_buffer, strlen(txt_buffer), 0);
                sprintf (txt_buffer, "wait response...\n");
                wait_launcher_msg = 1;
            }

            simu_ptr->last_message = melissa_get_time();
            if (recv_vect_size > 0)
            {
                buf_ptr += MAX_FIELD_NAME * sizeof(char);
                memcpy(simu_data->val, buf_ptr, recv_vect_size*sizeof(double));
            }
            server_ptr->total_mbytes_recv += zmq_msg_size (&msg);
            server_ptr->start_computation_time = melissa_get_time();

            if (simu_data->simu_id >= data_ptr[client_rank].step_simu.size)
            {
                uint32_t *item;
                for (i=data_ptr[client_rank].step_simu.size; i<simu_data->simu_id; i++)
                {
                    item = (uint32_t*)melissa_calloc((data_ptr[client_rank].options->nb_time_steps+31)/32, sizeof(uint32_t));
                    vector_add(&data_ptr[client_rank].step_simu, (void*)item);

                }
            }

            if (test_bit ((uint32_t*)data_ptr[client_rank].step_simu.items[simu_data->simu_id], simu_data->time_stamp) != 0)
            {
                // Time step already computed, message ignored.
                melissa_print (VERBOSE_WARNING,  "Allready computed time step (simulation %d, time step %d)\n", simu_data->simu_id, simu_data->time_stamp);
                new_data = 0;
            }

            if (new_data == 1)
            {
                if (recv_vect_size > 0)
                {
                    if (server_ptr->melissa_options.sobol_op != 1)
                    {
                        server_ptr->buff_tab_ptr[0] = (double*)buf_ptr;
                        // === Compute classical statistics === //
                        compute_stats (&data_ptr[client_rank],
                                       simu_data->time_stamp,
                                       simu_data->simu_id,
                                       1,
                                       server_ptr->buff_tab_ptr);
                    }
                    else
                    {
                        for (i=0; i<server_ptr->melissa_options.nb_parameters+2; i++)
                        {
                            server_ptr->buff_tab_ptr[i] = (double*)buf_ptr;
                            buf_ptr += recv_vect_size * sizeof(double);
                        }
                        // === Compute classical statistics + Sobol indices === //
                        compute_stats (&data_ptr[client_rank],
                                       simu_data->time_stamp,
                                       simu_data->simu_id,
                                       server_ptr->melissa_options.nb_parameters+2,
                                       server_ptr->buff_tab_ptr);
//                        confidence_sobol_martinez (&(data_ptr[client_rank].sobol_indices[simu_data->time_stamp]),
//                                server_ptr->melissa_options.nb_parameters,
//                                data_ptr[client_rank].vect_size);

                        if (server_ptr->comm_data.rank == 0 &&
                                simu_data->time_stamp == server_ptr->melissa_options.nb_time_steps -1)
                        {
                            // REM: atm only showing for last timestep on 0 rank
//                            log_confidence_sobol_martinez(&(data_ptr[client_rank].sobol_indices[simu_data->time_stamp]),
//                                    server_ptr->melissa_options.nb_parameters,
//                                    data_ptr[client_rank].vect_size);

                            send_message_confidence_interval("Sobol",
                                                             field_name_ptr,
                                                             simplified_confidence_sobol_martinez (data_ptr[client_rank].sobol_indices[simu_data->time_stamp].iteration),
                                                             server_ptr->text_pusher,
                                                             0);

                        }

                        server_ptr->nb_converged_fields += check_convergence_sobol_martinez(&(data_ptr[client_rank].sobol_indices),
                                                                                            0.01,
                                                                                            server_ptr->melissa_options.nb_time_steps,
                                                                                            server_ptr->melissa_options.nb_parameters);
                    }
                }
                set_bit((uint32_t*)data_ptr[client_rank].step_simu.items[simu_data->simu_id], simu_data->time_stamp);
            }
            server_ptr->end_computation_time = melissa_get_time();
            server_ptr->total_computation_time += server_ptr->end_computation_time - server_ptr->start_computation_time;

            if (wait_launcher_msg == 1)
            {
                wait_launcher_msg = 0;
                zmq_msg_t msg2;
//                    char text[melissa_get_message_len()];
                printf (" Waiting launcher message\n");
                zmq_msg_init (&msg2);
                zmq_msg_recv (&msg2, server_ptr->text_requester, 0);
                printf (" -- > message: %s\n", (char*)zmq_msg_data (&msg2));
                server_ptr->last_msg_launcher = melissa_get_time();
                process_launcher_message(zmq_msg_data (&msg2), server_ptr);
                zmq_msg_close (&msg2);
//                    zmq_recv (server_ptr->text_requester, text, melissa_get_message_len()-1, 0);
//                    server_ptr->last_msg_launcher = melissa_get_time();
//                    process_launcher_message(text, server_ptr);
            }
            memcpy(simu_data->parameters, simu_ptr->parameters, sizeof(double)*server_ptr->melissa_options.nb_parameters);


            // check the simulation progress //
            old_simu_state = simu_ptr->status;
            simu_ptr->status = check_simu_state(server_ptr->fields, server_ptr->melissa_options.nb_fields, simu_data->simu_id, server_ptr->melissa_options.nb_time_steps, &server_ptr->comm_data);
            simu_ptr->job_status = 1;
            melissa_print(VERBOSE_DEBUG, "Group %d, rank %d, status %d\n", simu_data->simu_id, server_ptr->comm_data.rank, simu_ptr->status);

#ifdef CHECK_SIMU_DECONNECTION
            // check if we recieved all the last timestep messages //
            if (simu_data->time_stamp == server_ptr->melissa_options.nb_time_steps-1)
            {
                old_last_time_step_state = simu_ptr->last_time_step;
                simu_ptr->last_time_step = check_last_timestep(server_ptr->fields, server_ptr->melissa_options.nb_fields, simu_data->simu_id, server_ptr->melissa_options.nb_time_steps, &server_ptr->comm_data);
                melissa_print(VERBOSE_DEBUG, "Group %d, rank %d, last timestep status: %d\n", simu_data->simu_id, server_ptr->comm_data.rank, simu_ptr->status);
            }
#endif // CHECK_SIMU_DECONNECTION

            if (simu_ptr->status == 2 && old_simu_state != 2)
            {
#ifdef CHECK_SIMU_DECONNECTION
                if (server_ptr->comm_data.rank != 0)
                {
                    server_ptr->nb_finished_simulations += 1;
                }
#else // CHECK_SIMU_DECONNECTION
                server_ptr->nb_finished_simulations += 1;
#endif // CHECK_SIMU_DECONNECTION
            }

            // === Send a message to the Python master in case of simulation status update === //  TODO: can't we put all this stuff into functions?  technical debt?
#ifdef CHECK_SIMU_DECONNECTION
            if (old_simu_state != simu_ptr->status && server_ptr->comm_data.rank == 0 && simu_ptr->status == 1)
#else // CHECK_SIMU_DECONNECTION
            if (old_simu_state != simu_ptr->status && server_ptr->comm_data.rank == 0)
#endif // CHECK_SIMU_DECONNECTION
            {
                send_message_simu_status(simu_data->simu_id, simu_ptr->status, server_ptr->text_pusher, 0);
                if (simu_ptr->status == 2)
                {
                    melissa_print(VERBOSE_INFO, "Simulation %d finished\n", simu_data->simu_id);
                    melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", server_ptr->nb_finished_simulations, server_ptr->simulations.size);
                }
            }

#ifdef CHECK_SIMU_DECONNECTION
            // === Send a message to the Python master in case of last timestep status update === //
            if (old_last_time_step_state != simu_ptr->last_time_step && server_ptr->comm_data.rank == 0 && simu_ptr->last_time_step == 1)
            {
                sprintf (txt_buffer, "timestep_state %d %d", simu_data->simu_id, simu_ptr->last_time_step);
                melissa_print(VERBOSE_DEBUG, "Send \"%s\" to launcher\n", txt_buffer);
                zmq_send(server_ptr->text_pusher, txt_buffer, strlen(txt_buffer), 0);
            }
#endif // CHECK_SIMU_DECONNECTION

            if (server_ptr->melissa_options.sobol_op != 1)
            {
                server_ptr->buff_tab_ptr[0] = NULL;
            }
            else
            {
                for (i=0; i<server_ptr->melissa_options.nb_parameters+2; i++)
                {
                    server_ptr->buff_tab_ptr[i] = NULL;
                }
            }
            buf_ptr = NULL;
            zmq_msg_close (&msg);
        }

#ifdef CHECK_SIMU_DECONNECTION
        if (items[3].revents & ZMQ_POLLIN)
        {
            if (server_ptr->comm_data.rank == 0)
            {
                server_ptr->start_comm_time = melissa_get_time();
                zmq_msg_init (&msg);
                zmq_msg_recv (&msg, server_ptr->deconnexion_responder, 0);
                memcpy(&simu_data->simu_id, zmq_msg_data (&msg), sizeof(int));
                zmq_msg_close (&msg);
                melissa_print (VERBOSE_DEBUG, "Group %d ask to disconnect \n", simu_data->simu_id);
                // simulation wants to disconnect
                simu_ptr = (melissa_simulation_t*)server_ptr->simulations.items[simu_data->simu_id];
                zmq_msg_init_size (&msg, sizeof(int));
                memcpy (zmq_msg_data (&msg), &simu_ptr->last_time_step, sizeof(int));
                zmq_msg_send (&msg, server_ptr->deconnexion_responder, 0);
                server_ptr->end_comm_time = melissa_get_time();
                server_ptr->total_comm_time += server_ptr->end_comm_time - server_ptr->start_comm_time;
                if (simu_ptr->status == 2)
                {
                    send_message_simu_status(simu_data->simu_id, simu_ptr->status, server_ptr->text_pusher, 0);
                    simu_ptr->status = 3;
                    server_ptr->nb_finished_simulations += 1;
                    melissa_print(VERBOSE_INFO, "Simulation %d finished\n", simu_data->simu_id);
                    melissa_print(VERBOSE_INFO, "Finished simulations: %d/%d\n", server_ptr->nb_finished_simulations, server_ptr->simulations.size);
                }
            }
        }
#endif // CHECK_SIMU_DECONNECTION

        // === Signal handling === //

        if (end_signal == SIGINT || end_signal == SIGUSR1 || end_signal == SIGUSR2)
        {
            server_ptr->start_save_time = melissa_get_time();
            if (server_ptr->comm_data.rank == 0)
            {
                melissa_print(VERBOSE_WARNING, "\n --- INTERUPTED ---\n");
            }
            for (i=0; i<server_ptr->melissa_options.nb_fields; i++)
            {
                save_stats (server_ptr->fields[i].stats_data, &server_ptr->comm_data, server_ptr->fields[i].name);
                if (server_ptr->comm_data.rank == 0)
                {
                    char dir[256];
                    getcwd(dir, 256*sizeof(char));
                    melissa_print(VERBOSE_INFO, "Statistic fields saved in %s\n\n", dir);
                }
            }
            save_simu_states (&server_ptr->simulations, &server_ptr->comm_data);
            if (end_signal == SIGINT)
            {
#ifdef BUILD_WITH_MPI
                MPI_Finalize ();
#endif // BUILD_WITH_MPI
                return;
            }
            server_ptr->end_save_time = melissa_get_time();
            melissa_print(VERBOSE_DEBUG, "Chekpoint time: %g (proc %d)\n", server_ptr->end_save_time - server_ptr->start_save_time, server_ptr->comm_data.rank);
            server_ptr->total_save_time += server_ptr->end_save_time - server_ptr->start_save_time;
            break;
        }

        // === Send a message to the Python Launcher in case of Sobol indices convergence === //

        if (server_ptr->first_init == 0 &&
            server_ptr->nb_converged_fields >= server_ptr->melissa_options.nb_fields * server_ptr->local_nb_messages &&
            server_ptr->melissa_options.sobol_op == 1 &&
            server_ptr->nb_finished_simulations > 1)
        {
            sprintf (txt_buffer, "converged %d", server_ptr->comm_data.rank);
            zmq_send(server_ptr->text_pusher, txt_buffer, strlen(txt_buffer), 0);
            //                if (strcmp ("stop", txt_buffer))
            //                {
            //                    break;
            //                }
        }

        if (server_ptr->nb_finished_simulations >= server_ptr->melissa_options.sampling_size && server_ptr->nb_finished_simulations > 0)
        {
            simu_data->status = 1;
            break;
        }

        if (new_data == 1 && server_ptr->melissa_options.learning > 0 && simu_data->val_size > 0)
        {
            break;
        }
    }

}

void melissa_server_finalize (void** server_handle, simulation_data_t *simu_data)
{
    melissa_server_t *server_ptr;
    double            interval1;
//    double            interval_tot;
    int               i;

    server_ptr = (melissa_server_t*)*server_handle;

    melissa_free (simu_data->val);

    for (i=0; i<server_ptr->melissa_options.nb_fields; i++)
    {
        save_stats (server_ptr->fields[i].stats_data, &server_ptr->comm_data, server_ptr->fields[i].name);
    }

    save_simu_states (&server_ptr->simulations, &server_ptr->comm_data);

    if (server_ptr->comm_data.rank == 0)
    {
        write_simu_param(&server_ptr->simulations,
                         server_ptr->melissa_options.nb_parameters);
    }

    if (end_signal == 0)
    {
        melissa_free (server_ptr->buff_tab_ptr);
    }

    if (server_ptr->comm_data.rank == 0)
    {
        melissa_free(server_ptr->port_names);
    }
    melissa_free(simu_data->parameters);

    interval1 = 0;
//    interval_tot = 0;
    if (server_ptr->melissa_options.sobol_op == 1)
    {
//        global_confidence_sobol_martinez (server_ptr->fields,
//                                          server_ptr->melissa_options.nb_fields,
//                                          &server_ptr->comm_data,
//                                          &interval1,
//                                          &interval_tot);
        interval1 = simplified_confidence_sobol_martinez (server_ptr->nb_finished_simulations);
    }

    if (end_signal == 0)
    {
        finalize_field_data (server_ptr->fields,
                             &server_ptr->comm_data,
                             &server_ptr->melissa_options,
                             &server_ptr->total_write_time);
    }
    melissa_free (server_ptr->first_send);
    free_simu_vector (server_ptr->simulations);

#ifdef BUILD_WITH_MPI
    double temp1;
    long int temp2;
    MPI_Reduce (&interval1, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    interval1 = temp1;
//    MPI_Reduce (&interval_tot, &temp1, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
//    interval_tot = temp1;
    MPI_Reduce (&server_ptr->total_comm_time, &temp1, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    server_ptr->total_comm_time = temp1 / server_ptr->comm_data.comm_size;
    MPI_Reduce (&server_ptr->total_mbytes_recv, &temp2, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    server_ptr->total_mbytes_recv = temp2 / 1000000;
#endif // BUILD_WITH_MPI
    if (server_ptr->comm_data.rank==0)
    {
        melissa_print (VERBOSE_INFO, " --- Number of simulations:           %d\n", server_ptr->melissa_options.nb_simu);
        melissa_print (VERBOSE_INFO, " --- Number of simulation processes:  %d\n", server_ptr->comm_data.client_comm_size);
        melissa_print (VERBOSE_INFO, " --- Number of analysis processes:    %d\n", server_ptr->comm_data.comm_size);
        melissa_print (VERBOSE_INFO, " --- Average communication time:      %g s\n", server_ptr->total_comm_time);
        melissa_print (VERBOSE_INFO, " --- Calcul time:                     %g s\n", server_ptr->total_computation_time);
        melissa_print (VERBOSE_INFO, " --- Waiting time:                    %g s\n", server_ptr->total_wait_time);
        melissa_print (VERBOSE_INFO, " --- Reading time:                    %g s\n", server_ptr->total_read_time);
        melissa_print (VERBOSE_INFO, " --- Writing time:                    %g s\n", server_ptr->total_write_time);
        melissa_print (VERBOSE_INFO, " --- Chekpointing time:               %g s\n", server_ptr->total_save_time);
        melissa_print (VERBOSE_INFO, " --- Total time:                      %g s\n", melissa_get_time() - server_ptr->start_time);
        melissa_print (VERBOSE_INFO, " --- MB received:                     %ld MB\n",server_ptr->total_mbytes_recv);
//        melissa_print (VERBOSE_INFO, " --- Stats structures memory:         %ld MB\n", mem_conso(&melissa_options));
//        melissa_print (VERBOSE_INFO, " --- Bytes written:                   %ld MB\n", count_mbytes_written(&server_ptr->melissa_options));
        if (server_ptr->melissa_options.sobol_op == 1)
        {
            melissa_print (VERBOSE_INFO, " --- Worst Sobol confidence interval: %g \n", interval1);
//            melissa_print (VERBOSE_INFO, " --- Worst Sobol confidence interval: %g (first order)\n", interval1);
//            melissa_print (VERBOSE_INFO, " --- Worst Sobol confidence interval: %g (total order)\n", interval_tot);
        }
//        melissa_print (VERBOSE_INFO, " \n");
    }

//#if MELISSA4PY != 1
//    melissa_close_output_lib();
//#endif // MELISSA4PY

    // === Sockets deconnexion === //

    zmq_close (server_ptr->connexion_responder);
#ifdef CHECK_SIMU_DECONNECTION
    zmq_close (server_ptr->deconnexion_responder);
#endif // CHECK_SIMU_DECONNECTION
    zmq_close (server_ptr->data_puller);

    if (server_ptr->comm_data.rank == 0 && end_signal == 0)
    {
        send_message_stop (server_ptr->text_pusher, 0);
    }
    zmq_close (server_ptr->text_pusher);
    zmq_close (server_ptr->text_puller);
    zmq_close (server_ptr->text_requester);
    zmq_ctx_term (server_ptr->context);

}

int main (int argc, char **argv)
{
    void* melissa_server_ptr;
    simulation_data_t simu_data;
    simu_data.status = 0;
    simu_data.val_size = 0;
    simu_data.max_val_size = 0;
    simu_data.val = (double*)melissa_malloc(0);
#ifdef BUILD_WITH_MPI
    int i;
    MPI_Init_thread (&argc, &argv, MPI_THREAD_FUNNELED , &i);
#endif // BUILD_WITH_MPI
    melissa_server_init (argc, argv, &melissa_server_ptr);
    while (simu_data.status != 1)
    {
        melissa_server_run (&melissa_server_ptr, &simu_data);
    }
    melissa_print(VERBOSE_INFO, "Finalizing, writing results...\n");
    melissa_server_finalize (&melissa_server_ptr, &simu_data);
#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
