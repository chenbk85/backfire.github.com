/**
 * =====================================================================================
 *       @file  express_thread.cpp
 *      @brief  发送执行命令的结果给oa_head
 *
 *     Created  2012-01-11 16:34:03
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include "express_thread.h"
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include "../lib/log.h"

static bool m_inited;
static bool m_continue_working;
static pthread_t m_work_thread_id;

static void * work_thread_proc(void *p_data)
{
    if(p_data == NULL) {
        ERROR_LOG("p_data=%p", p_data);
        return (void*)-1;
    }
    c_queue<u_return_t>* p_return_q = (c_queue<u_return_t> *)p_data;
    u_return_t return_t;
    int r;
    uint8_t n;
    uint8_t max = 10;
    while(m_continue_working) {
        ERROR_LOG("test empty");
        if(!p_return_q->empty()) {
            r = p_return_q->pop(&return_t);
            if(r == 0) {
                n = 0;
                uint32_t len = *((uint32_t *)return_t.return_val);
                uint32_t send_len = 0;
                uint32_t left_len = len;
                DEBUG_LOG("ready to send %u bytes by %d", *((uint32_t *)return_t.return_val), return_t.send_fd);
                while(send_len < len) {
                    DEBUG_LOG("start to send %u bytes by %d", left_len, return_t.send_fd);
                    r = send(return_t.send_fd, return_t.return_val + send_len, left_len, 0);
                    if(r > 0) {
                        send_len += r;
                        left_len -= r;
                        DEBUG_LOG("send %u bytes %s by %d", r, strerror(errno), return_t.send_fd);
                    } else if(r == 0) {
                        ERROR_LOG("%d close by peer.", return_t.send_fd);
                    } else {
                        usleep(200000);
                        ERROR_LOG("%d %u %s", return_t.send_fd, errno, strerror(errno));
                        n++;
                        if(n > max) {
                            break;
                        }
                    }
                }
                DEBUG_LOG("send %u bytes by %d - %d %s", *((uint32_t *)return_t.return_val), return_t.send_fd, r, strerror(errno));
                //print_bytes((uint8_t *)return_t.return_val, *((uint32_t *)return_t.return_val));
            } else {
                ERROR_LOG("pop from queue error %d", r);
            }
        } else {
            ERROR_LOG("queue is empty begin sleep one second");
            sleep(1);
            ERROR_LOG("queue is empty end sleep one second");
        }
    }
    ERROR_LOG("leave while");

    while(!p_return_q->empty()) {
        r = p_return_q->pop(&return_t);
        if(r == 0) {
            n = 0;
            uint32_t len = *((uint32_t *)return_t.return_val);
            uint32_t send_len = 0;
            uint32_t left_len = len;
            DEBUG_LOG("ready to send %u bytes by %d", *((uint32_t *)return_t.return_val), return_t.send_fd);
            while(send_len < len) {
                DEBUG_LOG("start to send %u bytes by %d", left_len, return_t.send_fd);
                r = send(return_t.send_fd, return_t.return_val + send_len, left_len, 0);
                if(r > 0) {
                    send_len += r;
                    left_len -= r;
                    DEBUG_LOG("send %u bytes %s by %d", r, strerror(errno), return_t.send_fd);
                } else if(r == 0) {
                    ERROR_LOG("%d close by peer.", return_t.send_fd);
                } else {
                    usleep(200000);
                    ERROR_LOG("%u %s", errno, strerror(errno));
                    n++;
                    if(n > max) {
                        break;
                    }
                }
            }
            DEBUG_LOG("send %u bytes by %d - %d %s", *((uint32_t *)return_t.return_val), return_t.send_fd, r, strerror(errno));
            //print_bytes((uint8_t *)return_t.return_val, *((uint32_t *)return_t.return_val));
        } else {
            ERROR_LOG("pop from queue error %d", r);
        }
    }
    ERROR_LOG("leave work_proc");
    return 0;
}

int express_init(c_queue<u_return_t>* p_return_q)
{
    if(m_inited) {
        ERROR_LOG("ERROR: express_thread has been inited.");
        return -1;
    }
    if(p_return_q == NULL) {
        ERROR_LOG("p_return_q=%p", p_return_q);
        return -1;
    }
    do {
        m_inited = false;
        m_continue_working = true;
        int result = pthread_create(&m_work_thread_id, NULL, work_thread_proc, p_return_q);
        if (result != 0) {
            ERROR_LOG("ERROR: pthread_create() failed.");
            m_work_thread_id = 0;
            break;
        }
        m_inited = true;
    } while(false);

    if (m_inited) {
        DEBUG_LOG("export_thread init successfully.");
    }
    return 0;
}

int express_uninit()
{
    if (!m_inited) {
        return -1;
    }
    assert(m_work_thread_id != 0);
    m_continue_working = false;
    pthread_join(m_work_thread_id, NULL);
    m_work_thread_id = 0;

    DEBUG_LOG("export thread uninit successfully.");
    return 0;
}

pthread_t get_express_pthread_id()
{
    return m_work_thread_id;
}
