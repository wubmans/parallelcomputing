#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

struct Pool;
typedef struct Pool Pool;

typedef void (*threadFunction)(void *arg);

Pool* createThreadPool(int numberOfThreads);
void addWork(Pool* pool, threadFunction func, void *arg);
// void tpool_destroy(tpool *tm);

// bool tpool_add_work(tpool *tm, threadFunction func, void *arg);
// void tpool_wait(tpool *tm);