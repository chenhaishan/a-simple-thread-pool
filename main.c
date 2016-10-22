#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include "threadpool.h"



int main(int argc, char *argv[])
{
    thread_pool pool;
    thread_pool_init(&pool, 10);

    return 0;
}
