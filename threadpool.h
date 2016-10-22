#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

typedef struct _thread_pool thread_pool;
typedef struct _thread_state thread_state;
typedef struct _thread_job thread_job;


struct _thread_pool{
    thread_state *threadstate;//head of thread_state
    int num_threads;//threads number

    thread_job *jobs;//head of thread_job
    pthread_mutex_t jobs_mutex;//mutex for jobs
    pthread_cond_t jobs_not_empty_cond;//condition 1 for jobs_mutex
    pthread_cond_t jobs_not_full_cond;//condition 2 for jobs_mutex

    int num_jobs;//jobs number
    pthread_mutex_t num_jobs_mutex;//mutex for num_jobs
};

struct _thread_state{
    pthread_t thread_id;//thread id
    thread_pool *pool;//thread_pool
    thread_state *prev;
    thread_state *next;
    int killed;//if killed == 1, thread thread_id will pthread_exit()
};

struct _thread_job{
    void (*job_function)(thread_job *job);//job's function
    void *user_data;
    thread_job *prev;
    thread_job *next;
};

extern int
thread_pool_init(thread_pool *pool, int num_workers);

extern void
thread_pool_shutdown(thread_pool *pool);

extern void
thread_pool_add_job(thread_pool *pool, thread_job *job);

extern void
thread_pool_add_job_ex(thread_pool *pool, thread_job *job);

extern void
thread_pool_wait(thread_pool *pool);

#endif // THREADPOOL_H
