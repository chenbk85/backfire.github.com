/**
 * =====================================================================================
 *       @file  export_thread.cpp
 *      @brief  convert collected data into xml and export to oa_head
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  08/25/2010 09:57:16 AM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  luis (程龙), luis@taomee.com
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:38:31
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <signal.h>

#include "../defines.h"
#include "../lib/log.h"
#include "../lib/stack.h"
#include "./export_thread.h"
#include "./network_process.h"

using namespace std;

typedef union {
    uint32_t count;
    char     str[OA_MAX_STR_LEN];
} stack_node_e;

/**
 * @struct 遍历hash表的回调函数参数
 */
typedef struct {
    heartbeat_val_t ht_val;
    bool up_fail;
    time_t fail_time;
} node_header_t;

/**
 * @struct 遍历hash表的回调函数参数
 */
typedef struct {
    int sockfd;  /**<send socket fd*/
    bool print;
} oa_foreach_arg_t;

/**
 * @struct 清理hash表的回调函数参数
 */
typedef struct {
    time_t rcv_time;  /**<metric保存时间*/
} oa_cleanup_arg_t;

/**
 * @struct 自定义的metric发送的内容
 */
typedef struct {
    char arg[OA_MAX_STR_LEN];  /**< 参数 */
    int dmax;   /**< metric hard time limit */
    int tmax;   /**< metric soft time limit */
    char units[OA_MAX_STR_LEN];/**< 返回值的单位的名称 */
    char fmt[OA_MAX_STR_LEN];  /**< 返回值显示的格式 */
    char desc[OA_MAX_STR_LEN]; /**< 返回值显示的格式 */
    char slope[OA_MAX_STR_LEN];/**< 存储在rrd的类型 */
    char sum_formula[OA_MAX_STR_LEN];    /**< metric进行汇总时的公式 */
    value_type_t type;    /**< 返回值的类型 */
    value_t value;   /**< metric的值 */
    int alarm_type; /**< 1为只写rrd,2为只报警，3为即写rrd又报警 */
    char alarm_formula[OA_MAX_STR_LEN];  /**< 报警格式(alarm_type为2、3的时候才有)*/
} metric_custom_store_t;

static bool m_inited;
static bool m_continue_working;
static pthread_t m_work_thread_id;
static c_listen_thread * m_p_listen_thread;

static int m_dmax;
static int m_time_interval;/**<心跳时间间隔*/
static char m_cluster_name[OA_MAX_STR_LEN];

static i_config *m_p_config;

static c_hash_table *m_p_hash_table;

static c_stack<stack_node_e> clean_list_heart;
static c_stack<stack_node_e> clean_list;

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
    do{\
        if(m_p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
            ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
        }\
    } while(0)

/**
 * @brief  清理过期metric信息的回调函数
 * @param  key: 传递给回调函数的hash键值
 * @param  val: key对应的val
 * @param  arg: 交互参数
 * @return 0-不满足条件， -1-有满足条件的，-2-出错
 */
int clean_metric_recall_before(const char * key, uint32_t value_length, const void * value_pointer, uint8_t level, void *arg)
{
    if(key == NULL || value_pointer == NULL || arg == NULL) {
        ERROR_LOG("key=%p[%s] value_pointer=%p[%s] arg=%p[%s]", key, key==0?"":key, value_pointer, value_pointer==0?"":(const char *)value_pointer, arg, arg==0?"":(char *)arg);
        return -1;
    }

    oa_cleanup_arg_t *p_cleanup_arg = (oa_cleanup_arg_t *) arg;

    int dmax = 0;

    stack_node_e node = {0};
    int metric_type = 0;

    if(level == 0) {
        node.count = 0;
        clean_list.push(node);
        dmax = m_dmax;
    }
    /* else if(level == 1) {
       metric_type = atoi(key);
       switch(metric_type) {
       case OA_CLEANUP_TYPE:
       case OA_UP_FAIL_TYPE:
       case OA_HEART_BEAT_TYPE:
       break;
       default:
       XDR x_metric;
       xdrmem_create(&x_metric, (char *)value_pointer + sizeof(metric_val_t),
       value_length - sizeof(metric_val_t), XDR_DECODE);
       char arg[OA_MAX_STR_LEN] = {0};
       char *p_arg = arg;
       if (!xdr_string(&x_metric, &p_arg, sizeof(arg))) {
       ERROR_LOG("ERROR: xdr_string() to user-defined arg, and clean it force.");
       clean_force = true;
       break;
       }
       if (!xdr_int(&x_metric, &dmax)) {
       ERROR_LOG("ERROR: xdr_int() to user-defined dmax, and clean it force.");
       dmax = 0;
       clean_force = true;
       }
       break;
       }
       }*/
    /*
       time_t now = time(NULL);
       time_t last_rcv_time;
       time_t tn = 0;
       if(value_pointer != NULL) {
       last_rcv_time = *(time_t *)value_pointer;
       tn = now - last_rcv_time;
       }
       p_cleanup_arg->rcv_time = last_rcv_time;
       if (clean_force || (dmax > 0 && tn > dmax)) {//信息过期
       if(level == 0) {
       strcpy(node.str, key);
       clean_list_heart.push(node);
       return OA_METRIC_EXPIRE;
       }
       if(level == 1) {
       if(metric_type == OA_HEART_BEAT_TYPE
       || metric_type == OA_CLEANUP_TYPE
       || metric_type == OA_UP_FAIL_TYPE) {
       return OA_METRIC_NORMAL;
       } else {
       uint32_t count = clean_list.pop()->count;
       strcpy(node.str, key);
       clean_list.push(node);
       node.count = count+1;
       clean_list.push(node);
       return OA_METRIC_EXPIRE;
       }
       }
       }
       return OA_METRIC_NORMAL;
       */

    time_t now = time(NULL);
    time_t last_rcv_time;
    time_t tn = 0;
    if(value_pointer != NULL) {
        last_rcv_time = *(time_t *)value_pointer;
        tn = now - last_rcv_time;
    }
    p_cleanup_arg->rcv_time = last_rcv_time;

    if(level == 0) {
        if (dmax > 0 && tn > dmax) {
            strcpy(node.str, key);
            clean_list_heart.push(node);
            return OA_METRIC_EXPIRE;
        }
    }
    if(level == 1) {
        if(metric_type == OA_HEART_BEAT_TYPE
                || metric_type == OA_CLEANUP_TYPE
                || metric_type == OA_UP_FAIL_TYPE) {
            return OA_METRIC_NORMAL;
        }
        metric_val_t * p_mv = (metric_val_t *)value_pointer;
        if(tn > (p_mv->collect_interval<<2)) {
            p_mv->cleaned = 1;
            uint32_t count = clean_list.pop()->count;
            strcpy(node.str, key);
            clean_list.push(node);
            node.count = count+1;
            clean_list.push(node);
            return OA_METRIC_EXPIRE;
        }
    }
    return OA_METRIC_NORMAL;
}

/**
 * @brief  清理过期metric信息的回调函数
 * @param  key: 传递给回调函数的hash键值
 * @param  val: key对应的val
 * @param  arg: 交互参数
 * @return 0-不满足条件， -1-有满足条件的，-2-出错
 */
int clean_metric_recall_after(const char * key, uint32_t value_length, const void * value_pointer, uint8_t level, void *arg)
{
    if(key == NULL || value_pointer == NULL || arg == NULL) {
        ERROR_LOG("key=%p[%s] value_pointer=%p[%s] arg=%p[%s]", key, key==0?"":key, value_pointer, value_pointer==0?"":(const char *)value_pointer, arg, arg==0?"":(char *)arg);
        return -1;
    }

    stack_node_e node = {0};

    if(level == 0) {
        node.count = 0;
        strcpy(node.str, key);
        clean_list.push(node);
    }

    return OA_METRIC_NORMAL;
}

/**
 * @brief  清理过期metric信息
 * @param  无
 * @return 0-success, -1-failed
 */
int clean_expire_metric()
{
    oa_cleanup_arg_t cleanup_arg = {0};
    void *p_cleanup_arg = (void *)&cleanup_arg;
    c_data value;
    //const char * pp_hash_key[2];

    clean_list_heart.clean();
    clean_list.clean();

    m_p_hash_table->foreach(&clean_metric_recall_before, p_cleanup_arg, &clean_metric_recall_after, p_cleanup_arg);

    if(m_dmax > 0) {
        while(!clean_list_heart.empty()) {
            const char * host = clean_list_heart.pop()->str;
            value = m_p_hash_table->search(host);
            if(value.not_null()) {
                time_t now = time(NULL);
                time_t last_rcv_time = *(time_t *)value.get_value();
                time_t tn = now - last_rcv_time;
                if(m_dmax > 0 && tn > m_dmax) {
                    DEBUG_LOG("remove %s", host);
                    m_p_hash_table->remove(host);
                }
            } else {
                ERROR_LOG("%s is null", host);
            }
        }
    }
    /*
       const char * host;
       const char * metric;
       uint32_t metric_count = 0;

       int dmax = 0;
       while(!clean_list.empty()) {
       host = clean_list.pop()->str;
       pp_hash_key[0] = host;
       metric_count = clean_list.pop()->count;
       for(uint32_t i=0; i<metric_count; i++) {
       metric = clean_list.pop()->str;
       pp_hash_key[1] = metric;
       value = m_p_hash_table->search(2, pp_hash_key);
       if(value.not_null()) {
       XDR x_metric;
       xdrmem_create(&x_metric, (char *)value.get_value() + sizeof(metric_val_t),
       value.get_length() - sizeof(metric_val_t), XDR_DECODE);
       char arg[OA_MAX_STR_LEN] = {0};
       char *p_arg = arg;
       if (!xdr_string(&x_metric, &p_arg, sizeof(arg)) && !xdr_int(&x_metric, &dmax)) {
       time_t now = time(NULL);
       time_t last_rcv_time = *(time_t *)value.get_value();
       time_t tn = now - last_rcv_time;
       if(dmax > 0 && tn > dmax) {
       DEBUG_LOG("remove %s %s", host, metric);
       m_p_hash_table->remove(2, pp_hash_key);
       }
       } else {
       DEBUG_LOG("remove %s %s", host, metric);
       m_p_hash_table->remove(2, pp_hash_key);
       }
       } else {
       ERROR_LOG("%s %s is null", host, metric);
       }
       }
       }
       */
    return 0;
}

/**
 * @brief  通过socket发送给定的字符串
 * @param  fd: 已经建立连接的socket描述符
 * @param  fmt: 需要发送的字符串
 * @return 0:success, -1:failed
 */
int xml_print(int fd, const char *fmt, ...)
{
    char buf[OA_MAX_BUF_LEN] = {0};

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);

    int bytes_left = strlen(buf);
    const char *ptr = buf;

    time_t start = time(NULL);
    while (bytes_left) {
        if (time(NULL) - start > OA_SOCKET_TIMEOUT) {
            ERROR_LOG("ERROR: send(...) timeout");
            return -1;
        }

        int bytes_send = send(fd, ptr, bytes_left, 0);
        if (bytes_send > 0) {
            bytes_left -= bytes_send;
            ptr += bytes_send;
        } else {
            // 发送出错
            if (errno == EAGAIN || errno == EINTR) {
                usleep(100);//待定
                continue;
            } else {
                ERROR_LOG("ERROR: send(...) error,errno[%d]:%s", errno, strerror(errno));
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief  xml特殊字符转义
 * @param  src: 待转义字符串
 * @param  src_len: 源字符串长度
 * @param  dst: 保存转义后的字符串
 * @param  dst_len: 目标字符串长度
 * @return 0:success, -1:failed
 */
int transfer_to_xml(const char *src, int src_len, char *dst, int dst_len)
{
    if (NULL == src || NULL == dst || src_len > dst_len) {
        ERROR_LOG("src=%p dst=%p src_len=%d [>] dst_len=%d", src, dst, src_len, dst_len);
        return -1;
    }

    int left_len = dst_len - 1;
    for (int i = 0; i < src_len && left_len > 0; ++i) {
        switch(src[i]) {
            case '<':
                if (left_len >= 4) {
                    strncpy(dst, "&lt;", 4);
                    dst += 4;
                    left_len -= 4;
                } else {
                    left_len = 0;
                }
                break;
            case '>':
                if (left_len >= 4) {
                    strncpy(dst, "&gt;", 4);
                    dst += 4;
                    left_len -= 4;
                } else {
                    left_len = 0;
                }
                break;
            case '\"':
                if (left_len >= 6) {
                    strncpy(dst, "&quot;", 6);
                    dst += 6;
                    left_len -= 6;
                } else {
                    left_len = 0;
                }
                break;
            case '\'':
                if (left_len >= 6) {
                    strncpy(dst, "&apos;", 6);
                    dst += 6;
                    left_len -= 6;
                } else {
                    left_len = 0;
                }
                break;
            case '&':
                if (left_len >= 5) {
                    strncpy(dst, "&amp;", 5);
                    dst += 5;
                    left_len -= 5;
                } else {
                    left_len = 0;
                }
                break;
            default:
                if (src[i] >= 32 && src[i] <= 126) {
                    *dst++ = src[i];
                }
                break;
        }
    }

    return 0;
}

/**
 * @brief  解析OA_VALUE的值和类型
 * @param  px_metric: 解析的xdr流
 * @param  type: oa_value类型
 * @param  fmt: oa_value显示的格式
 * @param  p_metric_value: oa_value值
 * @param  p_value_str: oa_value值的字符串表示(调用者分配空间)
 * @param  p_value_type: oa_value类型的字符串表示(调用者分配空间)
 * @return 0:success, -1:failed
 */
int parse_oa_value(XDR *px_metric, const value_type_t type, const char *fmt,
        value_t *p_metric_value, char *p_value_str, char *p_value_type)
{
    if(px_metric == NULL || fmt == NULL || p_metric_value == NULL || p_value_str == NULL || p_value_type == NULL) {
        ERROR_LOG("px_metric=%p fmt=%p p_metric_value=%p p_value_str=%p p_value_type=%p", px_metric, fmt, p_metric_value, p_value_str, p_value_type);
        return -1;
    }
    char *p_tmp_str = NULL;
    switch (type) {
        case OA_VALUE_STRING:
            p_tmp_str = p_metric_value->value_str;
            if (!xdr_string(px_metric, &p_tmp_str, OA_MAX_STR_LEN)) {
                ERROR_LOG("ERROR: xdr_string() OA_VALUE_STRING.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_str);
            sprintf(p_value_type, "%s", "string");
            break;
        case OA_VALUE_UNSIGNED_SHORT:
            if (!xdr_u_short(px_metric, &p_metric_value->value_ushort)) {
                ERROR_LOG("ERROR: xdr_u_short() to OA_VALUE_UNSIGNED_SHORT.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_ushort);
            sprintf(p_value_type, "%s", "uint16");
            break;
        case OA_VALUE_SHORT:
            if (!xdr_short(px_metric, &p_metric_value->value_short)) {
                ERROR_LOG("ERROR: xdr_u_short() to OA_VALUE_SHORT.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_short);
            sprintf(p_value_type, "%s", "int16");
            break;
        case OA_VALUE_UNSIGNED_INT:
            if (!xdr_u_int(px_metric, &p_metric_value->value_uint)) {
                ERROR_LOG("ERROR: xdr_u_int() to OA_VALUE_UNSIGNED_INT.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_uint);
            sprintf(p_value_type, "%s", "uint32");
            break;
        case OA_VALUE_INT:
            if (!xdr_int(px_metric, &p_metric_value->value_int)) {
                ERROR_LOG("ERROR: xdr_int() to OA_VALUE_INT.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_int);
            sprintf(p_value_type, "%s", "int32");
            break;
        case OA_VALUE_UNSIGNED_LONG_LONG:
            if (!xdr_u_longlong_t(px_metric, (u_quad_t*)&p_metric_value->value_ull)) {
                ERROR_LOG("ERROR: xdr_u_longlong_t() failed.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_ull);
            sprintf(p_value_type, "%s", "uint64");
            break;
        case OA_VALUE_LONG_LONG:
            if (!xdr_longlong_t(px_metric, (quad_t*)&p_metric_value->value_ll)) {
                ERROR_LOG("ERROR: xdr_longlong_t() failed.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_ll);
            sprintf(p_value_type, "%s", "int64");
            break;
        case OA_VALUE_FLOAT:
            if (!xdr_float(px_metric, &p_metric_value->value_f)) {
                ERROR_LOG("ERROR: xdr_float() to OA_VALUE_FLOAT.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_f);
            sprintf(p_value_type, "%s", "float");
            break;
        case OA_VALUE_DOUBLE:
            if (!xdr_double(px_metric, &p_metric_value->value_d)) {
                ERROR_LOG("ERROR: xdr_double() to OA_VALUE_DOUBLE.");
                return -1;
            }
            sprintf(p_value_str, fmt, p_metric_value->value_d);
            sprintf(p_value_type, "%s", "double");
            break;
        default:
            ERROR_LOG("ERROR: metric value type[%d] error.", type);
            return -1;
    }

    return 0;
}

/**
 * @brief  遍历metric信息的回调函数
 * @param  key: 传递给回调函数的hash键值
 * @param  val: key对应的val
 * @param  arg: 交互参数
 * @return 0-success, -1-failed
 */
int print_metric_recall_before(const char * key, uint32_t value_length, const void * value_pointer, uint8_t level, void *arg)
{
    if(key == NULL || value_pointer == NULL || arg == NULL) {
        ERROR_LOG("key=%p[%s] value_pointer=%p[%s] arg=%p[%s]", key, key==0?"":key, value_pointer, value_pointer==0?"":(const char *)value_pointer, arg, arg==0?"":(char *)arg);
        return -1;
    }

    oa_foreach_arg_t *p_metric_arg = (oa_foreach_arg_t *) arg;
    if (p_metric_arg->sockfd < 0 ) {
        ERROR_LOG("ERROR: arg is invalid.");
        return -1;
    }
    int sockfd = p_metric_arg->sockfd;
    time_t now = time(NULL);
    time_t tn = 0;

    if(level == 0)
    {
        //host信息


        // 对于每个host来说，默认都是要输出的
        // 除非有问题
        p_metric_arg->print = true;

        if(value_pointer == NULL) {
            p_metric_arg->print = false;
            ERROR_LOG("in print before[key=%s]:value_pointer == null", key);
            return -1;
        }
        heartbeat_val_t *p_heart = (heartbeat_val_t *)value_pointer;

        time_t heart_beat = p_heart->heart_beat;
        time_t tmax = m_time_interval;//待定  //XXX 可以考虑直接用心跳的时间间隔by luis
        bool up_fail = p_heart->up_fail;

        struct in_addr ia_tmp;
        ia_tmp.s_addr = p_heart->host_ip;
        char ip_str[16] = {0};
        if (inet_ntop(AF_INET, (void *)&ia_tmp, ip_str, sizeof(ip_str)) == NULL) {
            ERROR_LOG("ERROR: inet_ntop(%u) failed: %s", p_heart->host_ip, strerror(errno));
            p_metric_arg->print = false;
            return -1;
        }

        time_t now = time(NULL);
        if (now < p_heart->rcv_time) {
            ERROR_LOG("ERROR: host[%s] last_rcv_time[%u] > now[%u]",
                    ip_str, (u_int)p_heart->rcv_time, (u_int)now);
            p_metric_arg->print = false;
            return -1;
        }
        time_t tn = now - p_heart->rcv_time;

        int result = xml_print(sockfd, "<HOST NAME=\"%s\" IP=\"%s\""
                " TN=\"%d\" TMAX=\"%d\" DMAX=\"%d\" NODE_STARTED=\"%d\" UP_FAIL=\"%s\">\n",
                key, ip_str, tn, tmax, m_dmax, heart_beat, up_fail ? "YES" : "NO");
        if (result != 0) {
            ERROR_LOG("ERROR: xml_print(%s[%s] header) failed.", key, ip_str);
        }
        if (up_fail == true) {//更新失败信息成功发送一次之后便清理
            p_heart->up_fail = false;
            DEBUG_LOG("DEBUG: clean [%s:%s] update failed metric.", key, ip_str);
        }
        if (tn > 4 * tmax) 
        {
            p_metric_arg->print = false;
            DEBUG_LOG("host[%s:%s] info: tn[%u] > 4 * tmax[%u]", key, ip_str, (u_int)tn, (u_int)tmax);
            return 0;
        }

        return 0;
    }
    else if(level == 1)
    {
        if(!p_metric_arg->print) {
            return 0;
        } else {
            char metric_name[OA_MAX_STR_LEN] = {0};
            int metric_id;
            sscanf(key, "%d:%s", &metric_id, metric_name);

            metric_val_t *p_metric_val = (metric_val_t *)value_pointer;
            XDR x_metric;
            xdrmem_create(&x_metric, (char *)p_metric_val + sizeof(metric_val_t), value_length - sizeof(metric_val_t), XDR_DECODE);
            time_t last_rcv_time = p_metric_val->rcv_time;
            if (last_rcv_time > now) {
                ERROR_LOG("ERROR: metric_name[%s] last_rcv_time[%u] > now[%u]",
                        metric_name, (u_int)last_rcv_time, (u_int)now);
                return 0;
            }
            int collect_interval = p_metric_val->collect_interval;
            if (collect_interval < 0) {
                ERROR_LOG("ERROR: metric_name[%s] collect_interval[%d] < 0", metric_name, collect_interval);
                return 0;
            }
            tn = now - last_rcv_time;
            char value_str[OA_MAX_STR_LEN] = {0};
            char value[OA_MAX_STR_LEN] = {0};
            char desc[OA_MAX_STR_LEN] = {0};
            char value_type[10] = {0};
            char *tmp_str = NULL;

            switch (metric_id) {
                case OA_CLEANUP_TYPE:
                case OA_UP_FAIL_TYPE:
                case OA_HEART_BEAT_TYPE:
                    return 0;
                default:
                    metric_custom_store_t custom_info;
                    memset(&custom_info, 0, sizeof(custom_info));
                    tmp_str = custom_info.arg;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined arg.");
                        return -1;
                    }
                    if (!xdr_int(&x_metric, &custom_info.dmax)) {
                        ERROR_LOG("ERROR: xdr_int() to user-defined dmax.");
                        return -1;
                    }
                    if (!xdr_int(&x_metric, &custom_info.tmax)) {
                        ERROR_LOG("ERROR: xdr_int() to user-defined tmax.");
                        return -1;
                    }
                    tmp_str = custom_info.units;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined units.");
                        return -1;
                    }
                    tmp_str = custom_info.fmt;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined fmt.");
                        return -1;
                    }
                    tmp_str = custom_info.desc;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined desc.");
                        return -1;
                    }
                    if (0 != transfer_to_xml(custom_info.desc, strlen(custom_info.desc), desc, sizeof(desc))) {
                        ERROR_LOG("ERROR: transfer_to_xml() to user-defined desc.");
                        return -1;
                    }
                    tmp_str = custom_info.slope;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined slope.");
                        return -1;
                    }
                    tmp_str = custom_info.sum_formula;
                    if (!xdr_string(&x_metric, &tmp_str, OA_MAX_STR_LEN)) {
                        ERROR_LOG("ERROR: xdr_string() to user-defined sum_formula.");
                        return -1;
                    }
                    if (!xdr_enum(&x_metric, (enum_t *)&custom_info.type)) {
                        ERROR_LOG("ERROR: xdr_enum() to user-defined value type.");
                        return -1;
                    }
                    value_t ctv;
                    if (parse_oa_value(&x_metric, custom_info.type,
                                custom_info.fmt, &ctv, value_str, value_type) != 0) {
                        ERROR_LOG("ERROR: parse_oa_value(user-defined metric) failed.");
                        return -1;
                    }
                    if (0 != transfer_to_xml(value_str, strlen(value_str), value, sizeof(value))) {
                        ERROR_LOG("ERROR: transfer_to_xml() to user-defined value.");
                        return -1;
                    }
                    if (!xdr_int(&x_metric, &custom_info.alarm_type)) {
                        ERROR_LOG("ERROR: xdr_int() to user-defined alarm_type.");
                        return -1;
                    }
                    if (custom_info.alarm_type < OA_LIMIT_MIN || custom_info.alarm_type > OA_LIMIT_MAX) {
                        ERROR_LOG("Metric[%s] alarm type error: %d, should in [%d, %d].",
                                metric_name, custom_info.alarm_type, OA_LIMIT_MIN, OA_LIMIT_MAX);
                    }
                    return xml_print(p_metric_arg->sockfd,
                            "<METRIC NAME=\"%s\" ARG=\"%s\" VAL=\"%s\" TYPE=\"%s\" UNITS=\"%s\""
                            " TN=\"%d\" TMAX=\"%d\" DMAX=\"%d\" SLOPE=\"%s\" MT=\"%d\" CI=\"%d\" CLEANED=\"%d\"></METRIC>\n",
                            metric_name, custom_info.arg, value, value_type, custom_info.units, tn,
                            custom_info.tmax, custom_info.dmax, custom_info.slope, custom_info.alarm_type,
                            collect_interval, p_metric_val->cleaned);
            }
        }
    }
    return -1;
}

/**
 * @brief  遍历metric信息的回调函数
 * @param  key: 传递给回调函数的hash键值
 * @param  val: key对应的val
 * @param  arg: 交互参数
 * @return 0-success, -1-failed
 */
int print_metric_recall_after(const char * key, uint32_t value_length, const void * value_pointer, uint8_t level, void *arg)
{
    if(key == NULL || value_pointer == NULL || arg == NULL) {
        ERROR_LOG("key=%p[%s] value_pointer=%p[%s] arg=%p[%s]", key, key==0?"":key, value_pointer, value_pointer==0?"":(const char *)value_pointer, arg, arg==0?"":(char *)arg);
        return -1;
    }

    oa_foreach_arg_t *p_metric_arg = (oa_foreach_arg_t *) arg;
    if (p_metric_arg->sockfd < 0) {
        ERROR_LOG("ERROR: arg is invalid.");
        return -1;
    }

    if(level == 0) {
        if(xml_print(p_metric_arg->sockfd, "</HOST>\n") != 0) {
            if(value_pointer == NULL) {
                ERROR_LOG("in print after[key=%s]:value_pointer == null", key);
                return -1;
            }
            heartbeat_val_t *p_heart = (heartbeat_val_t *)value_pointer;
            struct in_addr ia_tmp;
            ia_tmp.s_addr = p_heart->host_ip;
            char ip_str[16] = {0};
            inet_ntop(AF_INET, (void *)&ia_tmp, ip_str, sizeof(ip_str));
            ERROR_LOG("ERROR: [%s:%s] xml_print(</HOST>) failed.", key, ip_str);
            return -1;
        }
    }

    return 0;
}

/**
 * @brief  发送xml给oa_head
 * @param  sockfd: 发送scoket fd
 * @return 0-success, -1-failed
 */
int send_xml_info(int sockfd)
{
    if (sockfd < 0) {
        ERROR_LOG("ERROR: wrong send sockfd: %d.", sockfd);
        return -1;
    }

    int ret_code = 0;
    ret_code = xml_print(sockfd, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n"
            "<OA_XML VERSION=\"%s\">\n", OA_VERSION);
    if (0 != ret_code) {
        ERROR_LOG("ERROR: xml_print(xml header) failed.");
        return -1;
    }
    ret_code = xml_print(sockfd, "<CLUSTER NAME=\"%s\" LOCALTIME=\"%d\" HOST_CHANGE=\"%s\">\n", m_cluster_name,
            (int)time(NULL),  m_p_listen_thread->reset_host_add() ? "YES" : "NO");
    if (0 != ret_code) {
        ERROR_LOG("ERROR: xml_print(CLUSTER header) failed.");
        return -1;
    }

    clean_list_heart.clean();
    oa_foreach_arg_t foreach_arg = {sockfd, true};
    m_p_hash_table->foreach(&print_metric_recall_before, &foreach_arg, &print_metric_recall_after, &foreach_arg);

    if(xml_print(sockfd, "</CLUSTER>\n</OA_XML>\n") != 0) {
        ERROR_LOG("ERROR: xml_print(</CLUSTER>) failed.");
        return -1;
    }

    //clean
    //const char * host;
    //const char * metric;
    //uint32_t metric_count = 0;
    //c_data value;
    //const char * pp_hash_key[2];

    //while(!clean_list.empty()) {
    //    host = clean_list.pop()->str;
    //    pp_hash_key[0] = host;
    //    metric_count = clean_list.pop()->count;
    //    DEBUG_LOG("metric_count[%u] in clean_list of host[%s].", metric_count, host);
    //    for(uint32_t i = 0; i < metric_count; i++) {
    //        metric = clean_list.pop()->str;
    //        pp_hash_key[1] = metric;
    //        value = m_p_hash_table->search(2, pp_hash_key);
    //        if(value.not_null()) {                
    //            DEBUG_LOG("remove %s %s", host, metric);
    //            m_p_hash_table->remove(2, pp_hash_key);
    //        } else {
    //            ERROR_LOG("%s %s is null", host, metric);
    //        }
    //    }
    //}
    return 0;
}

void export_sighandler(int sig)
{
    if(m_continue_working) {
        recv_cmd_t recv;
        oa_cmd_t *p_oa_cmd;
        while(!m_export_queue.empty()) {
            m_export_queue.pop(&recv);
            p_oa_cmd = (oa_cmd_t *)recv.data;
            if(p_oa_cmd->cmd_id == OA_CMD_GET_METRIC_INFO) {
                clean_expire_metric();
                send_xml_info(recv.peer_socket);
                close(recv.peer_socket);
            }
        }
    }
}

/**
 * @brief  子线程的工作函数
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success, (void *)-1:failed
 */
void * work_thread_proc(void *p_data)
{
    //收到信号后，就发送数据
    signal(SIGUSR2, export_sighandler);
    //一直等待，直到退出
    while (m_continue_working) {
        sleep(1);
    }
    return (void *)0;
}

/**
 * @brief  初始化函数
 * @param  p_config: 配置对象指针,获取相关配置信息
 * @param  p_listen_ip: export线程监听的IP
 * @param  p_listen_thread: listen线程,用于获取保存在内存中的metric信息
 * @param  p_heartbeat: 存储心跳实例
 * @param  p_buildin: 存储内置metric实例
 * @param  p_custom: 存储用户定义metric实例
 * @param  p_metric_info_map: 内置类型基本信息
 * @return 0-success, -1-failed
 */
int export_init(i_config *p_config, const char *p_listen_ip, c_listen_thread *p_listen_thread,
        c_hash_table *p_hash_table)
{
    if (m_inited) {
        ERROR_LOG("ERROR: c_export_thread has been inited.");
        return -1;
    }
    if (!p_config || !p_listen_ip || !p_listen_thread || !p_hash_table) {
        ERROR_LOG("p_config=%p p_listen_ip=%p p_listen_thread=%p p_hash_table=%p", p_config, p_listen_ip, p_listen_thread, p_hash_table);
        return -1;
    }
    m_p_config = p_config;
    m_p_listen_thread = p_listen_thread;
    m_p_hash_table = p_hash_table;

    do {
        //设置dmax的值
        char dmax[10] = {0};
        GET_CONFIG("node_info", "host_dmax", dmax);
        m_dmax = atoi(dmax);
        m_dmax = m_dmax < 0 ? 0 : m_dmax;
        //设置心跳时间间隔
        char time_interval[10] = {0};
        GET_CONFIG("node_info", "heartbeat_interval", time_interval);
        m_time_interval = atoi(time_interval);
        if (m_time_interval <= 0) {
            ERROR_LOG("ERROR: worng time_interval[%s] of heartbeat.", time_interval);
            break;
        }
        //获取机器所在cluster名称
        GET_CONFIG("node_info", "cluster_name", m_cluster_name);

        m_continue_working = true;
        int result = pthread_create(&m_work_thread_id, NULL, work_thread_proc, NULL);
        if (result != 0) {
            ERROR_LOG("ERROR: pthread_create() failed.");
            m_work_thread_id = 0;
            break;
        }
        m_inited = true;
    } while (false);

    if (m_inited) {
        DEBUG_LOG("export_thread init successfully.");
        return 0;
    } else {///初始化失败则重置成员变量
        m_continue_working = false;

        m_dmax = 0;
        m_time_interval = 0;
        m_p_listen_thread = NULL;
        m_p_config = NULL;
        m_p_hash_table = NULL;
        return -1;
    }
}

/**
 * @brief  反初始化
 * @param  无
 * @return 0-success, -1-failed
 */
int export_uninit()
{
    if (!m_inited) {
        return -1;
    }
    assert(m_work_thread_id != 0);

    m_continue_working = false;
    pthread_join(m_work_thread_id, NULL);
    m_work_thread_id = 0;

    m_dmax = 0;
    m_time_interval = 0;
    m_p_config = NULL;
    m_p_listen_thread = NULL;
    m_inited = false;

    DEBUG_LOG("export thread uninit successfully.");
    return 0;
}

/**
 * @brief  设置oa_node信息
 * @param  p_config: 配置对象指针,获取相关配置信息
 * @return 0-success, -1-failed
 */
void set_node_info(i_config *p_config)
{
    if(p_config == NULL) {
        ERROR_LOG("p_config=%p", p_config);
        return ;
    }
    m_p_config = p_config;
    char dmax[10] = {0};
    GET_CONFIG("node_info", "host_dmax", dmax);
    m_dmax = atoi(dmax);
    m_dmax = m_dmax < 0 ? 0 : m_dmax;
    //设置心跳时间间隔
    char time_interval[10] = {0};
    int  i_time_interval;
    GET_CONFIG("node_info", "heartbeat_interval", time_interval);
    i_time_interval = atoi(time_interval);
    if (i_time_interval <= 0) {
        ERROR_LOG("ERROR: worng time_interval[%s] of heartbeat.", time_interval);
    } else {
        m_time_interval = i_time_interval;
    }
    //获取机器所在cluster名称
    char cluster_name[OA_MAX_STR_LEN] = {0};
    GET_CONFIG("node_info", "cluster_name", cluster_name);
    if(cluster_name[0] != '0') {
        strcpy(m_cluster_name, cluster_name);
    }
}

/**
 * @brief  获得本线程id
 * @param  无
 * @return 本线程id
 */
pthread_t get_export_pthread_id()
{
    return m_work_thread_id;
}

#undef GET_CONFIG
