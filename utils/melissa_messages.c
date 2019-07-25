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

#include <string.h>
#include "melissa_messages.h"
#include "melissa_utils.h"
#include "zmq.h"

int get_message_type(char* buff)
{
    int* buff_ptr = buff;
    return *buff_ptr;
}

void send_message_hello (void* socket,
                         int   flags)
{
    zmq_msg_t       msg;
    int* buff_ptr = NULL;

    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = HELLO;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_alive (void *socket,
                         int   flags)
{
    zmq_msg_t       msg;
    int* buff_ptr = NULL;

    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = ALIVE;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_job (int    simu_id,
                       char*  job_id,
                       int    nb_param,
                       double *param_set,
                       void*  socket,
                       int    flags)
{
    zmq_msg_t        msg;
    char* buff_ptr = NULL;

    zmq_msg_init_size (&msg, 2 * sizeof(int) + nb_param * sizeof(double) + strlen(job_id)+1);
    buff_ptr = (char*)zmq_msg_data (&msg);
    *((int*)buff_ptr) = JOB;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, job_id);
    buff_ptr += strlen(job_id) +1;
    memcpy (buff_ptr, param_set, nb_param * sizeof(double));
    zmq_msg_send (&msg, socket, flags);
}

void send_message_drop (int   simu_id,
                        char* job_id,
                        void* socket,
                        int   flags)
{
    zmq_msg_t        msg;
    char* buff_ptr = NULL;

    fprintf(stdout, "send message with tag %d\n", DROP);

    zmq_msg_init_size (&msg, 2 * sizeof(int) + strlen(job_id) +1);
    buff_ptr = (char*)zmq_msg_data (&msg);
    *((int*)buff_ptr) = DROP;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, job_id);
    zmq_msg_send (&msg, socket, flags);
}

void send_message_stop (void* socket,
                        int   flags)
{
    zmq_msg_t msg;
    int*      buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = STOP;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_timeout (int   simu_id,
                           void* socket,
                           int   flags)
{
    zmq_msg_t msg;
    int*      buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = TIMEOUT;
    buff_ptr += sizeof(int);
    *buff_ptr = simu_id;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_simu_status (int   simu_id,
                               int   status,
                               void* socket,
                               int   flags)
{
    zmq_msg_t msg;
    int*      buff_ptr = NULL;

    zmq_msg_init_size (&msg, 3 * sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = SIMU_STATUS;
    buff_ptr ++;
    *buff_ptr = simu_id;
    buff_ptr ++;
    *buff_ptr = status;
    zmq_msg_send (&msg, socket, flags);
}

void send_message_server_name (char* node_name,
                               int   rank,
                               void* socket,
                               int   flags)
{
    zmq_msg_t msg;
    char*     buff_ptr = NULL;

    zmq_msg_init_size (&msg, 2 * sizeof(int) + strlen(node_name) +1);
    buff_ptr = zmq_msg_data (&msg);
    *((int*)buff_ptr) = SERVER;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = rank;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, node_name);
    zmq_msg_send (&msg, socket, flags);
}

void send_message_confidence_interval (char*  stat_name,
                                       char*  field_name,
                                       double value,
                                       void*  socket,
                                       int    flags)
{
    zmq_msg_t msg;
    char*     buff_ptr = NULL;

    zmq_msg_init_size (&msg, sizeof(int) + sizeof(double) + strlen(stat_name) + strlen(field_name) +2);
    buff_ptr = zmq_msg_data (&msg);
    *((int*)buff_ptr) = CONFIDENCE_INTERVAL;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, stat_name);
    buff_ptr += strlen(stat_name) +1;
    strcpy (buff_ptr, field_name);
    buff_ptr += strlen(field_name) +1;
    *((double*)buff_ptr) = value;
    zmq_msg_send (&msg, socket, flags);
}

