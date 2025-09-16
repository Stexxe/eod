#ifndef EOD_MEM_H
#define EOD_MEM_H

#include <stddef.h>

#define pool_alloc(size, type) ((type *) pool_alloc_align((size), __alignof(type)))
#define pool_alloc_struct(type) pool_alloc(sizeof(type), type)

int pool_init(size_t size);
void *pool_alloc_align(size_t size, size_t align);

#endif //EOD_MEM_H