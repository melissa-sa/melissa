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
 * @file server.h
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 **/

#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H
#include <zmq.h>
#include "melissa_fields.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#include "fault_tolerance.h"
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI

struct melissa_server_s
{
    melissa_options_t     melissa_options;
    melissa_field_t      *fields;
    comm_data_t           comm_data;
    int                   port_no;
    char                 *node_names;
    int                   rinit_tab[2];
    char                  node_name[MPI_MAX_PROCESSOR_NAME];
    void                 *context;
    void                 *connexion_responder;
    void                 *deconnexion_responder;
    void                 *data_puller;
    void                 *text_puller;
    void                 *text_pusher;
    int                   first_init;
    int                  *first_send;
    int                   local_nb_messages;
    int                   nb_bufferized_messages;
    int                   nb_converged_fields;
    double              **buff_tab_ptr;
    double                start_time;
    double                total_comm_time;
    double                start_comm_time;
    double                end_comm_time;
    double                total_computation_time;
    double                start_computation_time;
    double                end_computation_time;
    double                total_wait_time;
    double                start_wait_time;
    double                end_wait_time;
    double                total_save_time;
    double                start_save_time;
    double                end_save_time;
    double                total_read_time;
    double                start_read_time;
    double                end_read_time;
    double                total_write_time;
    long int              total_mbytes_recv;
    double                last_timeout_check;
    int                   detected_timeouts;
    int                   nb_finished_simulations;
    double                last_checkpoint_time;
    double                last_msg_launcher;
    double                timeout_launcher;
    vector_t              simulations;
};

typedef struct melissa_server_s melissa_server_t; /**< type corresponding to melissa_server_s */

struct simulation_data_s
{
    int     simu_id;
    int     time_stamp;
    int     first_init;
    int     end;
    double *val;
    int     val_size;
};

typedef struct simulation_data_s simulation_data_t; /**< type corresponding to simulation_data_s */

void melissa_server_init (int argc, char **argv, void **melissa_server_ptr);

void melissa_server_run (void **melissa_server_ptr, simulation_data_t *simu_data);

void melissa_server_finalize (void **melissa_server_ptr, simulation_data_t *simu_data);

int check_simu_state(melissa_field_t *field,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data);

long int count_mbytes_written (melissa_options_t  *options);

int string_recv (void *socket,
                 char *recv_buff);

void global_confidence_sobol_martinez(melissa_field_t *field,
                                      int              nb_fields,
                                      comm_data_t     *comm_data,
                                      double          *interval1,
                                      double          *interval_tot);

#endif // SERVER_HELPER_H
