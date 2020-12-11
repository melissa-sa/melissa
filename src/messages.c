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
 * @file melissa_messages.c
 * @brief pre-defined messages for launcher-server communication.
 * @author Terraz Théophile
 * @date 2019-16-07
 *
 **/

#include <melissa/messages.h>
#include <melissa/utils.h>

#include <stdio.h>
#include <string.h>
#include <zmq.h>

#ifdef DEBUG_MELISSA_MESSAGES
int msend(zmq_msg_t *msg, void *b, void *c, int line, char* file, char* func) {
    printf("sending message from line %d, file %s, function %s\n", line, file, func);
    printf("message_id: %d\n", *((int*)zmq_msg_data(msg)));
    return zmq_msg_send(msg,b,c);
}
#define zmq_msg_send(a,b,c) msend(a,b,c, __LINE__, __FILE__, __func__)
#endif

int get_message_type(char* buff)
{
//    int* buff_ptr = buff;
    int msg_type = *(int*)buff;
//    return *buff_ptr;
    return msg_type;
}

void message_hello (zmq_msg_t *msg)
{
    char* buff_ptr = NULL;
    zmq_msg_init_size (msg, sizeof(int));
    buff_ptr = (char*)zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)HELLO;
}

int send_message_hello (void* socket,
                        int   flags)
{
    zmq_msg_t msg;
    message_hello (&msg);
    return zmq_msg_send (&msg, socket, flags);
}

void message_alive (zmq_msg_t *msg)
{
    int* buff_ptr = NULL;
    zmq_msg_init_size (msg, sizeof(int));
    buff_ptr = zmq_msg_data (msg);
    *buff_ptr = (int)ALIVE;
}

int send_message_alive (void *socket,
                        int   flags)
{
    zmq_msg_t       msg;
    message_alive (&msg);
    return zmq_msg_send (&msg, socket, flags);
}

void message_job (zmq_msg_t *msg,
                  int        simu_id,
                  char*      job_id,
                  int        nb_param,
                  double    *param_set)
{
    char* buff_ptr = NULL;
    zmq_msg_init_size (msg, 2 * sizeof(int) + nb_param * sizeof(double) + strlen(job_id)+1);
    buff_ptr = (char*)zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)JOB;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, job_id);
    buff_ptr += strlen(job_id) +1;
    memcpy (buff_ptr, param_set, nb_param * sizeof(double));
}

int send_message_job(int    simu_id,
                     char*  job_id,
                     int    nb_param,
                     double *param_set,
                     void*  socket,
                     int    flags)
{
    zmq_msg_t msg;
    message_job (&msg,
                 simu_id,
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

void message_drop (zmq_msg_t *msg,
                   int        simu_id,
                   char*      job_id)
{
    char* buff_ptr = NULL;
    zmq_msg_init_size (msg, 2 * sizeof(int) + strlen(job_id) +1);
    buff_ptr = (char*)zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)DROP;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, job_id);
}

int send_message_drop (int   simu_id,
                       char* job_id,
                       void* socket,
                       int   flags)
{
    zmq_msg_t        msg;
    message_drop (&msg,
                  simu_id,
                  job_id);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_drop (char* msg_buffer,
                        int*  simu_id,
                        char* job_id)
{
    char* msg_ptr = msg_buffer;
    memcpy (simu_id, msg_ptr, sizeof(int));
    msg_ptr += sizeof(int);
    memset(job_id, 0, 256);
    strncpy(job_id, msg_ptr, 255);
}

void message_stop (zmq_msg_t *msg)
{
    char* buff_ptr = NULL;
    zmq_msg_init_size (msg, sizeof(int));
    buff_ptr = zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)STOP;
}

int send_message_stop(void* socket,
                      int   flags)
{
    zmq_msg_t msg;
    message_stop (&msg);
    return zmq_msg_send (&msg, socket, flags);
}

void message_timeout (zmq_msg_t *msg,
                      int simu_id)
{
    char*      buff_ptr = NULL;
    zmq_msg_init_size (msg, sizeof(int));
    buff_ptr = zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)TIMEOUT;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = simu_id;
}

int send_message_timeout (int   simu_id,
                          void* socket,
                          int   flags)
{
    zmq_msg_t msg;
    message_timeout (&msg,
                     simu_id);
    return zmq_msg_send (&msg, socket, flags);
}

void read_message_timeout (char* msg_buffer,
                           int*   simu_id)
{
    char* msg_ptr = msg_buffer;
    memcpy (simu_id, msg_ptr, sizeof(int));
}

void message_simu_status (zmq_msg_t *msg,
                          int simu_id,
                          int status)
{
    int*      buff_ptr = NULL;
    zmq_msg_init_size (msg, 3 * sizeof(int));
    buff_ptr = (int*)zmq_msg_data (msg);
    *buff_ptr = (int)SIMU_STATUS;
    buff_ptr ++;
    *buff_ptr = simu_id;
    buff_ptr ++;
    *buff_ptr = status;
}

int send_message_simu_status (int   simu_id,
                              int   status,
                              void* socket,
                              int   flags)
{
    zmq_msg_t msg;
    message_simu_status (&msg,
                         simu_id,
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
    memcpy(status, msg_ptr, sizeof(int));
}

void message_server_name (zmq_msg_t *msg,
                          char* node_name,
                          int   rank)
{
    char*     buff_ptr = NULL;
    zmq_msg_init_size (msg, 2 * sizeof(int) + strlen(node_name) +1);
    buff_ptr = zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)SERVER;
    buff_ptr += sizeof(int);
    *((int*)buff_ptr) = rank;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, node_name);
}

int send_message_server_name (char* node_name,
                              int   rank,
                              void* socket,
                              int   flags)
{
    zmq_msg_t msg;
    message_server_name (&msg,
                         node_name,
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

void message_confidence_interval (zmq_msg_t *msg,
                                  const char*  stat_name,
                                  const char*  field_name,
                                  const double value)
{
    char*     buff_ptr = NULL;
    zmq_msg_init_size (msg, sizeof(int) + sizeof(double) + strlen(stat_name) + strlen(field_name) +2);
    buff_ptr = zmq_msg_data (msg);
    *((int*)buff_ptr) = (int)CONFIDENCE_INTERVAL;
    buff_ptr += sizeof(int);
    strcpy (buff_ptr, stat_name);
    buff_ptr += strlen(stat_name) +1;
    strcpy (buff_ptr, field_name);
    buff_ptr += strlen(field_name) +1;
    *((double*)buff_ptr) = value;
}

int send_message_confidence_interval (const char*  stat_name,
                                      const char*  field_name,
                                      const double value,
                                      void*  socket,
                                      int    flags)
{
    zmq_msg_t msg;
    message_confidence_interval (&msg,
                                 stat_name,
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
    strncpy(field_name, msg_ptr, MAX_FIELD_NAME_LEN);
    msg_ptr += strlen(field_name);
    memcpy (value, msg_ptr, sizeof(double));
}

void message_options (zmq_msg_t *msg,
                      char   buff[],
                      size_t buff_size)
{
    char*     buff_ptr = NULL;
    zmq_msg_init_size (msg, buff_size + sizeof(int));
    buff_ptr = zmq_msg_data(msg);
    *((int*)buff_ptr) = (int)OPTIONS;
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, buff, buff_size);
}

int send_message_options (char   buff[],
                          size_t buff_size,
                          void*  socket,
                          int    flags)
{
    zmq_msg_t msg;
    message_options (&msg,
                     buff,
                     buff_size);
    return zmq_msg_send (&msg, socket, flags);
}

void message_simu_data (zmq_msg_t *msg,
                        int      time_stamp,
                        int      simu_id,
                        int      client_rank,
                        int      vect_size,
                        int      nb_vect,
                        const char* field_name,
                        double** data_ptr)
{
    int       i;
    char*     buff_ptr = NULL;
    zmq_msg_init_size (msg, 4 * sizeof(int) + MAX_FIELD_NAME_LEN+1 + nb_vect * vect_size * sizeof(double));
    buff_ptr = zmq_msg_data (msg);
    memcpy (buff_ptr, &time_stamp, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &simu_id, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &client_rank, sizeof(int));
    buff_ptr += sizeof(int);
    memcpy (buff_ptr, &vect_size, sizeof(int));
    buff_ptr += sizeof(int);
    memset(buff_ptr, 0, MAX_FIELD_NAME_LEN+1);
    strncpy(buff_ptr, field_name, MAX_FIELD_NAME_LEN);
    buff_ptr += MAX_FIELD_NAME_LEN;
    for (i=0; i<nb_vect; i++)
    {
        memcpy (buff_ptr, data_ptr[i], vect_size * sizeof(double));
        buff_ptr += vect_size * sizeof(double);
    }
}

int send_message_simu_data (int      time_stamp,
                            int      simu_id,
                            int      client_rank,
                            int      vect_size,
                            int      nb_vect,
                            const char* field_name,
                            double** data_ptr,
                            void*    socket,
                            int      flags)
{
    zmq_msg_t msg;
    message_simu_data (&msg,
                       time_stamp,
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
    msg_ptr += MAX_FIELD_NAME_LEN * sizeof(char);
    *data_ptr = (double*)msg_ptr;
}
