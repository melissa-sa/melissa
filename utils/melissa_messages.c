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
 * @file melissa_messages.c
 * @brief pre-defined messages for launcher-server communication.
 * @author Terraz Théophile
 * @date 2019-16-07
 *
 **/

#include "melissa_messages.h"
#include "melissa_utils.h"
#include "zmq.h"

enum message_type get_message_type(char* buff)
{
    enum message_type* buff_ptr = buff;
    return *buff_ptr;
}

void send_message_hello (void *socket,
                           int   flags)
{
    zmq_msg_t             msg;
    int* buff_ptr = NULL;

    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = hello;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_job (char* buff,
                         int   simu_id,
                         char* job_id,
                         int   nb_param,
                         int*  param_set)
{

}

void send_message_drop (char* buff,
                          int   simu_id,
                          char* job_id)
{

}

void send_message_stop (char* buff)
{

}

void send_message_timeout (char* buff,
                             int   simu_id)
{

}

void send_message_simu_status (int   simu_id,
                               int   status,
                               void* socket,
                               int   flags)
{
    zmq_msg_t             msg;
    int* buff_ptr = NULL;

    zmq_msg_init_size (&msg, 3*sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = simu_status;
    buff_ptr ++;
    *buff_ptr = simu_id;
    buff_ptr ++;
    *buff_ptr = status;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_server (char* buff,
                            char* node_name)
{

}

