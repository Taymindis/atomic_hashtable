#ifndef HEADER_ATOMICHASH_N
#define HEADER_ATOMICHASH_N

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef void* (*atomic_hash_n_malloc_fn)(size_t);
typedef void (*atomic_hash_n_free_fn)(void*);

typedef void *(*atomic_hash_n_read_node_fn)(void*);
typedef void (*atomic_hash_n_free_node_fn)(void*);

void init_hash_n_malloc_free_hooker(atomic_hash_n_malloc_fn malloc_fun, atomic_hash_n_free_fn free_fun);

typedef unsigned long atom_NumKey;

struct __atomic_node_n {
    void *val;
    atom_NumKey key;
    /*atomic__*/ int used;
    struct __atomic_node_n *next;

    /*atomic__ */int reading_counter;
};

typedef struct {
    struct __atomic_node_n *node_buf;
    size_t total_size;

    /*atomic__ */ size_t size;

    // Handling realloc and destroy counter
    /*atomic__*/ int accessing_counter;

    atomic_hash_n_read_node_fn read_node_fn;
    atomic_hash_n_free_node_fn free_node_fn;

} __atomic_hash_n;


__atomic_hash_n* __atomic_hash_n_init(size_t , atomic_hash_n_read_node_fn , atomic_hash_n_free_node_fn );
int __atomic_hash_n_put(__atomic_hash_n *, atom_NumKey , void *);
void* __atomic_hash_n_replace(__atomic_hash_n *, atom_NumKey , void *);
void* __atomic_hash_n_pop(__atomic_hash_n *, atom_NumKey );
void* __atomic_hash_n_get(__atomic_hash_n *, atom_NumKey);
void* __atomic_hash_n_read(__atomic_hash_n *, atom_NumKey);
void __atomic_hash_n_destroy(__atomic_hash_n *);

#ifdef __cplusplus
}
#endif

#endif
