/**
 * =====================================================================================
 *       @file  flush_cleanup_thread.cpp
 *      @brief 
 *
 *  flush the metric of this source,and cleanup the metrics timeouted
 *
 *   @internal
 *     Created  10/18/2010 09:57:16 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason , mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "../lib/log.h"
#include "../lib/type_hash.h"
#include "../proto.h"
#include "../lib/hash.h"
#include "../lib/utils.h"
#include "../db_operator.h"
#include "./flush_cleanup_thread.h"

c_flush_cleanup_thread::c_flush_cleanup_thread(): m_inited(false), m_work_thread_id(0), m_continue_working(false), m_p_config(NULL),m_db_conn(NULL),m_p_root(NULL),m_p_db_conf(NULL)
{
}

c_flush_cleanup_thread::~c_flush_cleanup_thread()
{
    uninit();
}

/**
 * @brief  初始化对象,要么成功，要么失败会uninit已经init成功的变量
 * @param  p_config: 指向config类的指针
 * @param  p_root    保存本地数据源的根节点的指针
 * @param  p_rrd     rrd操作对象的指针
 * @param  p_db_conf 数据库配置的指针
 * @return 0:success, -1:failed
 */
int c_flush_cleanup_thread::init(const collect_conf_t *p_config, source_t *p_root, const db_conf_t *p_db_conf)
{
    if(m_inited) {
        ERROR_LOG("ERROR: c_flush_cleanup_thread has been inited.");
        return -1;
    } 

    if(p_config == NULL || p_root == NULL || p_db_conf == NULL) {
        ERROR_LOG("ERROR: parameter invalid, cann't be NULL");
        return -1;
    }

    m_p_root = p_root;
    m_p_config = (collect_conf_t *)p_config;
    m_p_db_conf = p_db_conf;

    if(create_mysql_iface_instance(&m_db_conn) != 0) {
        ERROR_LOG("create mysql instance failed."); 
        m_continue_working = false;
        m_work_thread_id = 0;
        m_p_config = NULL;
        m_p_root = NULL;
        m_db_conn = NULL;
        m_p_db_conf = NULL;
        return -1;
    }

    //if(m_db_conn->init(p_db_conf->db_host, p_db_conf->db_port, p_db_conf->db_name,
    //            p_db_conf->db_user, p_db_conf->db_pass, "utf8") != 0)
    //{
    //    ERROR_LOG("init mysql failed."); 
    //    m_continue_working = false;
    //    m_work_thread_id = 0;
    //    m_p_config = NULL;
    //    m_p_root = NULL;
    //    m_db_conn->release();
    //    m_db_conn = NULL;
    //    return -1;
    //}

    m_continue_working = true;
    if(0 != pthread_create(&m_work_thread_id, NULL, flush_main, this)) {
        ERROR_LOG("Pthread_create() failed."); 
        m_continue_working = false;
        m_work_thread_id = 0;
        m_p_config = NULL;
        m_p_root = NULL;
        //m_db_conn->uninit();
        m_db_conn->release();
        m_db_conn = NULL;
        m_p_db_conf = NULL;
        return -1;
    }

    DEBUG_LOG("Init Info: flush_cleanup_thread init succ.");
    m_inited = true;
    return 0;
}

/**
 * @brief  反初始化
 * @param  无
 * @return 0:success, -1:failed
 */
int c_flush_cleanup_thread::uninit()
{
    if(!m_inited)
    {
        return -1; 
    } 

    assert(m_work_thread_id != 0);

    m_continue_working = false;
    pthread_join(m_work_thread_id, NULL);
    m_work_thread_id = 0;

    m_db_conn->uninit();
    m_db_conn->release();
    m_db_conn = NULL;
    m_p_db_conf = NULL;

    m_p_config = NULL;
    m_p_root = NULL;
    m_inited = false;

    return 0;
}

/**
 * @brief  子线程的线程函数 
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success
 */
void *c_flush_cleanup_thread::flush_main(void *p_data)  
{
    assert(NULL != p_data);
    c_flush_cleanup_thread *p_instance = (c_flush_cleanup_thread *)p_data;

    while(p_instance->m_continue_working) {   
        unsigned int sec = 0;
        while(p_instance->m_continue_working && sec++ < p_instance->m_p_config->summary_interval) {
            sleep(1);
        }

        if(!p_instance->m_continue_working) {
            break;
        }

        DEBUG_LOG("Start to connecting to mysql.");
        ///连接数据库
        if(p_instance->m_db_conn->init(p_instance->m_p_db_conf->db_host, p_instance->m_p_db_conf->db_port,
                    p_instance->m_p_db_conf->db_name, p_instance->m_p_db_conf->db_user,
                    p_instance->m_p_db_conf->db_pass, "utf8") != 0) {
            continue;
        }

        //先执行清理
        DEBUG_LOG("Cleanup start");
        cleanup_arg_t cleanup;  
        struct timeval tv;
        gettimeofday(&tv, NULL);
        cleanup.tv = &tv;
        cleanup.key = 0;
        cleanup.val = 0;
        cleanup.hashval = 0;
        cleanup.db_conn = p_instance->m_db_conn;

        hash_walkfrom(p_instance->m_p_root->children, cleanup.hashval, cleanup_source, (void *)&cleanup);
        DEBUG_LOG("Cleanup finished");

        hash_foreach(p_instance->m_p_root->children, do_flush_data, (void*)p_instance);
        ///断开数据库连接
        p_instance->m_db_conn->uninit();
    }
    DEBUG_LOG("Exit the main while loop of summary&cleanup thread.");
    return NULL;
}

/**
 * @brief  汇总子数据源的summary信息到根数据源的回调函数
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return 0-success
 */
int c_flush_cleanup_thread::do_flush_data(datum_t *key, datum_t *val, void *arg)
{
    source_t *source = (source_t *)val->data;

    if(source->ds->dead) {
        DEBUG_LOG("Ds [%u] dead.", source->ds->ds_id);
        return 0;
    }

    //如果是个cluster数据源则将下面的host信息写到数据库
    if(source->children && source->node_type == CLUSTER_NODE) {
        hash_foreach(source->children, update_host_info, arg);
    } else {
        DEBUG_LOG("DS %u have not children or is not a cluster.", source->ds->ds_id);
    }

    return 0;
}

/**
 * @brief 将host的信息写到数据库中
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return 0-success, 1-failed
 */
int c_flush_cleanup_thread::update_host_info(datum_t *key, datum_t *val, void *arg)
{
    c_mysql_iface *db_conn = ((c_flush_cleanup_thread*)arg)->m_db_conn;
    db_update_host_info(db_conn, (const char*)key->data, NULL, (const host_t*)val->data);
    return 0;
}

/**
 * @brief  将数据源里时间大于dmax的数据全部删掉  
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return (void *)0:success, (void *)-1:failed
 */
int c_flush_cleanup_thread::cleanup_source(datum_t *key, datum_t *val, void *arg)
{
    cleanup_arg_t *p_node_cleanup = (cleanup_arg_t *)arg; 
    source_t *p_source = (source_t *)val->data;
    datum_t *rv = NULL;

    c_mysql_iface *db_conn = p_node_cleanup->db_conn;
    cleanup_arg_t cleanup;
    cleanup.tv = p_node_cleanup->tv;
    cleanup.key = 0;
    cleanup.hashval = 0;

    /*
     * 当metric的dmax不为0且metric的时间间隔大于dmax时，hash_walkfrom返回-1
     * 由于hash_walkfrom里面会对hash加锁，所以不能在回调函数里面操作这个hash，必须等hash_walkfrom返回再操作
     */
    while(hash_walkfrom(p_source->metric_summary, cleanup.hashval, cleanup_metric, (void *)&cleanup)) {
        // 将metric_summary里时间大于dmax的数据删掉 
        DEBUG_LOG("cleanup delete metric_summary:%s", (char *)cleanup.key->data);
        if(cleanup.key) {
            cleanup.hashval = hashval(cleanup.key, p_source->metric_summary);
            rv = hash_delete(cleanup.key, p_source->metric_summary);
            if(rv) {
                datum_free(rv);
            }
            cleanup.key=0;
        }

        break;
    } 

    if(!p_source->children) {
        return 0; 
    }

    cleanup.tv = p_node_cleanup->tv;
    cleanup.key = 0;
    cleanup.hashval = 0;

    // 当机器的tmax大于dmax时，hash_walkfrom会返回-1
    while(hash_walkfrom(p_source->children, cleanup.hashval, cleanup_node, (void *)&cleanup)) {
        if(cleanup.key) { 
            DEBUG_LOG("cleanup deleting host:%s", (char *)cleanup.key->data);

            host_t *p_node = (host_t *)cleanup.val->data;
            hash_t *metrics_hash = p_node->metrics;
            hash_t *metrics_status_hash = p_node->metrics_status;
            char host_name[17] = {0};
            char host_ip[16] = {0};
            strncpy(host_name, (const char*)(cleanup.key->data), sizeof(host_name) - 1);
            strncpy(host_ip, p_node->ip, sizeof(host_ip) - 1);

            // 从hash里删掉这个host
            cleanup.hashval = hashval(cleanup.key, p_source->children);
            rv = hash_delete(cleanup.key, p_source->children);
            if(rv) {
                db_del_host_info(db_conn, host_name, host_ip);
                ///释放metrics hash表
                hash_destroy(metrics_hash);
                ///释放metrics_status hash表
                hash_destroy(metrics_status_hash);
                datum_free(rv);
            }
            cleanup.key = 0;
        }
        break;
    }

    return 0;
}

/**
 * @brief  将host里时间大于dmax的数据全部删掉  
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return (void *)0:success, (void *)-1:failed
 */
int c_flush_cleanup_thread::cleanup_node(datum_t *key, datum_t *val, void *arg)
{
    cleanup_arg_t *p_node_cleanup = (cleanup_arg_t *)arg;
    host_t *p_host = (host_t *)val->data; 
    datum_t *rv = NULL;

    unsigned interval = p_node_cleanup->tv->tv_sec - p_host->t0.tv_sec; 
    if(p_host->dmax && interval > p_host->dmax) {
        //Host is older than dmax. Delete.
        p_node_cleanup->key = key;
        p_node_cleanup->val = val;
        return -1;
    }

    cleanup_arg_t cleanup;
    cleanup.tv = p_node_cleanup->tv;
    cleanup.key = 0;
    cleanup.hashval = 0;

    while(hash_walkfrom(p_host->metrics, cleanup.hashval, cleanup_metric, (void *)&cleanup)) {
        // 将metric里时间大于dmax的数据删掉 
        if(cleanup.key) {
            cleanup.hashval = hashval(cleanup.key, p_host->metrics);
            rv = hash_delete(cleanup.key, p_host->metrics);
            if(rv) {
                datum_free(rv);
            }
            cleanup.key = 0;
        }
        break;
    } 

    cleanup.key = 0;
    cleanup.hashval = 0;
    rv = NULL;

    while(hash_walkfrom(p_host->metrics_status, cleanup.hashval, cleanup_metric_status, (void *)&cleanup)) {
        // 将metrics_status里状态是ok的清除
        if(cleanup.key) {
            cleanup.hashval = hashval(cleanup.key, p_host->metrics_status);
            rv = hash_delete(cleanup.key, p_host->metrics_status);
            if(rv) {
                datum_free(rv);
            }
            cleanup.key = 0;
        }
        break;
    } 
    return 0;
}

/**
 * @brief  找出时间大于dmax的metric  
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return (void *)0:找不到, (void *)-1:找到大于dmax的metric
 */
int c_flush_cleanup_thread::cleanup_metric(datum_t *key, datum_t *val, void *arg)
{
    cleanup_arg_t *p_cleanup = (cleanup_arg_t *)arg;
    metric_t *p_metric = (metric_t *)val->data; 

    unsigned int interval = p_cleanup->tv->tv_sec - p_metric->t0.tv_sec; 

    // Never delete a metric if its DMAX=0(表示随时间不会变化的指标，如cpu个数)
    if(p_metric->dmax && interval > p_metric->dmax) {
        p_cleanup->key = key;
        return -1;
    }
    return 0;
}

/**
 * @brief  找出metric的过期的且警报状态是ok的
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return 0,-1
 */
int c_flush_cleanup_thread::cleanup_metric_status(datum_t *key, datum_t *val, void *arg)
{
    cleanup_arg_t        *p_cleanup = (cleanup_arg_t *)arg;
    metric_status_info_t *p_metric_status = (metric_status_info_t *)val->data; 

    if(time(0) - p_metric_status->last_alarm > 300 && p_metric_status->cur_status == STATUS_OK) {
        p_cleanup->key = key;
        return -1;
    }

    return 0;
}
