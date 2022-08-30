#ifndef POOL_H
#define POOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef void *(*worker_func)(void *);

struct pool_task_t {
    worker_func func;
    void* arg;
    int done;
};

struct pool_t {
    int workers;

    struct pool_task_t* tasks;
    struct pool_task_t* task_next;
    size_t pool_size;

    int* condition;
};

struct pool_t* pool_init();
void pool_free(struct pool_t*);

void pool_submit(struct pool_t*, worker_func, void*);
void pool_complete(struct pool_t*);

#ifdef __cplusplus
}
#endif

#endif