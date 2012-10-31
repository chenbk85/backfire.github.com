/**
 * =====================================================================================
 *       @file  network_process.cpp
 *      @brief
 *
 *     Created  2011-12-27 15:37:04
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/shm.h>

#include "./network_process.h"
#include "./export_thread.h"
#include "./listen_thread.h"
#include "./express_thread.h"
#include "../proto.h"
#include "../lib/log.h"

volatile static sig_atomic_t g_got_sig_term = 0;    // 接收monitor发出的退出信号

static c_listen_thread m_listen_thread;
static bool g_is_recv_udp = false;
static char m_parent_ip[OA_MAX_STR_LEN];

c_queue<recv_cmd_t> m_export_queue;
//c_queue<recv_cmd_t> m_express_queue;

i_config *p_config = NULL;

static void load_node_info(i_config* p_config)
{
    if(p_config == NULL) {
        ERROR_LOG("p_config=%p", p_config);
        return ;
    }
    if(p_config->get_config("node_info", "parent_ip", m_parent_ip, sizeof(m_parent_ip)) != 0) {
        ERROR_LOG("ERROR: get [%s]:[%s] failed.", "node_info", "parent_ip");
        strcpy(m_parent_ip, DEFAULT_HEAD);
    } else {
        //strcpy(m_parent_ip, DEFAULT_HEAD);
    }
    set_node_info(p_config);
}

/**
 * @brief 信号处理函数
 * @param sig 触发的信号
 */
static void signal_handler(int sig)
{
    switch (sig) {
    case SIGTERM:
        g_got_sig_term = 1;
        break;
    case SIGUSR1:   //重新读取配置
        if(uninit_and_release_config_instance(p_config) == 0) {
            if (create_and_init_config_instance(&p_config) == 0) {
                load_node_info(p_config);
            } else {
                if (p_config) {
                    p_config->release();
                    p_config = NULL;
                }
            }
        }
        break;
    default:
        ERROR_LOG("ERROR: it should never come here!");
        break;
    }
}

/**
 * @brief  接受命令请求
 * @param  peer_fd: 对端socket fd
 * @param  buffer: 存放命令请求缓存
 * @param  buf_len: buffer大小
 * @return 0-success, -1-failed
 */
static int rcv_cmd(int peer_fd, char *buffer, int buf_len)
{
    if (buffer == NULL || (size_t)buf_len < sizeof(oa_cmd_t)) {
        ERROR_LOG("buffer=%p buf_len=%d[<%lu]", buffer, buf_len, sizeof(oa_cmd_t));
        return -1;
    }
    oa_cmd_t *p_oa_cmd = (oa_cmd_t *) buffer;
    //设置非阻塞模式
    int flag = fcntl(peer_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(peer_fd, F_SETFL, flag);

    time_t start = time(NULL);
    time_t end = start;
    int rcv_buf_len = 0;
    int rcv_len = 0;
    while (true) {
        end = time(NULL);
        if (end - start > OA_SOCKET_TIMEOUT) {
            ERROR_LOG("ERROR: recv(...) timeout");
            return -1;
        }

        rcv_len = recv(peer_fd, buffer + rcv_buf_len, buf_len - rcv_buf_len, 0);
        if (rcv_len > 0) {
            rcv_buf_len += rcv_len;
        } else if (rcv_len == 0) {
            return -1;
        } else {
            if (errno == EAGAIN || errno == EINTR) {
                usleep(100);//2011-04-07
                continue;
            } else {
                ERROR_LOG("ERROR: recv(...) failed.");
                return -1;
            }
        }

        if ((size_t)rcv_buf_len >= sizeof(oa_cmd_t)) {
            if (p_oa_cmd->pkg_len > (uint32_t)buf_len) {
                ERROR_LOG("ERROR: buf_len[%d] < pkg_len[%d].", buf_len, p_oa_cmd->pkg_len);
                return -1;
            }
            if ((uint32_t)rcv_buf_len >= p_oa_cmd->pkg_len) {
                break;
            }
        }
    }

    return 0;
}

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
do{\
     if(p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
         ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
         return -1;\
     }\
 } while(0)

int network_run(int argc, char ** argv, void * p_arg)
{
    if(p_arg == NULL) {
        ERROR_LOG("p_arg=%p", p_arg);
        return -1;
    }
    u_shmid_t * p_shmid = (u_shmid_t *)p_arg;
    set_proc_name(argc, argv, "%s_network", OA_BIN_FILE_SAMPLE);    //oa_node-network
    write_pid_to_file(OA_NETWORK_FILE);

    if(0 != change_user(g_user_list[OA_USER_NOBODY])) {
        ERROR_LOG("ERROR: change to %s failed.", g_user_list[OA_USER_NOBODY]);
        return -1;
    }
    // 处理信号
    mysignal(SIGTERM, signal_handler);
    mysignal(SIGUSR1, signal_handler);

    c_queue<u_return_t>* p_return_q = NULL;
    c_queue<u_command_t>* p_command_q = NULL;
    u_sh_mem_used_t* p_used = NULL;

    int flags;
    int reuse;
    int m_listen_fd;
    bool listen_thread_started;
    sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    int peer_socket = -1;
    char peer_ip[16] = {0};
    char cmd[OA_MAX_BUF_LEN] = {0};
    oa_cmd_t *p_oa_cmd = (oa_cmd_t *) cmd;
    struct timeval tv;
    fd_set read_set;
    int result = 0;
    m_export_queue.clean();
    //m_express_queue.clean();
    recv_cmd_t recv_cmd;

    //u_return_t return_t;
    u_command_t command_t;
    c_hash_table m_hash_table;

    if(p_shmid != NULL && p_shmid->cmd_shmid >= 0 && p_shmid->ret_shmid >= 0 && p_shmid->used_shmid >= 0) {
        p_command_q = (c_queue<u_command_t>*)shmat(p_shmid->cmd_shmid, 0, 0);
        if(p_command_q == (void*)(-1)) {
            ERROR_LOG("link command_queue share memory[%d] failed[%s]", p_shmid->cmd_shmid, strerror(errno));
            return -1;
        }
        p_return_q = (c_queue<u_return_t>*)shmat(p_shmid->ret_shmid, 0, 0);
        if(p_return_q == (void*)(-1)) {
            ERROR_LOG("link return_queue share memory[%d] failed[%s]", p_shmid->ret_shmid, strerror(errno));
            return -1;
        }
        p_used = (u_sh_mem_used_t*)shmat(p_shmid->used_shmid, 0, 0);
        if(p_used == (void*)(-1)) {
            ERROR_LOG("link u_sh_mem_used_t share memory[%d] failed[%s]", p_shmid->used_shmid, strerror(errno));
            return -1;
        }
        p_used->command_t_used = 1;
    }
    // 创建配置文件对象
    if (create_and_init_config_instance(&p_config) != 0) {
        if (p_config) {
            p_config->release();
            p_config = NULL;
        }
        return -1;
    }

    //GET_CONFIG("node_info", "parent_ip", m_parent_ip);
    if(p_config->get_config("node_info", "parent_ip", m_parent_ip, sizeof(m_parent_ip)) != 0) {
        ERROR_LOG("ERROR: get [%s]:[%s] failed.", "node_info", "parent_ip");
        strcpy(m_parent_ip, DEFAULT_HEAD);
    } else {
        //strcpy(m_parent_ip, DEFAULT_HEAD);
    }

    char inside_ip[16] = {0};
    if(get_ip(p_config, inside_ip) == NULL) {
        ERROR_LOG("get ip error");
        return -1;
    }

    DEBUG_LOG("inside ip: %s", inside_ip);

    char port_str[6] = {0};
    int listen_port = 55000;
    if(p_config->get_config("node_info", "listen_port", port_str, sizeof(port_str)) == 0) {
        listen_port = atoi(port_str);
        if (listen_port < 0 || listen_port > 65535) {
            ERROR_LOG("ERROR: illegal network:listen_port[%s].", port_str);
            listen_port = 55000;
        }
    } else {
        ERROR_LOG("ERROR: get [%s]:[%s] failed.", "node_info", "listen_port");
    }

    sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(listen_port);
    listen_addr.sin_addr.s_addr = inet_addr(inside_ip);
    if (listen_addr.sin_addr.s_addr == INADDR_NONE) {
        ERROR_LOG("ERROR: illegal oa-head:listen_ip[%s].", inside_ip);
        goto exit;
    }

    m_listen_fd = -1;
    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0) {
        ERROR_LOG("socket(SOCK_STREAM) failed: %s",strerror(errno));
        goto exit;
    }
    DEBUG_LOG("network_process inited listen fd: %d", m_listen_fd);
    ///设置地址重用
    reuse = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        ERROR_LOG("Setsockopt(SO_REUSEADDR) Error: %s",strerror(errno));
        goto exit;
    }

    if (bind(m_listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        ERROR_LOG("Bind Error: %s",strerror(errno));
        goto exit;
    }
    ///设置非阻塞模式
    flags = fcntl(m_listen_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(m_listen_fd, F_SETFL, flags);

    if (listen(m_listen_fd, 80) < 0) {
        ERROR_LOG("Listen Error:%s",strerror(errno));
        goto exit;
    }

    listen_thread_started = false;

    if (set_recv_udp(p_config, &g_is_recv_udp) != 0) {
        ERROR_LOG("ERROR: set_recv_udp() failed.");
        return -1;
    }

    if (g_is_recv_udp) {
        if (m_listen_thread.init(p_config, inside_ip, &m_hash_table, &listen_thread_started) != 0) {
            ERROR_LOG("ERROR: listen thread init().");
            goto exit;
        }

        if (export_init(p_config, inside_ip, &m_listen_thread, &m_hash_table) != 0) {
            ERROR_LOG("ERROR: export thread init().");
            m_listen_thread.uninit();
            goto exit;
        }
        // 等待listen线程启动完成
        while (!listen_thread_started) {
            usleep(10);
        }
    }
    //启动express线程，发送执行命令的结果
    express_init(p_return_q);

    while (g_got_sig_term == 0) {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&read_set);
        FD_SET(m_listen_fd, &read_set);
        ERROR_LOG("ready to select");
        result = select(m_listen_fd + 1, &read_set, NULL, NULL, &tv);
        if (result < 0) {
            if (errno == EINTR) {
                DEBUG_LOG("DEBUG: select() failed, errno: %d, reason: %s", errno, strerror(errno));
                continue;
            } else if (errno == ENOMEM) {
                sleep(1);
                DEBUG_LOG("DEBUG: select() failed, errno: %d, reason: %s", errno, strerror(errno));
                continue;
            } else {
                ERROR_LOG("ERROR: select() failed, errno: %d, reason: %s", errno, strerror(errno));
                break;
            }
        } else if (result == 0) {//超时
            continue;
        } else {
            if (!FD_ISSET(m_listen_fd, &read_set)) {
                ERROR_LOG("sleep 1 second");
                sleep(1);//2011-04-07
                continue;
            }
        }
        ERROR_LOG("select success");
        while (g_got_sig_term == 0) {
            peer_socket = accept(m_listen_fd, (struct sockaddr*)&peer_addr, &addr_len);
            if (peer_socket < 0) {
                if (errno == EINTR) {
                    DEBUG_LOG("DEBUG: accept() failed, errno:%d, reason:%s.", errno, strerror(errno));
                    continue;
                } else {
                    ERROR_LOG("ERROR: accept() failed, errno:%d, reason:%s.", errno, strerror(errno));
                    sleep(1);
                    continue;
                }
            } else {
                break;
            }
        }
        ERROR_LOG("accept success");
        if (g_got_sig_term != 0) {
            close(peer_socket);
            break;
        }

        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, sizeof(peer_ip));

        if (rcv_cmd(peer_socket, cmd, sizeof(cmd)) != 0) {
            close(peer_socket);
            ERROR_LOG("ERROR: rcv_cmd from %s failed!", peer_ip);
            continue;
        }
        DEBUG_LOG("command from [%s]: [pkg_len:%u, version:%d, cmd_id:0x%08X %u]",
                peer_ip, p_oa_cmd->pkg_len, p_oa_cmd->version, p_oa_cmd->cmd_id, p_oa_cmd->cmd_id);

        if (p_oa_cmd->cmd_id == OA_CMD_GET_METRIC_INFO) {// 获取采集数据信息(xml格式)
            if(g_is_recv_udp) {
                if (NULL == strstr(m_parent_ip, peer_ip)) {
                    close(peer_socket);
                    ERROR_LOG("ERROR: peer_ip[%s] is not in parent_ip_list[%s]", peer_ip, m_parent_ip);
                    continue;
                }
                DEBUG_LOG("receive get metric info command");
                recv_cmd.peer_socket = peer_socket;
                recv_cmd.date_length = p_oa_cmd->pkg_len;
                memcpy(recv_cmd.data, cmd, p_oa_cmd->pkg_len);
                m_export_queue.push(&recv_cmd);
                pthread_kill(get_export_pthread_id(), SIGUSR2);
            } else {
                //ingore
                DEBUG_LOG("i'm not recv host");
                close(peer_socket);
                continue;
            }
        } else if((p_oa_cmd->cmd_id & 0xFF00) == OA_CMD_MYSQL_MGR){// 数据库管理命令的接口
            DEBUG_LOG("put into queue 0x%08X", p_oa_cmd->cmd_id);
            command_t.command_type = OA_SO;
            command_t.command_id = p_oa_cmd->cmd_id;
            command_t.send_fd = peer_socket;
            memcpy(command_t.command_data, p_oa_cmd, p_oa_cmd->pkg_len);
            p_command_q->push(&command_t);
        } else {
            ERROR_LOG("receive unknown command[%u]", p_oa_cmd->cmd_id);
        }
    }
    p_used->command_t_used = 0; //通知command进程，没有命令了
    //todo：等待return_t_used == 0，并将所有的返回结果都发送
    while(p_used->return_t_used != 0) {
        DEBUG_LOG("wait for command process");
        sleep(1);
    }

exit:
    if (g_is_recv_udp) {
        export_uninit();
        m_listen_thread.uninit();
    }

    express_uninit();

    uninit_and_release_config_instance(p_config);

    if (m_listen_fd >= 0) {
        close(m_listen_fd);
    }
    return 0;
}

#undef GET_CONFIG

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
do{\
     if(p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
         ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
         return false;\
     }\
 } while(0)

#undef GET_CONFIG
