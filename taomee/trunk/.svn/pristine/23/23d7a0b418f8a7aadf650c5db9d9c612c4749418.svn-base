/**
 * =====================================================================================
 *       @file  alarm_thread.cpp
 *      @brief  
 *
 *  handle the alarm 
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "../lib/log.h"
#include "./alarm_thread.h"

using namespace std;

/** 
 * @brief  构造函数
 * @param   
 * @return   
 */
c_alarm_thread::c_alarm_thread() :
    m_inited(false), 
    m_pid(0),
    m_p_queue_map(NULL),
    m_stop(false), 
    m_alarm_interval(0)
{
    m_alarm_counter.clear();
    clean_notice_map();
}

/** 
 * @brief  析构函数
 * @param   
 * @return   
 */
c_alarm_thread::~c_alarm_thread()
{
    uninit();
}


/** 
 * @brief  反初始化
 * @param   
 * @return   
 */
int c_alarm_thread::uninit()
{
    if(!m_inited) {
        return -1;
    }

    assert(m_pid !=  0);
    m_stop = true;
    pthread_join(m_pid, NULL);

    if(!m_alarm_counter.empty()) {
        m_alarm_counter.clear();
    }

    clean_notice_map();

    m_stop = false;
    m_pid  = 0;
    m_alarm_interval = 0;
    m_p_queue_map = NULL;
    m_inited = false;

    return 0;
}

/** 
 * @brief   初始化函数,要么init成功，要么失败会uninit已经init成功的变量
 * @param   p_config config对象指针
 * @param   p_queue_map  ds_queue_map_t对象指针
 * @return  0 = success -1 = failed
 */
int c_alarm_thread::init(
        const collect_conf_t *p_config,
        ds_queue_map_t *p_queue_map,
        const metric_alarm_vec_t *p_default_alarm_span,
        const metric_alarm_map_t *p_special_alarm_span)
{
    if(m_inited) {
        ERROR_LOG("ERROR: c_alarm_thread has been inited.");
        return -1;
    }

    if(!p_config || !p_queue_map || !p_default_alarm_span || !p_special_alarm_span) {
        ERROR_LOG("ERROR: Paraments cannot be NULL.");
        return -1;
    }

    m_p_queue_map = p_queue_map;
    m_p_default_alarm_span = p_default_alarm_span;
    m_p_special_alarm_span = p_special_alarm_span;
    m_alarm_interval = p_config->alarm_interval;

    if(m_alarm_interval <= 0 || m_alarm_interval > 200) {
        DEBUG_LOG("alarm_interval config error,use default 10");
        m_alarm_interval = 10;
    }

    strncpy(m_alarm_server_url, p_config->alarm_server_url, sizeof(m_alarm_server_url) - 1);
    m_alarm_server_url[sizeof(m_alarm_server_url) - 1] = '\0';

    if(0 != pthread_create(&m_pid, NULL, alarm_main, this)) {
        m_stop = false;
        m_pid  = 0;
        m_p_queue_map = NULL;
        m_p_default_alarm_span = NULL;
        m_p_special_alarm_span = NULL;
        m_alarm_interval = 0;
        m_alarm_counter.clear();
        return -1;
    }

    DEBUG_LOG("Init Info: alarm_thread init succ.");
    m_inited = true;
    return 0;
}

/** 
 * @brief   获取alarm_cmd中key对应的value
 * @param   p_cmd: 待查找的字符串
 * @param   p_key: 键值
 * @return  NULL-failed, 否则-success
 */
char * c_alarm_thread::get_key_value(const char *p_cmd, const char *p_key)
{
    if (NULL == p_key || NULL == p_cmd) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return NULL;
    }

    static char value[MAX_STR_LEN] = {0};
    const char * p_start = strstr(p_cmd, p_key);
    if (NULL == p_start) {
        ERROR_LOG("Cannot find [%s] in [%s]", p_key, p_cmd);
        return NULL;
    }
    p_start = index(p_start, '=');
    if (NULL == p_start) {
        return NULL;
    }
    ++p_start;

    uint32_t len = 0;
    char * p_val = value;
    while (*p_start != '&' && *p_start && len + 1< sizeof(value)){
        *p_val++ = *p_start++;
        ++len;
    }
    *p_val = 0;
    DEBUG_LOG("KEY INFO: key[%s]-->value[%s] in cmd[%s]", p_key, value, p_cmd);

    return value;
}

/** 
 * @brief   处理告警命令
 * @param   p_data: 接收到的事件
 * @return  0-success, -1-failed
 */
int c_alarm_thread::handle_alarm_cmd(const char *p_data)
{
    if (NULL == p_data) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }

    alarm_event_t *p_event = (alarm_event_t *)p_data;
    if (strlen(p_event->host_ip) < 2 || index(p_event->host_ip, '%')) {
        ERROR_LOG("ALARM INFO: invalid host_ip[%s] in cmd[%s]", p_event->host_ip, p_event->cmd_data);
        return -1;
    }

    alarm_notice_t notice_info = {0};
    notice_info.cmd_id = p_event->cmd_id;
    strncpy(notice_info.alarm_key, p_event->key, sizeof(notice_info.alarm_key) -1);
    strncpy(notice_info.alarm_cmd, p_event->cmd_data, sizeof(notice_info.alarm_cmd) -1);
    switch (p_event->cmd_id) {
        case OA_HOST_ALARM:
            {
                ///清理该机器所有其他告警
                notice_map_t::iterator it_map = m_notice_map.find(p_event->host_ip);
                if (it_map == m_notice_map.end()) {
                    DEBUG_LOG("ALARM INFO: Receive a new cmd[%s]", p_event->cmd_data);
                    notice_vec_t *p_notice_vec = new notice_vec_t;
                    if (NULL == p_notice_vec) {
                        ERROR_LOG("ERROR: new notice_vec_t failed!");
                        return -1;
                    }
                    p_notice_vec->push_back(notice_info);
                    m_notice_map.insert(pair<string, notice_vec_t*>(p_event->host_ip, p_notice_vec));
                } else {
                    //TODO: 增加获取OA_HOST_ALARM的值，并填写到notice_info中
                    notice_vec_t *p_vec = it_map->second;
                    p_vec->clear();
                    p_vec->push_back(notice_info);//不存在则增加
                }
                break;
            }
        case OA_HOST_METRIC:
        case OA_UPDATE_FAIL:
        case OA_DS_DOWN:
            {
                notice_map_t::iterator it_map = m_notice_map.find(p_event->host_ip);
                if (it_map == m_notice_map.end()) {
                    DEBUG_LOG("ALARM INFO: Receive a new cmd[%s]", p_event->cmd_data);
                    notice_vec_t *p_notice_vec = new notice_vec_t;
                    if (NULL == p_notice_vec) {
                        ERROR_LOG("ERROR: new notice_vec_t failed!");
                        return -1;
                    }
                    p_notice_vec->push_back(notice_info);
                    m_notice_map.insert(pair<string, notice_vec_t*>(p_event->host_ip, p_notice_vec));
                } else {
                    notice_vec_t *p_vec = it_map->second;
                    notice_vec_t::iterator it_vec = p_vec->begin();
                    while (it_vec != p_vec->end()) {
                        if (it_vec->cmd_id == p_event->cmd_id && !strcmp(it_vec->alarm_key, p_event->key)) {
                            DEBUG_LOG("ALARM INFO: Storage has the same cmd[%s]", p_event->cmd_data);
                            if (OA_HOST_METRIC == p_event->cmd_id) {//OA_HOST_METRIC则盖以前的告警消息
                                strcpy(it_vec->alarm_cmd, p_event->cmd_data);
                            }
                            return 0;
                        }
                        ++it_vec;
                    }
                    p_vec->push_back(notice_info);//不存在则增加
                }
                break;
            }
        case OA_HOST_METRIC_RECOVERY:
        case OA_HOST_METRIC_CLEANED:
        case OA_HOST_RECOVERY:
        case OA_UPDATE_RECOVERY:
        case OA_DS_RECOVERY:
            {
                int reverse_status = get_reverse_status(p_event->cmd_id);
                ///清理之前的reverse_status告警
                notice_map_t::iterator it_map = m_notice_map.find(p_event->host_ip);
                if (it_map != m_notice_map.end()) {
                    notice_vec_t *p_vec = it_map->second;
                    notice_vec_t::iterator it_vec = p_vec->begin();
                    while (it_vec != p_vec->end()) {
                        if (reverse_status == it_vec->cmd_id && !strcmp(it_vec->alarm_key, p_event->key)) {
                            int alarm_cnt = it_vec->alarm_cnt;
                            p_vec->erase(it_vec);//删除之前的reverse_status告警信息
                            if (alarm_cnt > 0) {//发过告警信息,则增加recovery通知
                                p_vec->push_back(notice_info);
                            }
                            return 0;
                        }
                        ++it_vec;
                    }
                }
                ERROR_LOG("ALARM INFO: recv recovery[%d] but no reverse status[%d] in storage,recv cmd[%s]",
                        p_event->cmd_id, reverse_status, p_event->cmd_data);
                return -1;
            }
        default:
            ERROR_LOG("ERROR: Wrong cmd_id[%d] in cmd[%s].", p_event->cmd_id, p_event->cmd_data);
            return -1;
    }

    return 0;
}

/** 
 * @brief   获取下一次发送时间跨度
 * @param   alarm_cnt: 告警次数
 * @param   p_cmd: 告警命令
 * @return  非负数-时间跨度, -1-failed
 */
int c_alarm_thread::get_send_span(int alarm_cnt, const char *p_cmd)
{
    char host_ip[16] = {0};
    char metric_name[128] = {0};
    //char metric_arg[128] = {0};
    char *p_val = get_key_value(p_cmd, "host");
    if (NULL == p_val) {
        ERROR_LOG("ALARM INFO: cannot get host from cmd[%s]", p_cmd);
        return -1;
    }
    strncpy(host_ip, p_val, sizeof(host_ip) - 1);

    p_val = get_key_value(p_cmd, "metric");
    if (NULL == p_val) {
        ERROR_LOG("ALARM INFO: cannot get metric from cmd[%s]", p_cmd);
        return -1;
    }
    strncpy(metric_name, p_val, sizeof(metric_name) - 1);

    //p_val = get_key_value(p_cmd, "arg");
    //if (NULL == p_val) {
    //    ERROR_LOG("ALARM INFO: cannot get arg from cmd[%s]", p_cmd);
    //    return -1;
    //}
    //strncpy(metric_arg, p_val, sizeof(metric_arg) - 1);

    ///查找告警时间间隔规则字符串
    char alarm_span[OA_MAX_STR_LEN] = {0};
    bool found = false;
    metric_alarm_map_t::const_iterator it_map = m_p_special_alarm_span->find(host_ip);
    if (it_map != m_p_special_alarm_span->end()) {
        metric_alarm_vec_t::iterator it_vec = it_map->second->begin();
        while (it_vec != it_map->second->end()) {
            if (!strcmp(metric_name, it_vec->metric_name)) {
                strcpy(alarm_span, it_vec->alarm_span);
                found = true;
                break;
            }
            ++it_vec;
        }
    }
    if (!found) {/**<查找不到再查默认的*/
        metric_alarm_vec_t::const_iterator it_vec = m_p_default_alarm_span->begin();
        while (it_vec != m_p_default_alarm_span->end()) {
            if (!strcmp(metric_name, it_vec->metric_name)) {
                strcpy(alarm_span, it_vec->alarm_span);
                found = true;
                break;
            }
            ++it_vec;
        }
    }
    if (!found) {
        ERROR_LOG("ALARM INFO: cannot find host[%s] metric[%s] span, use default[1-5:5;5-10:20;11-0:120]",
                host_ip, metric_name);
        strcpy(alarm_span, "1-5:5;5-10:20;11-0:120");
        found = true;
    }
    char log_alarm_span[OA_MAX_STR_LEN] = {0};
    strcpy(log_alarm_span, alarm_span);

    int span = -1;
    int next_alarm_cnt = alarm_cnt + 1;
    if (found) {
        int start = -1;
        int end = -1;
        char *p_line = NULL;//中横杠位置
        char *p_colon = NULL;//冒号位置
        char *p_semi = NULL;//分号位置
        char *p_pre = alarm_span;
        ///时间规则: 1-5:5;6-0:10
        while (p_pre && *p_pre && span == -1) {
            p_line = index(p_pre, '-');
            if (NULL == p_line) {
                ERROR_LOG("ALARM INFO: \"-\" is wrong in alarm_span[%s]", alarm_span);
                break;
            }
            *p_line = 0;
            start = atoi(p_pre);
            p_pre = p_line + 1;
            p_colon = index(p_pre, ':');
            if (NULL == p_colon) {
                ERROR_LOG("ALARM INFO: \":\" is wrong in alarm_span[%s]", alarm_span);
                break;
            }
            *p_colon = 0;
            end = atoi(p_pre);
            p_pre = p_colon + 1;
            if (p_line > p_colon) {
                ERROR_LOG("ALARM INFO: \":\" before \"-\" in alarm_span[%s]", alarm_span);
                break;
            }
            p_semi = index(p_pre, ';');
            if (NULL != p_semi) {
                *p_semi = 0;
            }
            if (next_alarm_cnt >= start && (next_alarm_cnt <= end || end == 0)) {
                span = atoi(p_pre);
            }
            p_pre = (NULL == p_semi) ? NULL : p_semi + 1;
        }
    }

    DEBUG_LOG("ALARM INFO: host_ip[%s] metric[%s] %dth send span[%d], in alarm_span[%s]",
            host_ip, metric_name, next_alarm_cnt, span, log_alarm_span);
    return span == -1 ? -1 : span*60;
}

/** 
 * @brief   发送告警命令
 * @param   p_next_send_span: 下次以命令待发间隔
 * @return  0-success, -1-failed
 */
int c_alarm_thread::send_alarm_cmd(uint32_t *p_next_send_span)
{
    if (NULL == p_next_send_span) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }
    int min_span = 0;
    int span = 0;
    notice_vec_t *p_vec = NULL;
    notice_map_t::iterator it_map = m_notice_map.begin();
    char send_data[MAX_STR_LEN] = {0};
    string ret_str = "";
    //TODO:增加一个程序运行时间差
    time_t now = time(NULL);
    while (it_map != m_notice_map.end()) {
        p_vec = it_map->second;
        notice_vec_t::iterator it_vec = p_vec->begin();
        while (it_vec != p_vec->end()) {
            bool need_erase = false;//it_vec清除标志
            if (it_vec->next_alarm_time <= now) {//达到发送时间
                bool need_send = true;//数据发送标志
                switch (it_vec->cmd_id) {
                    case OA_HOST_METRIC:
                    case OA_HOST_ALARM:
                    case OA_UPDATE_FAIL:
                    case OA_DS_DOWN:
                        {//调整下次发送时间和已发送次数
                            ++it_vec->alarm_cnt;//本次告警序列号,从0开始计数
                            span = get_send_span(it_vec->alarm_cnt, it_vec->alarm_cmd);
                            if (span <= 0) {
                                ERROR_LOG("ALARM INFO: get_send_span() of cmd[%s] failed, take half an hour as default.",
                                        it_vec->alarm_cmd);
                                span = 30*60;//取不到值则默认半个小时
                            }
                            if (min_span > span || min_span == 0) {
                                min_span = span;
                            }
                            it_vec->next_alarm_time = now + span;
                            break;
                        }
                    case OA_HOST_METRIC_RECOVERY:
                    case OA_HOST_RECOVERY:
                    case OA_UPDATE_RECOVERY:
                    case OA_DS_RECOVERY:
                        {//状态恢复的告警发送出去即清理掉
                            need_erase = true;
                            break;
                        }
                    default :
                        ERROR_LOG("ALARM INFO: Wrong cmd_id[%d] in cmd[%s].", it_vec->cmd_id, it_vec->alarm_cmd);
                        need_send = false;
                        break;
                }

                ///发送告警信息
                if (need_send) {
                    sprintf(send_data, "%s&send_time=%lu", it_vec->alarm_cmd, now);
                    m_http_transfer.http_post(m_alarm_server_url, send_data);
                    ret_str = m_http_transfer.get_post_back_data(); 
                    DEBUG_LOG("ALARM INFO: http_transfer send[%s], return[%s].", send_data, ret_str.c_str());
                }
            } else {
                span = it_vec->next_alarm_time - now;
                if (min_span > span || min_span == 0) {
                    min_span = span;
                }
            }

            ///清理一次性告警信息
            if (need_erase) {
                it_vec = p_vec->erase(it_vec);//TODO:待检查
            } else {
                ++it_vec;
            }
        }
        ++it_map;
    }
    *p_next_send_span = min_span > 0 ? min_span : 1;

    return 0;
}

/** 
 * @brief   线程主函数
 * @param   p_data  用户数据
 * @return  NULL = success UNNULL = failed
 */
void* c_alarm_thread::alarm_main(void  *p_data)
{
    c_alarm_thread *alarmer = (c_alarm_thread*)p_data;
    ds_queue_map_t *queues = alarmer->m_p_queue_map;              /**<queue的指针的map*/
    char post_data[MAX_STR_LEN * 2] = {'\0'};
    alarm_event_t *p_event = (alarm_event_t *)post_data;

    uint32_t next_send_span = 0;

    DEBUG_LOG("Enter the main while loop of alarm_thread");
    int event_cnt = 0;
    while(!alarmer->m_stop) {
        ds_queue_map_t::iterator it;
        i_ring_queue  *queue = NULL;
        for(it = queues->begin(); it != queues->end(); it++) {
            memset(post_data, 0, sizeof(post_data));
            queue = it->second;
            if(queue == NULL) {
                //unlikely to come here
                ERROR_LOG("Critical error.the ring queue has uninited.");
                return (void*)-1;
            }
            //队列里没有数据或出错
            if(queue->get_data_len() <= 0) {
                DEBUG_LOG("No data in queue[%u]", it->first);
                continue;
            }

            int ret = queue->pop_data(post_data, sizeof(post_data) - 1, 8);
            if(ret < 0) {
                ERROR_LOG("Dummy data from [%u]'s queue failed", it->first);
                continue;
            } else if(ret == 0) {
                //pop data time out
                ERROR_LOG("Dummy data from [%u]'s queue timeout", it->first);
                continue;
            } else {
                ++event_cnt;
                DEBUG_LOG("[%dth]Get the alarm data of ds[%u], the post data is:[%s].",
                        event_cnt, it->first, p_event->cmd_data);
                alarmer->handle_alarm_cmd(post_data);
            }
        }
        alarmer->send_alarm_cmd(&next_send_span);

        uint32_t sec = 0;
        uint32_t sleep_time = MIN(next_send_span , alarmer->m_alarm_interval);
        DEBUG_LOG("Alarm_thread take a break(%ds).....", sleep_time);
        ///每隔一秒检测是否需要退出
        while(!alarmer->m_stop && sec < sleep_time) {
            sleep(1);
            sec++;
        }
    }

    DEBUG_LOG("Exit main while loop of alarm_thread");
    return NULL;
}

