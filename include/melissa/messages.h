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
 * @file melissa/messages.h
 * @author Terraz Théophile
 * @date 2019-16-07
 *
 **/

#ifndef MELISSA_MESSAGES_H_
#define MELISSA_MESSAGES_H_

#include <zmq.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HELLO 10
#define JOB 1
#define DROP 2
#define STOP 3
//#define TIMEOUT 4
#define SIMU_STATUS 5
#define SERVER 6
#define ALIVE 7
#define CONFIDENCE_INTERVAL 8
#define OPTIONS 9

// SimulationStatus as defined in the launcher:
enum SimulationStatus {
    NOT_SUBMITTED = -1,
    PENDING = 0,
    WAITING = 0,
    RUNNING = 1,
    FINISHED = 2,
    TIMEOUT = 4
};

int get_message_type(char* buff);

void message_hello(zmq_msg_t *msg);

int send_message_hello (void* socket,
                        int   flags);

void message_alive (zmq_msg_t *msg);

int send_message_alive(void* socket,
                       int   flags);

void message_job (zmq_msg_t *msg,
                  int    simu_id,
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

void message_drop(zmq_msg_t *msg,
                  int   simu_id,
                  char* job_id);

int send_message_drop(int   simu_id,
                      char* job_id,
                      void* socket,
                      int   flags);

void read_message_drop (char* msg_buffer,
                        int*  simu_id,
                        char* job_id);

void message_stop (zmq_msg_t *msg);

int send_message_stop (void* socket,
                       int flags);

void message_timeout (zmq_msg_t *msg,
                      int simu_id);

int send_message_timeout (int   simu_id,
                          void* socket,
                          int   flags);

void read_message_timeout (char* msg_buffer,
                           int*   simu_id);

void message_simu_status (zmq_msg_t *msg,
                          int simu_id,
                          int status);

int send_message_simu_status (int   simu_id,
                              int   status,
                              void* socket,
                              int   flags);

void read_message_simu_status (char* msg_buffer,
                               int*  simu_id,
                               int*  status);

void message_server_name(zmq_msg_t *msg,
                         char* node_name,
                         int   rank);

int send_message_server_name (char* node_name,
                              int   rank,
                              void* socket,
                              int   flags);

void read_message_server_name (char* msg_buffer,
                               char* node_name,
                               int*  rank);

void message_confidence_interval (zmq_msg_t *msg,
                                  const char*  stat_name,
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

void message_options (zmq_msg_t *msg,
                      char   buff[],
                      size_t buff_size);

int send_message_options(char   buff[],
                           size_t buff_size,
                           void*  socket,
                           int    flags);

void message_simu_data (zmq_msg_t *msg,
                        int      time_stamp,
                        int      simu_id,
                        int      client_rank,
                        int      vect_size,
                        int      nb_vect,
                        const char* field_name,
                        double** data_ptr);

int send_message_simu_data(int      time_stamp,
                             int      simu_id,
                             int      client_rank,
                             int      vect_size,
                             int      nb_vect,
                             const char* field_name,
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

#ifdef __cplusplus
}
#endif

#endif /* MELISSA_MESSAGES_H_ */
