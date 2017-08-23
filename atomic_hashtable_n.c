#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atomic_hashtable_n.h"

/***
* Author: taymindis@gmail.com - taymindis
* Atomic Hashtable for Integer, The faster free and writeable, travese and read without locking
* priority for multithreading purposes.
* Atomic Hashtable care your modify share memory when other people using.
* Version 0.0.1, 22-AUG-2017
****/
void* __atomic_hash_n_alloc_slot_(__atomic_hash_n *atom_hash, unsigned long hash_index);
int __atomic_hash_n_release_slot_(__atomic_hash_n *atom_hash, struct __atomic_node_n *n);

int __atomic_hash_n_realloc_buffer_(__atomic_hash_n *atom_hash);


#define get_hash_index(s) s%atom_hash->total_size


static atomic_hash_n_malloc_fn __atomic_hash_n_malloc_fn = malloc;
static atomic_hash_n_free_fn __atomic_hash_n_free_fn = free;

void add_one_(__atomic_hash_n *atom_hash);

void
init_malloc_free_hooker(atomic_hash_n_malloc_fn malloc_fun, atomic_hash_n_free_fn free_fun) {
    __atomic_hash_n_malloc_fn = malloc_fun;
    __atomic_hash_n_free_fn = free_fun;
}

void*
__atomic_hash_n_alloc_slot_(__atomic_hash_n *atom_hash, unsigned long hash_index) {
    // printf("hash index is %lu\n", hash_index);
    struct __atomic_node_n * return_ = atom_hash->node_buf + hash_index;
    size_t total_size = atom_hash->total_size, i;
RETRY:
    for (i = 0; i < total_size && __sync_lock_test_and_set(&return_->used, 1) ; i++) {
        // return_++;
        return_ = return_->next;
    }

    // To identify if i less than total size means slot found
    if (i < total_size) {
        return return_;
    }
    // if i over finding, but size still less than total size , retry to find a slot, it might happen when reach the last slot
    else if (__atomic_load_n(&atom_hash->size, __ATOMIC_SEQ_CST) < atom_hash->total_size) {
        return_ = atom_hash->node_buf + hash_index; // reset to starting position and retry
        goto RETRY;
    }
    // No more slot, it might not happen
    else {
        return NULL;
    }

}

int
__atomic_hash_n_release_slot_(__atomic_hash_n *atom_hash, struct __atomic_node_n *n) {

    n->key = 0;
    n->val = NULL;

    // To make it unused slot
    __sync_lock_release(&n->used);

    return 1;
}

/*API Endpoint*/
__atomic_hash_n*
__atomic_hash_n_init(size_t hash_size, atomic_hash_n_read_node_fn read_node_fn_, atomic_hash_n_free_node_fn free_node_fn_) {
    if (hash_size <= 1) {
        perror("ERROR:: Hash size must at least more than 1" );
        return NULL;
    }

    if (read_node_fn_ == NULL)
        printf("%s\n", "WARNING:: Must have read function provided to read the value, else do not use read function");

    if (free_node_fn_ == NULL)
        printf("%s\n", "WARNING:: Must have free node function provided to free the node, else it won't free the value when destroying the hashtable");


    __atomic_hash_n *atom_hash = __atomic_hash_n_malloc_fn(sizeof(__atomic_hash_n));

    if (atom_hash == NULL) return 0;

    atom_hash->total_size = hash_size;


    /** Pre-allocate all nodes **/
    atom_hash->node_buf = __atomic_hash_n_malloc_fn(hash_size * sizeof(struct __atomic_node_n));


    if (atom_hash->node_buf == NULL) {
        __atomic_hash_n_free_fn(atom_hash);
        return 0;
    }

    size_t i = 0;
    for (i = 0; i < hash_size; i++) {
        atom_hash->node_buf[i].key = 0;
        // atom_hash->node_buf[i].key_len = 0;
        atom_hash->node_buf[i].val = NULL;
        atom_hash->node_buf[i].next = atom_hash->node_buf + i + 1;
        atom_hash->node_buf[i].used = 0;
        // Atomic Init
        atom_hash->node_buf[i].reading_counter = 0;
        // atomic_flag_clear(&atom_hash->node_buf[i].used);

    }
    //For Last Node refer to first node
    atom_hash->node_buf[hash_size - 1].next = atom_hash->node_buf;

    // Atomic Init
    atom_hash->size = 0;

    // Atomic Init
    atom_hash->accessing_counter = 0;

    // atom_hash->is_maintaining = (atomic_flag)ATOMIC_FLAG_INIT;
    // atom_hash->resize = 1; // starting is 1

    // read function when get the obejct for secure copy
    atom_hash->read_node_fn = read_node_fn_;
    atom_hash->free_node_fn = free_node_fn_;

    return atom_hash;
}

int
__atomic_hash_n_realloc_buffer_(__atomic_hash_n *atom_hash) {

    struct __atomic_node_n *old_node_buf = atom_hash->node_buf;
    size_t total_size = atom_hash->total_size;
    size_t new_total_size = atom_hash->total_size * 2;
    
    /** Pre-allocate all nodes **/
    struct __atomic_node_n *new_node_buf = __atomic_hash_n_malloc_fn(new_total_size * sizeof(struct __atomic_node_n));
    // printf("Realloc Starting\n\n");

    if (new_node_buf == NULL) {
        return 0; // no more memory
    }

    for (size_t i = 0; i < new_total_size; i++) {
        new_node_buf[i].used = 0;
    }

    // Reform the hash code algorithm by starting to set total size at first
    atom_hash->total_size = new_total_size;


    for (size_t old_hash_index = 0; old_hash_index < total_size; old_hash_index++) {
        if (atom_hash->node_buf[old_hash_index].used == 1) {
            size_t new_hash_index = get_hash_index(atom_hash->node_buf[old_hash_index].key);
            // Linear Probing Logic
            while (new_node_buf[new_hash_index].used != 0) {
                new_hash_index++;
                // wrap around
                new_hash_index %= new_total_size;
            }
            memcpy(new_node_buf + new_hash_index, atom_hash->node_buf + old_hash_index, sizeof(struct __atomic_node_n));
        }

    }

    for (size_t i = 0; i < new_total_size; i++) {
        if (new_node_buf[i].used == 0) {
            new_node_buf[i].key = 0;
            // new_node_buf[i].key_len = 0;
            new_node_buf[i].val = NULL;
            new_node_buf[i].reading_counter = 0;
        }

        new_node_buf[i].next = new_node_buf + i + 1;

    }

    atom_hash->node_buf = new_node_buf;
    // atom_hash->resize = new_resize;

    //For Last Node refer to the first node
    atom_hash->node_buf[new_total_size - 1].next = atom_hash->node_buf;


    __atomic_hash_n_free_fn(old_node_buf);

    return 1;
}

void
add_one_(__atomic_hash_n *atom_hash) {
    if (__atomic_fetch_add(&atom_hash->size, 1, __ATOMIC_ACQUIRE) >= atom_hash->total_size) {
        int replace_val = -1,
            expected_val = 0;


        // means spinning lock for reading counter
        while (!__sync_bool_compare_and_swap(
                    &atom_hash->accessing_counter
                    , expected_val
                    , replace_val));

        if (__atomic_load_n(&atom_hash->size, __ATOMIC_SEQ_CST) > atom_hash->total_size) {
            __atomic_hash_n_realloc_buffer_(atom_hash);
        }

        // reset back the accessing counter, the reason accessing counter is global share, you have to use atomic exchange to change value
        // eventually reset accessing counter to 0 to let other thread accessing back
        if (atom_hash->accessing_counter % 2 != 0)
            __atomic_fetch_add(&atom_hash->accessing_counter, 1, __ATOMIC_RELAXED);

    }
}

int
__atomic_hash_n_put(__atomic_hash_n *atom_hash, atom_NumKey key_, void *value) {
    int success = 0;

    add_one_(atom_hash);

    while (__atomic_fetch_add(&atom_hash->accessing_counter, 2, __ATOMIC_ACQUIRE) % 2 != 0 ) {
        __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);
    }

    // it is spin lock allocation
    struct __atomic_node_n *node = __atomic_hash_n_alloc_slot_(atom_hash, get_hash_index(key_));

    if (node) {
        node->val = value;
        node->key = key_;
        success = 1;
    } else {
        __atomic_fetch_sub(&atom_hash->size, 1, __ATOMIC_RELAXED); //ROLLBACK If not allocable
    }

    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back
    return success;
}

void*
__atomic_hash_n_replace(__atomic_hash_n *atom_hash, atom_NumKey key_, void *value) {
    size_t total_size = atom_hash->total_size;
    void *found = NULL;

    add_one_(atom_hash);

    while (__atomic_fetch_add(&atom_hash->accessing_counter, 2, __ATOMIC_ACQUIRE) % 2 != 0 ) {
        __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);
    }

    struct __atomic_node_n *buffer = atom_hash->node_buf + (get_hash_index(key_));

    for (size_t i = 0; i < total_size ; i++) {
        // Use even number to prevent conflict issue with pop, even number means popable
        if (__atomic_fetch_add(&buffer->reading_counter, 2, __ATOMIC_ACQUIRE) % 2 == 0 &&
                buffer->used == 1 && key_ == buffer->key)
            goto SUCCESS;
        __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
        buffer = buffer->next;
    }
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);  // release access back
    return found;
SUCCESS:
    __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // still need to release read back

    // it is spin lock allocation
    struct __atomic_node_n *node = __atomic_hash_n_alloc_slot_(atom_hash, get_hash_index(key_));

    if (node) {

        int replace_val = -1;
        int expected_val = 0;

        while (!__sync_bool_compare_and_swap(
                    &buffer->reading_counter
                    , expected_val
                    , replace_val));

        found = buffer->val;

        if (buffer->used == 1 && key_ == buffer->key) {
            node->val = value;
            node->key = key_;
            // node->key_len = buffer->key_len;
            __atomic_hash_n_release_slot_(atom_hash, buffer); // release the old slot
        } else {
            // if someone has replace this slot, release back the
            __atomic_hash_n_release_slot_(atom_hash, node);
            found = NULL;
        }
        __atomic_add_fetch(&buffer->reading_counter, 1, __ATOMIC_RELEASE );
    }

    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back
    return found;
}

void*
__atomic_hash_n_pop(__atomic_hash_n *atom_hash, atom_NumKey key_) {
    void * return_ = NULL;
    size_t total_size = atom_hash->total_size;

    while (__atomic_fetch_add(&atom_hash->accessing_counter, 2, __ATOMIC_ACQUIRE) % 2 != 0 ) {
        __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);
    }

    struct __atomic_node_n *buffer = atom_hash->node_buf + (get_hash_index(key_));

    for (size_t i = 0; i < total_size ; i++) {
        if (__atomic_fetch_add(&buffer->reading_counter, 2, __ATOMIC_ACQUIRE) % 2 == 0 &&
                buffer->used == 1 && key_ == buffer->key)
            goto SUCCESS;
        __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
        buffer = buffer->next;
    }
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back
    return return_;
SUCCESS:
    __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back

    int replace_val = -1;
    int expected_val = 0;

    // means spinning lock for reading counter
    while (!__sync_bool_compare_and_swap(&buffer->reading_counter
                                         , expected_val
                                         , replace_val));

    return_ = buffer->val;

    if (buffer->used == 1 && key_ == buffer->key &&
            __atomic_hash_n_release_slot_(atom_hash, buffer)) {
        __atomic_fetch_sub(&atom_hash->size, 1, __ATOMIC_RELAXED);
    } else {
        return_ = NULL;
    }
    __atomic_add_fetch(&buffer->reading_counter, 1, __ATOMIC_RELEASE );

    // release access back
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);

    return return_;
}

/** get is getting the reference pointer from the hashtable, it is not good for multi concurrent write 
while other thread is free the field value, should use read **/
void*
__atomic_hash_n_get(__atomic_hash_n *atom_hash, atom_NumKey key_) {
    void * return_ = NULL;
    size_t total_size = atom_hash->total_size;

    while (__atomic_fetch_add(&atom_hash->accessing_counter, 2, __ATOMIC_ACQUIRE) % 2 != 0 ) {
        __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);
    }

    struct __atomic_node_n *buffer = atom_hash->node_buf + (get_hash_index(key_));

    for (size_t i = 0; i < total_size ; i++) {
        // Use even number to prevent conflict issue with pop, even number means popable
        if (__atomic_fetch_add(&buffer->reading_counter, 2, __ATOMIC_ACQUIRE) % 2 == 0 &&
                buffer->used == 1 && key_ == buffer->key) {
            // if (i > 2)printf("loop index %zu\n", i );
            goto SUCCESS;
        }
        __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
        buffer = buffer->next;
    }
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back
    return return_;
SUCCESS:
    return_ = buffer->val;
    __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back

    return return_;
}

void*
__atomic_hash_n_read(__atomic_hash_n *atom_hash, atom_NumKey key_) {
    void * return_ = NULL;
    size_t total_size = atom_hash->total_size;

    while (__atomic_fetch_add(&atom_hash->accessing_counter, 2, __ATOMIC_ACQUIRE) % 2 != 0 ) {
        __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE);
    }

    struct __atomic_node_n *buffer = atom_hash->node_buf + (get_hash_index(key_));

    for (size_t i = 0; i < total_size ; i++) {
        // Use even number to prevent conflict issue with pop, even number means popable
        if (__atomic_fetch_add(&buffer->reading_counter, 2, __ATOMIC_ACQUIRE) % 2 == 0 &&
                buffer->used == 1 && key_ == buffer->key) {
            // if (i > 2)printf("loop index %zu\n", i );
            goto SUCCESS;
        }
        __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
        buffer = buffer->next;
    }
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back
    return return_;
SUCCESS:
    return_ = atom_hash->read_node_fn(buffer->val);
    __atomic_fetch_sub(&buffer->reading_counter, 2,  __ATOMIC_RELEASE); // release read back
    __atomic_fetch_sub(&atom_hash->accessing_counter, 2,  __ATOMIC_RELEASE); // release access back

    return return_;
}

void
__atomic_hash_n_destroy(__atomic_hash_n *atom_hash) {
    int replace_val = -1;
    int expected_val = 0;
    size_t total_size = atom_hash->total_size;


    // means spinning lock for reading counter
    while (!__sync_bool_compare_and_swap(&atom_hash->accessing_counter
                                         , expected_val
                                         , replace_val));

    // this is loop from start
    struct __atomic_node_n *buffer = atom_hash->node_buf;


    for (size_t i = 0; i < total_size ; i++) {
        if (buffer->used) {
            buffer->key = 0;
            if (atom_hash->free_node_fn)
                atom_hash->free_node_fn(buffer->val);
            // else __atomic_hash_n_free_fn(buffer->val);
        }
        buffer = buffer->next;
    }

    __atomic_hash_n_free_fn(atom_hash->node_buf);

    __atomic_hash_n_free_fn(atom_hash);

    atom_hash = 0;
}