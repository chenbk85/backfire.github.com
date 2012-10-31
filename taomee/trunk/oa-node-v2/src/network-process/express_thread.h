/**
 * =====================================================================================
 *       @file  express_thread.h
 *      @brief  发送执行命令的结果给oa_head
 *
 *     Created  2011-12-28 17:20:08
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  EXPRESS_THREAD_H
#define  EXPRESS_THREAD_H

#include "../monitor-process/monitor_process.h"
#include "../lib/queue.h"

int express_init(c_queue<u_return_t>* p_return_q);
int express_uninit();

pthread_t get_express_pthread_id();

void print_bytes(uint8_t * buf, uint32_t len);
#endif  /*EXPRESS_THREAD_H*/
