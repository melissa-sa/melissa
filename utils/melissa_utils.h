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
 * @file melissa_utils.h
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#ifndef MELISSA_UTILS_H
#define MELISSA_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#if BUILD_WITH_MPI == 0
#undef BUILD_WITH_MPI
#endif // BUILD_WITH_MPI

#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include <stdio.h>

#ifndef MPI_MAX_PROCESSOR_NAME
#define MPI_MAX_PROCESSOR_NAME 256 /**< Define the macro if mpi.h not present */
#endif

#ifndef MAX_FIELD_NAME
#define MAX_FIELD_NAME 128 /**< Define name size if not defined */
#endif

#define MELISSA_ERROR 0                           /**< display only errors  */
#define MELISSA_WARNING 1                         /**< display errors and warnings  */
#define MELISSA_INFO 2                            /**< display usefull messages */
#define MELISSA_DEBUG 3                           /**< display all messages */
#define VERBOSE_ERROR MELISSA_ERROR, __func__     /**< display only errors  */
#define VERBOSE_WARNING MELISSA_WARNING, __func__ /**< display errors and warnings  */
#define VERBOSE_INFO MELISSA_INFO, __func__       /**< display usefull messages */
#define VERBOSE_DEBUG MELISSA_DEBUG, __func__     /**< display all messages */

extern int MELISSA_MESSAGE_LEN;

int melissa_get_message_len();

void melissa_logo ();

void* melissa_malloc (size_t size);

void* melissa_calloc (size_t num,
                      size_t size);

void* melissa_realloc (void   *ptr,
                       size_t  size);

void melissa_free (void *ptr);

void melissa_bind (void       *socket,
                   const char *port_name);

void melissa_connect (void *socket,
                   char *port_name);

double melissa_get_time ();

void melissa_get_node_name (char *node_name);

void init_verbose_lvl(int verbose_level);

void melissa_print (int         msg_type,
                    const char* func_name,
                    const char* format, ...);

void set_bit (int32_t *vect, int pos);

void clear_bit (int32_t *vect, int pos);

int test_bit (int32_t *vect, int pos);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_UTILS_H
