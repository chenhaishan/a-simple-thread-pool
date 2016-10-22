#include "threadpool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//in this design, jobs' priority is last is highest!!
//new jobs become the head of job list
//and job_function handle the head of job list firstly.

//insert to head of list
#define ADD_THREAD(item, list) { \
    item->prev = NULL; \
    item->next = list; \
    list = item; \
}

//remove item from list
#define REMOVE_JOB(item, list) {\
    if(item->prev != NULL) item->prev->next = item->next; \
    if(item->next != NULL) item->next->prev = item->prev; \
    if(list == item) list = item->next; \
    item->prev = item->next = NULL; \
}

static void *thread_function(void *ptr){
    thread_state* threads = (thread_state*)ptr;
    thread_job *job;

    while(1){
        //lock num_job_mutex and jobs_mutex
        pthread_mutex_lock(&threads->pool->num_jobs_mutex);
        pthread_mutex_lock(&threads->pool->jobs_mutex);
        //if no jobs, wait for pthread_cond_signal.
        //all alive threads are looping here, await for a job!!!
        while(threads->pool->jobs == NULL){
            pthread_cond_wait(&threads->pool->jobs_not_empty_cond, &threads->pool->jobs_mutex);
        }

        job = threads->pool->jobs;
        //if have job, get a job and remove it from jobs list
        if(job != NULL){
            REMOVE_JOB(job, threads->pool->jobs);
            threads->pool->num_jobs--;
            //then signal one pthread09
            pthread_cond_signal(&threads->pool->jobs_not_full_cond);
        }

        pthread_mutex_unlock(&threads->pool->num_jobs_mutex);
        pthread_mutex_unlock(&threads->pool->num_jobs_mutex);

        if(threads->killed)break;
        if(job == NULL)continue;
        //if have job, handle it
        job->job_function(job);
    }

    free(threads);//before threads is freed, the job list's head has already been replaced with threads->next
    pthread_exit(NULL);
}

//why thread_pool_init doesn't init thread_job *jobs ?
//because jobs are managed by users through thread_pool_add_job & thread_pool_add_job_ex.
int thread_pool_init(thread_pool *pool, int num_threads){
    int i = 0;
    thread_state *threads;
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

    if(num_threads < 1)num_threads = 1;
    memset(pool, 0, sizeof(*pool));
    memcpy(&pool->jobs_mutex, &blank_mutex, sizeof(pool->jobs_mutex));
    memcpy(&pool->num_jobs_mutex, &blank_mutex, sizeof(pool->num_jobs_mutex));
    memcpy(&pool->jobs_not_empty_cond, &blank_cond, sizeof(pool->jobs_not_empty_cond));
    memcpy(&pool->jobs_not_full_cond, &blank_cond, sizeof(pool->jobs_not_full_cond));
    pool->num_threads = num_threads;
    pool->num_jobs = 0;

    for(i = 0; i < num_threads; ++i){
        if((threads = malloc(sizeof(thread_state))) == NULL){
            fprintf(stderr, "Failed to allocate threads");
            return -1;
        }
        memset(threads, 0, sizeof(thread_state));
        threads->pool = pool;
        if(pthread_create(&threads->thread_id, NULL, thread_function, (void *)threads)){
            fprintf(stderr, "Failed to start all threads");
            free(threads);
            return -1;
        }
        ADD_THREAD(threads, threads->pool->threadstate);
    }
    return 0;
}

//set threadstate->killed with true(in thread_function(), if killed is true,
//the specific node of threadstate will be freed.), and notisfy all threads
//that are looping for jobs_not_empty_cond.
void thread_pool_shutdown(thread_pool *pool){
    thread_state *threads = NULL;
    for(threads = pool->threadstate; threads != NULL; threads = threads->next){
        threads->killed = 1;
    }

    pthread_mutex_lock(&pool->jobs_mutex);
    pool->threadstate = NULL;
    pool->jobs = NULL;
    pthread_cond_broadcast(&pool->jobs_not_empty_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);
}

//unlimit limitation of number of "handing jobs".
//one job come in , if there is idle threads, the job can be "on handling"
void thread_pool_add_job(thread_pool *pool, thread_job *job){
    pthread_mutex_lock(&pool->jobs_mutex);
    ADD_THREAD(job, pool->jobs);
    pthread_cond_signal(&pool->jobs_not_empty_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);
}

//arrange "handling jobs" at most 2 times of threads. other jobs will wait for handling.
void thread_pool_add_job_ex(thread_pool *pool, thread_job *job){
    pthread_mutex_lock(&pool->jobs_mutex);
    //mantain the number of jobs is 2 times of threads, wait for jobs_not_full_cond
    //because jobs are too many to handle, must wait for thread_function() to take
    //from jobs list, who will pthread_cond_signal jobs_not_full_cond.
    while(pool->num_jobs == 2 * pool->num_threads){//any reasons?why?
        pthread_cond_wait(&pool->jobs_not_full_cond, &pool->jobs_mutex);
    }

    ADD_THREAD(job, pool->jobs);
    pool->num_jobs++;
    //now there is one job in jobs list at least.
    //so thread_function() will be notisfied with jobs_not_empty_cond
    pthread_cond_signal(&pool->jobs_not_empty_cond);
    pthread_mutex_unlock(&pool->jobs_mutex);
}

//exit all threads in thread_pool
void thread_pool_wait(thread_pool *pool){
    thread_state *threads = NULL;
    for(threads = pool->threadstate; threads != NULL; threads = threads->next){
        pthread_join(threads->thread_id, NULL);
    }
}
