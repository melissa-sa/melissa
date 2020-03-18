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
 * @file melissa_messages.h
 * @author Terraz Théophile
 * @date 2019-16-07
 *
 **/

#include "melissa_utils.h"
#include "zmq.h"

enum message_type {HELLO = 0,
                   JOB = 1,
                   DROP = 2,
                   STOP = 3,
                   TIMEOUT = 4,
                   SIMU_STATUS = 5,
                   SERVER = 6,
                   ALIVE = 7,
                   CONFIDENCE_INTERVAL = 8,
                   OPTIONS = 9
                  };

int get_message_type(char* buff);

zmq_msg_t message_hello ();

int send_message_hello (void* socket,
                        int   flags);

zmq_msg_t message_alive ();

int send_message_alive(void* socket,
                       int   flags);

zmq_msg_t message_job (int    simu_id,
                       char*  job_id,
                       int    nb_param,
                       double *param_set);

int send_message_job (int     simu_id,
                      char*   job_id,
                      int     nb_param,
                      double* param_set,
                      void*   socket,
                      int     flags);

void read_message_job (char*    msg_buffer,
                       int*     simu_id,
                       char    *job_id[],
                       int      nb_param,
                       double **param_set);

zmq_msg_t message_drop (int   simu_id,
                        char* job_id);

int send_message_drop(int   simu_id,
                      char* job_id,
                      void* socket,
                      int   flags);

void read_message_drop (char* msg_buffer,
                        int*  simu_id,
                        char* job_id[]);

zmq_msg_t message_stop ();

int send_message_stop (void* socket,
                       int flags);

zmq_msg_t message_timeout (int simu_id);

int send_message_timeout (int   simu_id,
                          void* socket,
                          int   flags);

void read_message_timeout (char* msg_buffer,
                           int*   simu_id);

zmq_msg_t message_simu_status (int simu_id,
                               int status);

int send_message_simu_status (int   simu_id,
                              int   status,
                              void* socket,
                              int   flags);

void read_message_simu_status (char* msg_buffer,
                               int*  simu_id,
                               int*  status);

zmq_msg_t message_server_name (char* node_name,
                               int   rank);

int send_message_server_name (char* node_name,
                              int   rank,
                              void* socket,
                              int   flags);

void read_message_server_name (char* msg_buffer,
                               char* node_name,
                               int*  rank);

zmq_msg_t message_confidence_interval (const char*  stat_name,
                                       const char*  field_name,
                                       const double value);

int send_message_confidence_interval (const char*  stat_name,
                                      const char*  field_name,
                                      const double value,
                                      void*  socket,
                                      int    flags);

void read_message_confidence_interval (char*   msg_buffer,
                                       char*   stat_name,
                                       char*   field_name,
                                       double* value);

zmq_msg_t message_options (char   buff[],
                           size_t buff_size);

int send_message_options(char   buff[],
                           size_t buff_size,
                           void*  socket,
                           int    flags);

zmq_msg_t message_simu_data (int      time_stamp,
                             int      simu_id,
                             int      client_rank,
                             int      vect_size,
                             int      nb_vect,
                             char*    field_name,
                             double** data_ptr);

int send_message_simu_data(int      time_stamp,
                             int      simu_id,
                             int      client_rank,
                             int      vect_size,
                             int      nb_vect,
                             char*    field_name,
                             double** data_ptr,
                             void*    socket,
                             int      flags);

void read_message_simu_data (char*    msg_buffer,
                             int*     time_stamp,
                             int*     simu_id,
                             int*     client_rank,
                             int*     recv_vect_size,
                             char**   field_name_ptr,
                             double** data_ptr);
