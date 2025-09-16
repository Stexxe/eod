#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void *pool_base;
static void *pool_current;
static size_t pool_size;

int pool_init(size_t size) {
    pool_size = size;
    pool_base = malloc(size);

    if (pool_base == NULL) {
        return -1;
    }

    pool_current = pool_base;
    return 0;
}

void *pool_alloc_align(size_t size, size_t align) {
    uintptr_t current_address = (uintptr_t) pool_current;
    uintptr_t aligned_address = (current_address + align - 1) & ~(align - 1);

    assert(aligned_address + size <= (uintptr_t) pool_base + pool_size);

    memset((void *) aligned_address, 0, size);

    pool_current = (void *) (aligned_address + size);
    return (void *) aligned_address;
}