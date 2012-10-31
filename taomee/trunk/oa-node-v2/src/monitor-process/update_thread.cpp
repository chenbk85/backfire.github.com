/**
 * =====================================================================================
 *       @file  update_thread.cpp
 *      @brief
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  11/24/2010 04:27:54 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:33:21
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../lib/log.h"
#include "../lib/utils.h"
#include "./update_thread.h"

using namespace std;

/**
 * @brief  构造函数
 */
c_update_thread::c_update_thread(bool * need_update)
{
    m_inited = false;
    m_cpu_bit = 8 * sizeof(char *);
    m_p_need_update = need_update;
    *m_p_need_update = false;
}

/**
 * @brief  析构函数
 */
c_update_thread::~c_update_thread()
{
    uninit();
}

/**
 * @brief  初始化函数
 * @param  p_config: 配置对象指针指针
 * @param  reboot: true-重启，false-未重启
 * @param  thread_id: 调用线程id，用于唤醒id对应的线程
 * @return 0-success, -1-failed
 */
int c_update_thread::init(i_config *p_config, pthread_t thread_id)
{
    if (m_inited) {
        ERROR_LOG("c_update_thread had been inited.");
        return -1;
    }

    if (p_config == NULL) {
        ERROR_LOG("p_config=%p", p_config);
        return -1;
    }

    m_continue_working = true;
    m_p_config = p_config;
    m_call_thread_id = thread_id;

    do {
        if (m_p_config->get_config("node_info", "update_url", m_update_url, sizeof(m_update_url)) != 0) {
            ERROR_LOG("ERROR: cannot get config[node_info:update_url].");
            break;
        }

        char update_interval[32] = {0};
        if (m_p_config->get_config("node_info", "update_interval", update_interval, sizeof(update_interval)) != 0) {
            ERROR_LOG("ERROR: cannot get config[node_info:update_interval].");
            break;
        }
        m_update_interval = atoi(update_interval);
        //更新间隔不能小于1分钟也不能大于1天
        if (m_update_interval < 60 || m_update_interval > 24 * 3600) {
            m_update_interval = 3600;//默认值为1小时
        }
        srand((u_int) time(NULL));
        m_rand = rand() % (m_update_interval / 10) * (rand() % 2 == 0 ? 1 : -1);

        memset(&m_update_info, 0, sizeof(m_update_info));

        if(get_ip(m_p_config, m_inside_ip) == NULL) {
            ERROR_LOG("get ip error");
            break;
        }
        m_inited = true;
    } while (false);

    if (m_inited) {
        DEBUG_LOG("update thread init succ.");
        return 0;
    } else {
        m_continue_working = false;
        m_update_status = 0;
        m_call_thread_id = 0;
        m_p_config = NULL;
        m_update_interval = 0;
        return -1;
    }
}

/**
 * @brief  反初始化函数
 * @param  无
 * @return 0-success, -1-failed
 */
int c_update_thread::uninit()
{
    if (!m_inited) {
        return -1;
    }

    m_continue_working = false;

    m_update_status = 0;
    m_call_thread_id = 0;
    m_p_config = NULL;
    m_update_interval = 0;
    m_inited = false;

    DEBUG_LOG("update thread uninit succ.");
    return 0;
}

/**
 * @brief  备份conf,bin,so目录下面的所有文件
 * @param  无
 * @return 0-success, -1-failed
 */
int c_update_thread::bak_related_file()
{
    ///备份bin目录下文件
    if (link(OA_BIN_FILE, OA_BIN_FILE_BAK) != 0) {
        ERROR_LOG("link(%s, %s) failed: %s", OA_BIN_FILE, OA_BIN_FILE_BAK, strerror(errno));
        return -1;
    }
    DEBUG_LOG("Success to backup bin.");
    ///备份conf目录下文件
    if (link(OA_CONF_FILE, OA_CONF_FILE_BAK) != 0) {
        ERROR_LOG("link(%s, %s) failed: %s", OA_CONF_FILE, OA_CONF_FILE_BAK, strerror(errno));
        return -1;
    }
    DEBUG_LOG("Success to backup conf.");
    ///备份so目录下文件
    DIR *p_dir = opendir(OA_SO_PATH);
    if (p_dir == NULL) {
        ERROR_LOG("opendir(%s) failed: %s", OA_SO_PATH, strerror(errno));
        return -1;
    }

    char src_path[PATH_MAX] = {0};
    char des_path[PATH_MAX] = {0};
    struct dirent *p_drt = NULL;
    while ((p_drt = readdir(p_dir)) != NULL) {
        if (strncmp(p_drt->d_name, ".", 1) == 0         //本级目录或隐藏文件
            || strcmp(p_drt->d_name, "..") == 0) {      //上级目录
            continue;
        }

        sprintf(src_path, "../so/%s", p_drt->d_name);
        sprintf(des_path, "%s%s", OA_SO_BAK_PATH, p_drt->d_name);
        if (link(src_path, des_path) != 0) {
            ERROR_LOG("link(%s) failed: %s", src_path, strerror(errno));
            closedir(p_dir);
            return -1;
        }
    }
    closedir(p_dir);
    DEBUG_LOG("Success to backup all so.");
    return 0;
}

/**
 * @brief  清理bak目录下的所有目录和文件
 * @param  无
 * @return 0-success, -1-failed
 */
int c_update_thread::remove_bak_file()
{
    unlink(OA_BIN_FILE_BAK);
    unlink(OA_CONF_FILE_BAK);
    ///unlink备份so目录下文件
    DIR *p_dir = opendir(OA_SO_BAK_PATH);
    if (p_dir == NULL) {
        return 0;
    }

    char del_file[PATH_MAX] = {0};
    struct dirent *p_drt = NULL;
    while ((p_drt = readdir(p_dir)) != NULL) {
        if (strncmp(p_drt->d_name, ".", 1) == 0         //本级目录或隐藏文件
            || strcmp(p_drt->d_name, "..") == 0) {      //上级目录
            continue;
        }

        sprintf(del_file, "%s%s", OA_SO_BAK_PATH, p_drt->d_name);
        unlink(del_file);
    }

    closedir(p_dir);
    return 0;
}

/**
 * @brief  子线程的工作函数
 * @param  p_data: 指向当前对象的this指针
 * @return (void *)0:success, (void *)-1:failed
 */
int c_update_thread::work_thread_proc(time_t * start)//void *p_data)
{
    time_t end = time(NULL);
    time_t interval = end - *start + 1;
    if (m_update_interval + m_rand <= interval) {
        *start = end;
        memset(&m_update_info, 0, sizeof(m_update_info));

        m_rand = rand() % (m_update_interval / 10) * (rand() % 2 == 0 ? 1 : -1);

        if (exc_update(m_inside_ip, m_update_url)) {
            DEBUG_LOG("exc_update(..) failed, rand update_interval: %us",
                    (u_int)(m_update_interval + m_rand));
        } else {
            DEBUG_LOG("exc_update(..) succ, rand update_interval: %us", (u_int)(m_update_interval + m_rand));
        }
    }
    return 0;
}

/**
 * @brief  执行更新
 * @param  p_ip:  本机的ip地址
 * @param  p_url: 下载服务URL
 * @return 0-success, -1-failed
 */
int c_update_thread::exc_update(const char *p_ip, const char *p_url)
{
#ifdef __TEST_NOT_UPDATE
    return 0;
#endif
    if (p_ip == NULL || p_url == NULL) {
        ERROR_LOG("p_ip=%p p_url=%p", p_ip, p_url);
        strcpy(m_err_reason, "Invalid parameter, cannot be null.");
        return -1;
    }

    char url[OA_MAX_BUF_LEN] = {0};
    strcpy(url, p_url);
    strcpy(m_update_info.program_name, OA_BIN_FILE);
    ///去掉p_url最后的一个路径名
    char *p_last = strrchr(url, '/');
    if (p_last == NULL) {
        strcpy(m_err_reason, "url is invalid.");
        ERROR_LOG("ERROR: url: %s is invalid.", p_url);
        return -1;
    }
    *p_last = 0;
    ///获取待更新列表信息
    char update_list[OA_MAX_BUF_LEN] = {0};
    char md5[33] = {0};
    if (get_file_md5(OA_BIN_FILE, md5) != 0) {
        strcpy(m_err_reason, "calculate oa_node md5 sum");
        return -1;
    }
    DEBUG_LOG("%s %s", OA_BIN_FILE, md5);
    sprintf(update_list, "cmd=10001&version=%s&oa_node_update=%d;%s;%s", OA_UPDATE_VERSION, m_cpu_bit, p_ip, md5);

    if (get_file_md5(OA_CONF_FILE, md5) != 0) {
        strcpy(m_err_reason, "calculate oa_node.ini md5 sum");
        return -1;
    }
    DEBUG_LOG("%s %s", OA_CONF_FILE, md5);

    int pos = 0;
    pos = strlen(update_list);
    sprintf(&update_list[pos], ";%s", md5);
    ///求so目录下文件md5值
    DIR *p_dir = opendir(OA_SO_PATH);
    if (p_dir == NULL) {
        strcpy(m_err_reason, strerror(errno));
        ERROR_LOG("opendir(%s) failed: %s", OA_SO_PATH, strerror(errno));
        return -1;
    }

    char so_path[PATH_MAX] = {0};
    char so_name_list[OA_MAX_BUF_LEN] = {0};
    struct dirent *p_drt = NULL;
    int count = 0;
    while ((p_drt = readdir(p_dir)) != NULL) {
        if (p_drt->d_type == 4) { //目录
            continue;
        }
        if (strncmp(p_drt->d_name, ".", 1) == 0         //本级目录或隐藏文件
            || strcmp(p_drt->d_name, "..") == 0) {      //上级目录
            continue;
        }

        ++count;
        sprintf(so_path, "../so/%s", p_drt->d_name);
        if (get_file_md5(so_path, md5) != 0) {
            sprintf(m_err_reason, "calculate %s md5 sum", so_path);
            closedir(p_dir);
            return -1;
        }
        DEBUG_LOG("%s %s", so_path, md5);
        pos = strlen(so_name_list);
        sprintf(&so_name_list[pos], ";%s;%s", p_drt->d_name, md5);
    }
    closedir(p_dir);//后面不再使用则释放资源

    pos = strlen(update_list);
    sprintf(&update_list[pos], ";%d%s", count, so_name_list);

    if(m_inited) {
        DEBUG_LOG("DEBUG: update_list: %s", update_list);
    }
    m_http_transfer.http_post(p_url, update_list);
    string ret_list = m_http_transfer.get_post_back_data();
    if(m_inited) {
        DEBUG_LOG("DEBUG: ret_list: %s", ret_list.c_str());
    }
    ///解析更新命令
    char up_cmd[OA_MAX_BUF_LEN] = {0};
    oa_update_cmd_t *p_up_cmd = (oa_update_cmd_t *)up_cmd;
    if (analysis_update_cmd(ret_list, p_up_cmd) != 0) {
        strcpy(m_err_reason, "Analysis update command failed.");
        return -1;
    }

    if (p_up_cmd->up_status) {
        ///有文件更新则备份
        remove_bak_file();
        if (m_inited && bak_related_file() != 0) {
            strcpy(m_err_reason, "Backup related files failed.");
            return -1;
        }
        ///开始下载
        m_downloading = true;
        if (download_update_file(p_up_cmd, url, p_ip)) {
            strcpy(m_err_reason, "Download update file failed.");
            ///恢复备份的文件
            if (strlen(m_update_info.program_name) == 0) {
                del_update_and_recover_old(p_up_cmd->conf_cmd != OA_NO_UPDATE, p_up_cmd->so_num > 0, NULL);
            } else {
                del_update_and_recover_old(p_up_cmd->conf_cmd != OA_NO_UPDATE, p_up_cmd->so_num > 0,
                        m_update_info.program_name);
            }
            return -1;
        }
        m_downloading = false;
        ///唤醒main检查更新
        if (m_inited) {
            DEBUG_LOG("kill(%u, SIGUSR2).", (u_int)m_call_thread_id);
            *m_p_need_update = true;
        }
    }
    return 0;
}

/**
 * @brief  解析更新命令
 * @param  rcv_cmd: 接受到的命令字符串
 * @param  p_up_cmd: 解析后的命令
 * @return 0-success, -1-failed, 1-return
 * @note 命令格式: bin_cmd;conf_cmd;so_num;so1_name;so1_cmd;...
 */
int c_update_thread::analysis_update_cmd(string rcv_cmd, oa_update_cmd_t *p_up_cmd)
{
    if (p_up_cmd == NULL) {
        ERROR_LOG("p_up_cmd=%p", p_up_cmd);
        return -1;
    }
    if (rcv_cmd.empty()) {
        p_up_cmd->up_status = false;
        return 0;
    }

    string::size_type start = 0;
    string::size_type end = 0;
    int cmd_id = 0;
    int so_num = 0;
    int index = 0;/**<数组so_cmd的下标*/
    int serial_num = 0;/**<请求命令中第几个分号*/
    string tmp_str;

    do {
        end = rcv_cmd.find_first_of(';', start);
        if (end == string::npos) {
            break;
        }

        ++serial_num;
        tmp_str = rcv_cmd.substr(start, end - start);
        //DEBUG_LOG("tmp_str = %s", tmp_str.c_str());
        switch (serial_num) {
        case 1:
            p_up_cmd->up_status = atoi(tmp_str.c_str());
            //DEBUG_LOG("in 1 case : p_up_cmd->up_status = %d", p_up_cmd->up_status);
            if (!p_up_cmd->up_status) {
                return 0;
            }
            break;
        case 2://bin
            cmd_id = atoi(tmp_str.c_str());
            //DEBUG_LOG("in bin case : cmd_id = %d", cmd_id);
            if (cmd_id != OA_NO_UPDATE && cmd_id != OA_NEED_UPDATE) {
                ERROR_LOG("ERROR: bin cmd_id[%d] wrong: %s", cmd_id, rcv_cmd.c_str());
                return -1;
            }
            p_up_cmd->bin_cmd = cmd_id;
            break;
        case 3://conf
            cmd_id = atoi(tmp_str.c_str());
            //DEBUG_LOG("in conf case : cmd_id = %d", cmd_id);
            if (cmd_id != OA_NO_UPDATE && cmd_id != OA_NEED_UPDATE) {
                ERROR_LOG("ERROR: conf cmd_id[%d] wrong: %s", cmd_id, rcv_cmd.c_str());
                return -1;
            }
            p_up_cmd->conf_cmd = cmd_id;
            break;
        case 4://so_num
            so_num = atoi(tmp_str.c_str());
            //DEBUG_LOG("in so_num case : so_num = %d", so_num);
            if (so_num > 0) {
                p_up_cmd->so_num = so_num;
            } else {
                p_up_cmd->so_num = 0;
            }
            break;
        default:
            if (serial_num % 2 != 0) {
                strcpy(p_up_cmd->so_cmd[index].name, tmp_str.c_str());
                //DEBUG_LOG("in default case : so_cmd[index].name = %s", p_up_cmd->so_cmd[index].name);
            } else {
                cmd_id = atoi(tmp_str.c_str());
                //DEBUG_LOG("in default case : cmd_id = %d", cmd_id);
                switch (cmd_id) {
                case OA_NO_UPDATE:
                case OA_NEED_UPDATE:
                case OA_NEED_DELETE:
                case OA_NEED_ADD:
                    break;
                default:
                    //ERROR_LOG("ERROR: so[%s] cmd_id[%d] wrong: %s",p_up_cmd->so_cmd[index].name, cmd_id, rcv_cmd.c_str());
                    return -1;
                }
                p_up_cmd->so_cmd[index].cmd_id = cmd_id;
                ++index;
            }
            break;
        }

        start = end + 1;
    } while(true);

    if (index != so_num) {
        ERROR_LOG("so_num[%d] is not equal to so name real count[%d]: %s", so_num, index, rcv_cmd.c_str());
        return -1;
    }

    if (p_up_cmd->bin_cmd != OA_NO_UPDATE || p_up_cmd->conf_cmd != OA_NO_UPDATE || p_up_cmd->so_num > 0) {
        p_up_cmd->up_status = true;
    } else {
        p_up_cmd->up_status = false;
    }
    return 0;
}

/**
 * @brief  下载更新文件
 * @param  p_up_cmd: 解析后的更新命令
 * @param  p_url: 下载服务URL
 * @param  p_ip: 待下载机器IP
 * @return 0-success, -1-failed
 */
int c_update_thread::download_update_file(const oa_update_cmd_t *p_up_cmd, const char *p_url, const char *p_ip)
{
    if (p_up_cmd == NULL || p_url == NULL || p_ip == NULL) {
        ERROR_LOG("p_up_cmd=%p p_url=%p p_ip=%p", p_up_cmd, p_url, p_ip);
        return -1;
    }

    char url[OA_MAX_BUF_LEN] = {0};
    char so_path[PATH_MAX] = {0};

    if (m_inited && p_up_cmd->bin_cmd == OA_NEED_UPDATE) {
        time_t now = time(NULL);
        struct tm *p_tm = localtime(&now);
        if (p_tm == NULL) {
            ERROR_LOG("localtime() return NULL: %s", strerror(errno));
            sprintf(m_update_info.program_name, "%s_%d_%u", OA_BIN_FILE, m_cpu_bit, (u_int)now);
        } else {
            char curr_time[256] = {0};
            strftime(curr_time, 256, "%Y%m%d%H%M%S", p_tm);
            sprintf(m_update_info.program_name, "%s_%d_%s", OA_BIN_FILE, m_cpu_bit, curr_time);
        }

        sprintf(url, "%s/bin/%s/%d/oa_node", p_url, OA_UPDATE_VERSION, m_cpu_bit);
        m_http_transfer.download_file(url, m_update_info.program_name);
        DEBUG_LOG("download %s", m_update_info.program_name);
        if (chmod(m_update_info.program_name, 0755)) {
            ERROR_LOG("chmod(%s, 0755) failed: %s", m_update_info.program_name, strerror(errno));
            return -1;
        }
        m_update_info.exec_updated = 1;
        DEBUG_LOG("Dowload bin[oa_node_%u] succ: %s", (u_int)now, url);
    }

    if (p_up_cmd->conf_cmd == OA_NEED_UPDATE) {
        if(oa_unlink(OA_CONF_FILE)) {
            return -1;
        }
        sprintf(url, "%s/conf/%s", p_url, p_ip);
        m_http_transfer.download_file(url, OA_CONF_FILE);
        m_update_info.config_updated = 1;
        DEBUG_LOG("Dowload conf succ: %s", url);
    }

    if (p_up_cmd->so_num > 0) {  //没有分别是哪个进程的so改变了
        i_config *p_config = NULL;
        if (create_config_instance(&p_config) != 0) {
            if (p_config) {
                p_config->release();
                p_config = NULL;
            }
            m_update_info.so_updated_root = 1;
            m_update_info.so_updated_nobody = 1;
            m_update_info.so_updated_command = 1;
            return 0;
        } else if(p_config->init(g_config_file_list, g_config_file_count) != 0) {
            m_update_info.so_updated_root = 1;
            m_update_info.so_updated_nobody = 1;
            m_update_info.so_updated_command = 1;
            return 0;
        }

        int index = 0;
        int cmd_id = 0;
        const char *p_so_name = NULL;
        while (index < p_up_cmd->so_num) {
            cmd_id = p_up_cmd->so_cmd[index].cmd_id;
            p_so_name = p_up_cmd->so_cmd[index].name;
            switch (cmd_id) {
            case OA_NO_UPDATE:
                break;
            case OA_NEED_UPDATE:
            case OA_NEED_ADD:
                sprintf(url, "%s/so/%d/%s", p_url, m_cpu_bit, p_so_name);
                sprintf(so_path, "../so/%s", p_so_name);
                if (oa_unlink(so_path) != 0) {
                    return -1;
                }
                m_http_transfer.download_file(url, so_path);
                DEBUG_LOG("Download so[%s]: %s", p_so_name, url);
                if(is_collect_so(p_so_name, p_config)) {
                    m_update_info.so_updated_root = 1;
                    m_update_info.so_updated_nobody = 1;
                } else {
                    m_update_info.so_updated_command = 1;
                }
    #if 0
                if(m_update_info.so_updated_root == 0) {
                    if(need_so(p_so_name, OA_USER_ROOT, p_config)) {
                        m_update_info.so_updated_root = 1;
                    }
                }
                if(m_update_info.so_updated_nobody == 0) {
                    if(need_so(p_so_name, OA_USER_NOBODY, p_config)) {
                        m_update_info.so_updated_nobody = 1;
                    }
                }
    #endif
                break;
            case OA_NEED_DELETE:    //删除so，config可能已经改变，不进行判断
#if 0
                sprintf(so_path, "../so/%s", p_so_name);
                if (unlink(so_path) != 0) {
                    ERROR_LOG("unlink(%s) failed: %s", so_path, strerror(errno));
                }
                m_update_info.so_updated_root = 1;
                m_update_info.so_updated_nobody = 1;
#endif
                break;
            default:
                ERROR_LOG("ERROR: update[%s] command id(%d) wrong.", p_so_name, cmd_id);
                break;
            }

            ++index;
        }

        if(p_config) {
            if (p_config->uninit() != 0) {
        		ERROR_LOG("ERROR: p_config->uninit().");
        	} else if (p_config->release() != 0) {
        		ERROR_LOG("ERROR: p_config->release().");
    	    }
        }
    }

    return 0;
}

/**
 * @brief  发送获取更新URL的广播
 * @param  p_snd_data: 待发送的数据
 * @param  data_len: 数据的长度
 * @param  p_ip: 机器IP
 * @param  port: 发送的端口
 * @return 0-success, -1-failed
 */
int c_update_thread::send_update_broadcast(const char *p_snd_data, int data_len, const char *p_ip, int port)
{
    ///参数检查
    //if (p_snd_data == NULL || data_len < sizeof(oa_cmd_t))
    if (p_snd_data == NULL || data_len <= 0 || p_ip == NULL || port <=0 || port > 65535) {
        ERROR_LOG("p_snd_data=%p p_ip=%p data_len=%d[<=0] port=%d[<=0 >65535]", p_snd_data, p_ip, data_len, port);
        return -1;
    }

    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, p_ip, &peer_addr.sin_addr.s_addr) <= 0) {
        ERROR_LOG("ERROR: wrong broadcast IP: %s", p_ip);
        return -1;
    }

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        ERROR_LOG("socket(SOCK_DGRAM) failed: %s", strerror(errno));
        return -1;
    }
    ///设置要加入组播的地址
    struct ip_mreq mreq;
    memcpy(&mreq.imr_multiaddr.s_addr, &peer_addr.sin_addr, sizeof(struct in_addr));
    ///设置发送组播消息的源主机的地址信息
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) != 0) {
        ERROR_LOG("setsockopt(IP_ADD_MEMBERSHIP) failed, %s:%d(IP:Port)", p_ip, port);
        return -1;
    }
    ///设置地址重用
    int reuse_flag = true;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag, sizeof(reuse_flag)) != 0) {
        ERROR_LOG("setsockopt(IP[%s], %d, SO_REUSEADDR) failed: %s", p_ip, port, strerror(errno));
        return -1;
    }

    if (bind(socket_fd, (struct sockaddr *) &peer_addr, sizeof(peer_addr)) == -1) {
        ERROR_LOG("Bind(%s:%d) failed: %s", p_ip, port, strerror(errno));
        return -1;
    }
    ///发送广播
    int snd_len = sendto(socket_fd, p_snd_data, data_len, 0, (struct sockaddr*)&peer_addr, sizeof(struct sockaddr_in));

    if (snd_len < 0) {
        ERROR_LOG("ERROR: sendto() failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/**
 * @brief  接受更新URL
 * @param  p_buff: 保存URL的缓冲区
 * @param  buff_len: 缓冲区大小
 * @param  timeout: 超时限制
 * @return 0-success, -1-failed
 */
int c_update_thread::recv_update_url(const char *p_ip, int port, char *p_buff, int buff_len, int timeout)
{
    if (p_buff == NULL || buff_len <= 0 || timeout < 0) {
        ERROR_LOG("p_buff=%p buff_len=%d[<=0] timeout=%d", p_buff, buff_len, timeout);
        return -1;
    }

    struct sockaddr_in lsn_addr;
    memset(&lsn_addr, 0, sizeof(lsn_addr));
    lsn_addr.sin_family = AF_INET;
    lsn_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, p_ip, &lsn_addr.sin_addr.s_addr) <= 0) {
        ERROR_LOG("ERROR: wrong listen IP: %s", p_ip);
        return -1;
    }

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        ERROR_LOG("socket(SOCK_DGRAM) failed: %s", strerror(errno));
        return -1;
    }
    ///设置地址重用
    int reuse_flag = true;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag, sizeof(reuse_flag)) != 0) {
        ERROR_LOG("setsockopt(IP[%s], %d, SO_REUSEADDR) failed: %s", p_ip, port, strerror(errno));
        return -1;
    }
    if (bind(socket_fd, (struct sockaddr *) &lsn_addr, sizeof(lsn_addr)) == -1) {
        ERROR_LOG("Bind(%s:%d) failed: %s", p_ip, port, strerror(errno));
        return -1;
    }
    ///接受UDP
    struct timeval tv;
    fd_set rd_set;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&rd_set);
    FD_SET(socket_fd, &rd_set);

    int result = select(socket_fd + 1, &rd_set, NULL, NULL, &tv);
    if (result < 0) {
        ERROR_LOG("ERROR: select() failed: %s", strerror(errno));
        return -1;
    } else if (result == 0) {   //超时
        DEBUG_LOG("DEBUG: select() timeout[%d]", timeout);
        return -1;
    }

    int rcv_len = recv(socket_fd, p_buff, buff_len, 0);
    if (rcv_len < 0) {
        ERROR_LOG("ERROR: recv() failed: %s", strerror(errno));
        return -1;
    }
    if ((size_t)rcv_len <= sizeof(response_cmd_t)) {
        ERROR_LOG("ERROR: rcv_len[%d] smaller than header len[%lu].", rcv_len, sizeof(response_cmd_t));
        return -1;
    }

    response_cmd_t *p_cmd = (response_cmd_t *)p_buff;
    if (rcv_len < p_cmd->pkg_len) {
        ERROR_LOG("ERROR: rcv_len[%d] smaller than pkg_len[%d].", rcv_len, p_cmd->pkg_len);
        return -1;
    }
    p_buff[p_cmd->pkg_len] = 0;
    return 0;
}

/**
 * @brief  通过广播执行更新命令
 * @param  p_host_ip: 本机的ip地址
 * @param  p_broadcast_ip: 广播ip地址
 * @return 0-success, -1-failed
 */
int c_update_thread::exc_update_by_broadcast(const char *p_host_ip, const char *p_broadcast_ip, int timeout = 5)
{
    if (p_host_ip == NULL || p_broadcast_ip == NULL) {
        ERROR_LOG("p_host_ip=%p p_broadcast_ip=%p", p_host_ip, p_broadcast_ip);
        return -1;
    }

    char buff[OA_MAX_UDP_MESSAGE_LEN] = {0};
    response_cmd_t *p_cmd = (response_cmd_t *)buff;
    p_cmd->pkg_len = sizeof(response_cmd_t);
    p_cmd->cmd_id = OA_UPDATE_REQ_CMD_ID;

    if (send_update_broadcast(buff, p_cmd->pkg_len, p_broadcast_ip, OA_UPDATE_PORT) != 0) {
        ERROR_LOG("ERROR: send_update_broadcast() failed.");
        return -1;
    }

    memset(buff, 0, sizeof(buff));
    if (recv_update_url(p_host_ip, OA_UPDATE_PORT, buff, sizeof(buff), timeout) != 0) {
        ERROR_LOG("ERROR: recv_update_url() failed.");
        return -1;
    }

    return exc_update(p_host_ip, p_cmd->url);
}

bool c_update_thread::is_collect_so(const char * so_name, i_config *p_config)
{
    /*
    char section[OA_MAX_STR_LEN];
    char name[OA_MAX_STR_LEN];
    char buffer[OA_MAX_STR_LEN];
    int so_num = 0;

    sprintf(name, "%s", "so_num");
    if(p_config->get_config("node_info", name, buffer, OA_MAX_STR_LEN) == 0) {
        so_num = atoi(buffer);
    } else {    //获取配置出错
        ERROR_LOG("get config [%s:%s] error", "node_info", name);
        return false;
    }

    sprintf(section, "%s", "so_name");
    for(int i=0; i<so_num; i++) {
            sprintf(name, "%s_%d", "so_name", i+1);
        if(p_config->get_config(section, name, buffer, OA_MAX_STR_LEN) == 0) {
            if(strcmp(so_name, buffer) == 0) {
                return true;
            }
        } else { //获取配置出错
            ERROR_LOG("get config [%s:%s] error", section, name);
            return false;
        }
    }

    return false;
    */
    if(strstr(so_name, "mgr.so") != 0) {    //command进程使用so，必须以mgr.so结尾
        return false;
    } else {
        return true;
    }
}
