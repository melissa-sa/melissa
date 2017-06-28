 
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "melissa_utils.h"

void alloc_vector(vector_t *v, int capacity)
{
    v->capacity = capacity;
    v->total = capacity;
    v->items = melissa_calloc(v->capacity, sizeof(void *));
}

int vector_total(vector_t *v)
{
    return v->total;
}

static void resize_vector(vector_t *v, int capacity)
{
    void **items = realloc(v->items, sizeof(void *) * capacity);
    if (items)
    {
        v->items = items;
        v->capacity = capacity;
    }
}

void vector_add(vector_t *v, void *item)
{
    if (v->capacity == v->total)
    {
        resize_vector(v, v->capacity * 2);
    }
    v->items[v->total++] = item;
}

void vector_set(vector_t *v, int index, void *item)
{
    if (index >= 0 && index < v->total)
    {
        v->items[index] = item;
    }
}

void *vector_get(vector_t *v, int index)
{
    if (index >= 0 && index < v->total)
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
    int i;

    if (index < 0 || index >= v->total)
    {
        return;
    }

    v->items[index] = NULL;

    for (i = index; i < v->total - 1; i++)
    {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->total -= 1;

    if (v->total > 0 && v->total == v->capacity / 4)
    {
        resize_vector(v, v->capacity / 2);
    }
}

void free_vector(vector_t *v)
{
    melissa_free(v->items);
}

void alloc_bits_array_vector(vector_t *v, int capacity, int item_size)
{
    int i;

    v->item_size = item_size;
    alloc_vector(v, capacity);

    for (i=0; i<v->total; i++)
    {
        v->items[i] = melissa_calloc(sizeof(int32_t), item_size);
    }
}

int bits_array_vector_total(vector_t *v)
{
    return vector_total(v);
}

void bits_array_vector_push_to(vector_t *v,
                               int   pos)
{
    int i;

    if (pos >= v->total)
    {
        while (v->capacity <= pos)
        {
            resize_vector(v, v->capacity * 2);
        }

        for (i=v->total; i<=pos; i++)
        {
            v->items[i] = melissa_calloc(sizeof(int32_t), v->item_size);
        }
        v->total = pos + 1;
    }
}

void free_bits_array_vector(vector_t *v)
{
    int i;

    for (i=0; i<v->total; i++)
    {
        melissa_free (v->items[i]);
    }
    free_vector(v);
}
