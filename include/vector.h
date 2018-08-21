/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#ifndef EASYVPN_PLUGIN_VECTOR_H_
#define EASYVPN_PLUGIN_VECTOR_H_

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct vector vector_t;

int vector_alloc(vector_t **, size_t);
void vector_free(vector_t *) /* TODO: free func param */;
size_t vector_size(vector_t *);
size_t vector_capacity(vector_t *);
bool vector_empty(vector_t *);
int vector_push_back(vector_t *, void *);
/* void vector_insert(vector_t *, size_t, void *); */
void * vector_at(vector_t *, size_t);
void * vector_begin(vector_t *);
void * vector_end(vector_t *);
void * vector_next(vector_t *, void *);

/* void vector_erase(vector_t *, size_t); */

#ifdef	__cplusplus
}
#endif

#endif  /* EASYVPN_PLUGIN_VECTOR_H_ */
