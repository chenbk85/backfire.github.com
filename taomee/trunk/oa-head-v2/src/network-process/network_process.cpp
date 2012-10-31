/**
 * =====================================================================================
 *       @file  network_process.cpp
 *      @brief 
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  12/19/2011 13:57:16 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "../lib/log.h"
#include "../lib/utils.h"
#include "../lib/http_transfer.h"
#include "../proto.h"
#include "./network_process.h"

using namespace std;

/**
 * @brief  构造函数
 */
c_network_process::c_network_process():m_inited(false), m_continue_working(false),
            m_listen_fd(-1), m_p_config(NULL),
            m_p_node_conn(NULL), m_p_ip_port(NULL)
{
    m_work_thread_id_vec.clear();
}

/**
 * @brief  析构函数
 */
c_network_process::~c_network_process()
{
}


/**
 * @brief  初始化
 * @param  p_config: 指向config类的指针
 * @return 0:success, -1:failed
 */
int c_network_process::uninit()
{
    if (!m_inited) {
        return -1;
    }
    m_continue_working = false;
    if (m_listen_fd >= 0) {
        close(m_listen_fd);
    }
    m_listen_fd = -1;

    m_p_config = NULL;
    m_p_ip_port = NULL;
    for(int index = 0; index < m_thread_cnt; index++) {
        if(NULL != m_p_node_conn[index]) {
            m_p_node_conn[index]->uninit();
            m_p_node_conn[index]->release();
        }
    }
    free(m_p_node_conn);

    vector<pthread_t>::iterator it = m_work_thread_id_vec.begin();
    for (; it != m_work_thread_id_vec.end(); it++) {
        assert(*it != 0);
        pthread_join(*it, NULL);
    }
    m_work_thread_id_vec.clear();
    pthread_mutex_destroy(&m_accept_mutex);

    return 0;
}

/**
 * @brief  初始化
 * @param  p_config: 指向config类的指针
 * @param  p_ip_port: <IP, port>列表
 * @return 0:success, -1:failed
 */
int c_network_process::init(network_conf_t *p_config, ip_port_map_t *p_ip_port)
{
    if (m_inited) {
        ERROR_LOG("ERROR: c_network_process has been inited.");
        return -1;
    }
    if (NULL == p_config || NULL == p_ip_port) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }
    if (p_config->listen_port <= 0 || p_config->listen_port > 65535) {
        ERROR_LOG("ERROR: wrong listen IP[%s] or port[%d]", p_config->listen_ip, p_config->listen_port);
        return -1;
    }

    char inside_ip[16] = {0};
#ifdef _DEBUG
    if (0 != get_host_ip(NET_OUTSIDE_TYPE, inside_ip)) {
        ERROR_LOG("ERROR: get_host_ip(NET_OUTSIDE_TYPE) failed.");
#else
    if (0 != get_host_ip(NET_INSIDE_TYPE, inside_ip)) {
        ERROR_LOG("ERROR: get_host_ip(NET_INSIDE_TYPE) failed.");
#endif
        return -1;
    }
    DEBUG_LOG("Listen ip[%s] port[%u].", inside_ip, p_config->listen_port);

    sockaddr_in listen_addr = {0};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(p_config->listen_port);
    listen_addr.sin_addr.s_addr = inet_addr(inside_ip);

    m_p_config = p_config;
    m_p_ip_port = p_ip_port;

    do {
        m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == m_listen_fd) {
            ERROR_LOG("ERROR: socket(AF_INET, SOCK_STREAM, 0) failed: %s", strerror(errno));
            break;
        }

        int reuse = 1;
        if (-1 == setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
            ERROR_LOG("ERROR: setsockopt(%d, SOL_SOCKET, SO_REUSEADDR) failed: %s", strerror(errno));
            break;
        }

        if (-1 == bind(m_listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr))) {
            ERROR_LOG("ERROR: setsockopt(%d, SOL_SOCKET, SO_REUSEADDR) failed: %s", m_listen_fd, strerror(errno));
            break;
        }

        ///设置socket为非阻塞模式
        int flag = fcntl(m_listen_fd, F_GETFL);
        flag |= O_NONBLOCK;
        if (-1 == fcntl(m_listen_fd, F_SETFL, flag)) {
            ERROR_LOG("ERROR: fcntl(%d, F_SETFL, O_NONBLOCK) failed: %s", m_listen_fd, strerror(errno));
            break;
        }

        if (-1 == listen(m_listen_fd, 80)) {
            ERROR_LOG("ERROR: listen(%d, 80) failed: %s", m_listen_fd, strerror(errno));
            break;
        }
        //TODO
        m_continue_working = true;
        m_thread_cnt = m_p_config->network_thread_cnt;
        pthread_t work_thread_id;

        m_p_node_conn = (c_net_client_impl**)malloc(sizeof(c_net_client_impl*) * m_thread_cnt);
        if(m_p_node_conn == NULL) {
            ERROR_LOG("ERROR: create m_p_node_conn[%d] failed.", m_thread_cnt);
            break;
        }

        int index = 0;
        for (index = 0; index < m_thread_cnt; index++) {
            m_p_node_conn[index] = NULL;
        }

        DEBUG_LOG("accept_thread_count: %d", m_thread_cnt);
        pthread_mutex_init(&m_accept_mutex, NULL);
        work_proc_param_t param = {this, 0};
        m_pkg_num = 0;
        for (index = 0; index < m_thread_cnt; index++) {
            if(0 != create_net_client_instance(&m_p_node_conn[index])) {
                ERROR_LOG("ERROR: create_net_client_instance(%d) failed.", index);
                break;
            }
            param.index = index;
            if (0 != pthread_create(&work_thread_id, NULL, work_thread_proc, &param)) {
                ERROR_LOG("ERROR: pthread_create[%d](...) failed: %s", index + 1, strerror(errno));
                break;
            }
            m_work_thread_id_vec.push_back(work_thread_id);
            usleep(1000);
        }
        if (index != m_thread_cnt) {
            break;
        }

        m_inited = true;
    } while(false);

    ///初始化失败
    if (!m_inited) {
        m_continue_working = false;
        vector<pthread_t>::iterator it = m_work_thread_id_vec.begin();
        for (; it != m_work_thread_id_vec.end(); it++) {
            pthread_join(*it, NULL);
        }
        m_work_thread_id_vec.clear();
        pthread_mutex_destroy(&m_accept_mutex);

        m_p_config = NULL;
        if (m_listen_fd >= 0) {
            close(m_listen_fd);
            m_listen_fd = -1;
        }
        int index = 0;
        for(; index < m_thread_cnt; index++) {
            if(NULL != m_p_node_conn[index]) {
                m_p_node_conn[index]->release();
            }
        }
        free(m_p_node_conn);
        m_p_ip_port = NULL;
    }

    return m_inited ? 0 : -1;
}

/**
 * @brief  子线程的处理函数 
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success, (void *)-1:failed
 */
void *c_network_process::work_thread_proc(void *p_data)
{
    assert(NULL != p_data);
    work_proc_param_t *param = (work_proc_param_t *)p_data;
    c_network_process *p_network_process = param->process;
    int index = param->index;
    assert(NULL != p_network_process && index >= 0);
    DEBUG_LOG("DEBUG: create %dth network thread[%u] succ.", index + 1, pthread_self());

    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_conn = -1;
    char client_ip[16] = {0};
    int listen_fd = p_network_process->m_listen_fd;
    fd_set read_set;
    struct timeval tv = {0, 0};
    while (p_network_process->m_continue_working) {
        pthread_mutex_lock(&p_network_process->m_accept_mutex);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&read_set);
        FD_SET(listen_fd, &read_set);
        int slt_ret = select(listen_fd + 1, &read_set, NULL, NULL, &tv);
        if (-1 == slt_ret) {//error
            ERROR_LOG("ERROR: select(fd:%d) failed[%s]", listen_fd, strerror(errno));
            pthread_mutex_unlock(&p_network_process->m_accept_mutex);
            if (errno == EINTR) {
                continue;
            }
            sleep(1);
            continue;
        } else if (0 == slt_ret) {//timeout
            pthread_mutex_unlock(&p_network_process->m_accept_mutex);
            //do nothing
        } else if (slt_ret > 0) {//start to accept
            int accept_fail_count = 1;
            bool accept_succ = false;
            while (p_network_process->m_continue_working) {
                client_conn = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
                if (-1 == client_conn) {
                    ++accept_fail_count;
                    ERROR_LOG("ERROR: %dth accept() failed[%s]", accept_fail_count, strerror(errno));
                    if (errno == EINTR && accept_fail_count < 100) {
                        continue;
                    }
                    break;
                }
                else {
                    accept_succ = true;
                    break;
                }
            }
            pthread_mutex_unlock(&p_network_process->m_accept_mutex);
            if (false == accept_succ) {
                continue;
            }

            //验证对端IP
            if (NULL == inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip))) {
                ERROR_LOG("ERROR: client_fd[%d] inet_ntop() failed", client_conn);
                continue;
            }
            char *p_trust_ip = p_network_process->m_p_config->trust_host;
            if (strcmp(client_ip, "127.0.0.0") && !strstr(p_trust_ip, client_ip)) {
                ERROR_LOG("ERROR: client_ip[%s] not in trust ip[%s]", client_ip, p_trust_ip);
                close(client_conn);
                continue;
            }

            //handle connect
            char rcv_buff[OA_MAX_BUF_LEN] = {0};
            if (0 != p_network_process->recv_cmd(client_conn, rcv_buff, sizeof(rcv_buff))) {
                ERROR_LOG("ERROR: recv command from client_ip[%s] failed.", client_ip);
                close(client_conn);
                continue;
            }
            pkg_head_t *p_head_cmd = (pkg_head_t *)rcv_buff;
            if (0 != p_network_process->check_cmd_valid(p_head_cmd)) {
                ERROR_LOG("ERROR: invalid command[%x] from client_ip[%s], so close client connect.",
                        p_head_cmd->cmd_id, client_ip);
                close(client_conn);
                continue;
            }
            p_network_process->m_pkg_num++;
            DEBUG_LOG("");
            DEBUG_LOG("=============%dth request package from client_ip[%s] handle_thread[%d]=============",
                    p_network_process->m_pkg_num, client_ip, index + 1);
            DEBUG_LOG("FROM WEB: cmd_id[%x] pkg_len[%u] serial_no1[%u] serial_no2[%u]",
                    p_head_cmd->cmd_id, p_head_cmd->pkg_len, p_head_cmd->serial_no1, p_head_cmd->serial_no2);

            char response_buff[OA_MAX_BUF_LEN] = {0};
            int buff_len = sizeof(response_buff);
            char buff[MAX_STR_LEN] = {0};
            pkg_head_t *p_rps_head = (pkg_head_t *)response_buff;

            memcpy(response_buff, rcv_buff, sizeof(pkg_head_t));
            p_rps_head->pkg_len = sizeof(pkg_head_t);
            p_rps_head->result = RESULT_OK;
            p_rps_head->send_time = time(NULL);
            sprintf(buff, "%s%u%u%u", HEAD_SECURITY_CODE, p_rps_head->cmd_id,
                    p_rps_head->send_time, p_rps_head->serial_no1);
            str2md5(buff, p_rps_head->auth_code);
            
            int notice_type = (p_head_cmd->cmd_id >> 28) & 0x000F;
            if (NOTI_SOCK != notice_type) {//非socket告知的消息则立即告知接受到消息
                if (0 != p_network_process->respond_cmd_by_sock(client_conn, response_buff, sizeof(pkg_head_t))) {
                    ERROR_LOG("ERROR: send cmd_id[%x] to client_ip[%s] failed.", p_head_cmd->cmd_id, client_ip);
                }
                close(client_conn);
            }
            
            p_network_process->handle_cmd(rcv_buff, response_buff, &buff_len, index);
#ifdef _DEBUG
            DEBUG_LOG("start to print_bytes to terminal ...");
            print_bytes((uint8_t *)response_buff, buff_len);
#endif
            switch(notice_type) {
            case NOTI_SOCK:
                DEBUG_LOG("SOCKET TO WEB BEGIN: cmd_id[%x] pkg_len[%u] result[%u]  no1[%u] no2[%u] to client_ip[%s].",
                        p_rps_head->cmd_id, p_rps_head->pkg_len, p_rps_head->result,
                        p_rps_head->serial_no1, p_rps_head->serial_no2, client_ip);
                if (0 != p_network_process->respond_cmd_by_sock(client_conn, response_buff, buff_len)) {
                    ERROR_LOG("ERROR: send cmd_id[%x] to client_ip[%s] failed.", p_rps_head->cmd_id, client_ip);
                    break;
                }
                DEBUG_LOG("SOCKET TO WEB END: cmd_id[%x] to client_ip[%s].", p_rps_head->cmd_id, client_ip);
                break;
            case NOTI_HTTP:
                DEBUG_LOG("HTTP TO WEB BEGIN: cmd_id[%x] pkg_len[%u] result[%u]  no1[%u] no2[%u] to client_ip[%s].",
                        p_rps_head->cmd_id, p_rps_head->pkg_len, p_rps_head->result,
                        p_rps_head->serial_no1, p_rps_head->serial_no2, client_ip);
                if (0 != p_network_process->respond_cmd_by_http(response_buff, buff_len)) {
                    ERROR_LOG("ERROR: send cmd_id[%x] to client_ip[%s] through url[%s] failed.",
                            p_rps_head->cmd_id, client_ip, p_network_process->m_p_config->noti_url);
                    break;
                }
                DEBUG_LOG("HTTP TO WEB END: cmd_id[%x] to client_ip[%s].", p_rps_head->cmd_id, client_ip);
                break;
            default:
                break;
            }
        }
    }

    DEBUG_LOG("Exit network thread:[%u]", pthread_self());
    return (void *)0;
}

/**
 * @brief  检查命令的有效性
 * @param  p_head_cmd: 命令请求
 * @return 0:success, -1:failed
 */
int c_network_process::check_cmd_valid(pkg_head_t *p_head_cmd)
{
    if(NULL == p_head_cmd) {
        ERROR_LOG("ERROR: parameter[p_head_cmd] cannot be NULL.");
        return -1;
    }
    int ret_code = 0;

    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%s%u%u%u", HEAD_SECURITY_CODE, p_head_cmd->cmd_id, p_head_cmd->send_time, p_head_cmd->serial_no1); 
    char rcv_md5[33] = {0};
    strncpy(rcv_md5, p_head_cmd->auth_code, sizeof(p_head_cmd->auth_code));
    char check_md5[33] = {0};
    str2md5(buff, check_md5);
    if (strcmp(rcv_md5, check_md5)) {
        ERROR_LOG("ERROR: rcv_md5[%s] is not equal to check_md5[%s].", rcv_md5, check_md5);
        ret_code = -1;
    }

    int notice_type = (p_head_cmd->cmd_id >> 28) & 0x000F;
    switch(notice_type) {
    case NOTI_SOCK:
    case NOTI_HTTP:
        break;
    default:
        ERROR_LOG("ERROR: notice type[%d] of cmd_id[%x] is wrong.", notice_type, p_head_cmd->cmd_id);
        ret_code = -1;
        break;
    }

    uint32_t proto_type = p_head_cmd->cmd_id & 0xF000;
    switch (proto_type) {
    case PROTO_DB:
    case PROTO_SYS:
        break;
    default:
        ERROR_LOG("ERROR: wrong proto_type[%x] in cmd_id[%x].", proto_type, p_head_cmd->cmd_id);
        ret_code = -1;
        break;
    }

    return ret_code;
}

/**
 * @brief  转发命令给node, 并获取返回结果
 * @param  p_head_cmd: 请求命令
 * @param  p_buff: node命令执行结果
 * @param  buff_len: 缓存大小
 * @param  index: m_p_node_conn下标
 * @return 0:success, -1:failed
 */
uint32_t c_network_process::handle_cmd(const char *p_rcv_cmd, char *p_buff, int *p_buff_len, int index)
{
    if(NULL == p_rcv_cmd || NULL == p_buff || NULL == p_buff_len || *p_buff_len < sizeof(pkg_head_t)) {
        ERROR_LOG("ERROR: parameter wrong.");
        return RESULT_ESYSTEM;
    }

    pkg_head_t *p_rcv_head = (pkg_head_t *)p_rcv_cmd;
    pkg_head_t *p_rps_head = (pkg_head_t *)p_buff;

    //memcpy(p_buff, p_rcv_cmd, sizeof(pkg_head_t));
    //p_rps_head->pkg_len = sizeof(pkg_head_t);
    //p_rps_head->send_time = time(NULL);

    uint32_t handle_result = RESULT_OK;
    uint32_t proto_type = p_rcv_head->cmd_id & 0xF000;
    switch (proto_type) {
    case PROTO_DB:
        handle_result = handle_db_cmd(p_rcv_cmd, p_buff, p_buff_len, index);
        break;
    case PROTO_SYS:
        //handle_result = handle_sys_cmd(p_rcv_cmd, p_buff, p_buff_len, index);
        break;
    default:
        ERROR_LOG("ERROR: wrong cmd_id[%x].", p_rcv_head->cmd_id);
        handle_result = RESULT_ECOMMAND;
        break;
    }
    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%s%u%u%u", HEAD_SECURITY_CODE, p_rps_head->cmd_id, p_rps_head->send_time, p_rps_head->serial_no1);
    str2md5(buff, p_rps_head->auth_code);

    if (RESULT_OK != handle_result) {
        p_rps_head->result = handle_result;
        p_rps_head->pkg_len = sizeof(pkg_head_t);
        *p_buff_len = sizeof(pkg_head_t);
    }
    DEBUG_LOG("cmd_id[%X] receive node pkg_len[%d] result[%u] handle_result[%u]",
            p_rcv_head->cmd_id, *p_buff_len, p_rps_head->result, handle_result);
    return handle_result;
}

/**
 * @brief  转发database命令给node, 并获取返回结果
 * @param  p_head_cmd: 请求命令
 * @param  p_buff: node命令执行结果
 * @param  buff_len: 缓存大小
 * @param  index: m_p_node_conn下标
 * @return 0:success, -1:failed
 */
uint32_t c_network_process::handle_db_cmd(const char *p_rcv_cmd, char *p_buff, int *p_buff_len,  int index)
{
    if (NULL == p_rcv_cmd || NULL == p_buff || NULL == p_buff_len || *p_buff_len < sizeof(pkg_head_t)) {
        ERROR_LOG("ERROR: parameter wrong.");
        return RESULT_ESYSTEM;
    }
    assert(index >= 0);
    c_net_client_impl *p_node_conn = m_p_node_conn[index];
    pkg_head_t *p_head = (pkg_head_t *)p_rcv_cmd;
    db_body_t *p_body = (db_body_t *)(p_rcv_cmd + sizeof(pkg_head_t));

    char ip[16] = {0};
    long2ip(p_body->dst_ip, ip);
    uint32_t inside_ip = 0;
    if (1 != inet_pton(AF_INET, ip, &inside_ip)) {
        ERROR_LOG("inet_pton(%s) failed.", ip);
        return RESULT_ESYSTEM;
    }
    DEBUG_LOG("DB Command: pkg_len[%u] cmd_id[%X](serial_no1[%u], serial_no2[%u]) to sql_ip[%s]",
            p_head->pkg_len, p_head->cmd_id, p_head->serial_no1, p_head->serial_no2, ip);

    ip_port_map_t::iterator it = m_p_ip_port->find(inside_ip);
    if (it == m_p_ip_port->end()) {
        ERROR_LOG("ERROR: cannot find send port of node[%s].", ip);
        return RESULT_ESYSTEM;
    }
    uint32_t node_ip = it->second.listen_type == NET_INSIDE_TYPE ? inside_ip : it->second.outside_ip;
    int node_port = it->second.listen_port;
    DEBUG_LOG("listen_type[%d], node_ip[%u], node_port[%u]", it->second.listen_type, node_ip, node_port);
    if (0 != p_node_conn->init(node_ip, node_port, 20)) {
        ERROR_LOG("ERROR: init connect to node[%s:%u] failed[%s].", ip, node_port, strerror(errno));
        return RESULT_EUNCONNECT;
    }

    //发送命令请求
    char send_buff[OA_MAX_BUF_LEN] = {0};
    memcpy(send_buff, p_rcv_cmd, p_head->pkg_len);
    node_head_t *p_node_head = (pkg_head_t *)send_buff;
    p_node_head->version = NETWORK_PROCESS_VERSION;

    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%s%u%u%u", NODE_SECURITY_CODE, p_node_head->cmd_id, p_node_head->send_time, p_node_head->serial_no1);
    str2md5(buff, p_node_head->auth_code);

    uint32_t ret_code = RESULT_OK;
    do {
        DEBUG_LOG("Command Exec Time: begin to send cmd_id[%x] to node[%s:%u]", p_node_head->cmd_id, ip, node_port);
        if (0 != p_node_conn->send_data(send_buff, sizeof(send_buff))) {
            ERROR_LOG("ERROR: send cmd[%x] to node[%s:%u] failed.", p_node_head->cmd_id, ip, node_port);
            ret_code = RESULT_EUNCONNECT;
            break;
        }

        //接受命令返回
        if (0 != p_node_conn->do_io()) {
            ERROR_LOG("ERROR: do_io() for cmd[%x] to node[%s:%u] failed.",
                    p_node_head->cmd_id, ip, node_port);
            ret_code = RESULT_EUNCONNECT;
            break;
        }

        node_head_t *p_rcv_head = (pkg_head_t *)p_buff;
        char *recv_pos = p_buff;
        int remain_len = *p_buff_len;
        int recv_total_len = 0;
        int recv_len = 0;
        time_t start_time = time(NULL);
        bool recv_succ = false;
        while ((recv_len = p_node_conn->recv_data(recv_pos, remain_len)) >= 0) {
            if (recv_len > 0) {
                remain_len -= recv_len;
                recv_total_len += recv_len;
                recv_pos += recv_len;
            } else if (0 == recv_len) {
                //do nothing
            } 

            if (recv_total_len >= sizeof(node_head_t) && recv_total_len >= p_rcv_head->pkg_len) {
                DEBUG_LOG("Command Exec Time: end to recv cmd_id[%x] pkg_len[%u] from node[%s:%u]",
                        p_node_head->cmd_id, p_rcv_head->pkg_len, ip, node_port);
                recv_succ = true;
                break;
            }
            if (remain_len <= 0) {
                ERROR_LOG("ERROR: recv cmd_id[%x] from node[%s:%u] pkg_len[%u] > buff_len[%d]",
                        p_node_head->cmd_id, ip, node_port, p_rcv_head->pkg_len, *p_buff_len);
                ret_code = RESULT_ENOMEMORY;
                break;
            }
            if (time(NULL) - start_time >= NETWORK_MAX_RECV_TIME) {
                ERROR_LOG("ERROR: recv pkg of cmd_id[%x] from node[%s:%u] timeout[%d].",
                        p_node_head->cmd_id, ip, node_port, NETWORK_MAX_RECV_TIME);
                ret_code = RESULT_ETIMEOUT;
                break;
            }
            p_node_conn->do_io();
        }
        if (!recv_succ) {
            if (RESULT_OK == ret_code) {
                ret_code = RESULT_EUNCONNECT;//待定
            }
            break;
        }

        if (0 != check_rcv_node_head(send_buff, p_buff)) {
            ret_code = RESULT_ENODEHEAD;
            break;
        }
        p_node_head = (node_head_t *)p_buff;
        *p_buff_len = p_node_head->pkg_len;
    } while (false);

    p_node_conn->uninit();
    return ret_code;
}


/**
 * @brief  接受命令请求
 * @param  conn_fd: 客户端连接
 * @param  p_buf: 保存命令请求
 * @param  buf_len: 缓存大小
 * @return 0:success, -1:failed
 */
int c_network_process::recv_cmd(int conn_fd, char *p_buf, int buf_len)
{
    if (conn_fd < 0 || NULL == p_buf || buf_len < sizeof(pkg_head_t)) {
        ERROR_LOG("ERROR: wrong parameter.");
        return -1;
    }
    memset(p_buf, 0, buf_len);
    pkg_head_t *p_head_cmd = (pkg_head_t *)p_buf;

    int buf_left_len = buf_len -1;
    int buf_rcv_len = 0;
    int rcv_len = 0;
    time_t start = time(NULL);
    time_t end = start;
    while (buf_left_len > 0) {
        end = time(NULL);
        if (end - start >= RECV_TIMEOUT) {
            ERROR_LOG("ERROR: recv() timeout[%us]", RECV_TIMEOUT);
            return -1;
        }

        rcv_len = recv(conn_fd, p_buf + buf_rcv_len, buf_left_len, 0); 
        if (rcv_len > 0) {
            buf_left_len -= rcv_len;
            buf_rcv_len += rcv_len;
        }
        else if (0 == rcv_len) {
            ERROR_LOG("ERROR: recv() rcv_len[0] conn_fd[%d] failed[%s].", conn_fd, strerror(errno));
            return -1;
        }
        else {
            ERROR_LOG("ERROR: recv() conn_fd[%d] failed[%s].", conn_fd, strerror(errno));
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (buf_rcv_len >= sizeof(pkg_head_t)) {
            if (p_head_cmd->pkg_len >= buf_len) {
                ERROR_LOG("ERROR: pkg_len[%u] >= buf_len[%d]", p_head_cmd->pkg_len, buf_len);
                return -1;
            }
            if (buf_rcv_len >= p_head_cmd->pkg_len) {
                break;
            }
        }
    }

    return 0;
}

/**
 * @brief  回复命令请求
 * @param  conn_fd: 客户端连接
 * @param  p_buf: 待回复信息
 * @param  buf_len: 缓存大小
 * @return 0:success, -1:failed
 */
int c_network_process::respond_cmd_by_sock(int conn_fd, const char *p_buf, int buf_len)
{
    if (conn_fd < 0 || NULL == p_buf || buf_len < sizeof(pkg_head_t)) {
        ERROR_LOG("ERROR: wrong parameter.");
        return -1;
    }

    int buf_left_len = buf_len;
    int buf_snd_len = 0;
    int snd_len = 0;
    time_t start = time(NULL);
    time_t end = start;
    while (buf_left_len > 0) {
        end = time(NULL);
        if (end - start >= SEND_TIMEOUT) {
            ERROR_LOG("ERROR: send() timeout[%us]", SEND_TIMEOUT);
            return -1;
        }

        snd_len = send(conn_fd, p_buf + buf_snd_len, buf_left_len, 0); 
        if (snd_len > 0) {
            buf_left_len -= snd_len;
            buf_snd_len += snd_len;
        }
        else if (0 == snd_len) {
            ERROR_LOG("ERROR: send() snd_len[0] conn_fd[%d] failed[%s].", conn_fd, strerror(errno));
            return -1;
        }
        else {
            ERROR_LOG("ERROR: send() conn_fd[%d] failed[%s].", conn_fd, strerror(errno));
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -1;
        }
    }

    return 0;
}


/**
 * @brief  回复命令请求
 * @param  p_buf: 待回复信息
 * @param  buf_len: 缓存大小
 * @return 0:success, -1:failed
 */
int c_network_process::respond_cmd_by_http(const char *p_buf, int buf_len)
{
    if ( NULL == p_buf || buf_len < sizeof(pkg_head_t)) {
        ERROR_LOG("ERROR: wrong parameter.");
        return -1;
    }
    node_head_t *p_rcv_head = (node_head_t *)p_buf;
    int body_len = p_rcv_head->pkg_len - sizeof(node_head_t);
    unsigned int manage_id = p_rcv_head->serial_no1;
    DEBUG_LOG("HTTP POST: cmd_id[%u], result[%u], pkg_len[%u], manage_id[%u]",
            p_rcv_head->cmd_id, p_rcv_head->result, p_rcv_head->pkg_len, p_rcv_head->serial_no1);

    Chttp_transfer ht;
    char data[MAX_STR_LEN] = {0};
    if (0 != p_rcv_head->result && body_len <= 0) {
        snprintf(data, sizeof(data), "cmd=2011&manage_id=%u&value_id=0&sql_no=0&sql=&err_code=%u&err_desc=%s&switch=%s",
            manage_id, p_rcv_head->result, get_error_desc(p_rcv_head->result), SWITCH_CODE);
        ht.http_post(m_p_config->noti_url, data);
        string back = ht.get_post_back_data();
        DEBUG_LOG("http_post(%s, %s) return[%s]", m_p_config->noti_url, data, back.c_str());
        return 0;
    }
    if (body_len > buf_len) {
        ERROR_LOG("recv pkg error: body_len[%d] not in [0,%u]", body_len, buf_len);
        return -1;
    }

    const char *p_err = NULL;
    const char *p_sql = NULL;
    node_db_body_t *p_body = (node_db_body_t *)(p_buf + sizeof(node_head_t));
    db_body_fix_t *p_fix_body = NULL;
    const char *p_pos = p_buf + sizeof(node_head_t) + sizeof(node_db_body_t);
    int read_len = 0;
    int read_cnt = 0;
    DEBUG_LOG("sql_cnt[%u], body_len[%d]", p_body->sql_cnt, body_len);
    while (read_cnt < p_body->sql_cnt && read_len < body_len) {
        p_fix_body = (db_body_fix_t *)p_pos;
        p_sql = p_pos + sizeof(db_body_fix_t);
        p_err = p_sql + p_fix_body->sql_len;
        char sql[MAX_STR_LEN] = {0};
        char err[MAX_STR_LEN] = {0};
        if (p_fix_body->sql_len > 0) {
            urlencode(p_sql, sql);
        }
        if (p_fix_body->err_len > 0) {
            urlencode(p_err, err);
        }
        snprintf(data, sizeof(data),
                "cmd=2011&manage_id=%u&value_id=%u&sql_no=%u&sql=%s&err_code=%u&err_desc=%s&switch=",
                manage_id, p_fix_body->serial_id, p_fix_body->sql_no, sql, p_fix_body->err_no, err);

        ht.http_post(m_p_config->noti_url, data);
        string back = ht.get_post_back_data();
        DEBUG_LOG("http_post(%s, %s) return[%s]", m_p_config->noti_url, data, back.c_str());
        int move_len = sizeof(db_body_fix_t) + p_fix_body->sql_len + p_fix_body->err_len;
        p_pos += move_len;
        read_len += move_len;
        read_cnt++;
        usleep(1000);
    }
    DEBUG_LOG("After while: sql_cnt[%u], read_cnt[%d]", p_body->sql_cnt, read_cnt);
    if (read_cnt < p_body->sql_cnt) {
        ERROR_LOG("pkg_len too long: need_sql_cnt[%u] > recv_sql_cnt[%d]", p_body->sql_cnt, read_cnt);
        char buff[MAX_STR_LEN] = {0};
        snprintf(buff, sizeof(buff), "%s&switch=%s", data, SWITCH_CODE);
        ht.http_post(m_p_config->noti_url, buff);
        string back = ht.get_post_back_data();
        DEBUG_LOG("http_post(%s, %s) return[%s]", m_p_config->noti_url, buff, back.c_str());
    }

    return 0;
}

/**
 * @brief  回复命令请求
 * @param  conn_fd: 客户端连接
 * @param  p_buf: 待回复信息
 * @param  buf_len: 缓存大小
 * @return 0:success, -1:failed
 */
int c_network_process::check_rcv_node_head(const char *send_buff, const char *recv_buff)
{
    if(NULL == send_buff || NULL == recv_buff) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    const node_head_t *p_snd_head = (node_head_t *)send_buff;
    const node_head_t *p_rcv_head = (node_head_t *)recv_buff;

    int ret_code = 0;
    if (p_rcv_head->pkg_len < sizeof(node_head_t)) {
        ERROR_LOG("ERROR: recv pkg_len[%u] < node_head_t len[%u]", p_rcv_head->pkg_len, sizeof(node_head_t));
        ret_code = -1;
    }
    if (p_rcv_head->cmd_id != p_snd_head->cmd_id) {
        ERROR_LOG("ERROR: send cmd_id[%x], but recv cmd_id[%x]", p_snd_head->cmd_id, p_rcv_head->cmd_id);
        ret_code = -1;
    }
    if (p_rcv_head->serial_no1 != p_snd_head->serial_no1) {
        ERROR_LOG("ERROR: send serial_no1[%u], but recv serial_no1[%u]",
                p_snd_head->serial_no1, p_rcv_head->serial_no1);
        ret_code = -1;
    }
    if (p_rcv_head->serial_no2 != p_snd_head->serial_no2) {
        ERROR_LOG("ERROR: send serial_no2[%u], but recv serial_no2[%u]",
                p_snd_head->serial_no2, p_rcv_head->serial_no2);
        ret_code = -1;
    }

    //验证码检查
    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%s%u%u%u", NODE_SECURITY_CODE, p_rcv_head->cmd_id, p_rcv_head->send_time, p_rcv_head->serial_no1);
    char check_md5[33] = {0};
    str2md5(buff, check_md5);
    char rcv_md5[33] = {0};
    strncpy(rcv_md5, p_rcv_head->auth_code, sizeof(p_rcv_head->auth_code));
    if (strcmp(check_md5, rcv_md5)) {
        ERROR_LOG("ERROR: check md5[%s], but recv md5[%s]", check_md5, rcv_md5);
        ret_code = -1;
    }

    return ret_code;
}

/**
 * @brief  创建network进程
 * @param  fd: 与父进程通信的fd
 * @param  p_config: 指向config类的指针
 * @param  p_ip_port: <IP, port>列表
 * @return 0:success, -1:failed
 */
int network_process_run(int fd, network_conf_t *p_config, ip_port_map_t *p_ip_port)
{
    proc_log_init(PRE_NAME_NETWORK);
    if (fd < 0 || NULL == p_config || NULL == p_ip_port) {
        ERROR_LOG("ERROR: parameter wrong.");
        return -1;
    }

    DEBUG_LOG("======================Init Info======================");
    c_network_process network_process;
    int ret = network_process.init(p_config, p_ip_port);

    //通知父进程init状态
    char buff[MAX_STR_LEN] = {0};
    sprintf(buff, "%d", 0 == ret ? PROC_INIT_SUCC : PROC_INIT_FAIL);
    DEBUG_LOG("Notice monitor process PROC_INIT_STATUS[%s]", buff);
    DEBUG_LOG("=====================================================\n");
    if (write(fd, buff, strlen(buff)) < 0) {
        ERROR_LOG("ERROR: write(%s) to notice monitor process failed[%s].",
              (0 == ret) ? "PROC_INIT_SUCC" : "PROC_INIT_FAIL", strerror(errno));
        network_process.uninit();
        return -1;
    }
    
    DEBUG_LOG("======================Enter Main Loop======================");
    int read_len = 0;
    int cmd_id = 0;
    while (true) {
        read_len = read(fd, buff, sizeof(buff) - 1);
        if (read_len < 0) {
            ERROR_LOG("read(...) from parent process failed[%s].", strerror(errno));
            break;
        } else if (read_len > 0) {
            buff[read_len] = 0;
            DEBUG_LOG("recv parent cmd_id[%s]", buff);
            cmd_id = atoi(buff);
            if (PROC_NEED_STOP == cmd_id) {
                break;
            }
        }
        sleep(1);
    }
    DEBUG_LOG("======================Leave Main Loop======================");
    
    network_process.uninit();
    DEBUG_LOG("Network process[%d] exit succ.", getpid());
    return 0;
}
