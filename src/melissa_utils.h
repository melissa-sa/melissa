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


void* melissa_malloc (size_t size);

void* melissa_calloc (size_t num,
                      size_t size);

void melissa_free (void *ptr);

void melissa_bind (void *socket,
                   char *port_name);

void melissa_connect (void *socket,
                   char *port_name);

double melissa_get_time ();

void melissa_get_node_name (char *node_name);

#endif // MELISSA_UTILS_H
