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
 * @file melissa_utils.c
 * @brief Functions used in Melissa.
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 * @defgroup melissa_utils misc functions
 *
 **/

#include "melissa_utils.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <mpi.h>
#include <zmq.h>

int MELISSA_MESSAGE_LEN = 1024;

static int verbose_lvl; /**< verbosity lvl requested by user */

// To get MELISSA_MESSAGE_LEN into python
int melissa_get_message_len()
{
  return MELISSA_MESSAGE_LEN;
}

static void die(int error, const char* port_name)
{
    fprintf(
		stderr, "ERROR on port '%s': %s\n", port_name, zmq_strerror(error)
	);
    exit(EXIT_FAILURE);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Prints Melissa logo
 *
 *******************************************************************************
 *
 *******************************************************************************/

void melissa_logo ()
{
    melissa_print (VERBOSE_INFO, "\n");
    melissa_print (VERBOSE_INFO, "  _    _   ____   _      _    ____   ____   ___    \n");
    melissa_print (VERBOSE_INFO, " | \\  / | |  __| | |    | |  / ___| / ___| |   \\   \n");
    melissa_print (VERBOSE_INFO, " |  \\/  | | |_   | |    | | ( (__  ( (__   | |\\ \\  \n");
    melissa_print (VERBOSE_INFO, " |      | |  _|  | |    | |  \\__ \\  \\__ \\  | |_\\ \\ \n");
    melissa_print (VERBOSE_INFO, " | |\\/| | | |__  | |__  | |  ___) ) ___) ) |  ___ \\\n");
    melissa_print (VERBOSE_INFO, " |_|  |_| |____| |____| |_| |____/ |____/  |_|   \\/\n");
    melissa_print (VERBOSE_INFO, "  \n");
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around melissa_malloc
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
        fprintf (stdout, "ERROR melissa_malloc failed\n");
        exit(0);
    }
    else
    {
//        fprintf(stdout, "Allocated size : %d\n",size);
        return ptr;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around melissa_calloc
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
        fprintf (stdout, "ERROR melissa_calloc failed\n");
        exit(0);
    }
    else
    {
//        fprintf(stdout, "Allocated size : %d\n",size * num);
        return ptr;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Wraper around melissa_realloc
 *
 *******************************************************************************
 *
 * @param[in] *ptr
 * The pointer to the previously allocated memory
 *
 * @param[in] size
 * Number of bytes to allocate
 *
 * @return The pointer to the new allocated memory
 *
 *******************************************************************************/

void* melissa_realloc (void   *ptr,
                       size_t  size)
{
    ptr = realloc (ptr, size);
    if (ptr == NULL)
    {
        fprintf (stdout, "ERROR melissa_realloc failed\n");
        exit(0);
    }
    else
    {
//        fprintf(stdout, "Allocated size : %d\n",size);
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

void melissa_bind (void       *socket,
                   const char *port_name)
{
    if (zmq_bind (socket, port_name) < 0)
    {
        die(errno, port_name);
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
    if (zmq_connect (socket, port_name) < 0)
    {
        die(errno, port_name);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Return the time passed since some point in the past. The point in time is
 * system-dependent (e.g., the uptime) and only the difference between two
 * values is meaningful.
 *
 *******************************************************************************
 *
 * @return Time passed since an arbitrary fixed points in the past in seconds
 *
 *******************************************************************************/

double melissa_get_time ()
{
	struct timespec tp;

	if(clock_gettime(CLOCK_MONOTONIC, &tp) < 0)
	{
		perror("clock_gettime(CLOCK_MONOTONIC)");
		exit(EXIT_FAILURE);
	}

    return tp.tv_sec + tp.tv_nsec / 1e9;
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

void melissa_get_node_name (char* node_name, size_t buf_len)
{
	assert(node_name);
	assert(buf_len > 0);

    struct ifaddrs *ifap;
    char ok = 0;

    if(getifaddrs (&ifap) < 0) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

    for (struct ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET)
        {
            const struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
            const char* addr = inet_ntoa(sa->sin_addr);
            if (strcmp (ifa->ifa_name, "ib0") == 0)
            {
                strncpy(node_name, addr, buf_len);
                ok = 1;
                break;
            }
        }
    }
	freeifaddrs(ifap);

    if (ok == 0)
    {
        gethostname(node_name, buf_len);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * init verbose lvl
 *
 *******************************************************************************
 *
 * @param[in] *verbose_level
 * The requested verbose level for melissa_print
 *
 *******************************************************************************/

void init_verbose_lvl (int verbose_level)
{
    verbose_lvl = verbose_level;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Print a message depending on the verbose level
 *
 *******************************************************************************
 *
 * @param[in] *msg_priority
 * The level of priority for this message
 *
 * @param[out] *format
 * The format of the message to output
 *
 *******************************************************************************/

void melissa_print (int msg_priority, const char* func_name, const char* format, ...)
{
    va_list args;
    char* msg;

    if (msg_priority <= verbose_lvl)
    {
        msg = melissa_malloc(strlen (format) + strlen (func_name) + 12);
        if (msg_priority == MELISSA_ERROR)
        {
            strcpy(msg, "ERROR:   ");
            msg = strcat(msg, func_name);
            msg = strcat(msg, ":");
        }
        else if (msg_priority == MELISSA_WARNING)
        {
            strcpy(msg, "WARNING: ");
            msg = strcat(msg, func_name);
            msg = strcat(msg, ":");
        }
        else if (msg_priority == MELISSA_INFO)
        {
            strcpy(msg, "");
            if (verbose_lvl > MELISSA_INFO)
            {
                strcpy(msg, "INFO:    ");
                msg = strcat(msg, func_name);
                msg = strcat(msg, ":");
            }
        }
        else if (msg_priority >= MELISSA_DEBUG)
        {
            strcpy(msg, "DEBUG:   ");
            msg = strcat(msg, func_name);
            msg = strcat(msg, ":");
        }
        msg = strcat(msg, " ");
        msg = strcat(msg, format);
        va_start (args, format);
        vprintf (msg, args);
        va_end(args);
        melissa_free(msg);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Sets a bit to 1 in an array of bits
 *
 *******************************************************************************
 *
 * @param[out] *vect
 * pointer to the array of bits
 *
 * @param[in] *pos
 * position in the array of bits to set to 1
 *
 *******************************************************************************/

void set_bit (uint32_t *vect, int pos)
{
    vect[pos/32] |= UINT32_C(1) << (pos%32);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Sets a bit to 0 in an array of bits
 *
 *******************************************************************************
 *
 * @param[out] *vect
 * pointer to the array of bits
 *
 * @param[in] *pos
 * position in the array of bits to set to 0
 *
 *******************************************************************************/

void clear_bit (uint32_t *vect, int pos)
{
    vect[pos/32] &= ~(UINT32_C(1) << (pos%32));
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_utils
 *
 * Tests a value in an array of bits
 *
 *******************************************************************************
 *
 * @param[out] *vect
 * pointer to the array of bits
 *
 * @param[in] *pos
 * position to test in the array of bits
 *
 *******************************************************************************/

int test_bit (uint32_t *vect, int pos)
{
    if ( (vect[pos/32] & (UINT32_C(1) << (pos%32) )) != 0 )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
