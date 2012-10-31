/**
 * =====================================================================================
 *       @file  collect_process.h
 *      @brief
 *
 *     Created  2011-11-24 14:40:17
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  COLLECT_PROCESS_H
#define  COLLECT_PROCESS_H

#include "../monitor-process/monitor_process.h"

typedef struct {
    i_config ** config;
    uint32_t user;
} collect_arg_t;

int collect_run(int argc, char **argv, void * p_arg);

void set_last_update(bool b_last_update);

#endif //COLLECT_PROCESS_H
