/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 * by Brad nichols, Dick Buttlar, Jackie Farrell
 * O'Reilly & Associates, Inc.
 *
 ********************************************************
 * rdwr.c --
 *
 * Library of functions implementing reader/writer locks
 */

#include <pthread.h>
#include <stdio.h>
#include "rdwr.h"

c_pthread_rdwr::c_pthread_rdwr()
{
    readers_reading = 0;
    writer_writing = 0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&lock_free, NULL);
}

int c_pthread_rdwr::read_lock()
{
    pthread_mutex_lock(&mutex);
    while(writer_writing) {
        pthread_cond_wait(&lock_free, &mutex);
    }
    readers_reading++;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int c_pthread_rdwr::read_unlock()
{
    pthread_mutex_lock(&mutex);
    if(readers_reading == 0) {
        pthread_mutex_unlock(&mutex);
        return -1;
    } else {
        readers_reading--;
        if(readers_reading == 0) {
            pthread_cond_signal(&lock_free);
        }
        pthread_mutex_unlock(&mutex);
        return 0;
    }
}

int c_pthread_rdwr::write_lock()
{
    pthread_mutex_lock(&mutex);
    while(writer_writing || readers_reading) {
        pthread_cond_wait(&lock_free, &mutex);
    }
    writer_writing++;
    pthread_mutex_unlock(&mutex);
    return 0;
}

int c_pthread_rdwr::check()
{
    pthread_mutex_lock(&mutex);
    printf("reader[%u], writer[%u]\n", readers_reading, writer_writing);
    pthread_mutex_unlock(&mutex);
    return 0;
}

int c_pthread_rdwr::write_unlock()
{
    pthread_mutex_lock(&mutex);
    if(writer_writing == 0) {
        pthread_mutex_unlock(&mutex);
        return -1;
    } else {
        writer_writing = 0;
        pthread_cond_broadcast(&lock_free);
        pthread_mutex_unlock(&mutex);
        return 0;
    }
}
