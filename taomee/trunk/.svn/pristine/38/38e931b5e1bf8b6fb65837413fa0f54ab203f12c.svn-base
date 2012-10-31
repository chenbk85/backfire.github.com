/**
 * =====================================================================================
 *       @file  stack.h
 *      @brief
 *
 *     Created  2011-11-19 11:46:08
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef STACK_H_2011_11_19___
#define STACK_H_2011_11_19___

#include <stdint.h>
#include <string.h>
#include "log.h"

template <class T>
class c_stack {
    private:
        T * stack;
        uint32_t index;
        uint32_t length;

    public:
        c_stack();
        c_stack(uint32_t l);
        ~c_stack();

        bool empty();
        bool full();
        int  push(T const & v);
        T *  pop();
        void clean();
};

template <class T>
c_stack<T>::c_stack()
{
    stack = new T[10];
    //ERROR_LOG("stack = new T[10]; %p", stack);
    index = 0;
    length = 10;
}

template <class T>
c_stack<T>::c_stack(uint32_t l)
{
    stack = new T[l];
    //ERROR_LOG("stack = new T[l]; %p", stack);
    index = 0;
    length = l;
}

template <class T>
c_stack<T>::~c_stack()
{
    //ERROR_LOG("delete[] stack; %p", stack);
    delete[] stack;
}

template <class T>
bool c_stack<T>::empty()
{
    return index == 0;
}

template <class T>
bool c_stack<T>::full()
{
    return index == length;
}

template <class T>
int c_stack<T>::push(T const & v)
{
    if(index == length) {
        T * new_s = new T[length<<1];
        //ERROR_LOG("T * new_s = new T[length<<1]; %p", new_s);
        if(new_s == 0) {
            ERROR_LOG("new falied");
            return -1;
        } else {
            memcpy(new_s, stack, length*sizeof(T));
            //ERROR_LOG("delete[] stack; %p", stack);
            delete[] stack;
            stack = new_s;
            length = length<<1;
        }
    }
    memcpy(&stack[index], &v, sizeof(T));
    index++;
    return 0;
}

template <class T>
T * c_stack<T>::pop()
{
    if(index == 0) {
        return 0;
    }
    index--;
    return &stack[index];
}

template <class T>
void c_stack<T>::clean()
{
    index = 0;
}

#endif //STACK_H_2011_11_19___
