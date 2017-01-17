/**
 *
 * @file melissa_utils.c
 * @brief Functions used in Melissa.
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 * @defgroup melissa_utils misc functions
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#ifdef BUILD_WITH_ZMQ
#include <zmq.h>
#endif // BUILD_WITH_ZMQ
#include "stats.h"

static void print_zmq_error(int         ret,
                            const char* port_name)
{
    fprintf(stderr,"ERROR on binding (%s)\n", port_name);
    if (ret == EINVAL)
    {
        fprintf(stderr, "The endpoint supplied is invalid.\n");
    }
    else if (ret == EPROTONOSUPPORT)
    {
        fprintf(stderr, "The requested transport protocol is not supported.\n");
    }
    else if (ret == ENOCOMPATPROTO)
    {
        fprintf(stderr, "The requested transport protocol is not compatible with the socket type.\n");
    }
    else if (ret == EADDRINUSE)
    {
        fprintf(stderr, "The requested address is already in use.\n");
    }
    else if (ret == EADDRNOTAVAIL)
    {
        fprintf(stderr, "The requested address was not local.\n");
    }
    else if (ret == ENODEV)
    {
        fprintf(stderr, "The requested address specifies a nonexistent interface.\n");
    }
    else if (ret == ETERM)
    {
        fprintf(stderr, "The ZeroMQ context associated with the specified socket was terminated.\n");
    }
    else if (ret == ENOTSOCK)
    {
        fprintf(stderr, "The provided socket was invalid.\n");
    }
    else if (ret == EMTHREAD)
    {
        fprintf(stderr, "No I/O thread is available to accomplish the task.\n");
    }
    else
    {
        fprintf(stderr, "Unknown error.\n");
    }
    exit(0);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around malloc
 *
 *******************************************************************************
 *
 * @param[in] size
 * Number of bytes to allocate
 *
 * @return The pointer to the allocated memory
 *
 *******************************************************************************/

void* melissa_malloc (size_t size)
{
    void* ptr = NULL;
    ptr = malloc (size);
    if (ptr == NULL)
    {
        fprintf (stderr, "ERROR malloc failed");
        exit(0);
    }
    else
    {
        return ptr;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around calloc
 *
 *******************************************************************************
 *
 * @param[in] num
 * Number of elements to allocate
 *
 * @param[in] size
 * Size of one element
 *
 * @return The pointer to the allocated memory
 *
 *******************************************************************************/

void* melissa_calloc (size_t num,
                      size_t size)
{
    void* ptr = NULL;
    ptr = calloc (num, size);
    if (ptr == NULL)
    {
        fprintf (stderr, "ERROR calloc failed");
        exit(0);
    }
    else
    {
        return ptr;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Free and nullify a pointer
 *
 *******************************************************************************
 *
 * @param[in] *ptr
 * The pointer to free and nullify
 *
 *******************************************************************************/

void melissa_free (void *ptr)
{
    if (ptr != NULL)
    {
        free (ptr);
        ptr = NULL;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around zmq_bind
 *
 *******************************************************************************
 *
 * @param[in] *socket
 * ZMQ socket handler
 *
 * @param[in] port_name
 * Port name
 *
 *******************************************************************************/

void melissa_bind (void *socket,
                   char *port_name)
{
    int ret;
    ret = zmq_bind (socket, port_name);
    if (ret != 0)
    {
        ret = errno;
        print_zmq_error(ret, port_name);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around zmq_connect
 *
 *******************************************************************************
 *
 * @param[in] *socket
 * ZMQ socket handler
 *
 * @param[in] port_name
 * Port name
 *
 *******************************************************************************/

void melissa_connect (void *socket,
                   char *port_name)
{
    int ret;
    ret = zmq_connect (socket, port_name);
    if (ret != 0)
    {
        ret = errno;
        print_zmq_error(ret, port_name);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Returns an elapsed time
 *
 *******************************************************************************
 *
 * @return elapsed time
 *
 *******************************************************************************/

double melissa_get_time ()
{
#ifdef BUILD_WITH_MPI
    return MPI_Wtime();
#else // BUILD_WITH_MPI
    return (double)time(NULL);
#endif // BUILD_WITH_MPI
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Gets the name of the processus node
 *
 *******************************************************************************
 *
 * @param[out] *node_name
 * The node name
 *
 *******************************************************************************/

void melissa_get_node_name (char *node_name)
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
#ifdef BUILD_WITH_MPI
        int resultlen;
        MPI_Get_processor_name(node_name, &resultlen);
#else
        gethostname(node_name, MPI_MAX_PROCESSOR_NAME);
#endif // BUILD_WITH_MPI
    }
}
