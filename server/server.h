/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
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

#ifdef __cplusplus
extern "C" {
#endif

#include <zmq.h>
#include "melissa_fields.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#include "fault_tolerance.h"
#include <mpi.h>

struct melissa_server_s
{
    melissa_options_t     melissa_options;
    melissa_field_t      *fields;
    comm_data_t           comm_data;
    int                   port_no;
    char                 *port_names;
    int                   rinit_tab[2];
    char                  node_name[MPI_MAX_PROCESSOR_NAME];
    void                 *context;
    void                 *connexion_responder;
    void                 *deconnexion_responder;
    void                 *data_puller;
    void                 *text_puller;
    void                 *text_pusher;
    void                 *text_requester;
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
    double *parameters;
    int     nb_param;      /**< number of parameters */
    int     time_stamp;
    int     first_init;
    int     status;
    double *val;
    int     val_size;
    int     max_val_size;
    char*   nn_path_ptr;
};

typedef struct simulation_data_s simulation_data_t; /**< type corresponding to simulation_data_s */

void melissa_server_init (int argc, char **argv, void **melissa_server_ptr);

void melissa_server_run (void **melissa_server_ptr, simulation_data_t *simu_data);

void melissa_server_finalize (void **melissa_server_ptr, simulation_data_t *simu_data);

int create_port_number (comm_data_t *comm_data,
                        const char  *node_name,
                        const int    start_number,
                        const int    forbidden_port1,
                        const int    forbidden_port2,
                        const int    forbidden_port3,
                        const int    forbidden_port4,
                        const int    forbidden_port5);

int check_simu_state(melissa_field_t *field,
                     int              nb_fields,
                     int              group_id,
                     int              nb_time_steps,
                     comm_data_t     *comm_data);


void process_launcher_message (void*             msg_data,
                               melissa_server_t *server_ptr);

int check_last_timestep(melissa_field_t *fields,
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

#ifdef __cplusplus
}
#endif

#endif // SERVER_HELPER_H
