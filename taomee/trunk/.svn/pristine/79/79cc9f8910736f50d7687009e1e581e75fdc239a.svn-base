/**
 * =====================================================================================
 *       @file  collect_thread.h
 *      @brief  根据配置，创建相应的采集线程
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
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:37:59
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_COLLECT_THREAD_H_2010_10_15
#define H_COLLECT_THREAD_H_2010_10_15

#include <string>
#include <vector>
#include <list>
#include "../i_config.h"
#include "../proto.h"
#include "../defines.h"
#include "./processor_thread.h"

class c_collect_thread
{
public:
    c_collect_thread();
    ~c_collect_thread();

    /**
     * @brief  初始化
     * @param  p_config: 指向config类的指针
     * @param  p_collect_info_vec: 指向所有可以收集的数据信息的指针
     * @param  p_metric_info_map: 指向metric的id到完整信息映射的指针
     * @param  start_time: OA_NODE启动的时间
     * @param  is_recv_udp: 是否是接收udp的主机
     * @return 0:success, -1:failed
     */
    int init(i_config *p_config, const char *p_inside_ip, const collect_info_vec_t *p_collect_info_vec, const metric_info_map_t *p_metric_info_map, time_t start_time, bool update_failed, bool is_recv_udp, uint32_t user);

    /**
     * @brief  反初始化
     * @param  无
     * @return 0:success, -1:failed
     */
    int uninit();

    /**
     * @brief  listen线程用来通知collect线程集群里有新的机器加入或者重启
     * @param  无
     * @return 无
     */
    void set_hosts_change();


    /**
     * @brief  发送特殊metric信息给所有发送主机(只有metric_id，没有metric_value)
     * @param  int metric_id: 必须小于-1
     * @return success:0, failed:-1
     */
    int send_special_metric_info(int metric_id);

protected:
    /**
     * @struct metric_config_t
     * @brief  每个metric在配置文件里面读出来的配置信息
     */
    typedef struct {
        int metric_type;
        char metric_name[OA_MAX_STR_LEN];
        char arg[OA_MAX_STR_LEN];                       /**< 报警的回调函数的参数 */
        char formula[OA_MAX_STR_LEN];                   /**< 报警的公式  */
        float value_threshold;                          /**< 超过这个阀值了才发送数据 */
    } metric_config_t;

    /**
     * @struct group_config_t
     * @brief  每个group在配置文件里面读出来的配置信息 */
    typedef struct {
        int collect_interval;
        int time_threshold;
        int metric_num;
    } group_config_t;

    int set_collect_group_info(const collect_info_vec_t *p_collect_info_vec, uint32_t user);

    int set_interval_by_config_value(const collect_info_vec_t *p_collect_info_vec,
                                     const std::vector<group_config_t> &group_config_vec,
                                     const std::vector<metric_config_t> &metric_config_vec,
                                     int heartbeat_time
                                    );


private:
    enum {
        GROUP_RRD,
        GROUP_ALARM
    };

    bool m_inited;
    int m_sockfd;

    std::list<metric_collect_t> m_metric_collect_list;
    std::vector<collect_group_t> m_collect_group_rrd_vec;   /**< 收集写rrd和可以立即完成的探测的分组 */
    std::vector<collect_group_t> m_collect_group_alarm_vec; /**< 收集不能立即返回的报警信息的分组 */
    c_processor_thread *m_p_processor_thread_rrd;           /**< 指向收集rrd数据的线程的指针 */
    c_processor_thread *m_p_processor_thread_alarm;         /**< 指向收集alarm数据的线程的指针 */
    std::vector<send_addr_t> m_send_addr_vec;

    i_config *m_p_config;
    char m_server_tag[OA_MAX_STR_LEN];
    unsigned int m_host_ip;
};

#endif //H_COLLECT_THREAD_H_2010_10_15
