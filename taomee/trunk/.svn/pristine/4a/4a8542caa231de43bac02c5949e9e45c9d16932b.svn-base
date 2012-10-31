/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 * rdwr.h --
 *
 * Modified by Matt Massie April 2001
 *
 * Include file for reader/writer locks
 *
 */
#ifndef __RDWR_H
#define __RDWR_H

#include <pthread.h>

class c_pthread_rdwr
{
    private :
        int readers_reading;
        int writer_writing;
        pthread_mutex_t mutex;
        pthread_cond_t lock_free;

    public :
        c_pthread_rdwr();
        int read_lock();
        int read_unlock();
        int write_lock();
        int write_unlock();
        int check();
};

#endif /* __RDWR_H */
