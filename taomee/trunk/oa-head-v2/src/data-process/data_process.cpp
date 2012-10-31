/**
 * =====================================================================================
 *       @file  data_process.cpp
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
#include <algorithm>
#include <functional>
#include <string.h>
#include <stdint.h>
#include <netdb.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "../lib/log.h"
#include "../db_operator.h"
#include "./data_process.h"

using namespace std;

/** 
 * @brief  构造函数
 * @param   
 * @return  
 */
c_data_process::c_data_process():m_inited(false), m_continue_working(false), m_p_config(NULL),
    m_p_data_sources(NULL), m_p_default_alarm_info(NULL), m_p_special_alarm_info(NULL)
{
    memset(&m_root, 0 , sizeof(m_root));
    m_process_thread.clear();
    m_queues.clear();
}

/** 
 * @brief  析构函数
 * @param   
 * @return  
 */
c_data_process::~c_data_process()
{
    uninit();
}

/** 
 * @brief   反初始化本对象
 * @return  0:success -1:failed 
 */
int c_data_process::uninit()
{
    if(!m_inited) {
        return -1;
    }

    //首先退出alarm和flush线程
    m_alarm_thread.uninit();
    m_flush_cleanup_thread.uninit();
    m_rrd_handler.uninit();

    //退出data_process threads
    if(!m_process_thread.empty()) {
        map<unsigned int, c_data_processer *>::iterator it_p;
        for(it_p = m_process_thread.begin(); it_p != m_process_thread.end(); it_p++) {
            c_data_processer  *tmp_process = it_p->second; 
            if(tmp_process != NULL) {
                tmp_process->uninit();
                tmp_process->release();
            }
        }
        m_process_thread.clear();
    }

    //销毁队列
    if(!m_queues.empty()) {
        map<unsigned int, i_ring_queue* >::iterator it_q;
        for(it_q = m_queues.begin(); it_q != m_queues.end(); it_q++) {
            i_ring_queue  *tmp_queue = it_q->second; 
            if(tmp_queue != NULL) {
                tmp_queue->uninit();
                tmp_queue->release();
            }
        }
        m_queues.clear();
    }

    if(NULL != m_root.children) {
        hash_destroy(m_root.children);
        m_root.children = NULL;
    }
    if(NULL != m_root.metric_summary) {
        hash_destroy(m_root.metric_summary);
        m_root.metric_summary = NULL;
    }
   
    m_p_config = NULL;
    m_p_data_sources = NULL;
    m_p_default_alarm_info = NULL;
    m_p_special_alarm_info = NULL;
    m_inited   = false;

    return 0;
}


/** 
 * @brief  初始化函数
 * @param   p_config 一般配置信息
 * @param   p_db_conf 数据库配置信息
 * @param   p_default_alarm_info 缺省告警配置
 * @param   p_special_alarm_info 特殊告警配置
 * @return  0success -1failed 
 */
int c_data_process::init(
        ds_t *p_data_sources,
        const collect_conf_t *p_config,
        const db_conf_t *p_db_conf,
        const metric_alarm_vec_t *p_default_alarm_info,
        const metric_alarm_map_t *p_special_alarm_info)
{
    if(m_inited) {
        ERROR_LOG("ERROR: c_data_process has been inited.");
        return -1;
    }
    if(NULL == p_config || NULL == p_data_sources || NULL == p_db_conf ||
            NULL == p_default_alarm_info || NULL == p_special_alarm_info) {
        ERROR_LOG("ERROR: Parament cannot be NULL.");
        return -1;
    }
    if (0 != m_rrd_handler.init(p_config->rrd_dir, p_config->grid_id)) {
        ERROR_LOG("ERROR: m_rrd_handler.init(%s, %u) failed.", p_config->rrd_dir, p_config->grid_id);
        return -1;
    }

    m_p_config = p_config;
    m_p_data_sources = p_data_sources;
    m_p_default_alarm_info = p_default_alarm_info;
    m_p_special_alarm_info = p_special_alarm_info;

    do {
        m_root.node_type = ROOT_NODE;/**<保存本地数据源信息的根节点*/
        m_root.children = hash_create(DEFAULT_GRIDSIZE);
        if(NULL == m_root.children) {
            ERROR_LOG("ERROR: Create children hash table for root data source failed.");
            break;
        }
        hash_set_flags(m_root.children, HASH_FLAG_IGNORE_CASE);

        m_root.metric_summary = hash_create(DEFAULT_METRICSIZE);
        if(NULL == m_root.metric_summary) {
            ERROR_LOG("ERROR: Create metric_summary hash table for root data source failed.");
            break;
        }
        hash_set_flags(m_root.metric_summary, HASH_FLAG_IGNORE_CASE);

        //数据解析线程，并把相关数据写入rrd
        if (0 != start_data_parse_thread()) {
            ERROR_LOG("ERROR: start_data_parse_thread() failed.");
            break;
        }
        DEBUG_LOG("start_data_parse_thread succ.");

        //告警线程
        if(0 != m_alarm_thread.init(m_p_config, &m_queues, m_p_default_alarm_info, m_p_special_alarm_info)) {
            ERROR_LOG("ERROR: init alarm_thread failed.");
            break;
        }
        DEBUG_LOG("alarm thread init succ.");

        //数据清理后写入mysql
        if(m_flush_cleanup_thread.init(p_config, &m_root, p_db_conf) != 0) {
            ERROR_LOG("ERROR: init flush_cleanup_thread failed.");
            m_alarm_thread.uninit();
            break;
        }
        DEBUG_LOG("flush thread init succ.");
        m_inited = true;
    } while (false);


    if (!m_inited) { //反初始化已经初始化的变量
        m_rrd_handler.uninit();
        if(!m_process_thread.empty()) {
            map<unsigned int, c_data_processer *>::iterator it_p;
            for(it_p = m_process_thread.begin(); it_p != m_process_thread.end(); it_p++) {
                c_data_processer  *tmp_process = it_p->second; 
                if(tmp_process != NULL) {
                    tmp_process->uninit();
                    tmp_process->release();
                }
            }
            m_process_thread.clear();
        }

        if(!m_queues.empty()) {
            map<unsigned int , i_ring_queue* >::iterator it_q;
            for(it_q = m_queues.begin(); it_q != m_queues.end(); it_q++) {
                i_ring_queue  *tmp_queue = it_q->second; 
                if(tmp_queue != NULL) {
                    tmp_queue->uninit();
                    tmp_queue->release();
                }
            }
            m_queues.clear();
        }

        m_p_config = NULL;
        m_p_data_sources = NULL;
        m_p_default_alarm_info = NULL;
        m_p_special_alarm_info = NULL;

        return -1;
    }

    return 0;
}

/** 
 * @brief  启动所有数据源的数据处理线程
 * @param  无
 * @return  0:success -1:failed 
 */
int c_data_process::start_data_parse_thread()
{
    int  queue_len = m_p_config->queue_len;
    //queue如果大于8M,当做8M,小于等于0当做8k
    if(queue_len <= 0) {
        queue_len = 8 * 1024;
    } else if(queue_len > 8 * 1024 * 1024) {
        queue_len = 8 * 1024 * 1024;
    } else {
        //正确的长度 do nothing here 
    }

    c_data_processer *processer = NULL; 
    i_ring_queue *raw_queue = NULL, *usable_queue = NULL; 

    map<unsigned int, data_source_list_t>::iterator it;
    for(it = m_p_data_sources->begin(); it != m_p_data_sources->end(); it++) {
        processer = NULL; 
        raw_queue = NULL;
        usable_queue = NULL; 

        if(0 != create_data_processer_instance(&processer)) {
            ERROR_LOG("Fail to create data_processer instance for datasource [%u].", it->first);
            continue;
        }

        if(0 != create_variable_queue_instance(&raw_queue, sizeof(uint16_t))) {
            ERROR_LOG("Fail to create the variable queue instance for datasource:[%u].", it->first);
            processer->release();
            continue;
        }

        if((usable_queue = create_waitable_queue_instance(raw_queue)) == NULL) {
            ERROR_LOG("Fail to create the waitable queue instance for datasource:[%u].", it->first);
            processer->release();
            raw_queue->release();
            continue;
        }

        if(0 != usable_queue->init(queue_len)) {
            ERROR_LOG("Fail to init the waitable queue for datasource:[%u].", it->first);
            processer->release();
            usable_queue->release();
            continue;
        }

        if(processer->init(&(it->second), &m_root, m_p_config, usable_queue, &m_rrd_handler,
                    m_p_default_alarm_info, m_p_special_alarm_info) != 0) {
            ERROR_LOG("Start data processer thread of data source:[%u] failed.", it->first);
            processer->release();
            usable_queue->uninit();
            usable_queue->release();
            continue;
        }

        m_queues.insert(map<unsigned int, i_ring_queue*>::value_type(it->first, usable_queue));
        m_process_thread.insert(map<unsigned int, c_data_processer*>::value_type(it->first, processer));
        DEBUG_LOG("Data processer thread of data source:[%u] starting.....", it->first);
    }

    return 0; 
}


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
        const metric_alarm_map_t *p_special_alarm_info)
{
    proc_log_init(PRE_NAME_COLLECT);
    DEBUG_LOG("=======================Init Info=======================");
    c_data_process data_process;
    int ret = data_process.init(p_data_sources, p_config, p_db_conf, p_default_alarm_info, p_special_alarm_info);
    //通知父进程init状态
    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%d", 0 == ret ? PROC_INIT_SUCC : PROC_INIT_FAIL);
    DEBUG_LOG("Notice monitor process PROC_INIT_STATUS[%s]", buff);
    DEBUG_LOG("========================================================\n");
    if (write(fd, buff, strlen(buff)) < 0) {
        ERROR_LOG("Collect Process: write(%s) to notice monitor process failed[%s].",
                (0 == ret) ? "PROC_INIT_SUCC" : "PROC_INIT_FAIL", strerror(errno));
        data_process.uninit();
        return -1;
    }

    DEBUG_LOG("=======================Enter Main Loop=======================");
    int read_len = 0;
    int cmd_id = 0;
    while (true) {
        read_len = read(fd, buff, sizeof(buff) - 1);
        if (read_len < 0) {
            ERROR_LOG("Collect Process: read(...) from parent process failed[%s].", strerror(errno));
            break;
        } else if (read_len > 0) {
            buff[read_len] = 0;
            DEBUG_LOG("recv parent cmd_id[%s]", buff);
            cmd_id = atoi(buff);
            if (PROC_NEED_STOP == cmd_id) {
                break;
            }
        }
        sleep(1);
    }
    DEBUG_LOG("=======================Leave Main Loop=======================");

    data_process.uninit();
    DEBUG_LOG("Data collect process[%d] exit succ.", getpid());
    return 0;
}
