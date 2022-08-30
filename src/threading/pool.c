#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#include "cpu.h"
#include "pool.h"

struct worker_t {
    struct pool_t* pool;
    int id;
};

struct pool_t* pool_init()
{
    struct pool_t* pool = malloc(sizeof(struct pool_t));

    pool->workers = get_core_count();
    pool->tasks = NULL;
    pool->task_next = NULL;
    pool->pool_size = 0;
    pool->condition = NULL;

    return pool;
}

void pool_free(struct pool_t* pool)
{
    if (pool->tasks)
        free(pool->tasks);
    free(pool);
}

void pool_submit(struct pool_t* pool, worker_func func, void* arg)
{
    pool->pool_size++;
    pool->tasks = realloc(pool->tasks, sizeof(struct pool_task_t) * (pool->pool_size));

    struct pool_task_t* task = &pool->tasks[pool->pool_size-1];
    task->func = func;
    task->arg = arg;
    task->done = 0;
}

static void* task_executor(void* pinfo)
{
    struct worker_t* worker = (struct worker_t*)pinfo;
    struct pool_t* pool = worker->pool;

    if (pool->task_next)
    {
        struct pool_task_t* pool_end = pool->tasks + pool->pool_size;
        struct pool_task_t* task = pool->task_next++;
        while (pool_end > task)
        {
            if (!task->done)
            {
                task->func(task->arg);
                task->done = 1;
            }

            task = pool->task_next++;
        }
    }

    return NULL;
}

void pool_complete(struct pool_t* pool)
{
    pthread_t* threads = malloc(sizeof(pthread_t) * pool->workers);
    struct worker_t* workers = malloc(sizeof(struct worker_t) * pool->workers);

    if (pool->pool_size)
        pool->task_next = &pool->tasks[0];

    for (int i = 0; i < pool->workers && (pool->condition == NULL || *pool->condition); ++i)
    {
        struct worker_t* worker = &workers[i];
        pthread_t* thread = &threads[i];

        worker->pool = pool;
        worker->id = i;

        pthread_create(thread, NULL, task_executor, worker);
    }

    for (int i = 0; i < pool->workers; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(workers);
}