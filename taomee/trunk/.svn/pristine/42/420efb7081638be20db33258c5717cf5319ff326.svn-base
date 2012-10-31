/**
 * =====================================================================================
 *       @file  summary_cleanup_thread.h
 *      @brief    
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  10/21/2010 08:34:12 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_FLUSH_CLEANUP_THREAD_H_2010_10_21
#define H_FLUSH_CLEANUP_THREAD_H_2010_10_21
#include "./rrd_handler.h"
#include "../lib/c_mysql_iface.h"
#include "../proto.h"

class c_flush_cleanup_thread
{
public:
    c_flush_cleanup_thread();
    ~c_flush_cleanup_thread();

    /**
     * @brief  初始化对象,要么成功，要么失败会uninit已经init成功的变量
     * @param  p_config: 指向config类的指针
     * @param  p_root    保存本地数据源的根节点的指针
     * @param  p_rrd     rrd操作对象的指针
     * @param  p_db_conf 数据库配置的指针
     * @return 0:success, -1:failed
     */
    int init(const collect_conf_t *p_config, source_t *p_root, const db_conf_t *p_db_info);

    /**
     * @brief  反初始化
     * @param  无
     * @return 0:success, -1:failed
     */
    int uninit();

protected:
    /**
     * @brief  子线程的线程函数 
     * @param  p_data: 指向当前对象的this指针
     * @return (void *)0:success, (void *)-1:failed
     */
    static void *flush_main(void *p_data);

    /**
     * @brief  写数据到mysql
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return 0:success, -1:failed
     */
    static int do_flush_data(datum_t *key, datum_t *val, void *arg);

    /**
     * @brief  将host的信息写到数据库中
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return 0:success, -1:failed
     */
    static int update_host_info(datum_t *key, datum_t *val, void *arg);

    /**
     * @brief  将数据源里时间大于dmax的数据全部删掉  
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return (void *)0:success, (void *)-1:failed
     */
    static int cleanup_source(datum_t *key, datum_t *val, void *arg);

    /**
     * @brief  将host里时间大于dmax的数据全部删掉  
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return (void *)0:success, (void *)-1:failed
     */
    static int cleanup_node(datum_t *key, datum_t *val, void *arg);

    /**
     * @brief  找出时间大于dmax的metric  
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return (void *)0:找不到, (void *)-1:找到大于dmax的metric
     */
    static int cleanup_metric(datum_t *key, datum_t *val, void *arg);

    /**
     * @brief  找出metric的警报状态是ok的
     * @param  key: hash的键
     * @param  val: hash里对应key的值
     * @param  arg: 传给回调函数的用户自定义参数
     * @return 0:不是ok状态,-1:是ok状态
     */
    static int cleanup_metric_status(datum_t *key, datum_t *val, void *arg);
private:
    /** @struct 传递给cleanup回调函数的参数 */
    typedef struct {
        struct   timeval *tv;
        datum_t *key;
        datum_t *val;
        size_t   hashval;
        c_mysql_iface *db_conn; 
    } cleanup_arg_t;

    bool m_inited;
    bool m_continue_working;    /**<控制是否继续循环的bool值*/
    pthread_t m_work_thread_id; /**<子线程id*/
    collect_conf_t *m_p_config;   /**<保存配置对象的指针*/
    source_t *m_p_root;         /**<保存本head数据源的根节点*/
    c_mysql_iface *m_db_conn;   /**<数据库对象指针*/
    const db_conf_t *m_p_db_conf;
};

#endif //H_FLUSH_CLEANUP_THREAD_H
