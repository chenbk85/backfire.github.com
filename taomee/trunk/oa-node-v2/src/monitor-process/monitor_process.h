/**
 * =====================================================================================
 *       @file  monitor_process.h
 *      @brief
 *
 *     Created  2011-11-24 14:11:51
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  MONITOR_PROCESS_H
#define  MONITOR_PROCESS_H

#include <stdint.h>
#include "../defines.h"
int monitor_run(int argc, char **argv);

#define MAX_CMD_STR_LEN (512)

typedef struct {
    uint8_t  command_type;
    uint32_t command_id;
    int send_fd;
    char command_data[OA_MAX_BUF_LEN];
} u_command_t;

typedef struct {
    uint32_t command_id;
    int send_fd;
    char return_val[OA_MAX_BUF_LEN];
} u_return_t;

typedef struct {
    int command_t_used;     //network进程是否使用u_command_t共享内存的标志位
    int return_t_used;      //command进程是否使用u_return_t共享内存的标志位
} u_sh_mem_used_t;

typedef struct {
    int cmd_shmid;
    int ret_shmid;
    int used_shmid;
} u_shmid_t;

#endif // MONITOR_PROCESS_H
