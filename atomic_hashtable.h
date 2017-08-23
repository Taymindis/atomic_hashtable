#ifndef HEADER_ATOMICHASH
#define HEADER_ATOMICHASH

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef void* (*atomic_hash_malloc_fn)(size_t);
typedef void (*atomic_hash_free_fn)(void*);

typedef void *(*atomic_hash_read_node_fn)(void*);
typedef void (*atomic_hash_free_node_fn)(void*);

void init_malloc_free_hooker(atomic_hash_malloc_fn malloc_fun, atomic_hash_free_fn free_fun);

typedef char* HashKey;

struct __atomic_node_s {
    void *val;
    HashKey key;
    // size_t key_len;
    /*atomic__*/ int used;
    struct __atomic_node_s *next;

    /*atomic__ */int reading_counter;
};

typedef struct {
    struct __atomic_node_s *node_buf;
    size_t total_size;

    /*atomic__ */ size_t size;

    // Handling realloc and destroy counter
    /*atomic__*/ int accessing_counter;

    atomic_hash_read_node_fn read_node_fn;
    atomic_hash_free_node_fn free_node_fn;

} __atomic_hash;


__atomic_hash* __atomic_hash_init(size_t , atomic_hash_read_node_fn , atomic_hash_free_node_fn );
int __atomic_hash_put(__atomic_hash *, HashKey , void *);
void* __atomic_hash_replace(__atomic_hash *, HashKey , void *);
void* __atomic_hash_pop(__atomic_hash *, HashKey );
void* __atomic_hash_read(__atomic_hash *, HashKey );
void __atomic_hash_destroy(__atomic_hash *);

#ifdef __cplusplus
}
#endif

#endif
