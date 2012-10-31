/**
 * =====================================================================================
 *       @file  db_operator.h
 *      @brief  
 *
 *      封装的数据库操作函数类
 *
 *   @internal
 *     Created  2011-04-18 11:13:42
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef DB_OPERATOR_H
#define DB_OPERATOR_H

#include "defines.h"
#include "proto.h"
#include "lib/c_mysql_iface.h"

/** 
* @brief   获取head更新标志位
* @param   db_conn  数据库连接
* @param   proc_type 进程类型
* @return  true 有更新 false 无更新
*/
bool db_get_update_status(c_mysql_iface* db_conn, int proc_type);

/** 
* @brief   设置head更新标志位
* @param   db_conn  数据库连接
* @param   proc_type 进程类型
* @return  0-success -1-failed
*/
int db_set_update_status(c_mysql_iface* db_conn, int proc_type);

/** 
* @brief   清理数据源的信息
* @param   db_conn     数据库连接
* @param   ds_id       the project(or module) id
* @param   project_id  the project id this data source belongs to
* @return  0-success -1-failed
*/
int db_clear_ds_info(c_mysql_iface* db_conn, unsigned int ds_id, unsigned int project_id, unsigned int ds_type);

/** 
* @brief   更新数据源的信息
* @param   db_conn     数据库连接
* @param   ds_id       the project(or module) id
* @param   project_id  the project id this data source belongs to
* @param   ds        根数据源结构指针
* @return  0-success -1-failed
*/
int db_update_ds_info(c_mysql_iface* db_conn, unsigned int ds_id, unsigned int project_id, unsigned int ds_type, const source_t* ds);

typedef struct update_host_info_arg_t
{
    c_mysql_iface* db_conn;
    const char* host_name;
}update_host_info_arg_t;
/** 
* @brief   更新host中数据库实例的信息
* @param   db_conn     数据库连接
* @param   host        host_t结构指针
* @return  0-success -1-failed
*/
int db_update_mysql_instance_info(c_mysql_iface* db_conn, const host_t* host);

/**
 * @brief  更新数据库实例信息到数据库的回调函数
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return 0:不是ok状态,-1:是ok状态
 */
int do_update_instance_info(datum_t *key, datum_t *val, void *arg);

/** 
* @brief   更新host的信息
* @param   db_conn     数据库连接
* @param   host_name   the host name
* @param   metric      指定要更新的metric，如果为空则全部metric更新
* @param   host        host_t结构指针
* @return  0-success -1-failed
*/
int db_update_host_info(c_mysql_iface* db_conn, const char *host_name, const char *metric, const host_t* host);


/** 
* @brief   从数据库中获取配置信息
* @param   db_conn   数据库连接
* @param   p_network_cfg  网络配置结构指针
* @return  0-success -1-failed
*/
int db_get_network_conf(c_mysql_iface* db_conn, network_conf_t *p_network_cfg);
int db_get_collect_conf(c_mysql_iface *db_conn, collect_conf_t *p_collect_cfg);

/** 
* @brief   从数据库中获取数据源信息
* @param   data_src 保存数据源的map的指针
* @param   db_conn  数据库连接
* @return  0-success -1-failed
*/
int db_get_data_source(ds_t *data_src, c_mysql_iface* db_conn);

int get_inside_ip_by_outside_ip(c_mysql_iface* db_conn, const char* outside_ip, char *inside_ip);
/** 
* @brief   从数据库删除主机的信息信息
* @param   db_conn  数据库连接
* @param   host_name  主机的server_tag
* @param   ip  主机的ip
* @return  0-success -1-failed
*/
int db_del_host_info(c_mysql_iface* db_conn, const char* host_name, const char* ip);

/** 
* @brief   从数据库中获取metric的缺省报警相关信息
* @param   p_metric_set 保存metric_alarm_t结构的vector指针
* @param   db_conn      数据库连接
* @return  0-success -1-failed
*/
int db_get_default_metric_alarm_info(c_mysql_iface* db_conn, metric_alarm_vec_t *p_metric_set);

/** 
* @brief   从数据库中获取metric的特殊报警相关信息
* @param   p_metric_map 保存metric_alarm_t结构的vector指针
* @param   db_conn      数据库连接
* @return  0-success -1-failed
*/
int db_get_special_metric_alarm_info(c_mysql_iface* db_conn, metric_alarm_map_t *p_metric_map);


/** 
* @brief   从数据库中获取head下面所有node命令通道端口
* @param   p_ip_port  <IP, port>map指针
* @param   db_conn   数据库连接
* @return  0-success -1-failed
*/
int db_get_ip_port(ip_port_map_t *p_ip_port, c_mysql_iface* db_conn);

/** 
* @brief   获取网段值
* @param   db_conn   数据库连接
* @return  -1-failed, >=0-网段值
*/
int db_get_segment(c_mysql_iface* db_conn);

#endif
