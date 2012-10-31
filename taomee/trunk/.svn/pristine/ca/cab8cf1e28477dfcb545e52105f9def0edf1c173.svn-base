/**
 * =====================================================================================
 *       @file  update_thread.h
 *      @brief  自动更新服务
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  11/24/2010 10:51:01 AM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-24 14:33:04
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef OA_UPDATE_THREAD_H_2010_11_24
#define OA_UPDATE_THREAD_H_2010_11_24

#include <string.h>
#include <errno.h>
#include "../proto.h"
#include "../i_config.h"
#include "../lib/http_transfer.h"

#define OA_NO_UPDATE    1000
#define OA_NEED_UPDATE  1001
#define OA_NEED_DELETE  1002
#define OA_NEED_ADD     1003
#define OA_WRONG_FMT    2000 /**<请求更新时协议格式有误*/

#define OA_SO_NAME_MAX_LEN      64
#define OA_UPDATE_REQ_CMD_ID    1001
#define OA_UPDATE_PORT          55000

/**
 * @class 更新线程
 */
class c_update_thread
{
public:
    c_update_thread(bool * need_updat);
    ~c_update_thread();

    /**
     * @brief  初始化函数
     * @param  p_config: 配置对象指针指针
     * @param  reboot: true-重启，false-未重启
     * @param  thread_id: 调用线程id，用于唤醒id对应的线程
     * @return 0-success, -1-failed
     */
    int init(i_config *p_config, pthread_t thread_id);

    /**
     * @brief  执行更新命令
     * @param  p_ip: 本机的ip地址
     * @param  p_url: 下载服务URL
     * @param  b_first_start: 第一次启动标志位
     * @return 0-success, -1-failed
     */
    int exc_update(const char *p_ip, const char *p_url);

    /**
     * @brief  通过广播执行更新命令
     * @param  p_host_ip: 本机的ip地址
     * @param  p_broadcast_ip: 广播ip地址
     * @return 0-success, -1-failed
     */
    int exc_update_by_broadcast(const char *p_host_ip, const char *p_broadcast_ip, int timeout);

    /**
     * @brief  获取最近一次错误描述
     * @param  无
     * @return 错误描述
     */
    char *get_last_error();

    /**
     * @brief  获取更新参数
     * @param  p_up_info: 更新信息
     * @return 无
     */
    void get_update_info(update_info_t *p_up_info);

    /**
     * @brief  设置更新状态
     * @param  status: true-更新成功，false-更新失败
     * @return 无
     */
    void set_update_status(bool status);

    /**
     * @brief  反初始化函数
     * @param  无
     * @return 0-success, -1-failed
     */
    int uninit();

//protected:
public:
    /**
     * @struct so更新命令结构
     */
    typedef struct {
        u_int cmd_id;
        char name[OA_SO_NAME_MAX_LEN];
    } oa_so_cmd_t;

    /**
     * @struct 更新命令结构
     */
    typedef struct {
        bool up_status;
        u_int bin_cmd;
        u_int conf_cmd;
        int so_num;
        oa_so_cmd_t so_cmd[0];
    } oa_update_cmd_t;

    /**
     * @struct 返回命令协议包
      */
    typedef struct {
        uint16_t pkg_len;
        uint16_t cmd_id;    /**<命令ID*/
        char url[0];        /**<命令内容*/
    } __attribute__((__packed__)) response_cmd_t;

    /**
     * @brief  删除存在的文件
     * @param  file_path: 文件路径
     * @return 0-success, -1-failed
     */
    int oa_unlink(const char* file_path);

    /**
     * @brief  清理bak目录下的所有目录和文件
     * @param  无
     * @return 0-success, -1-failed
     */
    int remove_bak_file();

    /**
     * @brief  备份conf,bin,so目录下面的所有文件
     * @param  无
     * @return 0-success, -1-failed
     */
    int bak_related_file();

    /**
     * @brief  解析更新命令
     * @param  rcv_cmd: 接受到的命令字符串
     * @param  p_up_cmd: 解析后的命令
     * @param  b_first_start: 第一次启动标志位
     * @return 0-success, -1-failed
     * @note 命令格式: bin_cmd;conf_cmd;so_num;so1_name;so1_cmd;...
     */
    int analysis_update_cmd(std::string rcv_cmd,  oa_update_cmd_t *p_up_cmd);

    /**
     * @brief  下载更新文件
     * @param  p_up_cmd: 解析后的更新命令
     * @param  p_url: 下载服务URL
     * @param  p_ip: 待下载机器IP
     * @return 0-success, -1-failed
     */
    int download_update_file(const oa_update_cmd_t *p_up_cmd, const char *p_url, const char *p_ip);

    /**
     * @brief  子线程的工作函数
     * @param  p_data: 指向当前对象的this指针
     * @return (void *)0:success, (void *)-1:failed
     */
    int work_thread_proc(time_t * start);//void *p_data);

    /**
     * @brief  发送获取更新URL的广播
     * @param  p_snd_data: 待发送的数据
     * @param  data_len: 数据的长度
     * @param  p_ip: 机器IP
     * @param  port: 发送的端口
     * @return 0-success, -1-failed
     */
    int send_update_broadcast(const char *p_snd_data, int data_len, const char *p_ip, int port);

    /**
     * @brief  接受更新URL
     * @param  p_buff: 保存URL的缓冲区
     * @param  buff_len: 缓冲区大小
     * @param  timeout: 超时限制
     * @return 0-success, -1-failed
     */
    int recv_update_url(const char *p_ip, int port, char *p_buff, int buff_len, int timeout);

    bool is_collect_so(const char * so_name, i_config *p_config);

private:
    bool m_inited;
    bool m_continue_working;
    bool m_downloading;         /**<true:正在下载， false:下载完成*/
    bool * m_p_need_update;
    int m_update_status;        /**<-1:上次更新失败，0:初始化，1:更新成功*/
    int m_cpu_bit;
    pthread_t m_call_thread_id;

    i_config *m_p_config;
    Chttp_transfer m_http_transfer;
    update_info_t m_update_info;

    char m_err_reason[1024];
    char m_update_url[1024];
    char m_inside_ip[16];
    time_t m_update_interval;
    int  m_rand;
};

/**
 * @brief  获取最近一次错误描述
 * @param  无
 * @return 错误描述
 */
inline char *c_update_thread::c_update_thread::get_last_error()
{
    return m_err_reason;
}

/**
 * @brief  获取更新参数
 * @param  p_up_info: 更新信息
 * @return 无
 */
inline void c_update_thread::get_update_info(update_info_t * p_up_info)
{
    if(p_up_info == NULL) {
        ERROR_LOG("ERROR: invalid parameter, cannot be NULL.");
        return;
    }

    if (m_downloading) {
        ERROR_LOG("cannot get update info while downloading");
        return;
    }

    memcpy(p_up_info, &m_update_info, sizeof(update_info_t));
}


/**
 * @brief  设置更新状态
 * @param  status: true-更新成功，false-更新失败
 * @return 无
 */
inline void c_update_thread::set_update_status(bool status)
{
    m_update_status = status ? 1 : -1;
}

/**
 * @brief  删除存在的文件
 * @param  file_path: 文件路径
 * @return 0-success, -1-failed
 */
inline int c_update_thread::oa_unlink(const char* file_path)
{
    if(file_path == NULL) {
        ERROR_LOG("ERROR: invalid parameter, cannot be NULL.");
        return -1;
    }
    ///不存在则返回
    if(access(file_path, F_OK)) {
        return 0;
    }

    if(unlink(file_path)) {
        ERROR_LOG("unlink(%s) failed: %s", file_path, strerror(errno));
        return -1;
    }

    return 0;
}

#endif
