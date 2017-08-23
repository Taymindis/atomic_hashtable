#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include "atomic_hashtable.h"

/** Test SCOPE **/
typedef struct {
    char* name;
} Object;


static void freeObject(void *node) {
    // printf("%s\n", "Free from POP");
    free(node);
}

static void* readObject(void *origin) {
    Object *a = malloc(sizeof(Object));

    memcpy(a, origin, sizeof(Object));

    size_t sz = (strlen(((Object*)origin)->name) + 1) * sizeof(char);

    a->name = malloc(sz);
    memcpy(a->name, ((Object*)origin)->name, sz);


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


__atomic_hash *my_hashtable;
int atomic_key_name;
int i ;

void *myThreadFun(void *vargp)
{
    // static int loop_index  = 0;

    // if(loop_index < 100) {
    //     usleep(1000*200);
    // }
    // atomic_fetch_add(&atomic_key_name, 1);

    // if(atomic_load(&atomic_key_name) >= 500) {
    //     atomic_fetch_sub_explicit(&atomic_key_name, 500, __ATOMIC_RELEASE);
    // }
    atomic_key_name = randr(0, 2000000);


    char str[10];
    sprintf(str, "%d", atomic_key_name);

    Object *o = (Object*)malloc(sizeof(Object));
    o->name = malloc(10*sizeof(char));
    memcpy(o->name, str, 10 * sizeof(char));
    // printf("hash is %lu, and resize is %zu \n", hash(str), my_hashtable->resize );
    __atomic_hash_put(my_hashtable, str, o);

    o = (Object*) __atomic_hash_read(my_hashtable, str);
    if (o) {
        printf("%s\n", o->name);
        free(o->name);
        free(o);
    } 

     o = (Object*) __atomic_hash_get(my_hashtable, str);
    if (!o) {
        // printf("%s\n", o->name);
        printf("%s\n", "not found");
    } else {
        // printf("%s\n", o->name);
    }

    // Object *s_st, *old_v ;
    // s_st = (Object*)malloc(sizeof(Object));
    // char *str2 = malloc(30 * sizeof(char));
    // memcpy(str2, "replacement", 12);
    // s_st->name = str;
    // old_v = __atomic_hash_replace(my_hashtable, str, s_st);
    // if (old_v) {
    //     free(old_v);
    // } else {
    //     free(s_st);
    // }

    // free(str2);

    // s_st = (Object*) __atomic_hash_pop(my_hashtable, str);
    // if (s_st) {

    //     // printf("%s\n", s_st->name);
    //     // printf("%.*s\n", 11, s_st->name);

    //     // free(s_st->name);
    //     free(s_st);
    // } else {
    //     printf("%s\n", "nothing");
    // }


    return NULL;
}


#define MAX_THREAD 5000

int main(void) {
    my_hashtable = __atomic_hash_init(4000, readObject, freeObject);
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

    __atomic_hash_destroy(my_hashtable);

    return 0;
}