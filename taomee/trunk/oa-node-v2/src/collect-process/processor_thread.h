/**

 * =====================================================================================
 *       @file  processor_thread.h
 *      @brief  创建一个新的线程采集相应的数据，并通过UDP发送
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  10/15/2010 08:34:12 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  luis (程龙), luis@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:36:48
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_PROCESSOR_THREAD_H_2010_10_15
#define H_PROCESSOR_THREAD_H_2010_10_15

#include <rpc/rpc.h>
#include "../i_config.h"
#include "../proto.h"

class c_processor_thread
{
public:
    c_processor_thread();
    ~c_processor_thread();

    /**
     * @brief  初始化
     * @param  p_config: 指向config类的指针
     * @param  p_collect_info_vec: 指向所有可以收集的数据信息的指针
     * @param  p_metric_info_map: 指向metric的id到完整信息映射的指针
     * @param  start_time: OA_NODE启动的时间
     * @return 0:success, -1:failed
     */
    int init(i_config *p_config,
             const collect_group_vec_t *p_collect_group_vec,
             const send_addr_vec_t *p_send_addr_vec,
             time_t start_time,
             const char *p_server_tag,
             int host_ip,
             int sock_fd
            );

    /**
     * @brief  反初始化
     * @param  无
     * @return 0:success, -1:failed
     */
    int uninit();

    /**
     * @brief  释放动态分配的内存
     * @param  无
     * @return 0:success, -1:failed
     */
    int release();

protected:
    typedef struct {
        value_t now_value;
        value_t last_value;
    } metric_value_t;

    typedef struct {
        int next_collection;
        int next_send;
        metric_value_t metric_value[OA_MAX_GROUP_NUM];
    } group_arg_t;

    typedef struct {
        char * value_str;
        void * next;
    } value_str_list_node;

    /**
     * @brief  搜集rrd的线程函数
     * @param  p_data: 指向当前对象的this指针
     * @return (void *)0:success, (void *)-1:failed
     */
    static void *work_thread_proc(void *p_data);

    /**
     * @brief  遍历collection group，采集到达了采集时间的metric的数据
     * @param  无
     * @return 无
     */
    void collection_group_collect();

    /**
     * @brief  遍历group里的发送数据，将到时的数据发送出去
     * @param  无
     * @return 无
     */
    void collection_group_send();

    /**
     * @brief  采集到达时间的一组metric
     * @param  group_iter: 指向需要收集的一组的迭代器
     * @return 无
     */
    void collect_group_data(int index);

    /**
     * @brief  判断新采集的数据是否超过了设定的阈值
     * @param  p_metric_collect: 保存的每个metric的采集信息
     * @param  p_value: 采集到的值
     * @return true:集群里有机器重启或新加入, false:集群里没有机器重启或新加入
     */
    bool check_value_threshold(const metric_value_t &value, float value_threshold, value_type_t type);

    /**
     * @brief  将一个metric数据打包发送出去
     * @param  p_send_data: 需要发送的metric数据
     * @return 0:success, -1:failed
     */
    int send_metric_data(metric_send_t *p_send_data);

    /**
     * @brief  将需要发送的一个metric的信息打包成相应的xdr格式
     * @param  p_send_data: 需要发送的数据
     * @param  p_x: XDR对象的指针
     * @param  p_msg_len: 需要发送的数据的长度
     * @return 0:success, -1:failed
     */
    int xdr_pack_send_data(metric_send_t *p_send_data, XDR *p_x, unsigned short *p_msg_len);

    /**
     * @brief  将metric的值打包成相应的xdr格式
     * @param  p_value: metric的值
     * @param  type: metric的值的类型
     * @param  p_x: XDR对象的指针
     * @return 0:success, -1:failed
     */
    int xdr_pack_metric_value(value_t *p_value, value_type_t type, XDR *p_x);

    /**
     * @brief  记录用户自定义的发送信息
     * @param  p_value: metric的值
     * @param  type: metric的值的类型
     * @param  p_str: metric值的字符串形式
     * @return 0:success, -1:failed
     */
    int get_metric_value(const value_t *p_value, value_type_t type, const char *fmt, char *p_str);

private:
    bool m_inited;
    pthread_t m_work_thread_id;
    bool m_continue_working;
    const char *m_p_server_tag;                                    /**< 二进制表示的用来唯一标识本机的ip */
    int m_host_ip;
    int m_wakeup_time;
    time_t m_start_time;

    int m_sockfd;

    std::vector<group_arg_t> m_group_arg_vec;

    i_config *m_p_config;
    const collect_group_vec_t *m_p_collect_group_vec;
    const send_addr_vec_t *m_p_send_addr_vec;

    value_str_list_node m_value_str_list;
    value_str_list_node *m_p_value_str_list_last;

};

int create_processor_thread_instance(c_processor_thread **pp_instance);

#endif //H_COLLECT_THREAD_H_2010_10_15
