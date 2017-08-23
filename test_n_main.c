#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "atomic_hashtable_n.h"

/** Test SCOPE **/
typedef struct {
    unsigned long name;
} Object;


static void freeObject(void *node) {
    // printf("%s\n", "Free from POP");
    free(node);
}

static void* readObject(void *origin) {
    Object *a = malloc(sizeof(Object));

    memcpy(a, origin, sizeof(Object));
    return a;
}


#define PRINT_INT(x) printf("DEBUG:: %d\n", x)
#define PRINT_SIZE(x) printf("DEBUG:: %zu\n", x)
#define PRINT_STR(x) printf("DEBUG:: %s\n", x)

unsigned int
randr(unsigned int min, unsigned int max)
{
    double scaled = (double)rand() / RAND_MAX;

    return (max - min + 1) * scaled + min;
}


__atomic_hash_n *my_hashtable;
int atomic_key_name;
int i ;

void *myThreadFun(void *vargp)
{

    atomic_key_name = randr(0, 2000000);


    atom_NumKey num_key = (atom_NumKey)atomic_key_name;

    Object *o = (Object*)malloc(sizeof(Object));
    o->name = num_key;
    // printf("hash is %lu, and resize is %zu \n", hash(num_key), my_hashtable->resize );
    __atomic_hash_n_put(my_hashtable, num_key, o);


    o = (Object*) __atomic_hash_n_read(my_hashtable, num_key);
    if (o) {
        printf("%lu\n", o->name);
        free(o);
    }



    o = (Object*) __atomic_hash_n_get(my_hashtable, num_key);
    if(!o) {
        printf("%s\n", "not found");
    }

    Object *s_st, *old_v ;
    s_st = (Object*)malloc(sizeof(Object));
    s_st->name = num_key;
    old_v = __atomic_hash_n_replace(my_hashtable, num_key, s_st);
    if (old_v) {
        free(old_v);
    } else {
        free(s_st);
    }

    /* Uncomment if you want to pop function */
    // s_st = (Object*) __atomic_hash_n_pop(my_hashtable, num_key);
    // if (s_st) {

    //     // printf("%s\n", s_st->name);
    //     // printf("%.*s\n", 11, s_st->name);

    //     free(s_st);
    // } else {
    //     printf("%s\n", "nothing");
    // }


    return NULL;
}


#define MAX_THREAD 5000

int main(void) {
    my_hashtable = __atomic_hash_n_init(2, readObject, freeObject);
    // atomic_key_name = ATOMIC_VAR_INIT(0);

    PRINT_SIZE(my_hashtable->total_size);

    struct timeval t0, t1;
    unsigned int i;

    pthread_t tid[MAX_THREAD];

    gettimeofday(&t0, NULL);
    for (i = 0; i < MAX_THREAD; i++) {
        pthread_create(&tid[i], NULL, myThreadFun, NULL);
//        pthread_join(tid[i], NULL);
    }


    for (i = 0; i < MAX_THREAD; i++)
        pthread_join(tid[i], NULL);

    gettimeofday(&t1, NULL);
    printf("Did %u calls in %.2g seconds\n", i, t1.tv_sec - t0.tv_sec + 1E-6 * (t1.tv_usec - t0.tv_usec));

    PRINT_INT(i);
    PRINT_STR("total size");
    PRINT_SIZE(my_hashtable->total_size);

    __atomic_hash_n_destroy(my_hashtable);

    return 0;
}