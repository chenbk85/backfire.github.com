/**
 * =====================================================================================
 *       @file  export_thread.h
 *      @brief  convert collected data into xml and export to oa_head
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  08/24/2010 08:34:12 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:38:42
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_EXPORT_THREAD_H_2010_08_24
#define H_EXPORT_THREAD_H_2010_08_24

#include <vector>
#include <map>
#include <pthread.h>

#include "../proto.h"
#include "../defines.h"
#include "../i_config.h"
#include "../lib/utils.h"
#include "../lib/hash.h"
#include "./listen_thread.h"

#define OA_MAX_IP_NUM 32

//和head里的send_timeout一致
#define OA_SOCKET_TIMEOUT 50

///OA错误编码
#define OA_METRIC_NORMAL 0
#define OA_METRIC_EXPIRE -1
#define OA_WRONG_PARAMETER -2

/**
 * @brief  初始化函数
 * @param  p_config: 配置对象指针,获取相关配置信息
 * @param  p_listen_ip: export线程监听的IP(发送TCP)
 * @param  p_listen_thread: listen线程,用于获取保存在内存中的metric信息
 * @return 0-success, -1-failed
 */
int export_init(i_config *p_config, const char *p_listen_ip, c_listen_thread *p_listen_thread,
        c_hash_table *p_hash_table);

/**
 * @brief  反初始化
 * @param  无
 * @return 0-success, -1-failed
 */
int export_uninit();

/**
 * @brief  设置oa_node信息
 * @param  p_config: 配置对象指针,获取相关配置信息
 * @return 0-success, -1-failed
 */
void set_node_info(i_config *p_config);

/**
 * @brief  获得本线程id
 * @param  无
 * @return 本线程id
 */
pthread_t get_export_pthread_id();

#endif //H_EXPORT_THREAD_H_2010_08_24
