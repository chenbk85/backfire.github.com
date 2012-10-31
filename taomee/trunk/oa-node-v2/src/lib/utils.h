/* vim: set tabstop=4 softtabstop=4 shiftwidth=4: */
/**
 * @file utils.h
 * @author richard <richard@taomee.com>
 * @date 2010-03-09
 */

#ifndef UTILS_H_2010_03_09
#define UTILS_H_2010_03_09

#include <stdint.h>
#include <rpc/rpc.h>

#include "../i_config.h"
#include "../defines.h"

/**
 * @brief 为指定的信号安装信号处理函数
 * @param sig 要处理的信号
 * @param signal_handler 信号处理函数
 * @return 成功返回0，失败返回-1
 */
int mysignal(int sig, void(*signal_handler)(int));

/**
 * @brief 将程序的pid写入文件
 * @return 成功返回0，失败返回-1
 */
int write_pid_to_file(const char * filename);

/**
 * @brief 创建文件
 * @return 成功返回0，失败返回-1
 */
int create_file(const char * filename);
/**
 * @brief 初始化设置proc标题
 * @param argc main函数的第一个参数
 * @param argv main函数的第二个参数
 * @return 无
 */
void init_proc_title(int argc, char *argv[]);

/**
 * @brief 设置proc标题
 * @param fmt 标题格式
 * @param ... 可变参数
 * @return 无
 */
void set_proc_title(const char *fmt, ...);

/**
 * @brief 反初始化设置proc标题
 * @return 无
 */
void uninit_proc_title();

/**
 * @brief 将数据发送到多播地址
 * @param sockfd    套接口描述符
 * @param data      需要发送的数据
 * @param length    需要发送的数据的长度
 * @param peer_ip   对端的ip地址
 * @param peer_port 对端的端口
 * @return 0:成功, -1:失败
 */
int publish_data(int sockfd, const char *data, const u_int length, const char *peer_ip, const int peer_port);

/**
 * @brief 删除更新的文件，并将上次正确运行的文件还原
 * @param config_updated  配置有更新为true，否则为false
 * @param so_updated      so有更新为true，否则为false
 * @param prog_name       更新的程序名，没有更新为NULL
 * @return 0:成功, -1:失败
 */
int del_update_and_recover_old(bool config_updated, bool so_updated, const char *prog_name);

/**
 * @brief  目录不存在则创建访问权限为mode的dir，否则更新dir的访问权限为mode
 * @param  dir: 待更新目录
 * @param  mode: 更新后目录的访问权限
 * @return 0-success, -1-failed
 */
int oa_change_dir(const char * dir, int mode);

/**
 * @brief  通过网卡名获取机器IP地址
 * @param  eth_name: 网卡类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip_by_name(const char *eth_name, char *host_ip);

/**
 * @brief  获取机器IP地址
 * @param  ip_type: IP类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip(int ip_type, char *host_ip);

char * get_ip(i_config * p_config, char * inside_ip);

void set_proc_name(int argc, char ** argv, const char *fmt, ...);

/**
 * @brief  计算文件md5值,文件不存在则返回空
 * @param  file_path: 文件路径
 * @param  md5: 计算出来的md5值，char md5[33]
 * @return 0-success, -1-failed
 */
int get_file_md5(const char *file_name, char *md5);

int create_and_init_config_instance(i_config **pp_config);

int uninit_and_release_config_instance(i_config *p_config);

/**
 * @brief  setuid到配置文件指定的用户
 * @return 0:success, -1:failed
 */
int change_user(const char * user);

/**
 * @brief  判断是否是接受udp数据的node
 * @param  p_config 配置类的指针
 * @return 0:success, -1:failed
 */
int set_recv_udp(i_config *p_config, bool * is_recv_udp);

/**
 * @brief  判断是否是内网主机
 * @param  p_config 配置类的指针
 * @return true:内网主机, false:外网主机
 */
bool get_listen_type(i_config * p_config);

#endif //UTILS_H_2010_03_09