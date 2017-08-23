# atomic_hashtable
Fast movement for free and writeable, and read without locking while multithreading purposes

## atomic_hashtable for string key
## atomic_hashtable_n for integer key

### How does atomic_hashtable work?
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
* arg2= the pre-allocated value 
* return 1 if true, 0 if false

###### void* __atomic_hash_replace(__atomic_hash *, HashKey , void *);
* arg0=The name hashtable
* arg1=the key in string
* arg2= the pre-allocated value to replace
* return previous held pre-allocated value, return NULL if cannot find

###### void* __atomic_hash_pop(__atomic_hash *, HashKey );
* arg0=The name hashtable
* arg1=the key in string
* return pop out pre-allocated value if found, else return NULL 

###### void* __atomic_hash_read(__atomic_hash *, HashKey );
* arg0=The name hashtable
* arg1=the key in string
* return duplicated value if found, else return NULL 
* need to have read function when initializing the atomic_hashtale

###### void __atomic_hash_destroy(__atomic_hash *);
* arg0=The name hashtable



### What is pre-allocated value in the readme
* it means the value must allocate memory before put into the hashtable. The buffer will keeping the same value until you pop out and free it


### what is atomic_hash_read_node_fn in the readme
* it mainly for __atomic_hash_read function, it will trigger the atomic_hash_read_node_fn to duplicate the new node, the current still remain on buffer. 
* see test_main.c for example.

### what is atomic_hash_free_node_fn in the readme
* it mainly for __atomic_hash_destroy function, it will iterate the buffer and trigger the atomic_hash_free_node_fn when destroying the node if any remaining memory inside.
* see test_main.c for example. 

### Simple Right?! Enjoy your lock free travelling!!

