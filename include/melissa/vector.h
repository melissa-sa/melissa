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
 * @file melissa/vector.h
 * @author Terraz Théophile
 * @date 2017-01-07
 *
 **/

#ifndef MELISSA_VECTOR_H
#define MELISSA_VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

struct vector_s {
    void **items;
    int capacity;
    int size;
};

typedef struct vector_s vector_t;

void alloc_vector(vector_t *v,
                  int capacity);

int vector_size(vector_t *v);

void vector_add(vector_t *v,
                void     *item);

void vector_set(vector_t *v,
                int       index,
                void     *item);

void resize_vector(vector_t *v,
                   int capacity);

void *vector_get(vector_t *v,
                 int index);

void vector_delete(vector_t *v,
                   int index);

void free_vector(vector_t *v);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_VECTOR_H

