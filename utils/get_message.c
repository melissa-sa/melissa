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
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <zmq.h>
#include "melissa_utils.h"

struct cmessage_s
{
    char text[256];
    void *context;
    void *message_puller;
    zmq_msg_t msg;
};

typedef struct cmessage_s cmessage_t;

void get_node_name (char *node_name);
void create_message();
void init_message();
void wait_message(char* msg);
void init_message_snd();
void send_message();
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
        if (ifa->ifa_addr->sa_family==AF_INET)
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

void init_message()
{
    int rcv_timeout = 1000; // miliseconds
    message.context = zmq_ctx_new ();
    message.message_puller = zmq_socket (message.context, ZMQ_PULL);
    zmq_setsockopt (message.message_puller, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(int));
    melissa_bind (message.message_puller, "tcp://*:5555");
}

void init_message_snd()
{
    int snd_timeout = 1000; // miliseconds
    message.context = zmq_ctx_new ();
    message.message_puller = zmq_socket (message.context, ZMQ_PUSH);
    zmq_setsockopt (message.message_puller, ZMQ_RCVTIMEO, &snd_timeout, sizeof(int));
    melissa_connect (message.message_puller, "tcp://localhost:5555");
}

void wait_message(char* msg)
{
    int size = zmq_recv (message.message_puller, message.text, 255, 0);
    if (size < 1)
    {
        sprintf (msg, "%s", "nothing");
    }
    else
    {
        message.text[size] = 0;
        sprintf (msg, "%s", message.text);
    }
}

void send_message()
{
    char msg[255];
    sprintf (msg, "Hello world");
    zmq_send (message.message_puller, msg, 255, 0);
}

void close_message()
{
    zmq_close (message.message_puller);
    zmq_ctx_term (message.context);
}
