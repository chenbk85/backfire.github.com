/**
 * =====================================================================================
 *       @file  data_processer.h
 *      @brief  
 *
 *  request the xml data from data source.then parse them ,and save them into haash tree
 *
 *   @internal
 *     Created  2010-10-18 11:13:42
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
 
 
#ifndef DATA_PROCESSER_H
#define DATA_PROCESSER_H

#include "../lib/i_ring_queue.h"
#include "../lib/c_mysql_iface.h"
#include "../lib/net_client_impl.h"
#include "../defines.h"
#include "../proto.h"
#include "./xml_parser.h"

const unsigned int MAX_BUF_SIZE = 8 * 1024 * 1024;

typedef struct host_status_arg
{
    const char *host_name;
    int         host_status;
}host_status_arg;
class c_data_processer
{
    public :
        c_data_processer();
        ~c_data_processer();
        int init(data_source_list_t *ds, source_t *p_root, const collect_conf_t *p_config,
                i_ring_queue*  p_queue, void *p_rrd,
                const metric_alarm_vec_t *default_metric_alarm_info,
                const metric_alarm_map_t *specified_metric_alarm_info);
        int uninit();
        int release();
    protected:
        /** 
         * @brief   线程主函数
         * @param   p_data  用户数据
         * @return  NULL success UNNULL failed
         */
        static void* data_processer_main(void *p_data);

        /** 
         * @brief   检查一个xml文档是不是有结束标签
         * @param   buffer  xml数据指针
         * @param   len     xml数据长度
         * @return  true success false failed
         */
        static bool is_match(const char* buffer, const int len);

        /** 
         * @brief   打印一个数据源的全景信息
         * @param   ds  数据源指针
         * @return  NULL success UNNULL failed
         */
        int print_data_source(const data_source_list_t* ds);

        void set_host_status(const char* ds_name, const char *host_name, int host_status);
        static int _set_host_status(datum_t *key, datum_t *val, void *arg);

        /** 
         * @brief   当一个数据源down时报警 
         * @param   ds 数据源结构 
         * @return  void  
         */
        void report_data_source_down(const char *ds_name, const data_source_list_t * ds);

        /** 
         * @brief   当一个数据源从down到up时收回报警 
         * @param   ds 数据源结构
         * @return  void  
         **/
        void revoke_data_source_down(const char *ds_name, const data_source_list_t* ds);

    private:
        bool                m_inited;            /**<是否初始化标志*/
        data_source_list_t  *m_ds;               /**<本线程管理的数据源信息*/
        char                m_buf[MAX_BUF_SIZE + 1];
        c_net_client_impl   *m_ds_conn;          /**<和本数据源建立的连接*/
        source_t            *m_p_root;           /**<保存本head数据源根节点*/
        i_ring_queue        *m_p_queue;          /**<ring_queue对象指针*/
        xml_parser          m_parser;           /**<xml_parser对象*/
        pthread_t           m_pid;              /**<线程id*/
        bool                m_stop;
        char                *m_grid_name;
        unsigned int        m_grid_id;
};

int create_data_processer_instance(c_data_processer  **pp_instance);

#endif
