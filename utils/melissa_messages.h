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

void send_message_hello (void* socket,
                         int   flags);

void send_message_alive (void* socket,
                         int   flags);

void send_message_job (int     simu_id,
                       char*   job_id,
                       int     nb_param,
                       double* param_set,
                       void*   socket,
                       int     flags);

void send_message_drop (int   simu_id,
                        char* job_id,
                        void* socket,
                        int   flags);

void send_message_stop (void* socket,
                        int flags);

void send_message_timeout (int   simu_id,
                           void* socket,
                           int   flags);

void send_message_simu_status (int   simu_id,
                               int   status,
                               void* socket,
                               int   flags);

void send_message_server_name (char* node_name,
                               int   rank,
                               void* socket,
                               int   flags);

void send_message_confidence_interval (char*  stat_name,
                                       char*  field_name,
                                       double value,
                                       void*  socket,
                                       int    flags);

void send_message_options (char   buff[],
                           size_t buff_size,
                           void*  socket,
                           int    flags);
