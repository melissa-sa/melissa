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

#define VERBOSE_ERROR 0   /**< display only errors  */
#define VERBOSE_WARNING 1 /**< display errors and warnings  */
#define VERBOSE_INFO 2    /**< display usefull messages */
#define VERBOSE_DEBUG 3   /**< display all messages */

void melissa_logo ();

void* melissa_malloc (size_t size);

void* melissa_calloc (size_t num,
                      size_t size);

void* melissa_realloc (void   *ptr,
                       size_t  size);

void melissa_free (void *ptr);

void melissa_bind (void *socket,
                   char *port_name);

void melissa_connect (void *socket,
                   char *port_name);

double melissa_get_time ();

void melissa_get_node_name (char *node_name);

void init_verbose_lvl(int verbose_level);

void melissa_print (int         msg_type,
                    const char* format, ...);

void set_bit (int32_t *vect, int pos);

void clear_bit (int32_t *vect, int pos);

int test_bit (int32_t *vect, int pos);

#endif // MELISSA_UTILS_H
