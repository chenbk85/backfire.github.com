/**
 * =====================================================================================
 *       @file  collect_thread.cpp
 *      @brief  并定时采集数据，插入共享的链表里面,并将到时的数据通过UDP发送出去
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  10/18/2010 09:57:16 AM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  luis (程龙), luis@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:37:25
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <new>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>

#include "../lib/utils.h"
#include "../lib/log.h"
#include "./collect_thread.h"
#include "../proto.h"

using namespace std;

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf, ret_code)\
    do \
    {\
        buf[0] = 0;\
        if(m_p_config->get_config(section, name, buf, sizeof(buf)) != 0) \
        {\
            ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
            ret_code;\
        }\
    } while(0)

c_collect_thread::c_collect_thread(): m_inited(false), m_sockfd(0), m_p_processor_thread_rrd(NULL), m_p_processor_thread_alarm(NULL), m_p_config(NULL), m_host_ip(0)
{
    memset(m_server_tag, 0, sizeof(m_server_tag));
}

c_collect_thread::~c_collect_thread()
{
    uninit();
}

/**
 * @brief  初始化
 * @param  p_config: 指向config类的指针
 * @param  p_collect_info_map: 指向所有可以收集的数据信息的指针
 * @param  p_metric_info_map: 指向metric的id到完整信息映射的指针
 * @param  start_time: OA_NODE启动的时间
 * @return 0:success, -1:failed
 */
int c_collect_thread::init(i_config *p_config,
        const char *p_inside_ip,
        const collect_info_vec_t *p_collect_info_vec,
        const metric_info_map_t *p_metric_info_map,
        time_t start_time,
        bool update_failed,
        bool is_recv_udp,
        uint32_t user
        )
{
    if (m_inited)
    {
        ERROR_LOG("ERROR: collect thread has been inited.");
        return -1;
    }

    if (NULL == p_config || NULL == p_collect_info_vec || NULL == p_metric_info_map || user >= sizeof(g_user_list)/sizeof(g_user_list[0]))
    {
        ERROR_LOG("p_config=%p p_collect_info_vec=%p p_metric_info_map=%p user=%u[>=%lu]", p_config, p_collect_info_vec, p_metric_info_map, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    m_p_config = p_config;

    do
    {
        // 获得唯一标识本机的标签
        GET_CONFIG("node_info", "server_tag", m_server_tag, break);

        // 获得唯一标识本机的ip地址
        if (inet_pton(AF_INET, p_inside_ip, &m_host_ip) <= 0)
        {
            ERROR_LOG("ERROR: ip:%s is invalid.", p_inside_ip);
            break;
        }

        char send_num[10] = {0};
        // 获得要发送数据的机器的个数
        GET_CONFIG("node_info", "send_addr_num", send_num, break);
        int num = atoi(send_num);
        if (num <= 0)
        {
            ERROR_LOG("ERROR: send_addr_num:%d is invalid.", num);
            break;
        }

        bool same_ip = false;
        for (int i = 0; i != num; ++i)
        {
            char tmp_name[50] = {0};
            snprintf(tmp_name, sizeof(tmp_name) - 1, "send_ip%d", i + 1);
            // 获取发送单播的ip地址
            char send_ip[16] = {0};
            GET_CONFIG("send_addr", tmp_name, send_ip, break);
            // 获取发送单播的端口
            snprintf(tmp_name, sizeof(tmp_name) - 1, "send_port%d", i + 1);
            int udp_port = 0;
            char port[10] = {0};
            GET_CONFIG("send_addr", tmp_name, port, break);
            udp_port = atoi(port);
            if (udp_port <= 0 || udp_port > 65535)
            {
                ERROR_LOG("ERROR: multicast port:%d", udp_port);
                break;
            }
            send_addr_t send_addr = {{0}};
            strcpy(send_addr.ip, send_ip);
            send_addr.port = udp_port;
            m_send_addr_vec.push_back(send_addr);

            if (!strcmp(send_ip, p_inside_ip))
            {
                same_ip = true;
            }
        }
        // 如果是接收udp的主机，则本机ip要在发送的ip里面
        if (is_recv_udp && !same_ip)
        {
            ERROR_LOG("set is_recv_udp, but send ip has no local host ip.");
            break;
        }

        if ((int)m_send_addr_vec.size() != num)
        {
            ERROR_LOG("send_addr_vec.size:%u not equal to addr num:%d.", (unsigned)m_send_addr_vec.size(), num);
            break;
        }
        // 根据配置设置所有的采集时间间隔及阈值
        if (set_collect_group_info(p_collect_info_vec, user) != 0)
        {
            ERROR_LOG("ERROR: set_collect_interval().");
            break;
        }
        m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_sockfd < 0)
        {
            ERROR_LOG("Create socket failed: %s", strerror(errno));
            break;
        }
        if (create_processor_thread_instance(&m_p_processor_thread_rrd) != 0)
        {
            ERROR_LOG("ERROR: create processor thread instance.");
            break;
        }
        if (m_p_processor_thread_rrd->init(p_config, &m_collect_group_rrd_vec,/* p_metric_info_map,*/ &m_send_addr_vec, start_time, m_server_tag, m_host_ip, m_sockfd) != 0)
        {
            ERROR_LOG("ERROR: processor thread rrd init failed.");
            m_p_processor_thread_rrd->release();
            break;
        }
        if (create_processor_thread_instance(&m_p_processor_thread_alarm) != 0)
        {
            ERROR_LOG("ERROR: create processor thread instance.");
            m_p_processor_thread_rrd->uninit();
            m_p_processor_thread_rrd->release();
            break;
        }
        if (m_p_processor_thread_alarm->init(p_config, &m_collect_group_alarm_vec,/* p_metric_info_map, */&m_send_addr_vec, start_time, m_server_tag, m_host_ip, m_sockfd) != 0)
        {
            ERROR_LOG("ERROR: processor thread alarm init failed.");
            m_p_processor_thread_alarm->release();
            m_p_processor_thread_rrd->uninit();
            m_p_processor_thread_rrd->release();
            break;
        }

        m_inited = true;
    } while (0);

    if (!m_inited)
    {
        m_p_config = NULL;
        if (m_sockfd)
        {
            close(m_sockfd);
            m_sockfd = 0;
        }
        m_p_processor_thread_rrd = NULL;
        m_p_processor_thread_alarm = NULL;
        m_metric_collect_list.clear();
        m_send_addr_vec.clear();
        m_collect_group_rrd_vec.clear();
        m_collect_group_alarm_vec.clear();
        memset(m_server_tag, 0, sizeof(m_server_tag));

        return -1;
    }
    DEBUG_LOG("collect thread init successfully.");
    //如果上次更新失败，则发送一条更新失败的信息
    //XXX 如果发送的机器是本机，那么则接收不到
    if (update_failed)
    {
        send_special_metric_info(OA_UP_FAIL_TYPE);
    }

    return 0;
}

/**
 * @brief  反初始化
 * @param  无
 * @return 0:success, -1:failed
 */
int c_collect_thread::uninit()
{
    if (!m_inited)
    {
        return -1;
    }

    assert(m_p_processor_thread_rrd != NULL);
    m_p_processor_thread_rrd->uninit();
    m_p_processor_thread_rrd->release();

    assert(m_p_processor_thread_alarm != NULL);
    m_p_processor_thread_alarm->uninit();
    m_p_processor_thread_alarm->release();

    m_metric_collect_list.clear();
    m_send_addr_vec.clear();
    m_collect_group_rrd_vec.clear();
    m_collect_group_alarm_vec.clear();

    m_p_config = NULL;
    memset(m_server_tag, 0, sizeof(m_server_tag));

    close(m_sockfd);
    m_sockfd = 0;
    m_inited = false;
    DEBUG_LOG("collect thread uninit successfully");
    return 0;
}

/**
 * @brief  根据配置文件，设置所有metric的采集时间
 * @param  无
 * @return 0:success, -1:failed
 */
int c_collect_thread::set_collect_group_info(const collect_info_vec_t *p_collect_info_vec, uint32_t user)
{
    if(user >= sizeof(g_user_list)/sizeof(g_user_list[0]) || p_collect_info_vec == NULL)
    {
        ERROR_LOG("p_collect_info_vec=%p user=%u[>=%lu]", p_collect_info_vec, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    char tmp[OA_MAX_STR_LEN] = {0};
    vector<group_config_t> group_config_vec;
    vector<metric_config_t> metric_config_vec;

    GET_CONFIG("node_info", "heartbeat_interval", tmp, return -1);
    int heartbeat_time = atoi(tmp);
    if (heartbeat_time <= 0)
    {
        ERROR_LOG("ERROR: heartbeat interval:%d", heartbeat_time);
        return -1;
    }
    // 获得配置里collect group的数量
    if(user == OA_USER_ROOT)
    {
        //GET_CONFIG("node_info", "root_collect_group_num", tmp, return -1);
        GET_CONFIG("node_info", "collect_group_num", tmp, return -1);
    } else
    {
        GET_CONFIG("node_info", "collect_group_num", tmp, return -1);
    }
    int group_num = atoi(tmp);
    if (group_num < 0)
    {
        ERROR_LOG("ERROR: group_num:%d.", group_num);
        return -1;
    }
    // 读取每个组的配置，并保存在group_config_vec,metric_config_vec这2个vector里面
    for (int i = 0; i != group_num; ++i)
    {
        group_config_t group_config = {0};
        char group_name[32] = {0};
        // group的名字按group1,group2...groupn命名
        if(user == OA_USER_ROOT)
        {
            //snprintf(group_name, sizeof(group_name) - 1, "root_group%d", i + 1);
            snprintf(group_name, sizeof(group_name) - 1, "group%d", i + 1);
        } else
        {
            snprintf(group_name, sizeof(group_name) - 1, "group%d", i + 1);
        }
        // 设置每组的采集间隔
        GET_CONFIG(group_name, "collect_interval", tmp, return -1);
        int interval = atoi(tmp);
        if (interval < 0)
        {
            ERROR_LOG("ERROR: %s interval:%d.", group_name, interval);
            return -1;
        } else if (0 == interval || interval > 60 * 60 * 24)
        {
            interval = 60 * 60 * 24;                // 对于只采集一次的，就把interval设为1天
        } else if(interval < 20)
        {
            interval = 20;
        }
        group_config.collect_interval = interval;
        group_config.time_threshold = group_config.collect_interval;
        // 设置每组的time_threshold
        /*
           GET_CONFIG(group_name, "time_threshold", tmp, return -1);
           group_config.time_threshold = atoi(tmp);
           if (group_config.time_threshold < 0)
           {
           ERROR_LOG("ERROR: %s time_threshold:%d.", group_name, group_config.time_threshold);
           return -1;
           }
           if (!group_config.time_threshold)
           {
           group_config.time_threshold = group_config.collect_interval;
           }
           */
        GET_CONFIG(group_name, "metric_num", tmp, return -1);
        group_config.metric_num = atoi(tmp);
        if (group_config.metric_num < 0 || group_config.metric_num > OA_MAX_GROUP_NUM)
        {
            ERROR_LOG("ERROR: %s metric num:%d, shuold [1,%d]", group_name, group_config.metric_num, OA_MAX_GROUP_NUM);
            return -1;
        }

        group_config_vec.push_back(group_config);
        // 读取group里每一个meric的配置
        for (int j = 0; j != group_config.metric_num; ++j)
        {
            metric_config_t metric_config = {0};

            char metric_name[OA_MAX_STR_LEN] = {0};
            snprintf(metric_name, sizeof(metric_name) - 1, "name%d", j + 1);
            char value_threshold[OA_MAX_STR_LEN] = {0};
            snprintf(value_threshold, sizeof(value_threshold) - 1, "value_threshold%d", j + 1);
            char type[OA_MAX_STR_LEN] = {0};
            snprintf(type, sizeof(type) - 1, "type%d", j + 1);
            char arg[OA_MAX_STR_LEN] = {0};
            snprintf(arg, sizeof(arg) - 1, "arg%d", j + 1);

            GET_CONFIG(group_name, metric_name, tmp, return -1);
            strcpy(metric_config.metric_name, tmp);
            DEBUG_LOG("metric name from conf:%s", tmp);

            GET_CONFIG(group_name, value_threshold, tmp, return -1);
            metric_config.value_threshold = static_cast<float>(atof(tmp));

            GET_CONFIG(group_name, type, tmp, return -1);
            metric_config.metric_type = atoi(tmp);
            if (metric_config.metric_type < OA_LIMIT_MIN || metric_config.metric_type > OA_LIMIT_MAX)
            {
                ERROR_LOG("metric_type:%d is invalid, should in [%d, %d].",
                        metric_config.metric_type, OA_LIMIT_MIN, OA_LIMIT_MAX);
                return -1;
            }

            GET_CONFIG(group_name, arg, tmp, ;);
            strcpy(metric_config.arg, tmp);

            metric_config_vec.push_back(metric_config);
        }
    }

    if (set_interval_by_config_value(p_collect_info_vec, group_config_vec, metric_config_vec, heartbeat_time) != 0)
    {
        ERROR_LOG("ERROR: set interval by config value.");
        return -1;
    }

    return 0;
}

int c_collect_thread::set_interval_by_config_value (
        const collect_info_vec_t *p_collect_info_vec,
        const vector<group_config_t> &group_config_vec,
        const vector<metric_config_t> &metric_config_vec,
        int heartbeat_time)
{
    if(p_collect_info_vec == NULL)
    {
        ERROR_LOG("p_collect_info_vec=%p", p_collect_info_vec);
        return -1;
    }
    // 从metric的名字到metric详细信息的映射
    map<string, const collect_info_t *> collect_name_map;

    vector<collect_info_t>::const_iterator collect_iter = p_collect_info_vec->begin();
    for (; collect_iter != p_collect_info_vec->end(); ++collect_iter)
    {
        if (!collect_iter->metric_info.name)
        {
            ERROR_LOG("metric name is NULL");
            return -1;
        }
        if (collect_name_map.find(collect_iter->metric_info.name) != collect_name_map.end())
        {
            ERROR_LOG("metric name:%s is already exitsts.", collect_iter->metric_info.name);
            continue;///////////////////////
        }

        collect_name_map.insert(pair<string, const collect_info_t *>(collect_iter->metric_info.name, &(*collect_iter)));
    }

    for (map<string, const collect_info_t *>::iterator iter = collect_name_map.begin(); iter != collect_name_map.end(); ++iter)
    {
        DEBUG_LOG("metric name: %s", iter->first.c_str());
    }
    // 设置心跳的搜集信息
    collect_group_t heartbeat_group = {0};
    heartbeat_group.collect_interval = heartbeat_time;
    heartbeat_group.time_threshold = heartbeat_time;
    heartbeat_group.num = 1;

    map<string, const collect_info_t *>::const_iterator sc_iter = collect_name_map.find("heartbeat");
    if (sc_iter == collect_name_map.end())
    {
        ERROR_LOG("ERROR: cann't find heartbeat in collect_name_map.");
        return -1;
    }
    metric_collect_t heartbeat_collect = {0};
    heartbeat_collect.p_collect_info = sc_iter->second;
    m_metric_collect_list.push_back(heartbeat_collect);

    heartbeat_group.p_metric_collect_arr[0] = &m_metric_collect_list.back();
    m_collect_group_rrd_vec.push_back(heartbeat_group);
    // 设置每个group的搜集信息
    int num = -1;
    vector<group_config_t>::const_iterator group_iter = group_config_vec.begin();
    for (; group_iter != group_config_vec.end(); ++group_iter)
    {
        collect_group_t collect_group = {0};

        collect_group.collect_interval = group_iter->collect_interval;
        collect_group.time_threshold = group_iter->time_threshold;
        collect_group.num = group_iter->metric_num;

        //int group_type = GROUP_RRD;
        for (int i = 0; i != collect_group.num; ++i)
        {
            metric_config_t metric_config = metric_config_vec[++num];

            sc_iter = collect_name_map.find(metric_config.metric_name);
            if (sc_iter == collect_name_map.end())
            {
                ERROR_LOG("ERROR: cann't find metric:%s in collect_name_map.", metric_config.metric_name);
                return -1;
            }

            metric_collect_t metric_collect = {0};
            metric_collect.metric_type = metric_config.metric_type;
            metric_collect.p_collect_info = sc_iter->second;
            metric_collect.value_threshold = metric_config.value_threshold;

            strcpy(metric_collect.arg, metric_config.arg);
            strcpy(metric_collect.formula, metric_config.formula);

            m_metric_collect_list.push_back(metric_collect);
            collect_group.p_metric_collect_arr[i] = &m_metric_collect_list.back();

            // 如果只报警的metric，在另一个线程里面采集，不影响写rrd的数据的采集
            //if (OA_ALARM == metric_collect.metric_type)
            // {
                //    group_type = GROUP_ALARM;
                //}
        }

        m_collect_group_rrd_vec.push_back(collect_group);
        //if (GROUP_RRD == group_type)
        // {
            //    m_collect_group_rrd_vec.push_back(collect_group);
            //} else
        // {
            //    m_collect_group_alarm_vec.push_back(collect_group);
            //}
    }

    return 0;
}

int c_collect_thread::send_special_metric_info(int metric_id)
{
    DEBUG_LOG("send special metric_id: %d.", metric_id);
    char buf[OA_MAX_BUF_LEN] = {0};
    unsigned short msg_len = 0;
    XDR x;

    xdrmem_create(&x, buf, sizeof(buf), XDR_ENCODE);
    // XDR头部是消息长度,数据全部打包之后再更改msg_len的值
    if (!xdr_u_short(&x, &msg_len))
    {
        ERROR_LOG("ERROR: xdr_short(msg_len).");
        return -1;
    }
    // 打包用二进制表示的机器ip，用来唯一标识一个机器
    char *p_tag = m_server_tag;
    if (!xdr_string(&x, const_cast<char **>(&p_tag), OA_MAX_STR_LEN))
    {
        ERROR_LOG("ERROR: xdr_string(server_tag).");
        return -1;
    }

    if (!xdr_u_int(&x, &m_host_ip))
    {
        ERROR_LOG("ERROR: xdr_u_int(host_ip).");
        return -1;
    }

    int collect_interval = 0;
    if (!xdr_int(&x, &collect_interval))
    {
        ERROR_LOG("ERROR: xdr_int(collect_interval).");
        return -1;
    }
    // metric_id用来标识是哪一种类型的metric
    if (!xdr_int(&x, &metric_id))
    {
        ERROR_LOG("ERROR: xdr_int(metric_id).");
        return -1;
    }
    // 更新实际数据的长度
    msg_len = xdr_getpos(&x);
    xdr_setpos(&x, 0);
    if (!xdr_u_short(&x, &msg_len))
    {
        ERROR_LOG("ERROR: xdr_int(msg_len).");
        return -1;
    }

    vector<send_addr_t>::const_iterator iter = m_send_addr_vec.begin();
    DEBUG_LOG("enter send update failed info, msg_len: %u", (unsigned)m_send_addr_vec.size());
    for (; iter != m_send_addr_vec.end(); ++iter)
    {
        publish_data(m_sockfd, buf, msg_len, iter->ip, iter->port);
        DEBUG_LOG("send update failed info.");
    }

    xdr_destroy(&x);
    return 0;
}

#undef GET_CONFIG
