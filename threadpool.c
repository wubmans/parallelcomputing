#include "threadpool.h"

int workCounter = 0;

typedef struct Work 
{
    threadFunction func;
    void* arg;
    struct Work* next;
    int i;

} Work;

typedef struct Pool
{
    struct Work* firstWork;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} Pool;


struct arguments
{
    Pool* pool;
    int thread;
};

// Work* createWork(threadFunction func, void* arg)

// }

Work* getWork(Pool* pool)
{
    Work* work;

    work = pool->firstWork;

    if (work == NULL)
    {
        return NULL;
    }

    pool->firstWork = work->next;
   
    return work;
}


void destroyWork(Work* work)
{
    if (work == NULL)
    {
        return;
    }

    //printf("\nfreeing: %i\n", work->i);
    free(work);

}

void* runWorker(void* args)
{
    struct arguments *wub = (struct arguments *) args;
    Pool* pool = wub->pool;
    int thread = wub->thread;

    Work* work;

    while(true)
    {
        // printf("running worker..\n");
        pthread_mutex_lock(&(pool->mutex));

        // while (pool->firstWork == NULL)
        // {
        //     pthread_cond_wait(&(pool->condition), &(pool->mutex));
        // }

        //printf("try to find work!");

        work = getWork(pool);
        pthread_mutex_unlock(&(pool->mutex));

        if (work != NULL)
        {
            // printf("thread %i running work %i\n", thread, work->i);
            work->func(work->arg);
            destroyWork(work);

        }

        //pthread_mutex_lock(&(Pool->mutex));
        //Pool->workers--;

    }
}

void addWork(Pool* pool, threadFunction func, void* arg)
{
    Work* work = malloc(sizeof(*work));
    work->func = func;
    work->arg = arg;
    work->next = NULL; 
    work->i = workCounter++;
    
    //printf("adding work");

    pthread_mutex_lock(&(pool->mutex));

    if (pool->firstWork == NULL)
    {
        pool->firstWork = work;
    } 
    else
    {
        Work* previous = pool->firstWork;

        while(previous != NULL && previous->next != NULL)
        {
            previous = previous->next;
        }

        previous->next = work; 
    }

    pthread_cond_broadcast(&(pool->condition));
    pthread_mutex_unlock(&(pool->mutex));

}


Pool* createThreadPool(int numberOfThreads)
{
    pthread_t thread;

    Pool* pool = calloc(1, sizeof(*pool));
  
    pool->firstWork = NULL;
    pthread_mutex_init(&(pool->mutex), NULL);
    pthread_cond_init(&(pool->condition), NULL);

    for (int i = 0; i < numberOfThreads; i++) {

        struct arguments *wub = malloc(sizeof(struct arguments));
        wub->pool = pool;
        wub->thread = i;
        //printf("creating thread %d\n", i);
        pthread_create(&thread, NULL, runWorker, (void *) wub);
        pthread_detach(thread);
    }

    return pool;
}