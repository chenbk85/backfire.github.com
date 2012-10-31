/**
 * =====================================================================================
 *       @file  network_process.h
 *      @brief  将指定路径上的总计信息以xml的形式返回给上层   
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  12/19/2011 11:34:12 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_NETWORK_PROCESS_H_20111219
#define H_NETWORK_PROCESS_H_20111219

#include <pthread.h>

#include "../defines.h"
#include "../proto.h"
#include "../lib/net_client_impl.h"
#include "./network_proto.h"

class c_network_process
{
public:
    c_network_process();
    ~c_network_process();

    /**
     * @brief  初始化
     * @param  p_config: 指向config类的指针
     * @return 0:success, -1:failed
     */
    int init(network_conf_t *p_config, ip_port_map_t *p_ip_port);

    /**
     * @brief  初始化
     * @param  无
     * @return 0:success, -1:failed
     */
    int uninit();
protected:
    typedef struct {
        c_network_process *process;
        int index;//m_p_node_conn中的下标
    } work_proc_param_t;

    /**
     * @brief  获取错误描述
     * @param  err_code: 错误码
     * @return 错误描述指针
     */
    const char *get_error_desc(int err_code);

    /**
     * @brief  接受命令请求
     * @param  conn_fd: 客户端连接
     * @param  p_buf: 保存命令请求
     * @param  buf_len: 缓存大小
     * @return 0:success, -1:failed
     */
    int recv_cmd(int conn_fd, char *p_buf, int buf_len);

    /**
     * @brief  回复命令请求
     * @param  conn_fd: 客户端连接
     * @param  p_buf: 保存命令请求
     * @param  buf_len: 缓存大小
     * @return 0:success, -1:failed
     */
    int respond_cmd_by_sock(int conn_fd, const char *p_buf, int buf_len);
    int respond_cmd_by_http(const char *p_buf, int buf_len);

    /**
     * @brief  检查命令的有效性
     * @param  p_head_cmd: 命令请求
     * @return 0:success, -1:failed
     */
    int check_cmd_valid(pkg_head_t *p_head_cmd);

    /**
     * @brief  回复命令请求
     * @param  conn_fd: 客户端连接
     * @param  p_buf: 待回复信息
     * @param  buf_len: 缓存大小
     * @return 0:success, -1:failed
     */
    int check_rcv_node_head(const char *send_buff, const char *recv_buff);

    /**
     * @brief  转发命令给node, 并获取返回结果
     * @param  p_head_cmd: 命令请求
     * @param  p_buff: node命令执行结果
     * @param  p_buff_len: 缓存实际大小
     * @return 0:success, >0:failed
     */
    uint32_t handle_cmd(const char *p_head_cmd, char *p_buff, int *p_buff_len, int index);
    uint32_t handle_db_cmd(const char *p_rcv_cmd, char *p_buff, int *p_buff_len,  int index);

    /**
     * @brief  子线程的处理函数 
     * @param  p_data: 指向当前对象的this指针
     * @return (void *)0:success, (void *)-1:failed
     */
    static void *work_thread_proc(void *p_data);

private:
    bool m_inited;
    bool m_continue_working;
    int m_listen_fd;
    network_conf_t *m_p_config;
    c_net_client_impl **m_p_node_conn;
    ip_port_map_t *m_p_ip_port;
    int m_thread_cnt;
    int m_pkg_num;//用于记录接受包的数量

    pthread_mutex_t m_accept_mutex;
    std::vector<pthread_t> m_work_thread_id_vec;

};

/**
 * @brief  获取错误描述
 * @param  err_code: 错误码
 * @return 错误描述指针
 */
inline const char *c_network_process::get_error_desc(int err_code)
{
    //RESULT_OK  = 0,
    //RESULT_ESYSTEM    = 0x1001, /**<系统错误*/
    //RESULT_ECOMMAND   = 0x1002, /**<cmd_id无效*/
    //RESULT_EUNCONNECT = 0x1003, /**<node连接不上*/
    //RESULT_ETIMEOUT   = 0x1004, /**<node处理超时*/
    //RESULT_ENOMEMORY  = 0x1005, /**<node返回包过长*/
    //RESULT_ENODEHEAD  = 0x1006, /**<node返回包头错误*/
    const static char *error_desc[] = {
        "unknow error",
        "OK",
        "system error",
        "wrong command id",
        "cannot connect to node",
        "command handle timeout",
        "package too long",
        "node return package head error",
    };

    int index = 0;
    switch (err_code) {
    case 0:
    case 0x1001:
    case 0x1002:
    case 0x1003:
    case 0x1004:
    case 0x1005:
    case 0x1006:
        index = err_code % 0x1000 + 1;
        break;
    default:
        index = 0;
        break;
    }

    return error_desc[index];
}

/**
 * @brief  启动network进程
 * @param  fd: 与父进程通信的fd
 * @param  p_config: 指向config类的指针
 * @param  p_ip_port: <IP, port>列表
 * @return 0:success, -1:failed
 */
int network_process_run(int fd, network_conf_t *p_config, ip_port_map_t *p_ip_port);

#endif
