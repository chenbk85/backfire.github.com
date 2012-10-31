/**
 * =====================================================================================
 *       @file  alarm_thread.h
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
 
 
#ifndef ALARM_THREAD_H
#define ALARM_THREAD_H

#include <map>
#include "../lib/i_ring_queue.h"
#include "../lib/http_transfer.h"
#include "../defines.h"
#include "../proto.h"

typedef std::map<unsigned int, i_ring_queue*> ds_queue_map_t; 
const unsigned int MAX_ALARM_ATTEMP_COUNT = 10;     /**<post报警数据失败后的最大重试次数*/ 

///告警事件结构体
typedef struct {
    uint16_t data_len;//结构体实际长度
    int cmd_id;
    char host_ip[16];
    char key[33];               /**<metric_name;metric_arg的md5值*/
    char cmd_data[MAX_STR_LEN];
} alarm_event_t;

class c_alarm_thread
{
public:
    c_alarm_thread();
    ~c_alarm_thread();
    int init(const collect_conf_t *p_config, ds_queue_map_t *p_queue_map,
            const metric_alarm_vec_t *p_default_alarm_span,
            const metric_alarm_map_t *p_special_alarm_span);
    int uninit();
protected:
    /** 
     * @brief   线程主函数
     * @param   p_data  用户数据
     * @return  NULL success UNNULL failed
     */
    static void* alarm_main(void *p_data);

    /** 
     * @brief   清理m_notice_map
     * @param   无
     * @return  无
     */
    void clean_notice_map();

    /** 
     * @brief   获取相对状态
     * @param   status: 状态
     * @return  -1-failed, other-相对状态值,
     */
    int get_reverse_status(int status);

    /** 
     * @brief   获取alarm_cmd中key对应的value
     * @param   p_cmd: 待查找的字符串
     * @param   p_key: 键值
     * @return  NULL-failed, 否则-success
     */
    char *get_key_value(const char *p_cmd, const char *p_key);

    /** 
     * @brief   处理告警命令
     * @param   p_cmd: 待查找的字符串
     * @return  0-success, -1-failed
     */
    int handle_alarm_cmd(const char *p_cmd);

    /** 
     * @brief   发送告警命令
     * @param   p_next_send_span: 下次以命令待发间隔
     * @return  0-success, -1-failed
     */
    int send_alarm_cmd(uint32_t *p_next_send_span);

    /** 
     * @brief   获取下一次发送时间跨度
     * @param   alarm_cnt: 告警次数
     * @param   p_cmd: 告警命令
     * @return  非负数-时间跨度, -1-failed
     */
    int get_send_span(int alarm_cnt, const char *p_cmd);
private:
    typedef struct {
        int cmd_id;     /**<告警命令*/
        int alarm_cnt; /**<已告警次数*/
        time_t next_alarm_time; /**<下一次告警时间*/
        char alarm_key[33];/**<cmd_id;metric_name;metric_arg的md5值*/
        char alarm_cmd[MAX_STR_LEN]; /**<告警命令*/
    } alarm_notice_t;
    typedef struct std::vector<alarm_notice_t> notice_vec_t;
    typedef struct std::map<std::string, notice_vec_t*> notice_map_t;/**<<IP, notice_vec_t>键值对*/

private:
    bool m_inited;                /**<是否初始化标志*/
    pthread_t m_pid;              /**<线程id*/
    ds_queue_map_t * m_p_queue_map;/**<ds_queue_map_t对象指针*/
    const metric_alarm_vec_t *m_p_default_alarm_span; /**<默认报警间隔*/
    const metric_alarm_map_t *m_p_special_alarm_span; /**<设置的特殊报警间隔*/
    notice_map_t m_notice_map;    /**<告警消息map*/
    bool m_stop;
    uint32_t m_alarm_interval;
    char m_alarm_server_url[MAX_URL_LEN + 1];
    Chttp_transfer m_http_transfer;    /**<Chttp_transfer对象*/
    std::map<unsigned int, unsigned int> m_alarm_counter;/**<保存失败post数据的计数*/
};

inline void c_alarm_thread::clean_notice_map()
{
    if (m_notice_map.empty()) {
        return;
    }
    notice_map_t::iterator it = m_notice_map.begin();
    while (it != m_notice_map.end()) {
        it->second->clear();
        ++it;
    }

    return;
}

inline int c_alarm_thread::get_reverse_status(int status)
{
    int reverse_status = -1;
    switch (status) {
    case OA_HOST_METRIC:
        reverse_status = OA_HOST_METRIC_RECOVERY;
        break;
    case OA_HOST_METRIC_RECOVERY:
    case OA_HOST_METRIC_CLEANED:
        reverse_status = OA_HOST_METRIC;
        break;
    case OA_HOST_ALARM:
        reverse_status = OA_HOST_RECOVERY;
        break;
    case OA_HOST_RECOVERY:
        reverse_status = OA_HOST_ALARM;
        break;
    case OA_UPDATE_FAIL:
        reverse_status = OA_UPDATE_RECOVERY;
        break;
    case OA_UPDATE_RECOVERY:
        reverse_status = OA_UPDATE_FAIL;
        break;
    case OA_DS_DOWN:
        reverse_status = OA_DS_RECOVERY;
        break;
    case OA_DS_RECOVERY:
        reverse_status = OA_DS_DOWN;
        break;
    default:
        reverse_status = -1;
        break;
    }

    return reverse_status;
}

#endif
