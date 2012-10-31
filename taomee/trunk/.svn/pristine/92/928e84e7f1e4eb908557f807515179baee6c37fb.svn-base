/**
 * =====================================================================================
 *       @file  listen_thread.h
 *      @brief  保存接受到的组播数据，并为获取、删除相应数据提供接口
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  18/10/2010 5:30:10 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:40:08
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_LISTEN_THREAD_H
#define H_LISTEN_THREAD_H
#include <map>
#include <string>
#include <time.h>
#include <pthread.h>

#include "../lib/hash.h"
#include "../i_config.h"
#include "../defines.h"

/**
 * @enum metric的存储类型
 */
typedef enum {
    e_invalid = 0,
    e_up_fail,
} metric_type_e;

typedef int metric_type_t;

/**
 * @struct 心跳存储在hash_t中的value值
 */
typedef struct {
    time_t rcv_time;
    u_int  host_ip;
    time_t heart_beat;
    bool   up_fail;
} heartbeat_val_t;

/**
 * @struct metric存储在hash_t中的value值
 */
typedef struct {
    time_t rcv_time;
    int collect_interval;
    int cleaned;
    char  data[0];
} __attribute__((packed)) metric_val_t;

/**
 * @class 监听线程，接受并保存采集的数据
 */
class c_listen_thread
{
public:
    c_listen_thread();
    ~c_listen_thread();

    /**
     * @brief  初始化
     * @param  p_config: config线程指针
     * @param  p_listen_ip: 内网IP(接受UDP包)
     * @param  p_hash_table: 存储数据的哈希表指针
     * @param  p_thread_started: listen线程启动标志位
     * @return 0-success, -1-failed
     */
    int init(i_config *p_config, const char *p_listen_ip, c_hash_table *p_hash_table, bool *p_thread_started);

    /**
     * @brief  重置机器新增标志位
     * @param  无
     * @return 前一次标志位值
     */
    bool reset_host_add();

    /**
     * @brief  反初始化
     * @param  无
     * @return 0-success, -1-failed
     */
    int uninit();

protected:
    /**
     * @brief  接受单播数据
     * @param  p_data_buf: 数据缓存区地址(空间由调用者分配)
     * @param  buf_len: 缓存区长度
     * @return 成功返回接受到数据包的长度，失败则返回-1
     */
    int recv_data(char *p_data_buf, const int buf_len);

    /**
     * @brief  保存接受到的xdr数据包
     * @param  p_data: 接受到的xdr数据包
     * @param  data_len: 数据包长度
     * @return 0-success, -1-failed
     * @note 当接收到的数据包不完整时就丢掉
     */
    int store_data(char *p_data, int data_len);

    /**
     * @brief  子线程的线程函数
     * @param  p_data: 指向当前对象的this指针
     * @return (void *)0:success, (void *)-1:failed
     */
    static void *work_thread_proc(void *p_data);

private:
    bool m_inited;
    bool m_continue_working;
    bool m_host_change;         /**<true:有新机器加入,false:机器列表无更新*/
    bool *m_p_thread_started;

    pthread_t m_work_thread_id;
    int m_listen_fd;

    i_config *m_p_config;
    c_hash_table *m_p_hash_table;
};

/**
 * @brief  重置机器新增标志位
 * @param  无
 * @return 前一次标志位值
 */
inline bool c_listen_thread::reset_host_add()
{
    bool old_status = m_host_change;
    m_host_change = false;
    return old_status;
}

#endif
