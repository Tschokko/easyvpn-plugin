/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "vector.h"

#define VECTOR_INIT_CAPACITY 4

/* 
 * vector contains all data elements for the vector implementation and it's 
 * opaque to prevent unexpected behavior.
*/
struct vector {
    size_t vec_elem_size;
    void *vec_elems;
    size_t vec_capacity;
    size_t vec_size;
    /* TODO: deep copy func */
};

int
i_vec_resize(vector_t *vec, size_t capacity)
{
    void *elems = 0;
    int err = 0;

    assert(vec != NULL);

#ifdef DEBUG
    printf("i_vec_resize: %d to %d\n", vec->vec_capacity, capacity);
#endif

    /* Allocate new buffer for elems. */
    if ((elems = calloc(capacity, vec->vec_elem_size)) == NULL) {
        return (ENOMEM);
    }

    /* Copy existing elems into new buffer, if size is greater 0. */
    bcopy(vec->vec_elems, elems, vec->vec_size * vec->vec_elem_size);

    /* Free old buffer. */
    if (vec->vec_elems != NULL) {
        free(vec->vec_elems);
    }

    /* Point vector to new buffer and update capacity. */
    vec->vec_elems = elems;
    vec->vec_capacity = capacity;

    return (0);
}

int 
vector_alloc(vector_t **vecp, size_t elem_size)
{
    int err = 0;

    if (vecp == NULL && elem_size <= 0) {
        return (EINVAL);
    }

    if ((*vecp = calloc(1, sizeof(vector_t))) == NULL) {
        return (ENOMEM);
    }
    
    /* Init new vector */
    (*vecp)->vec_elem_size = elem_size;
    (*vecp)->vec_size = 0;
    (*vecp)->vec_capacity = VECTOR_INIT_CAPACITY;

    /* Create the initial buffer based on the capacity and return result. */
    return (i_vec_resize(*vecp, (*vecp)->vec_capacity));
}

void
vector_free(vector_t *vec)
{
    if (vec == NULL)
        return;

    if (vec->vec_elems != NULL)
        free(vec->vec_elems);

    free(vec);
}


size_t
vector_size(vector_t *vec)
{
    assert(vec != NULL);

    return vec->vec_size;
}

size_t
vector_capacity(vector_t *vec)
{
    assert(vec != NULL);

    return vec->vec_capacity;
}

bool
vector_empty(vector_t *vec)
{
    assert(vec != NULL);

    return (vec->vec_size == 0);
}

int
vector_push_back(vector_t *vec, void *elem)
{
    int err = 0;

    if (vec->vec_capacity == vec->vec_size) {
        if ((err = i_vec_resize(vec, vec->vec_capacity * 2)) != 0)
            return (err);
    }

    /* Copy elem into buffer */
    bcopy(elem, vec->vec_elems + (vec->vec_size * vec->vec_elem_size), 
        vec->vec_elem_size);
    
    /* Increment vector size */
    vec->vec_size++;

    return (0);
}

/* void
vector_insert(vector_t *vec, size_t index, void *item)
{
    if (index >= 0 && index < v->total)
        v->items[index] = item;
} */

void * 
vector_at(vector_t *vec, size_t index)
{
    if (index < vec->vec_size) {
        return vec->vec_elems + (index * vec->vec_elem_size);
    }

    return NULL;
}

void *
vector_begin(vector_t *vec) 
{
    assert(vec != NULL);

    return vec->vec_elems;
}

void * 
vector_end(vector_t *vec)
{
    assert(vec != NULL);

    return vec->vec_elems + ((vec->vec_size) * vec->vec_elem_size);
}

void *
vector_next(vector_t *vec, void *elem)
{
    uintptr_t distance = 0;
    uint64_t mod = 0;

    assert(vec != NULL);
    assert(elem != NULL);
    assert(elem >= vec->vec_elems);

    /* Check if elem is aligned with reserved memory. */
    distance = elem - vec->vec_elems;
    if (distance != 0) {
        mod = (distance / vec->vec_elem_size) % 2;
        /* assert(mod != 0); */
        /* if(((distance / vec->vec_elem_size) % 2) != 0) {
            return vector_end(vec);
        } */
    }

    if (elem != vector_end(vec)) {
        return elem += (vec->vec_elem_size);
    }

    return vector_end(vec);
}

/* void
vector_erase(vector_t *vec, size_t index)
{
    if (index >= vec->vec_size)
        return;

    vec->vec_items[index] = NULL;

    for (int i = index; i < vec->vec_size - 1; i++) {
        vec->vec_items[i] = vec->vec_items[i + 1];
        vec->vec_items[i + 1] = NULL;
    }

    vec->vec_size--;

    if (vec->vec_size > 0 && vec->vec_size == vec->vec_capacity / 4) {
        i_vector_resize(vec, vec->vec_capacity / 2);
    }
}
*/
