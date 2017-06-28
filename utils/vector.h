/**
 *
 * @file vector.h
 * @author Terraz Théophile
 * @date 2017-01-07
 *
 **/

#ifndef MELISSA_VECTOR_H
#define MELISSA_VECTOR_H

struct vector_s {
    void **items;
    int capacity;
    int item_size;
    int total;
};

typedef struct vector_s vector_t;

void alloc_vector(vector_t *v,
                  int capacity);

int vector_total(vector_t *v);

void vector_add(vector_t *v, void *item);

void vector_set(vector_t *v,
                int index,
                void *item);

void *vector_get(vector_t *v,
                 int index);

void vector_delete(vector_t *v,
                   int index);

void free_vector(vector_t *v);

void alloc_bits_array_vector(vector_t *v,
                             int capacity,
                             int item_size);

int bits_array_vector_total(vector_t *v);

void bits_array_vector_push_to(vector_t *v,
                               int   pos);

void *bits_array_vector_get(vector_t *v,
                            int index);

void free_bits_array_vector(vector_t *v);

#endif // MELISSA_VECTOR_H

