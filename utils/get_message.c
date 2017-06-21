
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void create_message();
void init_message();
void wait_message();
void close_message();
cmessage_t message;

void init_message()
{
    int rcv_timeout = 1000; // miliseconds
    message.context = zmq_ctx_new ();
    message.message_puller = zmq_socket (message.context, ZMQ_PULL);
    zmq_setsockopt (message.message_puller, ZMQ_RCVTIMEO, &rcv_timeout, sizeof(int));
    melissa_bind (message.message_puller, "tcp://*:5555");
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

void close_message()
{
    zmq_close (message.message_puller);
    zmq_ctx_term (message.context);
}
