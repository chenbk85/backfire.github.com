/**
 * =====================================================================================
 *       @file  network_process.h
 *      @brief
 *
 *     Created  2011-12-27 15:36:40
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  NETWORK_PROCESS_H
#define  NETWORK_PROCESS_H

#include "../i_config.h"
#include "../defines.h"
#include "../lib/queue.h"
#include "../monitor-process/monitor_process.h"

/**
 * @struct √¸¡Ó–≠“È∞¸
 */
typedef struct {
    uint32_t pkg_len;
    uint32_t version;
    uint32_t cmd_id;    /**<√¸¡ÓID*/
    char cmd[0];        /**<√¸¡Óƒ⁄»›*/
} __attribute__((__packed__)) oa_cmd_t;

typedef struct {
    int peer_socket;
    uint32_t date_length;
    char data[OA_MAX_BUF_LEN];
} recv_cmd_t;

extern c_queue<recv_cmd_t> m_export_queue;
//extern c_queue<recv_cmd_t> m_express_queue;

int network_run(int argc, char ** argv, void * p_arg);

#endif  /*NETWORK_PROCESS_H*/
