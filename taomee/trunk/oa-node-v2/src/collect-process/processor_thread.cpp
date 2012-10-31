/**
 * =====================================================================================
 *       @file  processor_thread.cpp
 *      @brief  定时采集数据，并将到时的数据通过组播发送出去
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
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:36:35
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <new>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <openssl/md5.h>

#include "../proto.h"
#include "../defines.h"
#include "../lib/utils.h"
#include "../lib/log.h"
#include "./processor_thread.h"

using namespace std;

int create_processor_thread_instance(c_processor_thread **pp_instance)
{
    if (pp_instance == NULL)
    {
        ERROR_LOG("pp_instance=%p", pp_instance);
        return -1;
    }

    c_processor_thread *p_instance = new (std::nothrow)c_processor_thread();
    //ERROR_LOG("c_processor_thread *p_instance = new (std::nothrow)c_processor_thread(); %p", p_instance);
    if (p_instance == NULL)
    {
        return -1;
    } else
    {
        *pp_instance = dynamic_cast<c_processor_thread *>(p_instance);
        return 0;
    }
}

c_processor_thread::c_processor_thread(): m_inited(false), m_work_thread_id(0), m_continue_working(false), m_p_server_tag(NULL), m_host_ip(0), m_wakeup_time(0), m_start_time(0), m_sockfd(-1), m_p_config(NULL), m_p_collect_group_vec(NULL)//, m_p_metric_info_map(NULL)
{
}

c_processor_thread::~c_processor_thread()
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
int c_processor_thread::init(i_config *p_config,
        const collect_group_vec_t *p_collect_group_vec,
        const send_addr_vec_t *p_send_addr_vec,
        time_t start_time,
        const char *p_server_tag,
        int host_ip,
        int sock_fd)
{
    if (m_inited)
    {
        ERROR_LOG("ERROR: collect thread has been inited.");
        return -1;
    }

    if (NULL == p_config || NULL == p_collect_group_vec || NULL == p_server_tag/*|| NULL == p_metric_info_map*/)
    {
        ERROR_LOG("p_config=%p p_collect_group_vec=%p p_server_tag=%p", p_config, p_collect_group_vec, p_server_tag);
        return -1;
    }

    m_p_collect_group_vec = p_collect_group_vec;
    if (!m_p_collect_group_vec->size())
    {
        DEBUG_LOG("collect group size is 0.");
        m_inited = true;
        return 0;
    }

    m_sockfd = sock_fd;

    m_p_config = p_config;
    m_p_send_addr_vec = p_send_addr_vec;
    m_start_time = start_time;
    m_p_server_tag = p_server_tag;
    m_host_ip = host_ip;
    // 根据collect group的个数初始化每个collect group对应的参数
    for (collect_group_vec_t::const_iterator citer = m_p_collect_group_vec->begin(); citer != m_p_collect_group_vec->end(); ++citer)
    {
        group_arg_t group_arg = {0};
        m_group_arg_vec.push_back(group_arg);
    }

    m_continue_working = true;
    int result = pthread_create(&m_work_thread_id, NULL, work_thread_proc, this);
    if (result != 0)
    {
        ERROR_LOG("ERROR: pthread_create() failed.");
        m_continue_working = false;
        m_work_thread_id = 0;
        m_p_config = NULL;
        m_p_collect_group_vec = NULL;
        m_p_send_addr_vec = NULL;
        m_start_time = 0;
        m_p_server_tag = NULL;
        m_sockfd = -1;
        m_group_arg_vec.clear();

        return -1;
    }

    m_inited = true;
    DEBUG_LOG("processerer thread init successfully.");

    return 0;
}

/**
 * @brief  反初始化
 * @param  无
 * @return 0:success, -1:failed
 */
int c_processor_thread::uninit()
{
    if (!m_inited)
    {
        return -1;
    }

    if (!m_p_collect_group_vec->size())
    {
        m_p_collect_group_vec = NULL;
        m_inited = false;
        return 0;
    }

    assert(m_work_thread_id != 0);
    m_continue_working = false;
    pthread_join(m_work_thread_id, NULL);

    m_p_server_tag = 0;
    m_wakeup_time = 0;
    m_work_thread_id = 0;
    m_start_time = 0;
    m_sockfd = -1;

    m_group_arg_vec.clear();

    m_p_config = NULL;
    m_p_collect_group_vec = NULL;
    m_p_send_addr_vec = NULL;

    m_inited = false;

    DEBUG_LOG("collect thread uninit successfully");
    return 0;
}

/**
 * @brief  释放动态分配的内存
 * @param  无
 * @return 0:success, -1:failed
 */
int c_processor_thread::release()
{
    delete this;

    return 0;
}

/**
 * @brief  子线程的线程函数
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success, (void *)-1:failed
 */
void *c_processor_thread::work_thread_proc(void *p_data)
{
    c_processor_thread *p_instance = (c_processor_thread *)p_data;
    assert(p_instance != NULL);

    DEBUG_LOG("processor thread[%d] enter main loop.", getpid());

    while (p_instance->m_continue_working)
    {
        p_instance->m_wakeup_time = 0;
        // 收集时间到达了的每一组数据
        p_instance->collection_group_collect();
        if (!p_instance->m_continue_working)
        {
            break;
        }
        // 将到达发送时间的数据打包成xdr通过组播发送出去
        p_instance->collection_group_send();
        if (!p_instance->m_continue_working)
        {
            break;
        }
        // 保证wait是一个正数
        time_t now = time(NULL);
        int wait = p_instance->m_wakeup_time > now ? p_instance->m_wakeup_time - now : 1;
        DEBUG_LOG("thread:%u, wait:%d", (unsigned)pthread_self(), wait);
        for (int i = 0; i < wait; ++i)
        {
            if (!p_instance->m_continue_working)
            {
                break;
            }
            sleep(1);
        }
    }
    DEBUG_LOG("processor thread leave main loop.");

    return (void *)0;
}

/**
 * @brief  遍历所有的collection group，采集到达了采集时间的metric的数据
 * @param  无
 * @return 无
 */
void c_processor_thread::collection_group_collect()
{
    // 遍历每一个collect group，收集时间到达了的结点
    for (size_t i = 0; i < m_group_arg_vec.size(); ++i)
    {
        time_t now = time(NULL);
        if (m_group_arg_vec[i].next_collection <= now) {                 // 这一组达到了收集时间
            collect_group_data(i);
            // 更新这组的下次收集时间
            if (0 == m_group_arg_vec[i].next_collection)
            {
                m_group_arg_vec[i].next_collection = now + (*m_p_collect_group_vec)[i].collect_interval;
            } else
            {
                m_group_arg_vec[i].next_collection += (*m_p_collect_group_vec)[i].collect_interval;
            }
        }
        // 记录最快到达的下一次收集时间
        if (0 == m_wakeup_time)
        {
            m_wakeup_time = m_group_arg_vec[i].next_collection;
        } else if (m_group_arg_vec[i].next_collection < m_wakeup_time)
        {
            m_wakeup_time = m_group_arg_vec[i].next_collection;
        } else
        {
            // do nothing
        }
    }

    return;
}

/**
 * @brief  采集到达时间的一组metric
 * @param  index: 需要搜集的一组信息在vector里的索引
 * @return 无
 */
void c_processor_thread::collect_group_data(int index)
{
    const collect_group_t &collect_group = (*m_p_collect_group_vec)[index];
    group_arg_t &group_arg = m_group_arg_vec[index];

    for (int i = 0; i < collect_group.num; ++i)
    {
        const metric_collect_t *p_metric_collect = collect_group.p_metric_collect_arr[i];
        assert(p_metric_collect != NULL);
        memcpy(&group_arg.metric_value[i].last_value, &group_arg.metric_value[i].now_value, sizeof(value_t));

        const collect_info_t *p_collect_info = p_metric_collect->p_collect_info;
        assert(p_collect_info != NULL);

        if (p_collect_info->proto_handler)
        {
            DEBUG_LOG("collect metric:%s, proto_handler(%d, \"%s\")",
                    p_collect_info->metric_info.name, p_collect_info->index, p_metric_collect->arg);
            group_arg.metric_value[i].now_value = p_collect_info->proto_handler(p_collect_info->index, p_metric_collect->arg);
        } else
        {
            break;                              // so已经被unload了
        }

        if (OA_HEART_BEAT_TYPE == p_collect_info->metric_id || check_value_threshold(group_arg.metric_value[i],
                    p_metric_collect->value_threshold, p_metric_collect->p_collect_info->metric_info.type))
        {
            group_arg.next_send = 0;        // send immediately
        }
    } // end for i from zero to group_iter->num
    DEBUG_LOG("collect group data, next_send:%u", group_arg.next_send);
}

/**
 * @brief  判断新采集的数据是否超过了设定的阈值
 * @param  p_metric_collect: 指向每个metric详细信息的指针
 * @return true:值的改变超过了阈值, false:值的改变没有超过阈值
 */
bool c_processor_thread::check_value_threshold(const metric_value_t &value, float value_threshold, value_type_t type)
{
    bool gt_threshold = true;
    const value_t &now_value = value.now_value;
    const value_t &last_value = value.last_value;

    switch (type)
    {
        case OA_VALUE_UNSIGNED_SHORT:
            if (abs(now_value.value_ushort - last_value.value_ushort) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_SHORT:
            if (abs(now_value.value_short - last_value.value_short) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_UNSIGNED_INT:
            if (abs(static_cast<int>(now_value.value_uint - last_value.value_uint)) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_INT:
            if (abs(now_value.value_int - last_value.value_int) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_UNSIGNED_LONG_LONG:
            if (llabs(now_value.value_ull - last_value.value_ull) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_LONG_LONG:
            if (llabs(now_value.value_ll - last_value.value_ll) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_FLOAT:
            if (fabsf(now_value.value_f - last_value.value_f) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        case OA_VALUE_DOUBLE:
            if (fabs(now_value.value_d - last_value.value_d) < value_threshold)
            {
                gt_threshold = false;
            }
            break;
        default:
            break;
    }

    return gt_threshold;
}

/**
 * @brief  遍历链表，将到时的数据发送出去
 * @param  无
 * @return 无
 */
void c_processor_thread::collection_group_send()
{
    for (int i = 0; i != (int)m_group_arg_vec.size(); ++i)
    {
        time_t now = time(NULL);
        group_arg_t &group_arg = m_group_arg_vec[i];
        const collect_group_t &collect_group = (*m_p_collect_group_vec)[i];

        if (group_arg.next_send <= now) 
        {
            // 到达发送时间的group
            for (int j = 0; j != collect_group.num; ++j)
            {
                const metric_collect_t *p_metric_collect = collect_group.p_metric_collect_arr[j];
                assert(p_metric_collect != NULL);
                const collect_info_t *p_collect_info = p_metric_collect->p_collect_info;
                assert(p_collect_info != NULL);

                metric_send_t metric_send = {0};
                metric_send.server_tag = m_p_server_tag;
                metric_send.host_ip = m_host_ip;
                metric_send.metric_id = p_collect_info->metric_id;
                metric_send.collect_interval = collect_group.collect_interval;

                if (OA_HEART_BEAT_TYPE == metric_send.metric_id) 
                {
                    // 心跳信息
                    metric_send.heartbeat = group_arg.metric_value[j].now_value.value_uint;
                } 
                else
                {
                    // 自定义类型的信息
                    metric_t metric = {0};

                    metric.metric_type = p_metric_collect->metric_type;
                    if (metric.metric_type & OA_ALARM)
                    {
                        metric.alarm_formula = p_metric_collect->formula;
                    } else
                    {
                        metric.alarm_formula = NULL;
                    }

                    metric.tmax = p_collect_info->metric_info.tmax;
                    metric.dmax = p_collect_info->metric_info.dmax;
                    metric.name = p_collect_info->metric_info.name;
                    metric.units = p_collect_info->metric_info.units;
                    metric.fmt = p_collect_info->metric_info.fmt;
                    metric.desc = p_collect_info->metric_info.desc;
                    metric.slope = p_collect_info->metric_info.slope;
                    metric.sum_formula = p_collect_info->metric_info.sum_formula;
                    metric.arg = p_metric_collect->arg;
                    metric.type = p_collect_info->metric_info.type;
                    memcpy(&metric.value, &group_arg.metric_value[j].now_value, sizeof(value_t));
                    memcpy(&metric_send.metric, &metric, sizeof(metric));
                }

                if (send_metric_data(&metric_send) != 0)
                {
                    ERROR_LOG("ERROR: send_metric_data(), metric_id:%d", metric_send.metric_id);
                } else
                {
                    NOTI_LOG("send metric data:%d, time:%u", metric_send.metric_id, (unsigned)now);
                }

                assert(collect_group.time_threshold != 0);
                group_arg.next_send = now + collect_group.time_threshold;
                NOTI_LOG("metric:%d, next_send:%u", metric_send.metric_id, group_arg.next_send);
            } // end for j from 0 to collect_group->num
        } // end if group_arg.next_send <= now
        // 记录最快到达的下一次发送时间
        assert(m_wakeup_time != 0);
        if (group_arg.next_send < m_wakeup_time)
        {
            m_wakeup_time = group_arg.next_send;
        }
    } // end for i from 0 to m_group_arg_vec.size()

    return;
}

/**
 * @brief  将一个metric数据打包发送出去
 * @param  send_data: 需要发送的metric数据的引用
 * @return 0:success, -1:failed
 */
int c_processor_thread::send_metric_data(metric_send_t *p_send_data)
{
    if(p_send_data == NULL)
    {
        ERROR_LOG("p_send_data=%p", p_send_data);
        return -1;
    }
    XDR x;
    char msg_buf[OA_MAX_UDP_MESSAGE_LEN] = {0};
    unsigned short msg_len = 0;

    xdrmem_create(&x, msg_buf, sizeof(msg_buf), XDR_ENCODE);
    // 把数据打包到msg_buf里面
    if (xdr_pack_send_data(p_send_data, &x, &msg_len) != 0)
    {
        xdr_destroy(&x);
        return -1;
    }
    if (msg_len > OA_MAX_UDP_MESSAGE_LEN)
    {
        ERROR_LOG("ERROR: UDP message len:%d is too large.", msg_len);
        xdr_destroy(&x);
        return -1;
    }

    send_addr_vec_t::const_iterator iter = m_p_send_addr_vec->begin();
    for (; iter != m_p_send_addr_vec->end(); ++iter)
    {
        if (publish_data(m_sockfd, msg_buf, msg_len, iter->ip, iter->port) < 0)
        {
            ERROR_LOG("ERROR: publish_data().");
            xdr_destroy(&x);
            return -1;
        }
    }

    xdr_destroy(&x);
    return 0;
}

/**
 * @brief  将需要发送的一个metric的信息打包成相应的xdr格式
 * @param  send_data: 需要发送的数据
 * @param  p_x: XDR对象的指针
 * @param  p_msg_len: 需要发送的数据的长度
 * @return 0:success, -1:failed
 */
int c_processor_thread::xdr_pack_send_data(metric_send_t *p_send_data, XDR *p_x, unsigned short *p_msg_len)
{
    if(p_send_data == NULL || p_x == NULL || p_msg_len == NULL)
    {
        ERROR_LOG("p_send_data=%p p_x=%p p_msg_len=%p", p_send_data, p_x, p_msg_len);
        return -1;
    }
    if(p_send_data->host_ip == 0)
    {
        ERROR_LOG("host_ip = 0.0.0.0");
        return -1;
    }

    // XDR头部是消息长度,数据全部打包之后再更改msg_len的值
    if (!xdr_u_short(p_x, p_msg_len))
    {
        ERROR_LOG("ERROR: xdr_u_short(msg_len).");
        return -1;
    }
    // 打包用二进制表示的机器ip，用来唯一标识一个机器
    if (!xdr_string(p_x, (char **)&p_send_data->server_tag, OA_MAX_STR_LEN))
    {
        ERROR_LOG("ERROR: xdr_string(server_tag[%s]).", p_send_data->server_tag);
        return -1;
    }

    if (!xdr_u_int(p_x, &p_send_data->host_ip))
    {
        ERROR_LOG("ERROR: xdr_u_int(host_ip[%u]).", p_send_data->host_ip);
        return -1;
    }

    if (!xdr_int(p_x, &p_send_data->collect_interval))
    {
        ERROR_LOG("ERROR: xdr_int(collect_interval[%d]).", p_send_data->collect_interval);
        return -1;
    }
    // metric_id用来标识是哪一种类型的metric
    if (!xdr_int(p_x, &p_send_data->metric_id))
    {
        ERROR_LOG("ERROR: xdr_int(metric_id[%d]).", p_send_data->metric_id);
        return -1;
    }
    // 对三种类型的metric，分别用不同的格式打包数据
    // 现在不区分buildin和custom
    switch (p_send_data->metric_id)
    {
        case OA_HEART_BEAT_TYPE:
            if (!xdr_u_int(p_x, &p_send_data->heartbeat))
            {
                ERROR_LOG("ERROR: xdr_u_int(heartbeat[%u]).", p_send_data->heartbeat);
                return -1;
            }
            break;
        default:
            char metric_name[OA_MAX_STR_LEN] = {0};
            char metric_arg[OA_MAX_STR_LEN] = {0};
            char metric_val[OA_MAX_STR_LEN] = {0};        
            get_metric_value(&p_send_data->metric.value, p_send_data->metric.type,
                    p_send_data->metric.fmt, metric_val);
            DEBUG_LOG("metric name:[%s], arg:[%s], value:[%s], type:[%u]", p_send_data->metric.name,
                    p_send_data->metric.arg, metric_val, p_send_data->metric.type);
            if (!p_send_data->metric.arg || !strlen(p_send_data->metric.arg))
            {
                sprintf(metric_name, "%s", p_send_data->metric.name);
            } else
            {
                char dst_md5[33] = {0};
                unsigned char src_md5[16] = {0};
                MD5((unsigned char *)p_send_data->metric.arg, strlen(p_send_data->metric.arg), src_md5);
                char *p_md5 = dst_md5;
                for (int i = 0; i < 16; ++i)
                {
                    sprintf(p_md5, "%02x", src_md5[i]);
                    p_md5 += 2;
                }
                sprintf(metric_name, "%s_%s", p_send_data->metric.name, dst_md5);
                sprintf(metric_arg, "%s", p_send_data->metric.arg);
            }
            char *p_str = metric_name;
            if (!xdr_string(p_x, &p_str, sizeof(metric_name)))
            {
                ERROR_LOG("ERROR: xdr_string(metric_name[%s]).", metric_name);
                return -1;
            }
            p_str = metric_arg;
            if (!xdr_string(p_x, &p_str, sizeof(metric_arg)))
            {
                ERROR_LOG("ERROR: xdr_string(metric_arg[%s]).", metric_arg);
                return -1;
            }
            if (!xdr_int(p_x, &p_send_data->metric.dmax))
            {
                ERROR_LOG("ERROR: xdr_int(dmax[%d]).", p_send_data->metric.dmax);
                return -1;
            }
            if (!xdr_int(p_x, &p_send_data->metric.tmax))
            {
                ERROR_LOG("ERROR: xdr_int(tmax[%d]).", p_send_data->metric.tmax);
                return -1;
            }
            if (!xdr_string(p_x, const_cast<char **>(&p_send_data->metric.units), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string(units[%s]).", p_send_data->metric.units);
                return -1;
            }
            if (!xdr_string(p_x, const_cast<char **>(&p_send_data->metric.fmt), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string(fmt[%s]).", p_send_data->metric.fmt);
                return -1;
            }
            if (!xdr_string(p_x, const_cast<char **>(&p_send_data->metric.desc), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string(desc[%s]).", p_send_data->metric.desc);
                return -1;
            }
            if (!xdr_string(p_x, const_cast<char **>(&p_send_data->metric.slope), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string().");
                return -1;
            }
            if (!xdr_string(p_x, const_cast<char **>(&p_send_data->metric.sum_formula), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string(sum_formula[%s]).", p_send_data->metric.sum_formula);
                return -1;
            }
            if (!xdr_enum(p_x, (enum_t *)&p_send_data->metric.type))
            {
                ERROR_LOG("ERROR: xdr_enum(type[%d]).", p_send_data->metric.type);
                return -1;
            }
            if (-1 == xdr_pack_metric_value(&p_send_data->metric.value, p_send_data->metric.type, p_x))
            {
                return -1;
            }
            // metric_type用来标识是用来写rrd还是报警的metric
            if (!xdr_int(p_x, &p_send_data->metric.metric_type))
            {
                ERROR_LOG("ERROR: xdr_int(metric_type[%d]).", p_send_data->metric.metric_type);
                return -1;
            }
            if (p_send_data->metric.metric_type & OA_ALARM &&
                    !xdr_string(p_x, const_cast<char **>(&p_send_data->metric.alarm_formula), OA_MAX_STR_LEN))
            {
                ERROR_LOG("ERROR: xdr_string(alarm_formula[%s]).", p_send_data->metric.alarm_formula);
                return -1;
            }
            break;
    }
    // 更新实际数据的长度
    *p_msg_len = xdr_getpos(p_x);
    xdr_setpos(p_x, 0);
    if (!xdr_u_short(p_x, p_msg_len))
    {
        ERROR_LOG("ERROR: xdr_int(msg_len[%u]).", *p_msg_len);
        return -1;
    }

    return 0;
}

/**
 * @brief  将metric的值打包成相应的xdr格式
 * @param  value: metric的值的引用
 * @param  type: metric的值的类型
 * @param  p_x: XDR对象的指针
 * @return 0:success, -1:failed
 */
int c_processor_thread::xdr_pack_metric_value(value_t *p_value, value_type_t type, XDR *p_x)
{
    if(p_value == NULL || p_x == NULL)
    {
        ERROR_LOG("p_value=%p p_x=%p", p_value, p_x);
        return -1;
    }
    switch (type)
    {
        case OA_VALUE_STRING:
            {
                char *str = p_value->value_str;
                if (!xdr_string(p_x, &str, OA_MAX_STR_LEN))
                {
                    ERROR_LOG("ERROR: xdr_string().");
                    return -1;
                }
                break;
            }
        case OA_VALUE_UNSIGNED_SHORT:
            if (!xdr_u_short(p_x, &p_value->value_ushort))
            {
                ERROR_LOG("ERROR: xdr_u_short().");
                return -1;
            }
            break;
        case OA_VALUE_SHORT:
            if (!xdr_short(p_x, &p_value->value_short))
            {
                ERROR_LOG("ERROR: xdr_short().");
                return -1;
            }
            break;
        case OA_VALUE_UNSIGNED_INT:
            if (!xdr_u_int(p_x, &p_value->value_uint))
            {
                ERROR_LOG("ERROR: xdr_uint().");
                return -1;
            }
            break;
        case OA_VALUE_INT:
            if (!xdr_int(p_x, &p_value->value_int))
            {
                ERROR_LOG("ERROR: xdr_int().");
                return -1;
            }
            break;
        case OA_VALUE_UNSIGNED_LONG_LONG:
            if (!xdr_u_longlong_t(p_x, (u_quad_t*)&p_value->value_ull))
            {
                ERROR_LOG("ERROR: xdr_u_longlong_t() failed.");
                return -1;
            }
            break;
        case OA_VALUE_LONG_LONG:
            if (!xdr_longlong_t(p_x, (quad_t*)&p_value->value_ll))
            {
                ERROR_LOG("ERROR: xdr_longlong_t() failed.");
                return -1;
            }
            break;
        case OA_VALUE_FLOAT:
            if (!xdr_float(p_x, &p_value->value_f))
            {
                ERROR_LOG("ERROR: xdr_float().");
                return -1;
            }
            break;
        case OA_VALUE_DOUBLE:
            if (!xdr_double(p_x, &p_value->value_d))
            {
                ERROR_LOG("ERROR: xdr_double().");
                return -1;
            }
            break;
        default:
            ERROR_LOG("ERROR: cann't handler the type: %d", type);
            return -1;
    }
    return 0;
}

/**
 * @brief  记录用户自定义的发送信息
 * @param  p_value: metric的值
 * @param  type: metric的值的类型
 * @param  p_str: metric值的字符串形式
 * @return 0:success, -1:failed
 */
int c_processor_thread::get_metric_value(const value_t *p_value, value_type_t type, const char *fmt, char *p_str)
{
    if(p_value == NULL || fmt == NULL || p_str == NULL)
    {
        ERROR_LOG("p_value=%p fmt=%p p_str=%p", p_value, fmt, p_str);
        return -1;
    }
    switch (type)
    {
        case OA_VALUE_STRING:
            sprintf(p_str, fmt, p_value->value_str);
            break;
        case OA_VALUE_UNSIGNED_SHORT:
            sprintf(p_str, fmt, p_value->value_ushort);
            break;
        case OA_VALUE_SHORT:
            sprintf(p_str, fmt, p_value->value_short);
            break;
        case OA_VALUE_UNSIGNED_INT:
            sprintf(p_str, fmt, p_value->value_uint);
            break;
        case OA_VALUE_INT:
            sprintf(p_str, fmt, p_value->value_int);
            break;
        case OA_VALUE_UNSIGNED_LONG_LONG:
            sprintf(p_str, fmt, p_value->value_ull);
            break;
        case OA_VALUE_LONG_LONG:
            sprintf(p_str, fmt, p_value->value_ll);
            break;
        case OA_VALUE_FLOAT:
            sprintf(p_str, fmt, p_value->value_f);
            break;
        case OA_VALUE_DOUBLE:
            sprintf(p_str, fmt, p_value->value_d);
            break;
        default:
            ERROR_LOG("ERROR: cann't handler the type: %d", type);
            return -1;
    }

    return 0;
}
