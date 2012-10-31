/**
 * =====================================================================================
 *       @file  command_process.h
 *      @brief
 *
 *     Created  2011-11-30 08:41:50
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  COMMAND_PROCESS_H
#define  COMMAND_PROCESS_H

#include "../monitor-process/monitor_process.h"

int command_run(int argc, char **argv, void * p_arg);

typedef struct {    //供以后优化用，减少load so的次数
    char md5[33];
    char so_path[MAX_CMD_STR_LEN];
    void * so_handler;
} command_so_t;

#endif  /*COMMAND_PROCESS_H*/
