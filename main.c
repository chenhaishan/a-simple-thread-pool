#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <stdlib.h>
#include "threadpool.h"

#define NUMJOB 50
#define NUMTHREAD 12

struct{
    pthread_mutex_t m;
    unsigned int array[NUMJOB];
} job_area = {PTHREAD_MUTEX_INITIALIZER,{-1}};

unsigned int flag = 0;

void fill_thread_id(thread_job *job,unsigned int ttid){
    job_area.array[*((int *)(job->user_data))] = ttid;
    flag++;
    free(job->user_data);
    free(job);
    sleep(1);
}

int main()
{
    thread_pool pool;
    thread_pool_init(&pool, NUMTHREAD);

    int seq = 0;
    thread_job *job;
    while(seq != NUMJOB){
        job = malloc(sizeof(thread_job));
        job->job_function = fill_thread_id;
        job->user_data = malloc(sizeof(int));
        *((int*)(job->user_data)) = seq;
        job->prev = NULL;
        job->next = NULL;
        thread_pool_add_job(&pool, job);
        seq++;
    }

    while(flag != NUMJOB);
    for(seq = 0; seq != NUMJOB; ++seq){
        printf("sequence number of job is:%d, and handling thread id is:%d\n",
               seq, job_area.array[seq]);
    }


    thread_pool_shutdown(&pool);
    thread_pool_wait(&pool);


    return 0;
}
