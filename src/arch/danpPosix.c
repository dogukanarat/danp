/* danpPosix.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include "danp/danp.h"

/* Imports */


/* Definitions */


/* Types */

typedef struct archMutex_s
{
    pthread_mutex_t mutex;
} archMutex_t;

typedef struct archSem_s
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signaled;
} archSem_t;

typedef struct archQueue_s
 {
    uint8_t* buffer;
    int head;
    int tail;
    int count;
    int depth;
    int itemSize;
    pthread_mutex_t mutex;
    pthread_cond_t notEmpty;
} archQueue_t;

/* Forward Declarations */


/* Variables */


/* Functions */

void danpOsDelayMs(int ms) 
{
    usleep(ms * 1000);
}

uint32_t danpOsGetTickMs(void) 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

danpOsMutexHandle_t danpOsMutexCreate(void) 
{
    archMutex_t* m = malloc(sizeof(archMutex_t));
    pthread_mutex_init(&m->mutex, NULL);
    return m;
}

bool danpOsMutexLock(danpOsMutexHandle_t mutex, int timeoutMs) 
{
    archMutex_t* m = (archMutex_t*)mutex;
    // Note: pthread_mutex_lock does not support timeout directly.
    // For simplicity, we will ignore timeout here.
    pthread_mutex_lock(&m->mutex);
    return true;
}

void danpOsMutexUnlock(danpOsMutexHandle_t mutex) 
{
    archMutex_t* m = (archMutex_t*)mutex;
    pthread_mutex_unlock(&m->mutex);
}

danpOsSemaphoreHandle_t danpOsSemCreate(void) 
{
    archSem_t* s = malloc(sizeof(archSem_t));
    pthread_mutex_init(&s->mutex, NULL);
    pthread_cond_init(&s->cond, NULL);
    s->signaled = false;
    return s;
}

bool danpOsSemTake(danpOsSemaphoreHandle_t sem, int timeoutMs) 
{
    archSem_t* s = (archSem_t*)sem;
    pthread_mutex_lock(&s->mutex);
    
    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Add timeout to current time
    uint64_t nsec = tv.tv_usec * 1000 + (timeoutMs * 1000000ULL);
    ts.tv_sec = tv.tv_sec + nsec / 1000000000ULL;
    ts.tv_nsec = nsec % 1000000000ULL;

    while (!s->signaled) 
    {
        int rc = pthread_cond_timedwait(&s->cond, &s->mutex, &ts);
        if (rc == ETIMEDOUT) 
        {
            pthread_mutex_unlock(&s->mutex);
            return false;
        }
    }
    
    s->signaled = false; 
    pthread_mutex_unlock(&s->mutex);
    return true;
}

void danpOsSemGive(danpOsSemaphoreHandle_t sem) 
{
    archSem_t* s = (archSem_t*)sem;
    pthread_mutex_lock(&s->mutex);
    s->signaled = true;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
}

danpOsQueueHandle_t danpOsQueueCreate(int depth, int itemSize) 
{
    archQueue_t* q = malloc(sizeof(archQueue_t));
    q->buffer = malloc(depth * itemSize);
    q->depth = depth;
    q->itemSize = itemSize;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->notEmpty, NULL);
    return q;
}

bool danpOsQueueSend(danpOsQueueHandle_t qHandle, void* item, int timeoutMs) 
{
    archQueue_t* q = (archQueue_t*)qHandle;
    pthread_mutex_lock(&q->mutex);

    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nsec = tv.tv_usec * 1000 + (timeoutMs * 1000000ULL);
    ts.tv_sec = tv.tv_sec + nsec / 1000000000ULL;
    ts.tv_nsec = nsec % 1000000000ULL;

    while (q->count >= q->depth) 
    {
        int rc = pthread_cond_timedwait(&q->notEmpty, &q->mutex, &ts);
        if (rc == ETIMEDOUT) 
        {
            pthread_mutex_unlock(&q->mutex);
            return false; // Timeout, queue still full
        }
    }

    memcpy(q->buffer + (q->tail * q->itemSize), item, q->itemSize);
    q->tail = (q->tail + 1) % q->depth;
    q->count++;

    pthread_cond_signal(&q->notEmpty);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

bool danpOsQueueReceive(danpOsQueueHandle_t qHandle, void* item, int timeoutMs) 
{
    archQueue_t* q = (archQueue_t*)qHandle;
    pthread_mutex_lock(&q->mutex);

    struct timespec ts;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t nsec = tv.tv_usec * 1000 + (timeoutMs * 1000000ULL);
    ts.tv_sec = tv.tv_sec + nsec / 1000000000ULL;
    ts.tv_nsec = nsec % 1000000000ULL;

    while (q->count == 0) 
    {
        int rc = pthread_cond_timedwait(&q->notEmpty, &q->mutex, &ts);
        if (rc == ETIMEDOUT) 
        {
            pthread_mutex_unlock(&q->mutex);
            return false;
        }
    }

    memcpy(item, q->buffer + (q->head * q->itemSize), q->itemSize);
    q->head = (q->head + 1) % q->depth;
    q->count--;

    pthread_mutex_unlock(&q->mutex);
    return true;
}

void danpOsThreadCreate(DanpThreadFunc func, void* arg, const char* name) 
{
    pthread_t thread;
    pthread_create(&thread, NULL, (void* (*)(void*))func, arg);
}