/**
 * =====================================================================================
 *       @file  listen_thread.cpp
 *      @brief  保存接受到的组播数据，并为export线程获取、删除相应数据提供接口
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  10/18/2010 05:46:04 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:40:18
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <netdb.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <rpc/rpc.h>

#include "../lib/log.h"
#include "./listen_thread.h"

using namespace std;

c_listen_thread::c_listen_thread(): m_inited(false), m_continue_working(false), m_host_change(false),
    m_p_thread_started(NULL), /*m_work_thread_id(0),*/ m_listen_fd(-1), m_p_config(NULL), m_p_hash_table(NULL)
{
}

c_listen_thread::~c_listen_thread()
{
    uninit();
}

/**
 * @brief  初始化
 * @param  p_config: config线程指针
 * @param  p_listen_ip: 内网IP(接受UDP包)
 * @param  p_heartbeat: 存储心跳实例
 * @param  p_buildin: 存储内置metric实例
 * @param  p_custom: 存储用户定义metric实例
 * @param  p_thread_started: listen线程启动标志位
 * @return 0-success, -1-failed
 */
int c_listen_thread::init(i_config *p_config, const char *p_listen_ip, c_hash_table *p_hash_table, bool *p_thread_started)
{
    if (m_inited)
    {
        ERROR_LOG("ERROR: c_listen_thread has been inited");
        return -1;
    }

    if (!p_config || !p_listen_ip || !p_hash_table || !p_thread_started)
    {
        ERROR_LOG("p_config=%p p_listen_ip=%p p_hash_table=%p p_thread_started=%p", p_config, p_listen_ip, p_hash_table, p_thread_started);
        return -1;
    }
    m_p_config = p_config;
    m_p_hash_table = p_hash_table;
    m_p_thread_started = p_thread_started;

    do
    {
        ///初始化单播sockfd
        int result = 0;
        char port[OA_MAX_STR_LEN] = {0};
        result = m_p_config->get_config("node_info", "listen_port", port, sizeof(port));
        if (result != 0)
        {
            ERROR_LOG("ERROR: cannot get config[node_info:listen_port].");
            break;
        }

        int listen_port = atoi(port);
        if (strlen(p_listen_ip) < 7 || listen_port <= 0 || listen_port > 65535)
        {
            ERROR_LOG("ERROR: listen_ip[%s]\t listen_port[%s]\n", p_listen_ip, port);
            break;
        }
        ///创建socket
        m_listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_listen_fd == -1)
        {
            ERROR_LOG("ERROR: socket(SOCK_DGRAM) failed: %s", strerror(errno));
            break;
        }

        struct sockaddr_in peer_addr;
        memset(&peer_addr, 0, sizeof(peer_addr));
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(listen_port);
        if (inet_pton(AF_INET, p_listen_ip, &peer_addr.sin_addr.s_addr) <= 0)
        {
            ERROR_LOG("ERROR: Wrong listen IP: %s", p_listen_ip);
            break;
        }
        if (bind(m_listen_fd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == -1)
        {
            ERROR_LOG("Bind Failed[%s:%s]: %s", p_listen_ip, port, strerror(errno));
            break;
        }
        ///设置地址重用
        int reuse = 1;
        if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0)
        {
            ERROR_LOG("ERROR: setsockopt(SO_REUSEADDR) failed, IP:Port[%s:%d]", p_listen_ip, listen_port);
            break;
        }
        ///设置为非阻塞模式
        int flag = fcntl(m_listen_fd, F_GETFL);
        flag |= O_NONBLOCK;
        fcntl(m_listen_fd, F_SETFL, flag);

        m_continue_working = true;
        result = pthread_create(&m_work_thread_id, NULL, work_thread_proc, this);
        if (result != 0)
        {
            m_work_thread_id = 0;
            ERROR_LOG("ERROR: pthread_create(...) failed.");
            break;
        }
        m_inited = true;
    } while (false);

    if (m_inited)
    {
        DEBUG_LOG("c_listen_thread init successfully.");
        return 0;
    } else
    {
        m_continue_working = false;
        if (m_listen_fd >= 0)
        {
            close(m_listen_fd);
            m_listen_fd = -1;
        }

        m_host_change = false;
        m_p_config = NULL;
        m_p_hash_table = NULL;
        m_p_thread_started = NULL;
        return -1;
    }
}

/**
 * @brief  接受单播数据
 * @param  p_data_buf: 数据缓存区地址(空间由调用者分配)
 * @param  buf_len: 缓存区长度
 * @return 成功返回接受到数据包的长度，失败则返回-1
 */
int c_listen_thread::recv_data(char *p_data_buf, const int buf_len)
{
    if (p_data_buf == NULL || buf_len <= 0)
    {
        ERROR_LOG("p_data_buf=%p buf_len=%d[<=0]", p_data_buf, buf_len);
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_listen_fd, &rfds);
    int retval = select(m_listen_fd + 1, &rfds, NULL, NULL, &tv);
    if (retval < 0)
    {
        if(errno == EINTR) { //没有数据
            return 0;
        } else
        {
            ERROR_LOG("ERROR: select() failed, errno: %d, reason: %s", errno, strerror(errno));
            return -1;
        }
    } else if (retval == 0) { //timeout
        return 0;
    }

    int rcvlen = recvfrom(m_listen_fd, p_data_buf, buf_len, 0, NULL, NULL);
    if (rcvlen < 0)
    {
        ERROR_LOG("ERROR: recvfrom(...) failed: %s", strerror(errno));
    }

    return rcvlen;
}

/**
 * @brief  保存接受到的xdr数据包
 * @param  p_data: 接受到的xdr数据包
 * @param  data_len: 数据包长度
 * @return 0-success, -1-failed
 */
int c_listen_thread::store_data(char * p_data, int data_len)
{
    if (p_data == NULL || data_len <= 0)
    {
        ERROR_LOG("p_data=%p buf_len=%d[<0]", p_data, data_len);
        return -1;
    }

    XDR x_rcv;
    u_int head_len = 0;
    u_short msg_len = 0;
    char ip_str[16] = {0};
    struct in_addr ia_tmp;

    time_t heartbeat = 0;
    time_t now = time(NULL);

    char metric_name[OA_MAX_STR_LEN] = {0};
    char *p_metric_name = metric_name;
    char metric_key[OA_MAX_STR_LEN] = {0};
    char *p_metric_key = metric_key;        //metric_key = metric_type:metric_name
    char res_serial[OA_MAX_STR_LEN] = {0};
    char *p_res_serial = res_serial;

    char val_buff[OA_MAX_BUF_LEN] = {0};
    metric_val_t *p_val_data = (metric_val_t *)val_buff;
    heartbeat_val_t *p_hb_data = (heartbeat_val_t *)val_buff;
    heartbeat_val_t *p_hb_tmp = NULL;
    c_data data_tmp;

    const char * pp_hash_key[2];
    pp_hash_key[0] = p_res_serial;
    pp_hash_key[1] = p_metric_key;

    int ret_code = 0;
    do
    {
        xdrmem_create(&x_rcv, p_data, data_len, XDR_DECODE);
        if (!xdr_u_short(&x_rcv, &msg_len))
        {
            ERROR_LOG("ERROR: xdr_u_short() to message length.");
            ret_code = -1;
            break;
        }
        if (msg_len < data_len)
        {
            ERROR_LOG("Package Error: receive len[%d], but message len[%d]", data_len, msg_len);
            ret_code = -1;
            break;
        }
        ///机器资产序列号
        if (!xdr_string(&x_rcv, &p_res_serial, sizeof(res_serial)))
        {
            ERROR_LOG("ERROR: xdr_string() to machine serial.");
            ret_code = -1;
            break;
        }
        if (0 == strlen(p_res_serial))
        {
            ERROR_LOG("ERROR: server tag length is zero.");
            ret_code = -1;
            break;
        }
        ///机器IP
        unsigned int host_ip = 0;
        if (!xdr_u_int(&x_rcv, &host_ip))
        {
            ERROR_LOG("ERROR: xdr_string(host_ip).");
            ret_code = -1;
            break;
        }
        if (host_ip == 0)
        {
            ERROR_LOG("IP ERROR: Received host ip is zero.");
            ret_code = -1;
            break;
        }
        ia_tmp.s_addr = host_ip;
        if (NULL == inet_ntop(AF_INET, (void *)&ia_tmp, ip_str, sizeof(ip_str)))
        {
            ERROR_LOG("ERROR: inet_ntop(%u) failed: %s", host_ip, strerror(errno));
            ret_code = -1;
            break;
        }

        int collect_interval = 0;
        if (!xdr_int(&x_rcv, &collect_interval))
        {
            ERROR_LOG("ERROR: xdr_int(collect_interval) failed.");
            ret_code = -1;
            break;
        }

        int metric_id = -1000;
        if (!xdr_int(&x_rcv, &metric_id))
        {
            ERROR_LOG("ERROR: xdr_int(metric_id) failed.");
            ret_code = -1;
            break;
        }
        if (metric_id < OA_MIN_METRIC_ID)
        {
            ERROR_LOG("Receive metirc_id[%d] < OA_MIN_METRIC_ID[%d]", metric_id, OA_MIN_METRIC_ID);
            ret_code = -1;
            break;
        }
        NOTI_LOG("recv metric_id[%d] from [%s].", metric_id, ip_str);

        switch (metric_id)
        {
            case OA_CLEANUP_TYPE:
                DEBUG_LOG("receive OA_CLEANUP_TYPE from %s[%s].", p_res_serial, ip_str);
                m_host_change = true;
                ///清理重启前机器的数据
                if (m_p_hash_table->remove(p_res_serial) != 0)
                {
                    DEBUG_LOG("DEBUG: delete_data(%s[%s], OA_HEART_BEAT_TYPE) failed.", p_res_serial, ip_str);
                } else
                {
                    DEBUG_LOG("DEBUG: delete_data(%s[%s], OA_HEART_BEAT_TYPE) success.", p_res_serial, ip_str);
                }
                break;
            case OA_UP_FAIL_TYPE:
                ERROR_LOG("receive OA_UP_FAIL_TYPE from %s[%s].", p_res_serial, ip_str);

                data_tmp = m_p_hash_table->search(p_res_serial);
                if(data_tmp.not_null())
                {
                    p_hb_tmp = (heartbeat_val_t *)data_tmp.get_value();
                    p_hb_tmp->up_fail = true;
                    m_p_hash_table->update(sizeof(heartbeat_val_t), p_hb_tmp, p_res_serial);
                } else
                {
                    //can not be here
                    ERROR_LOG("ERROR: no host info [%s]", p_res_serial);
                }
                break;
            case OA_HEART_BEAT_TYPE:
                if (!xdr_u_int(&x_rcv, (u_int *)&heartbeat))
                {
                    ERROR_LOG("ERROR: xdr_u_int() to heartbeat.");
                    ret_code = -1;
                    break;
                }

                data_tmp = m_p_hash_table->search(p_res_serial);
                if (data_tmp.not_null())
                {
                    if (data_tmp.get_length() != sizeof(heartbeat_val_t))
                    {
                        ERROR_LOG("ERROR: store [%s:%s] heartbeat data size[%u] error: is not equal to %lu",
                                p_res_serial, ip_str, data_tmp.get_length(), sizeof(heartbeat_val_t));
                        ret_code = -1;
                        break;
                    }
                    p_hb_tmp = (heartbeat_val_t *)data_tmp.get_value();
                    if (p_hb_tmp->heart_beat < heartbeat) {//restart
                        DEBUG_LOG("DEBUG: host[%s:%s] restart time[%u], the lastest heartbeat[%u]",
                                p_res_serial, ip_str, (u_int)heartbeat, (u_int)p_hb_tmp->heart_beat);
                        //清理重启前机器的数据
                        if (m_p_hash_table->remove(p_res_serial) != 0)
                        {
                            DEBUG_LOG("DEBUG: delete_data(%s[%s], OA_HEART_BEAT_TYPE) failed.", p_res_serial, ip_str);
                        } else
                        {
                            DEBUG_LOG("DEBUG: delete_data(%s[%s], OA_HEART_BEAT_TYPE) success.", p_res_serial, ip_str);
                        }
                    } else if (p_hb_tmp->heart_beat > heartbeat) {///接受的数据比保存的数据还要旧
                        break;//jump switch
                    }
                } else
                {
                    m_host_change = true;
                    DEBUG_LOG("DEBUG: new host[%s:%s] added", p_res_serial, ip_str);
                }
                ///heartbeat hash_value
                p_hb_data->rcv_time = now;
                p_hb_data->heart_beat = heartbeat;
                p_hb_data->host_ip = host_ip;
                p_hb_data->up_fail = false;
                if (0 != m_p_hash_table->insert(sizeof(heartbeat_val_t), (void *)p_hb_data, p_res_serial))
                {
                    ERROR_LOG("insert_data(%s[%s], heart_beat) failed.", p_res_serial, ip_str);
                } else
                {
                    DEBUG_LOG("insert_data(%s[%s], heart_beat) success.", p_res_serial, ip_str);
                }
                break;
            default:
                if (collect_interval < 0)
                {
                    ERROR_LOG("Receive metric_id[%d], collect_interval[%d] < 0", metric_id, collect_interval);
                    ret_code = -1;
                    break;
                }

                if (!xdr_string(&x_rcv, &p_metric_name, sizeof(metric_name)))
                {
                    ERROR_LOG("ERROR: xdr_string() to [%s:%s]customer-defined metric name.", p_res_serial, ip_str);
                    ret_code = -1;
                    break;
                }
                if (0 == strlen(p_metric_name))
                {
                    ERROR_LOG("ERROR: [%s:%s]user-defined metric name length is zero.", p_res_serial, ip_str);
                    ret_code = -1;
                    break;
                }
                ///user-defined metric hash_key
                snprintf(p_metric_key, sizeof(metric_key)-1, "%d:%s", metric_id, p_metric_name);

                head_len = xdr_getpos(&x_rcv);
                if (head_len == 0 || head_len > msg_len)
                {
                    ERROR_LOG("ERROR: xdr_getpos() return error, head_len[%u], msg_len[%d].", head_len, msg_len);
                    ret_code = -1;
                    break;
                }
                ///user-defined metric hash_value
                p_val_data->rcv_time = now;
                p_val_data->collect_interval = collect_interval;
                p_val_data->cleaned = 0;
                memcpy(p_val_data->data, p_data + head_len, msg_len - head_len);

                if (0 != m_p_hash_table->insert(msg_len - head_len + sizeof(metric_val_t), (void *)p_val_data, 2, pp_hash_key))
                {
                    ERROR_LOG("insert_data(%s[%s]) from %s failed.", p_res_serial, p_metric_key, ip_str);
                } else
                {
                    DEBUG_LOG("insert_data(%s[%s]) from %s success.", p_res_serial, p_metric_key, ip_str);
                }
                break;
        }
        if (-1 == ret_code)
        {
            break;
        }

        data_len -= msg_len;
        p_data += msg_len;
    } while (data_len > 0);

    return ret_code;
}

/**
 * @brief  反初始化
 * @param  无
 * @return 0-success, -1-failed
 */
int c_listen_thread::uninit()
{
    if (!m_inited)
    {
        return -1;
    }
    assert(m_work_thread_id != 0);

    m_continue_working = false;
    pthread_join(m_work_thread_id, NULL);
    m_work_thread_id = 0;

    if (m_listen_fd >= 0)
    {
        close(m_listen_fd);
        m_listen_fd = -1;
    }

    m_p_thread_started = NULL;
    m_p_config = NULL;
    m_p_hash_table = NULL;
    m_host_change = false;
    m_inited = false;

    DEBUG_LOG("listen thread uninit successfully.");
    return 0;
}

/**
 * @brief  子线程的线程函数
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success, (void *)-1:failed
 */
void *c_listen_thread::work_thread_proc(void *p_instance)
{
    if (p_instance == NULL)
    {
        ERROR_LOG("p_instance=%p", p_instance);
        return (void *)-1;
    }

    c_listen_thread *p_listen_thread = (c_listen_thread *)p_instance;

    int rcv_len = 0;
    char data_buf[OA_MAX_UDP_MESSAGE_LEN];
    *p_listen_thread->m_p_thread_started = true;
    while (p_listen_thread->m_continue_working)
    {
        rcv_len = p_listen_thread->recv_data(data_buf, sizeof(data_buf));
        if (rcv_len > 0)
        {
            if(rcv_len > (int)sizeof(data_buf))
            {
                DEBUG_LOG("DEBUG: data buffer len[%lu] is smaller than rcv_len[%d].", sizeof(data_buf), rcv_len);
                rcv_len = sizeof(data_buf);
            }
            p_listen_thread->store_data(data_buf, rcv_len);
        } else
        {
            //如果recvfrom遇到EAGAIN错误睡眠1秒
            sleep(1);
        }
    }

    return (void *) 0;
}
