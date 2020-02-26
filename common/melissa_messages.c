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

int get_message_type(char* buff)
{
    int* buff_ptr = buff;
    return *buff_ptr;
}

zmq_msg_t message_hello ()
{
    zmq_msg_t msg;
    char* buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = HELLO;
    return msg;
}

int send_message_hello (void* socket,
                        int   flags)
{
    zmq_msg_t msg;
    msg = message_hello ();
    return zmq_msg_send (&msg, socket, flags);
}

zmq_msg_t message_alive ()
{
    zmq_msg_t msg;
    char* buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = ALIVE;
    return msg;
}

int send_message_alive (void *socket,
                        int   flags)
{
    zmq_msg_t       msg;
    msg = message_alive ();
    return zmq_msg_send (&msg, socket, flags);
}

zmq_msg_t message_job (int    simu_id,
                       char*  job_id,
                       int    nb_param,
                       double *param_set)
{
    zmq_msg_t msg;
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
    return msg;
}

int send_message_job(int    simu_id,
                     char*  job_id,
                     int    nb_param,
                     double *param_set,
                     void*  socket,
                     int    flags)
{
    zmq_msg_t msg;
    msg = message_job (simu_id,
                       job_id,
                       nb_param,
                       param_set);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_job (char*    msg_buffer,
                       int*     simu_id,
                       char*    job_id[],
                       int      nb_param,
                       double*  param_set[])
{
    char* msg_ptr = msg_buffer;
    int i;
    memcpy (simu_id, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    strcpy (*job_id, msg_ptr);
    msg_ptr += strlen(msg_ptr)+1;
    if (*param_set == NULL)
    {
        *param_set = melissa_malloc (nb_param * sizeof(double));
    }
    for (i=0; i<nb_param; i++)
    {
        memcpy(&(*param_set)[i], msg_ptr, sizeof(double));
        msg_ptr += sizeof(double);
    }

}

zmq_msg_t message_drop (int   simu_id,
                        char* job_id)
{
    zmq_msg_t msg;
    char* buff_ptr = NULL;
    zmq_msg_init_size (&msg, 2 * sizeof(int) + strlen(job_id) +1);
    buff_ptr = (char*)zmq_msg_data (&msg);
    *((int*)buff_ptr) = DROP;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, job_id);
    return msg;
}

int send_message_drop (int   simu_id,
                       char* job_id,
                       void* socket,
                       int   flags)
{
    zmq_msg_t        msg;
    msg = message_drop (simu_id,
                        job_id);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_drop (char* msg_buffer,
                        int*  simu_id,
                        char* job_id[])
{
    char* msg_ptr = msg_buffer;
    memcpy (simu_id, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    strcpy (*job_id, msg_ptr);
}

zmq_msg_t message_stop ()
{
    zmq_msg_t msg;
    char* buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = STOP;
    return msg;
}

int send_message_stop(void* socket,
                      int   flags)
{
    zmq_msg_t msg;
    msg = message_stop ();
    return zmq_msg_send (&msg, socket, flags);
}

zmq_msg_t message_timeout (int simu_id)
{
    zmq_msg_t msg;
    int*      buff_ptr = NULL;
    zmq_msg_init_size (&msg, sizeof(int));
    buff_ptr = (int*)zmq_msg_data (&msg);
    *buff_ptr = TIMEOUT;
    buff_ptr += sizeof(int);
    *buff_ptr = simu_id;
    return msg;
}

int send_message_timeout (int   simu_id,
                          void* socket,
                          int   flags)
{
    zmq_msg_t msg;
    msg = message_timeout (simu_id);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_timeout (char* msg_buffer,
                           int*   simu_id)
{
    char* msg_ptr = msg_buffer;
    memcpy (simu_id, msg_ptr, sizeof(int));
}

zmq_msg_t message_simu_status (int simu_id,
                               int status)
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
    return msg;
}

int send_message_simu_status (int   simu_id,
                              int   status,
                              void* socket,
                              int   flags)
{
    zmq_msg_t msg;
    msg = message_simu_status (simu_id,
                               status);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_simu_status (char* msg_buffer,
                               int*  simu_id,
                               int*  status)
{
    char* msg_ptr = msg_buffer;
    memcpy (simu_id, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    strcpy (status, msg_ptr);
}

zmq_msg_t message_server_name (char* node_name,
                               int   rank)
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
    return msg;
}

int send_message_server_name (char* node_name,
                              int   rank,
                              void* socket,
                              int   flags)
{
    zmq_msg_t msg;
    msg = message_server_name (node_name,
                               rank);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_server_name (char* msg_buffer,
                               char* node_name,
                               int*  rank)
{
    char* msg_ptr = msg_buffer;
    memcpy (rank, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    strcpy (node_name, msg_ptr);
}

zmq_msg_t message_confidence_interval (char*  stat_name,
                                       char*  field_name,
                                       double value)
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
    return msg;
}

int send_message_confidence_interval (char*  stat_name,
                                      char*  field_name,
                                      double value,
                                      void*  socket,
                                      int    flags)
{
    zmq_msg_t msg;
    msg = message_confidence_interval (stat_name,
                                       field_name,
                                       value);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_confidence_interval (char*   msg_buffer,
                                       char*   stat_name,
                                       char*   field_name,
                                       double* value)
{
    char* msg_ptr = msg_buffer;
    strcpy (stat_name, msg_ptr);
    msg_ptr += strlen(stat_name);
    strcpy (field_name, msg_ptr);
    msg_ptr += strlen(field_name);
    memcpy (value, msg_ptr, sizeof(double));
}

zmq_msg_t message_options (char   buff[],
                           size_t buff_size)
{
    zmq_msg_t msg;
    char*     buff_ptr = NULL;
    zmq_msg_init_size (&msg, buff_size + sizeof(int));
    buff_ptr = zmq_msg_data(&msg);
    *((int*)buff_ptr) = OPTIONS;
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, buff, buff_size);
    return msg;
}

int send_message_options (char   buff[],
                          size_t buff_size,
                          void*  socket,
                          int    flags)
{
    zmq_msg_t msg;
    msg = message_options (buff,
                           buff_size);
    return zmq_msg_send (&msg, socket, flags);
}

zmq_msg_t message_simu_data (int      time_stamp,
                             int      simu_id,
                             int      client_rank,
                             int      vect_size,
                             int      nb_vect,
                             char*    field_name,
                             double** data_ptr)
{
    zmq_msg_t msg;
    int       i;
    char*     buff_ptr = NULL;
    zmq_msg_init_size (&msg, 4 * sizeof(int) + MAX_FIELD_NAME + nb_vect * vect_size * sizeof(double));
    buff_ptr = zmq_msg_data (&msg);
    memcpy (buff_ptr, &time_stamp, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &simu_id, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &client_rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &vect_size, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, field_name, MAX_FIELD_NAME);
    buff_ptr += MAX_FIELD_NAME;
    for (i=0; i<nb_vect; i++)
    {
        memcpy (buff_ptr, data_ptr[i], vect_size * sizeof(double));
        buff_ptr += vect_size * sizeof(double);
    }
    return msg;
}

int send_message_simu_data (int      time_stamp,
                            int      simu_id,
                            int      client_rank,
                            int      vect_size,
                            int      nb_vect,
                            char*    field_name,
                            double** data_ptr,
                            void*    socket,
                            int      flags)
{
    zmq_msg_t msg;
    msg = message_simu_data (time_stamp,
                             simu_id,
                             client_rank,
                             vect_size,
                             nb_vect,
                             field_name,
                             data_ptr);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_simu_data (char*    msg_buffer,
                             int*     time_stamp,
                             int*     simu_id,
                             int*     client_rank,
                             int*     vect_size,
                             char**   field_name_ptr,
                             double** data_ptr)
{
    char* msg_ptr = msg_buffer;
    memcpy(time_stamp, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    memcpy(simu_id, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    memcpy(client_rank, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    memcpy(vect_size, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    *field_name_ptr = msg_ptr;
    msg_ptr += MAX_FIELD_NAME * sizeof(char);
    *data_ptr = (double*)msg_ptr;
}
