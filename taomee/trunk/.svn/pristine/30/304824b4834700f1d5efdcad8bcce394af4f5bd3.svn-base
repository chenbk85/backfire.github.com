/**
 * =====================================================================================
 *       @file  data_processer.cpp
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <new>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../lib/log.h"
#include "../lib/http_transfer.h"
#include "../lib/check_hostalive.h"
#include "../db_operator.h"
#include "./rrd_handler.h"
#include "./alarm_thread.h"
#include "./data_processer.h"

static const char *host_alarm_type[4] = {"", "agent_down", "host_unknown", "host_down"};

/** 
 * @brief  创建对象实例
 * @param  pp_instance 对象指针的指针 
 * @return  0 success -1 failed 
 */
int create_data_processer_instance(c_data_processer  **pp_instance)
{
    if(pp_instance == NULL) {
        ERROR_LOG("pp_instance is NULL.");
        return -1;
    }

    *pp_instance = new (std::nothrow)c_data_processer();
    if(*pp_instance == NULL) {
        ERROR_LOG("new failed.");
        return -1;
    }
    return 0;
}

/** 
 * @brief  构造函数
 * @param   
 * @return   
 */
c_data_processer::c_data_processer(): m_inited(false), m_ds_conn(NULL), m_ds(NULL), m_stop(false),
    m_p_root(NULL), m_p_queue(NULL), m_grid_name(NULL), m_pid(0)
{
}

/** 
 * @brief  析构函数
 * @param   
 * @return   
 */
c_data_processer::~c_data_processer()
{
    uninit();
}

/** 
 * @brief  释放对象函数与create_instance对应
 * @param   
 * @return   
 */
int c_data_processer::release()
{
    delete this;
    return 0;
}

/** 
 * @brief  反初始化
 * @param   
 * @return   
 */
int c_data_processer::uninit()
{
    if(!m_inited) {
        return -1;
    }

    assert(m_pid != 0);

    m_stop = true;
    pthread_join(m_pid, NULL);

    m_stop = false;
    m_pid  = 0;

    if(m_ds_conn != NULL) {
        m_ds_conn->uninit();
        m_ds_conn->release();
        m_ds_conn = NULL;
    }

    m_p_queue = NULL;
    m_p_root = NULL;
    m_ds = NULL;
    m_grid_name = NULL;
    m_inited = false;

    return 0;
}

/** 
 * @brief  初始化函数,要么init成功，要么失败会uninit已经init成功的变量
 * @param   ds 数据源
 * @param   p_config config对象指针
 * @param   p_root   保存本地数据源的根节点指针
 * @return   0 success -1 failed
 */
int c_data_processer::init(data_source_list_t *ds,
                    source_t *p_root,
                    const collect_conf_t *p_config,
                    i_ring_queue *p_queue,
                    void *p_rrd,
                    const metric_alarm_vec_t *default_metric_alarm_info,
                    const metric_alarm_map_t *specified_metric_alarm_info)
{
    if(m_inited) {
        return -1;
    }

    if(!ds || !p_root|| !p_config || !p_queue || !p_rrd ||
            !default_metric_alarm_info || !specified_metric_alarm_info) {
        ERROR_LOG("ERROR: Parament cannot be NULL.");
        return -1;
    }

    if(0 != create_net_client_instance(&m_ds_conn)) {
        ERROR_LOG("Create net client instance failed");
        return -1;
    }

    m_p_root  = p_root;
    m_p_queue = p_queue;
    m_grid_name = (char*)(p_config->grid_name);

    //strncpy(m_grid_name, p_config->grid_name, sizeof(m_grid_name));

    if(0 != m_parser.init(m_p_root, m_p_queue, (c_rrd_handler*)p_rrd, ds->step,
                default_metric_alarm_info, specified_metric_alarm_info)) {
        ERROR_LOG("init xml parser object failed");

        if(m_ds_conn != NULL) {
            m_ds_conn->release();
            m_ds_conn = NULL;
        }

        m_p_queue = NULL;
        m_p_root = NULL;

        return -1;
    }

    m_ds = ds;
    memset(m_buf, 0, sizeof(m_buf));

    if(0 != pthread_create(&m_pid, NULL, data_processer_main, this)) {
        m_stop = false;
        m_pid  = 0;

        if(m_ds_conn != NULL) {
            m_ds_conn->uninit();
            m_ds_conn->release();
            m_ds_conn = NULL;
        }

        m_p_queue = NULL;
        m_p_root = NULL;
        m_grid_name = NULL;
        m_ds = NULL;

        return -1;
    }

    m_inited = true;
    return 0;
}

/** 
 * @brief   线程主函数
 * @param   p_data  用户数据
 * @return  NULL success UNNULL failed
 */
void* c_data_processer::data_processer_main(void  *p_data)
{
    c_data_processer *processer = (c_data_processer*)p_data;
    data_source_list_t *ds = processer->m_ds;
    c_net_client_impl *ds_conn = processer->m_ds_conn;
    xml_parser *parser= &processer->m_parser;
    time_t start = 0;
    time_t end = 0;
    unsigned int elapsed = 0;

    while(!processer->m_stop) {
        //取得开始处理的时间
        start = time(NULL);

        int recv_byte = 0;
        int recv_sum = 0;
        char *write_pos = processer->m_buf;
        int remain_len = sizeof(processer->m_buf) - 1;
        int match_tag = 0;
        bool connected = false;
        time_t recv_begin = 0;
        int ret = -1;

        //尝试上一次连接成功的host
        int idx = ds->last_good_idx;
        struct sockaddr_in localaddr;
        socklen_t addr_len = 0;
        if(idx != -1) {
            do {
                if(ds_conn->init(ds->host_list[idx].server_ip, ds->host_list[idx].server_port, 30) != 0) {
                    break;
                }
                addr_len = sizeof(localaddr);
                if(getsockname(ds_conn->get_fd(), (struct sockaddr*)&localaddr, &addr_len) != 0) {
                    ds_conn->uninit();
                    break;
                }
                //本地地址和远端地址一样
                if((in_addr_t)(ds->host_list[idx].server_ip) == localaddr.sin_addr.s_addr &&  
                        htons(ds->host_list[idx].server_port) == localaddr.sin_port) {
                    ds_conn->uninit();
                    continue;
                } else {
                    connected = true;
                    break;
                }
            } while (true);
        }

        for(int host_idx = 0; !connected && host_idx < (int)ds->host_num && !processer->m_stop; host_idx++)
        {
            if(host_idx == idx) {
                continue;//如果之前连接失败了现在不要再做
            }

            do {
                if(ds_conn->init(ds->host_list[host_idx].server_ip, ds->host_list[host_idx].server_port, 30) != 0) {
                    break;
                }
                addr_len = sizeof(localaddr);
                if(getsockname(ds_conn->get_fd(), (struct sockaddr*)&localaddr, &addr_len) != 0) {
                    ds_conn->uninit();
                    break;
                }
                //本地地址和远端地址一样
                if((in_addr_t)(ds->host_list[host_idx].server_ip) == localaddr.sin_addr.s_addr 
                        && htons(ds->host_list[host_idx].server_port) == localaddr.sin_port) {
                    ds_conn->uninit();
                    continue;
                } else {
                    connected = true;
                    ds->last_good_idx = host_idx;
                    break;
                }
            } while (true);
        }

        if(!connected) {
            ++ds->disable_count;
            ERROR_LOG("No answer from any host of datasource[%u], continuous times: %d.",
                    ds->ds_id, ds->disable_count);
            if(ds->disable_count >= MAX_DISABLE_COUNT) {
                char ds_name[12] = {0};
                snprintf(ds_name, sizeof(ds_name), "%u", ds->ds_id);
                processer->report_data_source_down(ds_name, ds);
                ds->dead = 1;
                ds->disable_count = 0;
            }
            goto take_break;
        }
        else {
            ds->disable_count = 0;
        }

        //开始接收数据
        oa_cmd_t cmd;
        cmd.msg_len = sizeof(cmd) + 1;
        cmd.version = 1;
        cmd.msg_id  = GET_SUMMARY_INFO;
        ds_conn->send_data((char*)&cmd, sizeof(cmd));
        ds_conn->send_data((char*)"/", 1);

        ret = ds_conn->do_io();
        if(ret == -1) {
            ERROR_LOG("do_io() error,net connect error, idx[%d] host_num[%d].", ds->last_good_idx, ds->host_num);
            //如果循环一圈还没有可用的连接，那么标记这个数据源为dead
            if((ds->last_good_idx = (ds->last_good_idx + 1) % ds->host_num) == 0) {
                if(++ds->disable_count >= MAX_DISABLE_COUNT) {
                    char ds_name[12] = {0};
                    snprintf(ds_name, sizeof(ds_name), "%u", ds->ds_id);
                    processer->report_data_source_down(ds_name, ds);
                    ds->dead = 1;
                    ds->disable_count = 0;
                }
            }
            goto take_break;
        } else if(ret == -2) {
            DEBUG_LOG("Net connection closed by peer, may be no newer data.");
            goto take_break;
        } else {
            //do_io成功开始接收数据
        }

        //开始接收数据
        recv_begin = time(NULL);
        while((recv_byte = ds_conn->recv_data(write_pos, remain_len)) >= 0) {
            if(recv_byte > 0) {
                char const *pos = write_pos;
                if(strcasestr(pos, "<CLUSTER")) {
                    match_tag++; 
                }
                if(strcasestr(pos, "</CLUSTER>")) {
                    match_tag--; 
                }

                write_pos += recv_byte;
                recv_sum += recv_byte;
                if((remain_len -= recv_byte) <= 0) {
                    ERROR_LOG("No more buffer space for xml data.");
                    goto take_break;
                }
            } else if((unsigned int)(time(NULL) - recv_begin) >= SEND_TIMEOUT) {/// && recv_byte == 0
                //超时,这里的超时时间用oa_node的export线程发送xml数据的超时时间
                ERROR_LOG("Recv data timeout.");
                goto take_break;
            } else {
                //do nothing and goon
            }

            ret = ds_conn->do_io();
            if(ret == -2 && match_tag == 0) {	
                //如果对端关闭连接且标签匹配，那么数据已经传送完毕，转向处理这个xml数据
                goto do_parse;
            } else if(ret == -2 && match_tag != 0) {//对端关闭连接(ret==-2)但是标签不匹配(match_tag!=0)
                DEBUG_LOG("Connection closed by peer, while not all the data was sent.");
                goto take_break;
            } else if(ret == -1) {//当do_io出错(ret==-1)
                ERROR_LOG("do_io() error,net connect error, idx[%d] host_num[%d].", ds->last_good_idx, ds->host_num);
                if((ds->last_good_idx = (ds->last_good_idx + 1) % ds->host_num) == 0) {
                    if(++ds->disable_count >= MAX_DISABLE_COUNT) {
                        char ds_name[12] = {0};
                        snprintf(ds_name, sizeof(ds_name), "%u", ds->ds_id);
                        processer->report_data_source_down(ds_name, ds);
                        ds->dead = 1;
                        ds->disable_count = 0;
                    }
                }
                goto take_break;
            }
        }///end while

        if(recv_byte < 0) {
            ERROR_LOG("Xml buffer has no enough space,go to take_break.");
            goto take_break;
        }

do_parse:
        if(0 != parser->process_xml(ds, processer->m_buf)) {
            ERROR_LOG("Parse the xml data failed.");
        }

        if(ds->dead == 1) {
            //如果上一次data source是dead状态，收回报警
            char ds_name[12] = {0};
            snprintf(ds_name, sizeof(ds_name), "%u", ds->ds_id);
            processer->revoke_data_source_down(ds_name, ds);
            ds->dead = 0;
        }

take_break: 
        //断开连接
        ds_conn->uninit();
        if(parser->xml_parser_reset() < 0) {
            ERROR_LOG("Xml parser reset error.");
        }
        //清空接收缓冲
        memset(processer->m_buf, 0, sizeof(processer->m_buf));

        //取得处理结束的时间
        end = time(NULL);
        elapsed = end - start;
        //减去处理接收数据和解析数据的时间
        if(ds->step > elapsed) {
            unsigned int sec = 0;
            while(!processer->m_stop && sec++ < (ds->step - elapsed)) {
                sleep(1);
            }
        }
    }
    DEBUG_LOG("Exit main while loop of data_processer_thread of [%u].", ds->ds_id);
    return NULL;
}

/** 
* @brief   当一个数据源down时报警 
* @param   ds 数据源结构指针
* @return  void  
**/
void c_data_processer::report_data_source_down(const char *ds_name, const data_source_list_t *ds)
{
    if (ds == NULL || ds_name == NULL) {
        ERROR_LOG("ERROR: Parament cannot be NULL.");
        return;
    }

    if (ds->dead == 1) {
        return;
    }

    alarm_event_t event_info = {0};
    event_info.cmd_id = OA_DS_DOWN;
    strcpy(event_info.host_ip, "DS");
    str2md5(ds_name, event_info.key);

    char *write_pos = event_info.cmd_data;
    int   left_len = sizeof(event_info.cmd_data) - 1;
    int   len = 0;
    //len = snprintf(write_pos, left_len, "cmd=%d&host=ds&metric=%s;%s;", OA_DS_DOWN, m_grid_name, ds_name);

    //数据源down的报警
    len = snprintf(write_pos, left_len, "cmd=%d&host=DS&metric=%s&start_time=%lu&mute=0&ip_list=",
            OA_DS_DOWN, ds_name, time(NULL));
    for (unsigned int i = 0; i < ds->host_num; i++) {  
        struct in_addr tmp_addr;
        memcpy(&tmp_addr, &(ds->host_list[i].server_ip), sizeof(ds->host_list[i].server_ip)); 
        write_pos += len; 
        left_len -= len;
        if (left_len <= 0) {
            break;
        }
        len = snprintf(write_pos, left_len, "%s:%u%s",
                inet_ntoa(tmp_addr), ds->host_list[i].server_port, i == ds->host_num - 1 ? "" : ",");
    } 
    event_info.data_len = sizeof(event_info) - sizeof(event_info.cmd_data) + strlen(event_info.cmd_data);
    int ret = m_p_queue->push_data((char *)&event_info, event_info.data_len, 1);
    DEBUG_LOG("ALARM INFO: push alarm data:[%s]", event_info.cmd_data);
    DEBUG_LOG("ALARM INFO: push data ret:[%d], error str:[%s]", ret, m_p_queue->get_last_errstr());

    //报发送主机down的警
    //if (ds->last_good_idx != -1) {///曾经接到过数据
    //    event_info.cmd_id = OA_HOST_ALARM;

    //    left_len = sizeof(event_info.cmd_data) - 1;
    //    for(unsigned int j = 0; j < ds->host_num; j++) {   
    //        struct in_addr tmp_addr;
    //        char   host_ip[16] = {0};
    //        char   ping_ip[16] = {0};
    //        strncpy(host_ip, ds->host_list[j].server_inside_ip, sizeof(host_ip) - 1);
    //        strcpy(event_info.host_ip, host_ip);

    //        memcpy(&tmp_addr, &(ds->host_list[j].server_ip), sizeof(ds->host_list[j].server_ip)); 
    //        if(inet_ntop(AF_INET, (void*)&tmp_addr, ping_ip, sizeof(ping_ip)) != NULL) {
    //            int ret = check_hostalive(ping_ip, 10);
    //            host_status_t type = HOST_STATUS_OK;
    //            if(ret == STATE_WARNING || ret == -1) {//有丢包或者popen失败
    //                type = HOST_STATUS_HOST_UNKNOWN;
    //            } else if(ret == STATE_OK) {
    //                type = HOST_STATUS_AGENT_DOWN;
    //            } else {
    //                type = HOST_STATUS_HOST_DOWN;
    //            }
    //            set_host_status(ds_name, host_ip, type);
    //            str2md5(host_alarm_type[type], event_info.key);
    //            snprintf(event_info.cmd_data, sizeof(event_info.cmd_data) - 1,
    //                        "cmd=%d&host=%s&metric=%s&start_time=%lu&mute=0",
    //                        OA_HOST_ALARM, host_ip, host_alarm_type[type], time(NULL));
    //            event_info.data_len = sizeof(event_info) - sizeof(event_info.cmd_data) + strlen(event_info.cmd_data);
    //            if((ret = m_p_queue->push_data((char *)&event_info, event_info.data_len, 1)) != -1) {
    //                DEBUG_LOG("ALARM INFO: push alarm data:[%s]", event_info.cmd_data);
    //                DEBUG_LOG("ALARM INFO: push data ret:[%d], error str:[%s]", ret, m_p_queue->get_last_errstr());
    //            }
    //        }
    //    }  
    //}
    return;
}

void c_data_processer::set_host_status(const char* ds_name, const char *host_name, int host_status)
{
    datum_t   ds_key = {(void*)ds_name, strlen(ds_name) + 1};
    datum_t  *ds_datum = NULL;
    source_t  ds;
    ds_datum = hash_lookup(&ds_key, m_p_root->children);
    if(ds_datum == NULL) {
        return;
    } else {
        memcpy(&ds, ds_datum->data, ds_datum->size);
        datum_free(ds_datum);
    }
    host_status_arg arg = {host_name, host_status};
    hash_foreach(ds.children, _set_host_status, &arg);
    return;
}

int c_data_processer::_set_host_status(datum_t *key, datum_t *val, void *arg)
{
    host_status_arg *host_arg = (host_status_arg *)arg;
    int host_status = host_arg->host_status;
    const char *host_name = host_arg->host_name;
    host_t *p_host = (host_t *)val->data; 

    if(!strcmp(p_host->ip, host_name)) {
        p_host->host_status = host_status;
        return 1;
    }
    return 0;
}

/** 
* @brief  当一个数据源从down到up时收回报警 
* @param   ds 数据源结构
* @return  void  
**/
void c_data_processer::revoke_data_source_down(const char *ds_name, const data_source_list_t * ds)
{
    if(ds == NULL || ds_name == NULL) {
        ERROR_LOG("ERROR: parament cannot be NULL.");
        return;
    }

    alarm_event_t event_info = {0};
    event_info.cmd_id = OA_DS_RECOVERY;
    strcpy(event_info.host_ip, "DS");
    str2md5(ds_name, event_info.key);

    int   len = 0;
    int   left_len = sizeof(event_info.cmd_data) - 1;
    char *write_pos = event_info.cmd_data;

    len = snprintf(write_pos, left_len, "cmd=%d&host=ds&metric=%s&start_time=%lu&mute=0&ip_list=",
            OA_DS_RECOVERY, ds_name, time(NULL));
    //len = snprintf(write_pos, left_len, "cmd=20004&type=ds&ds_info=%s;%s;", m_grid_name, ds_name);
    for(unsigned int i = 0; i < ds->host_num; i++) {  
        struct in_addr tmp_addr;
        memcpy(&tmp_addr, &(ds->host_list[i].server_ip), sizeof(ds->host_list[i].server_ip)); 
        write_pos += len; 
        left_len -= len;
        if(left_len <= 0) {
            break;
        }
        len = snprintf(write_pos, left_len, "%s:%u%s",
                inet_ntoa(tmp_addr), ds->host_list[i].server_port, i == ds->host_num - 1 ? "" : ",");
    } 
    event_info.data_len = sizeof(event_info) - sizeof(event_info.cmd_data) + strlen(event_info.cmd_data);
    int ret = m_p_queue->push_data((char *)&event_info, event_info.data_len, 1);
    DEBUG_LOG("ALARM INFO: push alarm data:[%s]", event_info.cmd_data);
    DEBUG_LOG("ALARM INFO: push data ret:[%d], error str:[%s]", ret, m_p_queue->get_last_errstr());

    return;
}
