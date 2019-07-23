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


#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <zmq.h>
#include "melissa_utils.h"
#include "melissa_messages.h"

#define PORT_NUMBER 5555

struct cmessage_s
{
    void *context;
    void *message_puller;
    void *message_pusher;
    void *message_resp;
};

typedef struct cmessage_s cmessage_t;

void get_node_name (char *node_name);
void init_context();
void bind_message_rcv(char* port_number);
void bind_message_resp(char* port_number);
void bind_message_snd(char* port_number);
void connect_message_rcv(char* node_name,
                         char* port_number);
void connect_message_snd(char* node_name,
                         char* port_number);
void wait_message(char* buff);
void get_resp_message(char* msg);
void send_message(char* msg);
void send_resp_message(char* msg);
void send_hello();
void send_alive();
void send_job(int     simu_id,
              char*   job_id,
              int     nb_param,
              double *param_set);

void send_drop (int   simu_id,
                char* job_id);
void close_message();
cmessage_t message;


void get_node_name (char *node_name)
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char   *addr;
    char ok = 0;

    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET)
        {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            if (strcmp (ifa->ifa_name, "ib0") == 0)
            {
                sprintf(node_name, "%s", addr);
                ok = 1;
                break;
            }
        }
    }
    if (ok == 0)
    {
//#ifdef BUILD_WITH_MPI
//        int resultlen;
//        MPI_Get_processor_name(node_name, &resultlen);
//#else
        gethostname(node_name, MPI_MAX_PROCESSOR_NAME);
//#endif // BUILD_WITH_MPI
    }
}

void init_context ()
{
    message.context = zmq_ctx_new ();
}

void bind_message_rcv(char* port_number)
{
    int rcv_timeout = 1000; // miliseconds
    char name[255];
    message.message_puller = zmq_socket (message.context, ZMQ_PULL);
    zmq_setsockopt (message.message_puller, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(int));
    sprintf (name, "tcp://*:%s", port_number);
    melissa_bind (message.message_puller, name);
}

void connect_message_rcv(char* node_name, char* port_number)
{
    int rcv_timeout = 1000; // miliseconds
    char name[255];
    message.message_puller = zmq_socket (message.context, ZMQ_PULL);
    zmq_setsockopt (message.message_puller, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(int));
    sprintf (name, "tcp://%s:%s", node_name, port_number);
    melissa_connect (message.message_puller, name);
}

void bind_message_snd(char* port_number)
{
    int snd_timeout = 1000; // miliseconds
    char name[255];
    message.message_pusher = zmq_socket (message.context, ZMQ_PUB);
    zmq_setsockopt (message.message_pusher, ZMQ_SNDTIMEO, &snd_timeout, sizeof(int));
    zmq_setsockopt (message.message_pusher, ZMQ_LINGER, &snd_timeout, sizeof(int));
    sprintf (name, "tcp://*:%s", port_number);
    melissa_bind (message.message_pusher, name);
}

void connect_message_snd(char* node_name, char* port_number)
{
    int snd_timeout = 1000; // miliseconds
    char name[255];
    message.message_pusher = zmq_socket (message.context, ZMQ_PUSH);
    zmq_setsockopt (message.message_pusher, ZMQ_SNDTIMEO, &snd_timeout, sizeof(int));
    zmq_setsockopt (message.message_pusher, ZMQ_LINGER, &snd_timeout, sizeof(int));
    sprintf (name, "tcp://%s:%s", node_name, port_number);
    fprintf (stdout, "connecting launcher to %s\n", name);
    melissa_connect (message.message_pusher, name);
}

void bind_message_resp(char* port_number)
{
    int rcv_timeout = 1000; // miliseconds
    char name[255];
    message.message_resp = zmq_socket (message.context, ZMQ_REP);
    zmq_setsockopt (message.message_resp, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(int));
    zmq_setsockopt (message.message_resp, ZMQ_LINGER, &rcv_timeout, sizeof(int));
    sprintf (name, "tcp://*:%s", port_number);
    melissa_bind (message.message_resp, name);
}

//void wait_message(char* buff)
//{
//    zmq_msg_t msg;
//    fprintf(stdout, "wait message\n");
//    int size = zmq_msg_recv (&msg, message.message_puller, 0);
//    if (get_message_type(zmq_msg_get_data(&msg)) == STOP)
//    {
//        sprintf (buff, "stop");
//    }
//    else
//    {
//        memcpy (buff, zmq_msg_get_data(&msg), size);
//    }
//}

void wait_message(char* buff)
{
//    char text[MELISSA_MESSAGE_LEN];
    char* buff_ptr = NULL;
    zmq_msg_t msg;
    zmq_msg_init (&msg);
    int size = zmq_msg_recv (&msg, message.message_puller, 0);
    if (size < 1)
    {
        sprintf (buff, "%s ", "nothing");
    }
    else
    {
        buff_ptr = zmq_msg_data(&msg);
        switch (get_message_type(buff_ptr))
        {
        case STOP:
            sprintf (buff, "%s ", "stop");
            break;

        case SIMU_STATUS:
            sprintf (buff, "%s %d %d", "group_state", *((int*)buff_ptr + 1), *((int*)buff_ptr + 2));
            break;

        case SERVER:
            sprintf (buff, "%s %d %s", "server", *((int*)buff_ptr + 1), buff_ptr + 2 * sizeof(int));
            break;

        case TIMEOUT:
            sprintf (buff, "%s %d", "timeout", *((int*)buff_ptr + 1));
            break;

        default:
            buff_ptr[size] = 0;
            printf ("message : %s\n", buff_ptr);
            sprintf (buff, "%s", buff_ptr);
            break;
        }
    }
}

void get_resp_message(char* msg)
{
    char text[MELISSA_MESSAGE_LEN];
    int size = zmq_recv (message.message_resp, text, MELISSA_MESSAGE_LEN-1, 0);
    if (size < 1)
    {
        sprintf (msg, "%s", "nothing");
    }
    else
    {
        text[size] = 0;
        sprintf (msg, "%s", text);
    }
}

void send_message(char* msg)
{
    char text[MELISSA_MESSAGE_LEN];
    sprintf (text, "%s", msg);
    zmq_send (message.message_pusher, text, MELISSA_MESSAGE_LEN-1, 0);
}

void send_hello()
{
    send_message_hello(message.message_puller, 0);
}

void send_alive()
{
    send_message_alive(message.message_resp, 0);
}

void send_job(int     simu_id,
              char*   job_id,
              int     nb_param,
              double* param_set)
{
    send_message_job(simu_id,
                     job_id,
                     nb_param,
                     param_set,
                     message.message_puller,
                     0);
}

void send_drop(int   simu_id,
               char* job_id)
{
    send_message_drop(simu_id,
                     job_id,
                     message.message_puller,
                     0);
}

void send_resp_message(char* msg)
{
    char text[MELISSA_MESSAGE_LEN];
    sprintf (text, "%s", msg);
    zmq_send (message.message_resp, text, MELISSA_MESSAGE_LEN-1, 0);
}

void close_message()
{
    zmq_close (message.message_puller);
    zmq_close (message.message_pusher);
    zmq_close (message.message_resp);
    zmq_ctx_term (message.context);
}
