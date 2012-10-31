/**
 * =====================================================================================
 *       @file  queue.h
 *      @brief  加读写锁的模板队列类
 *
 *     Created  2011-11-23 15:50:35
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  QUEUE_H_2011_11_23___
#define  QUEUE_H_2011_11_23___

#ifndef  QUEUE_LENGTH
#define  QUEUE_LENGTH (10)
#endif

#include <stdint.h>
#include <string.h>

#include "rdwr.h"
#include "log.h"

//需要加锁

template <class T>
class c_queue
{
    private :
        T queue[QUEUE_LENGTH];
        uint32_t length;
        uint32_t tail;
        uint32_t head;
        c_pthread_rdwr lock;

    public :
        c_queue();
        ~c_queue();

        bool empty();
        bool full();
        int push(T const * p_v);
        int pop(T * p_v);
        void clean();
};

template <class T>
c_queue<T>::c_queue()
{
    //queue = new T[QUEUE_LENGTH];
    length = QUEUE_LENGTH;
    tail = 0;
    head = 0;
}

template <class T>
c_queue<T>::~c_queue()
{
//    delete[] queue;
}

template <class T>
bool c_queue<T>::empty()
{
    ERROR_LOG("in empty read_lock");
    lock.read_lock();
    bool r = (head == tail);
    lock.read_unlock();
    ERROR_LOG("in empty read_unlock");
    ERROR_LOG("%sempty", r?"":"not ");
    return r;
}

template <class T>
bool c_queue<T>::full()
{
    ERROR_LOG("in full read_lock");
    lock.read_lock();
    bool r = (head == ((tail + 1) % length));
    lock.read_unlock();
    ERROR_LOG("in full read_unlock");
    return r;
}

template <class T>
void c_queue<T>::clean()
{
    ERROR_LOG("in clean write_lock");
    lock.write_lock();
    head = 0;
    tail = 0;
    lock.write_unlock();
    ERROR_LOG("in clean write_unlock");
}

template <class T>
int c_queue<T>::push(T const * p_v)
{
    ERROR_LOG("in push write_lock");
    lock.write_lock();
    if(head == ((tail + 1) % length)) {
        lock.write_unlock();
        ERROR_LOG("in push write_unlock full!!!");
        return -1;
    } else {
        memcpy(&queue[tail], p_v, sizeof(T));
        tail++;
        tail %= length;
        lock.write_unlock();
        ERROR_LOG("in push write_unlock ok!!!");
        return 0;
    }
}

template <class T>
int c_queue<T>::pop(T * p_v)
{
    ERROR_LOG("in pop write_lock");
    lock.write_lock();
    if(head == tail) {
        lock.write_unlock();
        ERROR_LOG("in pop write_unlock empty!!!");
        return -1;
    } else {
        memcpy(p_v, &queue[(head++)%length], sizeof(T));
        head %= length;
        lock.write_unlock();
        ERROR_LOG("in pop write_unlock ok!!!");
        return 0;
    }
}

#endif  /*QUEUE_H_2011_11_23___*/
