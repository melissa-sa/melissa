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

enum message_type {hello = 0,
                   job = 1,
                   drop = 2,
                   stop = 3,
                   timeout = 4,
                   simu_status = 5,
                   server = 6
                  };

enum message_type get_message_type(char* buff);

void send_message_hello (void* socket,
                           int   flags);

void send_message_job (char* buff,
                         int   simu_id,
                         char*  job_id,
                         int   nb_param,
                         int*  param_set);

void send_message_drop (char* buff,
                          int   simu_id,
                          char* job_id);

void send_message_stop (char* buff);

void send_message_timeout (char* buff,
                             int   simu_id);

void send_message_simu_status (int   simu_id,
                               int   status,
                               void* socket,
                               int   flags);

void send_message_server (char* buff,
                            char* node_name);
