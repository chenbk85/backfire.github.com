/**
 * =====================================================================================
 *       @file  data_process.h
 *      @brief  
 *
 *  request the xml data from data source.then parse them ,and save them into hash tree
 *
 *   @internal
 *     Created  2010-10-18 11:13:42
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 *     @author  tonyliu, tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef DATA_PROCESS_H
#define DATA_PROCESS_H

#include <map>
#include <string>
#include "../lib/i_ring_queue.h"
#include "../lib/c_mysql_iface.h"
#include "../proto.h"
#include "./data_processer.h"
#include "./alarm_thread.h"
#include "./flush_cleanup_thread.h"

class c_data_process
{
public :
    c_data_process();
    ~c_data_process();
    int init(ds_t *p_data_sources,
            const collect_conf_t *p_config,
            const db_conf_t *p_db_conf,
            const metric_alarm_vec_t *p_default_alarm_info,
            const metric_alarm_map_t *p_special_alarm_info);
    int uninit();
protected :
    /** 
     * @brief  启动所有数据源的数据处理线程
     * @param   无
     * @return  0:success -1:failed 
     */
    int start_data_parse_thread();
private:
    bool m_inited;      /**<是否初始化标志*/
    bool m_continue_working;

    const collect_conf_t *m_p_config;   /**<config对象指针*/ 
    ds_t *m_p_data_sources;
    const metric_alarm_vec_t *m_p_default_alarm_info; /**<缺省报警配置*/
    const metric_alarm_map_t *m_p_special_alarm_info; /**<特殊报警配置*/

    source_t m_root; /**<数据源根节点*/
    std::map<unsigned int, i_ring_queue*> m_queues; /**<保存每个数据源线程的queue指针*/
    std::map<unsigned int, c_data_processer*> m_process_thread; /**<保存c_data_processer指针的map*/

    c_alarm_thread m_alarm_thread;  /**<报警线程*/
    c_flush_cleanup_thread m_flush_cleanup_thread; /**<汇总&清理线程*/
    c_rrd_handler m_rrd_handler; /**<rrd对象指针*/
};

/** 
 * @brief  初始化函数
 * @param   fd 与父进程通信的fd
 * @param   p_config 一般配置信息
 * @param   p_db_conf 数据库配置信息
 * @param   p_default_alarm_info 缺省告警配置
 * @param   p_special_alarm_info 特殊告警配置
 * @return  0success -1failed 
 */
int collect_process_run(int fd,
        ds_t *p_data_sources,
        const collect_conf_t *p_config,
        const db_conf_t *p_db_conf,
        const metric_alarm_vec_t *p_default_alarm_info,
        const metric_alarm_map_t *p_special_alarm_info);

#endif
