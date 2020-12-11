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

#include <melissa/utils.h>
#include <melissa/vector.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void alloc_vector(vector_t *v, int capacity)
{
	assert(v);
    v->capacity = capacity;
    v->size = 0;
    v->items = melissa_calloc(v->capacity, sizeof(void *));
}

int vector_size(vector_t *v)
{
	assert(v);
    return v->size;
}

void resize_vector(vector_t *v, int capacity)
{
	assert(v);
    void **items = realloc(v->items, sizeof(void *) * capacity);
    if (items)
    {
        v->items = items;
        v->capacity = capacity;
    }
}

void vector_add(vector_t *v, void *item)
{
	assert(v);

    if (v->capacity == v->size)
    {
        resize_vector(v, v->capacity * 2);
    }
    v->items[v->size++] = item;
}

void *vector_get(vector_t *v, int index)
{
	assert(v);
	assert(index >= 0);
	assert(index < v->size);

    if (index >= 0 && index < v->size)
    {
        return v->items[index];
    }
    else
    {
        return NULL;
    }
}

void vector_delete(vector_t *v, int index)
{
	assert(v);
	assert(index >= 0); // the wonders of signed integers
	assert(index < v->size);

    if (index < 0 || index >= v->size)
    {
        return;
    }

    v->items[index] = NULL;

    for (int i = index; i < v->size - 1; i++)
    {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->size -= 1;

    if (v->size > 0 && v->size == v->capacity / 4)
    {
        resize_vector(v, v->capacity / 2);
    }
}

void free_vector(vector_t *v)
{
	assert(v);
    melissa_free(v->items);
}
