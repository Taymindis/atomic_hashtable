# atomic_hashtable
Fast movement for free and writeable, and read without locking while multithreading purposes

### How does your atomic_hashtable work?
- it is traveling around the buffer(s), each buffer has it own metadata for atomic counter of read and write permission without affecting other buffer's transaction. Meanwhile, all buffer are controlled by parent to own an accessing atomic counter in order to controlled the memory relocation only. 

### Target Project
- this lib mostly target for big data read and write at the same time with multiple transaction commit at the same time without locking.

### Requirement
- gcc/clang/llvm-gcc (with Atomic-Builtins)


### Sample of concurrent Testing
- goto root directory
- gcc -std=c11 test_main.c atomic_hashtable.c -pthread -otest
- ./test

### API document
###### __atomic_hash* __atomic_hash_init(size_t , atomic_hash_read_node_fn , atomic_hash_free_node_fn )
* arg0=The size of table
* arg1=the function of read node(only for __atomic_hash_read) could not be NULL unless * you are not using __atomic_hash_read)
* arg2=The function of free node(if NULL, it just free)
* return=return the atomic hash

###### int __atomic_hash_put(__atomic_hash *, HashKey , void *);
* arg0=The name hashtable
* arg1=the key in char string
* arg2= the allocated value 
* return 1 if true, 0 if false

###### void* __atomic_hash_replace(__atomic_hash *, HashKey , void *);
* arg0=The name hashtable
* arg1=the key in string
* arg2= the allocated value to replace
* return previous held value, return NULL if cannot find



###### void* __atomic_hash_pop(__atomic_hash *, HashKey );
* arg0=The name hashtable
* arg1=the key in string
* return val if found, else return NULL 

###### void* __atomic_hash_read(__atomic_hash *, HashKey );
* arg0=The name hashtable
* arg1=the key in string
* return duplicated val if found, else return NULL 
* need to have read function when initializing the atomic_hashtale

###### void __atomic_hash_destroy(__atomic_hash *);
* arg0=The name hashtable