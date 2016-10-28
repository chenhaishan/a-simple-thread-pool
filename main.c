#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include "threadpool.h"

pid_t gettid(void){
    return syscall(SYS_gettid);
}

struct{
    pthread_mutex_t m;
    int array[100];
} job_area = {PTHREAD_MUTEX_INITIALIZER};

void fill_thread_id(thread_job *job){
    //we need not make job to be mutex,because we've delete
    //it from the jobs chain!!
    /*array[sequence of job] = thread id*/
    job_area.array[*((int*)(job->user_data))] = gettid();
    printf("sequence number of job is:%d, and handling thread id is:%d\n",
           *((int*)(job->user_data)), gettid());
    free(job->user_data);
    free(job);
}

int main()
{
    thread_pool pool;
    thread_pool_init(&pool, 10);

    int seq = 0;
    thread_job *job;
    while(seq != 500){
        job = malloc(sizeof(thread_job));
        job->job_function = fill_thread_id;
        job->user_data = malloc(sizeof(int));
        *((int*)(job->user_data)) = seq;
        job->prev = NULL;
        job->next = NULL;
        seq++;
        thread_pool_add_job(&pool, job);
    }
/*
    for(seq = 0; seq != 100; ++seq){
        printf("sequence number of job is:%d, and handling thread id is:%d\n",
               seq, job_area.array[seq]);
    }
*/
    thread_pool_shutdown(&pool);
    thread_pool_wait(&pool);


    return 0;
}
