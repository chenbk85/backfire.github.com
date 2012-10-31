/**
 * =====================================================================================
 *       @file  db_operator.cpp
 *      @brief   
 *
 *  数据库操作函数
 *
 *   @internal
 *     Created  18/04/2011 06:31:41 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason , mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "lib/log.h"
#include "lib/utils.h"
#include "db_operator.h"

//标示这个grid的id(定义在main.cpp里)
extern int g_grid_id; 
extern int g_grid_segment; 

using namespace std;

/** 
* @brief   获取head更新标志位
* @param   db_conn  数据库连接
* @param   proc_type 进程类型
* @return  true-有更新 false-无更新
*/
bool db_get_update_status(c_mysql_iface* db_conn, int proc_type)
{
    if(NULL == db_conn || NULL == db_conn->get_conn()) {
        ERROR_LOG("ERROR: paramter cannot be NULL.");
        return false;
    }

    char select_sql[MAX_STR_LEN] = {'\0'};
    //sprintf(select_sql, "SELECT update_status FROM v_grid_info WHERE grid_id=%d and grid_service_flag=%u",
    //g_grid_id, g_grid_service_flag);
    if (PROC_NETWORK == proc_type) {
        sprintf(select_sql, "SELECT network_update FROM t_head_status WHERE head_id=%d", g_grid_id);
    } else {
        sprintf(select_sql, "SELECT is_updated FROM t_head_status WHERE head_id=%d", g_grid_id);
    }

    MYSQL_ROW row = NULL;
    int  row_count = -1;
    row_count = db_conn->select_first_row(&row, select_sql);
    if (row_count < 0) {
        ERROR_LOG("Database ERROR: db_conn->select_first_row failed.SQL:[%s], error desc:[%s]", 
                select_sql, db_conn->get_last_errstr());
        return false;
    } else if (0 == row_count) {
        ERROR_LOG("Database ERROR: no record of grid[%d] in v_grid_info.", g_grid_id);
        return false;
    } else if (row_count > 1) {
        ERROR_LOG("Database ERROR: too many[%d] records of grid[%d] in v_grid_info.", row_count, g_grid_id);
        return false;
    }

    if (row[0] && row[0][0] == 'Y') {
        return true;
    } else {
        return false;
    }
}

/** 
* @brief   设置head更新标志位
* @param   db_conn  数据库连接
* @param   proc_type 进程类型
* @return  0-success -1-failed
*/
int db_set_update_status(c_mysql_iface* db_conn, int proc_type)
{
    if (NULL == db_conn || NULL == db_conn->get_conn()) {
        ERROR_LOG("ERROR: paramter cannot be NULL.");
        return -1;
    }

    char update_sql[MAX_STR_LEN] = {'\0'};
    if (PROC_NETWORK == proc_type) {
        sprintf(update_sql, "UPDATE t_head_status SET network_update='N' WHERE head_id=%d", g_grid_id);
    } else {
        sprintf(update_sql, "UPDATE t_head_status SET is_updated='N' WHERE head_id=%d", g_grid_id);
    }

    if(db_conn->execsql(update_sql) < 0) {
        ERROR_LOG("Database ERROR: db_conn->execsql failed.SQL:[%s] db error is:[%s]",
                update_sql, db_conn->get_last_errstr());
        return -1;
    }

    return 0;
}

/** 
* @brief   清理数据源的信息
* @param   db_conn     数据库连接
* @param   ds_id       the project(or module) id
* @param   project_id  the project id this data source belongs to
* @return  0-success -1-failed
*/
//int db_clear_ds_info(c_mysql_iface* db_conn, unsigned int ds_id, unsigned int project_id, unsigned int ds_type)
//{
//    if(db_conn == NULL)
//    {
//        ERROR_LOG("Parament error.");
//        return -1;
//    }
//
//    if(db_conn->execsql("UPDATE IGNORE t_ds_info SET metric_val=''\
//                WHERE ds_id=%u and project_id=%u and ds_type=%u", 
//                ds_id, project_id, ds_type) < 0)
//    {
//        ERROR_LOG("Could not clear the data source %s_%u's %s info in db,db error:%s", 
//                ds_type == 1 ? "grid" : "cluster", ds_id, "all", db_conn->get_last_errstr());
//    }
//
//    return 0;
//}

/** 
* @brief   更新数据源的信息
* @param   db_conn     数据库连接
* @param   ds_id       the project(or module) id
* @param   project_id  the project id this data source belongs to
* @param   root        根数据源结构指针
* @return  0-success -1-failed
*/
/*
int db_update_ds_info(c_mysql_iface* db_conn, unsigned int ds_id, unsigned int project_id, unsigned int ds_type, const source_t* ds)
{
    if(db_conn == NULL || ds == NULL)
    {
        ERROR_LOG("Parament error.");
        return -1;
    }

    source_t *p_ds = (source_t*)ds;
    datum_t   hash_key = {0}; 
    char      metirc_name[MAX_URL_LEN] = {0};
    datum_t  *cpu_num = NULL, *load_one = NULL, *load_five = NULL, *load_fifteen = NULL,
             *disk_total = NULL, *disk_free = NULL, *mem_total = NULL;

    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'hosts_up','%u') ON DUPLICATE KEY UPDATE metric_val='%u'",
    //            ds_id, project_id, ds_type, p_ds->hosts_up, p_ds->hosts_up) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, "hosts_up", db_conn->get_last_errstr());
    //}

    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'hosts_down','%u') ON DUPLICATE KEY UPDATE metric_val='%u'",
    //            ds_id, project_id, ds_type, p_ds->hosts_down, p_ds->hosts_down) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, "hosts_down", db_conn->get_last_errstr());
    //}

    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'local_time','%u') ON DUPLICATE KEY UPDATE metric_val='%u'",
    //            ds_id, project_id, ds_type, p_ds->localtime, p_ds->localtime) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, "local_time", db_conn->get_last_errstr());
    //}

    strncpy(metirc_name, "cpu_num", sizeof(metirc_name));
    hash_key.data = (void *)metirc_name;
    hash_key.size = strlen(metirc_name) + 1;
    cpu_num = hash_lookup(&hash_key, p_ds->metric_summary); 
    if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
                VALUES(%u,%u,%u,'%s','%.0f') ON DUPLICATE KEY UPDATE metric_val='%.0f'",
                ds_id, project_id, ds_type, metirc_name, 
                cpu_num != NULL ? ((metric_t *)cpu_num->data)->val.d : 0,
                cpu_num != NULL ? ((metric_t *)cpu_num->data)->val.d : 0) < 0)
    {
        ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
                ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    }

    //strncpy(metirc_name, "mem_total", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //mem_total = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            mem_total != NULL ? ((metric_t *)mem_total->data)->val.d : 0,
    //            mem_total != NULL ? ((metric_t *)mem_total->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    //strncpy(metirc_name, "load_one", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //load_one = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            load_one != NULL ? ((metric_t *)load_one->data)->val.d : 0,
    //            load_one != NULL ? ((metric_t *)load_one->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    //strncpy(metirc_name, "load_five", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //load_five = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            load_five != NULL ? ((metric_t *)load_five->data)->val.d : 0,
    //            load_five != NULL ? ((metric_t *)load_five->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    //strncpy(metirc_name, "load_fifteen", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //load_fifteen = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            load_fifteen != NULL ? ((metric_t *)load_fifteen->data)->val.d : 0,
    //            load_fifteen != NULL ? ((metric_t *)load_fifteen->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    //strncpy(metirc_name, "disk_total", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //disk_total = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            disk_total != NULL ? ((metric_t *)disk_total->data)->val.d : 0,
    //            disk_total != NULL ? ((metric_t *)disk_total->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    //strncpy(metirc_name, "disk_free", sizeof(metirc_name));
    //hash_key.data = (void *)metirc_name;
    //hash_key.size = strlen(metirc_name) + 1;
    //disk_free = hash_lookup(&hash_key, p_ds->metric_summary); 
    //if(db_conn->execsql("INSERT INTO t_ds_info(ds_id,project_id,ds_type,metric_name,metric_val)\
    //            VALUES(%u,%u,%u,'%s','%0.2f') ON DUPLICATE KEY UPDATE metric_val='%0.2f'",
    //            ds_id, project_id, ds_type, metirc_name, 
    //            disk_free != NULL ? ((metric_t *)disk_free->data)->val.d : 0,
    //            disk_free != NULL ? ((metric_t *)disk_free->data)->val.d : 0) < 0)
    //{
    //    ERROR_LOG("Could not update the data source %s_%u's %s info in db,db error:%s", 
    //            ds_type == 1 ? "grid" : "cluster", ds_id, metirc_name, db_conn->get_last_errstr());
    //}

    datum_free(cpu_num);  
    //datum_free(mem_total);  
    //datum_free(load_one);  
    //datum_free(load_five);  
    //datum_free(load_fifteen);  
    //datum_free(disk_total);  
    //datum_free(disk_free);  

    return 0;
}
*/
/** 
* @brief   更新host中数据库实例的信息
* @param   db_conn     数据库连接
* @param   host        host_t结构指针
* @return  0-success -1-failed
*/
//int db_update_mysql_instance_info(c_mysql_iface* db_conn, const host_t* host)
//{
//    if(db_conn == NULL || host == NULL)
//    {
//        ERROR_LOG("Parament error.");
//        return -1;
//    }
//    hash_t *metrics = host->metrics;
//    if(metrics == NULL) return -1;
//    update_host_info_arg_t tmp_arg = {db_conn, host->ip};
//    hash_foreach(metrics, do_update_instance_info, &tmp_arg);
//    return 0;
//}

/**
 * @brief  更新host到数据库的回调函数
 * @param  key: hash的键
 * @param  val: hash里对应key的值
 * @param  arg: 传给回调函数的用户自定义参数
 * @return 0:不是ok状态,-1:是ok状态
 */
int do_update_host_info(datum_t *key, datum_t *val, void *cb_arg)
{
    const char *alarm_type_str = getfield(((metric_t *)val->data)->strings, ((metric_t *)val->data)->alarm_type);
    int alarm_type = 0;
    if(alarm_type_str == NULL || strlen(alarm_type_str) <= 0 || 
            !is_integer(alarm_type_str) || (alarm_type = atoi(alarm_type_str)) > 255)
        return 0;

    if((((unsigned char)alarm_type) & MYSQL_METRIC_TYPE) == 0)
        return 0;

    const char *val_str = getfield(((metric_t *)val->data)->strings, ((metric_t *)val->data)->valstr);
    if(val_str == NULL || strlen(val_str) <= 0)
        return 0;

    const char *arg = getfield(((metric_t *)val->data)->strings, ((metric_t *)val->data)->arg);
    const char *metric_name = (const char*)key->data;
    c_mysql_iface* db_conn = ((update_host_info_arg_t*)cb_arg)->db_conn;
    const char *host_name = ((update_host_info_arg_t*)cb_arg)->host_name;

    string tmp_metric_name(metric_name);
    if(arg && strlen(arg) > 0)
    {    
        char arg_md5[33] = {0}; 
        unsigned char md5_buf[16] = {0}; 
        MD5((const unsigned char *)arg, strlen(arg), md5_buf);
        char *p_md5 = arg_md5;
        for(int j = 0; j < 16; j++) 
        {    
            sprintf(p_md5, "%02x", md5_buf[j]);
            p_md5 += 2;
        }    

        size_t found = tmp_metric_name.find(arg_md5);
        if(found != string::npos)
            tmp_metric_name = tmp_metric_name.substr(0, found - 1);
    }    

    char escaped_val_str[1024] = {0};
    char escaped_arg_str[1024] = {0};
    mysql_real_escape_string(db_conn->get_conn(), escaped_val_str, val_str, strlen(val_str));
    if(arg && strlen(arg) > 0)
        mysql_real_escape_string(db_conn->get_conn(), escaped_arg_str, arg, strlen(arg));
    if(db_conn->execsql("INSERT INTO t_host_info(host_name,metric_name,metric_arg,metric_val)\
                VALUES('%s','%s','%s','%s') ON DUPLICATE KEY UPDATE metric_val='%s'",
                host_name, tmp_metric_name.c_str(), escaped_arg_str, escaped_val_str, escaped_val_str) < 0)
    {
        ERROR_LOG("Could not update the host %s's metrics info in db,db error:%s", 
                host_name, db_conn->get_last_errstr());
    }

    return 0;
}

/** 
* @brief   更新host的信息
* @param   db_conn     数据库连接
* @param   host_name   the host name
* @param   metric      指定要更新的metric，如果为空则更新全部metric
* @param   host        host_t结构指针
* @return  0-success -1-failed
*/
int db_update_host_info(c_mysql_iface* db_conn, const char *host_name, const char *metric, const host_t* host)
{
    if(db_conn == NULL || host_name == NULL || host == NULL) {
        ERROR_LOG("ERROR: Parament cannot be NULL.");
        return -1;
    }

    if(db_conn->get_conn() == NULL) {
        return -1;
    }

    if(metric == NULL) {///更新全部metric
        db_update_host_info(db_conn, host_name, "host_status", host); 
        db_update_host_info(db_conn, host_name, "node_start", host); 
        db_update_host_info(db_conn, host_name, "last_report", host); 

        hash_t *metrics = host->metrics;
        if(metrics == NULL) {
            return -1;
        }
        update_host_info_arg_t tmp_arg = {db_conn, host_name};
        hash_foreach(metrics, do_update_host_info, &tmp_arg);
        return 0;
    } else {
        if(!strcmp(metric, "host_status")) {
            if(db_conn->execsql("INSERT INTO t_host_info(host_name,metric_name,metric_arg,metric_val)\
                        VALUES('%s','host_status','%s','%d') ON DUPLICATE KEY UPDATE metric_val='%d'",
                        host_name, host->ip, host->host_status, host->host_status) < 0) {
                ERROR_LOG("Could not update the host %s's %s info in db,db error:%s", 
                        host->ip, "host_status", db_conn->get_last_errstr());
            }
        } else if(!strcmp(metric, "node_start")) {
            if(db_conn->execsql("INSERT INTO t_host_info(host_name,metric_name,metric_arg,metric_val)\
                        VALUES('%s','node_start','','%u') ON DUPLICATE KEY UPDATE metric_val='%u'",
                        host_name, host->started, host->started) < 0) {
                ERROR_LOG("Could not update the host %s's %s info in db,db error:%s", 
                        host->ip, "node_start", db_conn->get_last_errstr());
            }
        } else if(!strcmp(metric, "last_report")) {
            if(db_conn->execsql("INSERT INTO t_host_info(host_name,metric_name,metric_arg,metric_val)\
                        VALUES('%s','last_report','','%u') ON DUPLICATE KEY UPDATE metric_val='%u'",
                        host_name, host->t0.tv_sec, host->t0.tv_sec) < 0) {
                ERROR_LOG("Could not update the host %s's %s info in db,db error:%s", 
                        host->ip, "last_report", db_conn->get_last_errstr());
            }
        } else {
            datum_t hash_key = {0}; 
            const char *tmp_str = NULL;
            const char *arg = NULL;
            hash_key.data = (void *)metric;
            hash_key.size = strlen(metric) + 1;
            datum_t *metric_data = hash_lookup(&hash_key, host->metrics);
            tmp_str = metric_data != NULL ? 
                getfield(((metric_t *)metric_data->data)->strings, ((metric_t *)metric_data->data)->valstr) : "Unknown";
            arg = metric_data != NULL ? 
                getfield(((metric_t *)metric_data->data)->strings, ((metric_t *)metric_data->data)->arg) : "";

            char escaped_val_str[256] = {0};
            char escaped_arg_str[256] = {0};
            mysql_real_escape_string(db_conn->get_conn(), escaped_val_str, tmp_str, strlen(tmp_str));
            mysql_real_escape_string(db_conn->get_conn(), escaped_arg_str, arg, strlen(arg));
            if(db_conn->execsql("INSERT INTO t_host_info(host_name,metric_name,metric_arg,metric_val)\
                        VALUES('%s','%s','%s','%s') ON DUPLICATE KEY UPDATE metric_val='%s'",
                        host_name, metric, escaped_arg_str, escaped_val_str, escaped_val_str) < 0) {
                ERROR_LOG("Could not update the host %s's %s info in db,db error:%s", 
                        host->ip, metric, db_conn->get_last_errstr());
            }
            datum_free(metric_data);
        }
    }

    return 0;
}

/** 
* @brief   从数据库中获取配置信息
* @param   db_conn   数据库连接
* @param   p_network_cfg  网络配置结构指针
* @return  0-success -1-failed
*/
int db_get_network_conf(c_mysql_iface* db_conn, network_conf_t *p_network_cfg)
{
    if(NULL == p_network_cfg || NULL == db_conn || NULL == db_conn->get_conn()) {
        ERROR_LOG("Monitor Process: parameter cannot be NULL.");
        return -1;
    }

    int idx_ip = 0, idx_port = 1, idx_trust = 2;
    char select_sql[MAX_STR_LEN] = {'\0'};
    if(g_grid_id > 0) {
        sprintf(select_sql, "SELECT listen_ip, listen_port, trust_host FROM v_grid_info WHERE grid_id=%d", g_grid_id);
    } else {
        ERROR_LOG("Database Error: critical error, no grid[%d] info.", g_grid_id);
        return -1;
    }

    MYSQL_ROW row = NULL;
    int  row_count = -1;
    row_count = db_conn->select_first_row(&row, select_sql);
    if(row_count < 0) {
        ERROR_LOG("Database Error: select_first_row(%s) failed. DB error[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    } else if(row_count == 0) {
        ERROR_LOG("Database Error: no record of grid[%d] in db.", g_grid_id);
        return -1;
    } else if(row_count > 1) {
        ERROR_LOG("Database Error: too many[%d] records of grid[%d] in db, SQL[%s]", row_count, g_grid_id, select_sql);
        return -1;
    }

    if(row[idx_ip]) {
#ifdef   RELEASE_VERSION
        if(strncmp(p_network_cfg->listen_ip, "192.168.", 8)) {
            strncpy(p_network_cfg->listen_ip, row[idx_ip], sizeof(p_network_cfg->listen_ip));
        }
#else 
        if(strncmp(p_network_cfg->listen_ip, "10.1.", 5)) {
            strncpy(p_network_cfg->listen_ip, row[idx_ip], sizeof(p_network_cfg->listen_ip));
        }
#endif
    }

    if(row[idx_port]) {
        p_network_cfg->listen_port = atoi(row[idx_port]);
        p_network_cfg->listen_port = p_network_cfg->listen_port <= 1024 || p_network_cfg->listen_port >= 65536 ?
            59000 + g_grid_id : p_network_cfg->listen_port;
    } else {
        //如果监听端口错误，或者没有监听端口，则用59000加grid id产生一个监听端口
        p_network_cfg->listen_port = 59000 + g_grid_id;
    }

    if(row[idx_trust]) {
        //strlen("127.0.0.1") = 9
        //strlen("127.0.0.1,") = 10
        strcpy(p_network_cfg->trust_host + 9, ",");
        strncpy(p_network_cfg->trust_host + 10, row[idx_trust], sizeof(p_network_cfg->trust_host) - 10);
    }

    //print the debug info
    DEBUG_LOG("=====================Network Process Configure=====================");
    DEBUG_LOG(" |--grid_id=[%d]", g_grid_id);
    DEBUG_LOG(" |--listen_ip=[%s],listen_port=[%u]", p_network_cfg->listen_ip, p_network_cfg->listen_port);
    DEBUG_LOG(" |--trust_host=[%s]", p_network_cfg->trust_host);
    DEBUG_LOG(" |--notice_url=[%s]", p_network_cfg->noti_url);
    DEBUG_LOG(" |--network_thread_count=[%u]", p_network_cfg->network_thread_cnt);
    DEBUG_LOG("===================================================================");
    return 0;
}

/** 
* @brief   从数据库中获取collect进程配置信息
* @param   db_conn   数据库连接
* @param   p_collect_cfg  collect配置结构指针
* @return  0-success -1-failed
*/
int db_get_collect_conf(c_mysql_iface *db_conn, collect_conf_t *p_collect_cfg)
{
    if(NULL == p_collect_cfg || NULL == db_conn || NULL == db_conn->get_conn()) {
        ERROR_LOG("Monitor Process: parameter cannot be NULL.");
        return -1;
    }

    int idx_iterval = 0, idx_url = 1, idx_dir = 2;
    char select_sql[MAX_STR_LEN] = {'\0'};
    if(g_grid_id > 0) {
        sprintf(select_sql, "SELECT summary_interval,alarm_server_url,rrd_rootdir \
                FROM v_grid_info WHERE grid_id=%d", g_grid_id);
    } else {
        ERROR_LOG("Database Error: critical error, no grid[%d] info.", g_grid_id);
        return -1;
    }

    MYSQL_ROW row = NULL;
    int  row_count = -1;
    row_count = db_conn->select_first_row(&row, select_sql);
    if(row_count < 0) {
        ERROR_LOG("Database Error: select_first_row(%s) failed. DB error[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    } else if(row_count == 0) {
        ERROR_LOG("Database Error: no record of grid[%d] in db.", g_grid_id);
        return -1;
    } else if(row_count > 1) {
        ERROR_LOG("Database Error: too many[%d] records of grid[%d] in db, SQL[%s]",
              row_count, g_grid_id, select_sql);
        return -1;
    }

    if(row[idx_iterval]) {
        p_collect_cfg->summary_interval = atoi(row[idx_iterval]);
    }

    if(row[idx_url]) {
        strncpy(p_collect_cfg->alarm_server_url, row[idx_url], sizeof(p_collect_cfg->alarm_server_url));
    }

    if(row[idx_dir]) {
        strncpy(p_collect_cfg->rrd_dir, row[idx_dir], sizeof(p_collect_cfg->rrd_dir));
    }

    //print the debug info
    DEBUG_LOG("=====================Collect Process Configure=====================");
    DEBUG_LOG(" |--grid_id=[%d]", g_grid_id);
    DEBUG_LOG(" |--summary_interval=[%u]", p_collect_cfg->summary_interval);
    DEBUG_LOG(" |--alarm_server_url=[%s]", p_collect_cfg->alarm_server_url);
    DEBUG_LOG(" |--rrd_rootdir=[%s]", p_collect_cfg->rrd_dir);
    DEBUG_LOG("===================================================================");
    return 0;
}

int get_wildcard_ip(char *segment, char *dst_ip)
{
    if(NULL == segment || NULL == dst_ip) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    char *p_start = segment;
    char *p_end = index(segment, '~');
    if (NULL == p_end) {
        ERROR_LOG("segment[%s] format is wrong.", segment);
        return -1;
    }
    int len = p_end - p_start;
    strncpy(dst_ip, p_start, len);
    dst_ip[len] = 0;
    p_end = rindex(dst_ip, '.');
    if (NULL == p_end) {
        ERROR_LOG("segment[%s] format is wrong.", segment);
        return -1;
    }
    *(p_end+1) = '%';
    *(p_end+2) = 0;
    str_trim(dst_ip);

    return 0;
}

int get_ip_segment(const char *segment)
{
    if(NULL == segment) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }
    //192.168.0.1~192.168.0.255/24
    char str[MAX_STR_LEN] = {0};
    strncpy(str, segment, sizeof(str) - 1);

    char *p_pos = index(str, '~');
    if (NULL == p_pos) {
        ERROR_LOG("segment[%s] format is wrong.", segment);
        return -1;
    }
    *p_pos = 0;
    p_pos = rindex(str, '.');
    if (NULL == p_pos) {
        ERROR_LOG("segment[%s] format is wrong.", segment);
        return -1;
    }
    *p_pos = 0;
    p_pos = rindex(str, '.');
    if (NULL == p_pos || '\0' == *(p_pos + 1)) {
        ERROR_LOG("segment[%s] format is wrong.", segment);
        return -1;
    }

    return atoi(p_pos + 1);
}

/** 
* @brief   获取网段值
* @param   db_conn   数据库连接
* @return  -1-failed, >=0-网段值
*/
int db_get_segment(c_mysql_iface* db_conn)
{
    if(db_conn == NULL || db_conn->get_conn() == NULL) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    char select_sql[MAX_STR_LEN] = {0};
    if(g_grid_id > 0) {
        //监听类型,1内网,2外网
        sprintf(select_sql, "SELECT segment, listen_type FROM t_network_segment_info WHERE segment_id=%d", g_grid_id);
    } else {
        ERROR_LOG("Critical error,no grid[%d] info.", g_grid_id);
        return -1;
    }

    MYSQL_ROW row = NULL;
    int  row_count = -1;
    row_count = db_conn->select_first_row(&row, select_sql);
    if(row_count < 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    } else if(row_count == 0) {
        ERROR_LOG("Database ERROR: no record of grid[%d] in db, SQL[%s].", g_grid_id, select_sql);
        return -1;
    } else if(row_count > 1) {
        ERROR_LOG("Database ERROR: too many[%d] records of grid[%d] in db, SQL[%s].", row_count, g_grid_id, select_sql);
        return -1;
    }

    if(row[0]) {
        return get_ip_segment(row[0]);
    } else {
        ERROR_LOG("Database ERROR: segment is NULL. SQL[%s].", select_sql);
        return -1;
    }
}

/** 
* @brief   从数据库中获取head下面所有node命令通道端口
* @param   p_ip_port  <IP, port>map指针
* @param   db_conn   数据库连接
* @return  0-success -1-failed
*/
int db_get_ip_port(ip_port_map_t *p_ip_port, c_mysql_iface* db_conn)
{
    if(p_ip_port == NULL || db_conn == NULL || db_conn->get_conn() == NULL) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    char select_sql[MAX_STR_LEN] = {0};
    if(g_grid_id > 0) {
        //监听类型,1内网,2外网
        sprintf(select_sql, "SELECT segment, listen_type FROM t_network_segment_info WHERE segment_id=%d", g_grid_id);
    } else {
        ERROR_LOG("Critical error,no grid[%d] info.", g_grid_id);
        return -1;
    }

    MYSQL_ROW row = NULL;
    int  row_count = -1;
    row_count = db_conn->select_first_row(&row, select_sql);
    if(row_count < 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    } else if(row_count == 0) {
        ERROR_LOG("There is no record of grid[%d] in db, SQL[%s].", g_grid_id, select_sql);
        return -1;
    } else if(row_count > 1) {
        ERROR_LOG("There are too many[%d] records of grid[%d] in db, SQL[%s].", row_count, g_grid_id, select_sql);
        return -1;
    }

    char segment[MAX_STR_LEN] = {0};
    if(row[0]) {
        if (0 != get_wildcard_ip(row[0], segment)) {
            return -1;
        }
    } else {
        ERROR_LOG("segment is NULL. SQL[%s].", select_sql);
        return -1;
    }
    int listen_type = 0;
    if(row[1]) {
        listen_type = atoi(row[1]);
        if (listen_type != 1 && listen_type != 2) {
            ERROR_LOG("listen_type[%s] is wrong, must be [1,2]. SQL[%s].", row[1], select_sql);
            return -1;
        }
    } else {
        ERROR_LOG("listen_type is NULL. SQL[%s].", select_sql);
        return -1;
    }
    DEBUG_LOG("grid[%d] wildcard ip is: %s", g_grid_id, segment);

    sprintf(select_sql, "SELECT ip_inside, ip_outside, listen_port FROM t_node_info, t_server_info, t_service_info WHERE node_id=service_id AND server_id=machine_id AND ip_inside LIKE '%s'", segment);

    ///出错或者是没有查到行
    if(db_conn->select_first_row(&row, "%s", select_sql) <= 0) {//因sql语句中有%, 而%对于函数vsprintf是特殊字符
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    }

    DEBUG_LOG("================================Node Listen Info================================");
    ip_port_t listen_info = {0};
    for (;NULL != row; row = db_conn->select_next_row(false)) {
        if(row[0] == NULL ||  row[2] == NULL) {
            ERROR_LOG("Database ERROR: inside_ip or listen_port is NULL.");
            continue;
        }

        if (listen_type != NET_INSIDE_TYPE && NULL == row[1]) {
            ERROR_LOG("Database ERROR: inside_ip[%s], listen_type[NET_OUTSIDE_TYPE] but outside_ip is NULL.", row[0]);
            continue;
        }

        uint32_t inside_ip = 0;
        if (1 != inet_pton(AF_INET, row[0], &inside_ip)) {
            ERROR_LOG("ERROR: inet_pton(inside_ip[%s]) failed.", row[0]);
            continue;
        }
        uint32_t outside_ip = 0;
        if (row[1]) {
            if (1 != inet_pton(AF_INET, row[1], &outside_ip)) {
                ERROR_LOG("ERROR: inet_pton(outside_ip[%s]) failed.", row[1]);
            }
        }

        int listen_port = atoi(row[2]);
        if (listen_port < 0 || listen_port > 65535) {
            ERROR_LOG("Database ERROR: ip[%s] has wrong listen_port[%s].", row[0], row[2]);
            continue;
        }
        listen_info.outside_ip = outside_ip;
        listen_info.listen_port = listen_port;
        listen_info.listen_type = listen_type;
        p_ip_port->insert(map<uint32_t, ip_port_t>::value_type(inside_ip, listen_info));
        DEBUG_LOG(" |--inside_ip[%s],outside_ip[%s],listen_port[%s],listen_type[%d]",
                row[0], row[1], row[2], listen_info.listen_type);
    }
    DEBUG_LOG("================================================================================");

    return 0;
}

/** 
* @brief   从数据库中获取数据源信息
* @param   data_src 保存数据源的map的指针
* @param   db_conn  数据库连接
* @return  0-success -1-failed
*/
int db_get_data_source(ds_t *data_src, c_mysql_iface *db_conn)
{
    if(data_src == NULL || db_conn == NULL || db_conn->get_conn() == NULL) {
        return -1;
    }

    char select_sql[MAX_STR_LEN] = {'\0'};
    if(g_grid_id > 0) {
        snprintf(select_sql, sizeof(select_sql), "SELECT export_hosts FROM v_grid_info WHERE grid_id=%d", g_grid_id);
    }
    else { 
        ERROR_LOG("Critical error,no grid id.");
        return -1;
    }

    MYSQL_ROW row = NULL;
    if(db_conn->select_first_row(&row, select_sql) <= 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    }

    char tmp_hosts[1024] = {0};
    if(row[0] && strlen(row[0]) > 0 && strlen(row[0]) < sizeof(tmp_hosts)) {
        strncpy(tmp_hosts, row[0], sizeof(tmp_hosts));
    }
    else {
        ERROR_LOG("Database ERROR: wrong export_hosts[%s]", row[0] ? row[0] : "");
        return -1;
    }

    str_trim(tmp_hosts);
    char *fields[MAX_HOST_NUM] = {NULL};
    int field_count = sizeof(fields) / sizeof(fields[0]);
    if(split(tmp_hosts, ',', fields, &field_count) != 0) {
        ERROR_LOG("Database ERROR: wrong export_hosts[%s]", tmp_hosts);
        return -1;
    }

    unsigned int host_counter = 0;
    data_source_list_t ds;
    memset(&ds, 0, sizeof(ds));

    for(int i = 0; i < field_count; i++) {
        char *ip_port = fields[i];
        char *delimiter = NULL;
        str_trim(ip_port);
        delimiter = index(ip_port, ':');
        if(delimiter == NULL) {
            if(is_inet_addr(ip_port)) {
                strncpy(ds.host_list[host_counter].server_inside_ip, ip_port, 16);
#ifdef RELEASE_VERSION
                if(strncmp(ip_port, "192.168.", 8) && strncmp(ip_port, "10.", 3)) {//发送主机不是个内网地址
                    char inside_ip[16] = {0};
                    if(0 == get_inside_ip_by_outside_ip(db_conn, ip_port, inside_ip)) {
                        memcpy(ds.host_list[host_counter].server_inside_ip, inside_ip, 16);
                    }
                }
#endif
                inet_pton(AF_INET, ip_port, (void*)&(ds.host_list[host_counter].server_ip));
                ds.host_list[host_counter].server_port = 55000;///使用默认的55000端口
                host_counter++;
            }
            else {
                continue;
            }
        } else {
            *delimiter = '\0';
            char *port = delimiter + 1;
            int tmp_port = atoi(port);
            if(is_inet_addr(ip_port) && tmp_port > 1024 && tmp_port < 65536) {
                strncpy(ds.host_list[host_counter].server_inside_ip, ip_port, 16);
#ifdef RELEASE_VERSION
                if(strncmp(ip_port, "192.168.", 8) && strncmp(ip_port, "10.", 3)) {//发送主机不是个内网地址
                    char inside_ip[16] = {0};
                    if(get_inside_ip_by_outside_ip(db_conn, ip_port, inside_ip) == 0) {
                        memcpy(ds.host_list[host_counter].server_inside_ip, inside_ip, 16);
                    }
                }
#endif
                inet_pton(AF_INET, ip_port, (void*)&(ds.host_list[host_counter].server_ip));
                ds.host_list[host_counter].server_port = (unsigned short)tmp_port;
                host_counter++;
            } else {
                continue;
            }
        }
    }

    //ds.ds_id = g_grid_id* 10 + 1;///十进制数左移一位加1
    ds.ds_id = g_grid_id;
    ds.step = REQUEST_INTERVAL;
    ds.host_num = host_counter;
    ds.last_good_idx = -1;

    data_src->insert(map<unsigned int, data_source_list_t>::value_type(ds.ds_id, ds));

    ds_t::iterator di = data_src->begin();
    for(; di != data_src->end(); di++) {
        DEBUG_LOG("Ds_id=[%u]step=[%u]last_goos_idx=[%d]host_num=[%u]dead=[%u].",
                di->second.ds_id, di->second.step, di->second.last_good_idx, di->second.host_num, di->second.dead);
        for(unsigned int k = 0; k < di->second.host_num; k++) {
            struct in_addr tmp_addr;
            memcpy(&tmp_addr, &(di->second.host_list[k].server_ip), sizeof(tmp_addr)); 
            DEBUG_LOG("    |__host[%u]=>ip:[%s] port:[%u].", k + 1,  
                    inet_ntoa(tmp_addr), di->second.host_list[k].server_port);
        }
    }
    return 0;
}

/** 
* @brief   根据外网地址获取内网地址
* @param   db_conn      数据库连接
* @param   outside_ip   外网ip地址
* @param   inside_ip    保存内网ip地址的缓存
* @return  0-success -1-failed
*/
int get_inside_ip_by_outside_ip(c_mysql_iface* db_conn, const char* outside_ip, char *inside_ip)
{
    if(outside_ip == NULL || inside_ip == NULL || db_conn == NULL || db_conn->get_conn() == NULL){
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }

    char select_sql[MAX_STR_LEN] = {'\0'};
    snprintf(select_sql, sizeof(select_sql) - 1,
            "SELECT ip_inside FROM t_server_info WHERE ip_outside='%s'", outside_ip);

    MYSQL_ROW row = NULL;
    if(db_conn->select_first_row(&row, select_sql) != 1) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]",
                select_sql, db_conn->get_last_errstr());
        return -1;
    }

    if(row[0] == NULL || !is_inet_addr(row[0])) {
        return -1;
    }

    strcpy(inside_ip, row[0]);
    return 0;
}

/** 
* @brief   从数据库删除主机的信息信息
* @param   db_conn  数据库连接
* @param   host_name  主机的server_tag
* @param   ip  主机的ip
* @return  0-success -1-failed
*/
int db_del_host_info(c_mysql_iface* db_conn, const char* host_name, const char* ip)
{
    if(host_name == NULL || ip == NULL || db_conn == NULL || db_conn->get_conn() == NULL) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }

    char delete_sql[MAX_STR_LEN] = {'\0'};
    sprintf(delete_sql, 
            "DELETE FROM t_host_info WHERE host_name='%s' AND metric_arg='%s' AND metric_name='host_status'",
            host_name, ip);

    if(db_conn->execsql(delete_sql) < 0) {
        ERROR_LOG("db_conn->execsql failed.SQL:[%s] db error is:[%s]", delete_sql, db_conn->get_last_errstr());
        return -1;
    }

    return 0;
}

/** 
* @brief   从数据库中获取metric的特殊报警相关信息
* @param   p_metric_map 保存metric_alarm_t结构的vector指针
* @param   db_conn      数据库连接
* @return  0-success -1-failed
*/
int db_get_special_metric_alarm_info(c_mysql_iface* db_conn, metric_alarm_map_t *p_metric_map)
{
    if(p_metric_map == NULL || db_conn == NULL || db_conn->get_conn() == NULL){
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }

    char select_sql[MAX_STR_LEN] = {0};
    sprintf(select_sql, "SELECT LOWER(SRI.server_tag),ASI.metric_name,ASI.arg,ASI.warning_val,\
        ASI.critical_val,ASI.operation, MI.normal_interval,MI.retry_interval,\
        MI.max_attempt, MI.metric_type, ADI.default_value, SRI.ip_inside \
        FROM t_alarm_info AS AI,t_service_info as SEI,t_alarm_default_info AS ADI,\
        t_alarm_strategy_info AS ASI,t_server_info AS SRI,t_metric_info AS MI \
        WHERE AI.alarm_strategy_id=ASI.id \
        AND AI.service_id=SEI.service_id \
        AND AI.service_type=SEI.service_type \
        AND SEI.machine_id=SRI.server_id \
        AND MI.metric_name=ASI.metric_name \
        AND ASI.alarm_expr=ADI.default_key \
        AND MI.metrictype_id&1=1 \
        AND MI.metric_type<>2 \
        AND SRI.ip_inside LIKE '192.168.%d.\%' \
        ORDER BY SRI.ip_inside", g_grid_segment);

    int idx_tag = 0, idx_name = 1, idx_arg = 2, idx_warn = 3, idx_crit = 4;
    int idx_opt = 5, idx_normal = 6, idx_retry = 7, idx_max = 8, idx_type = 9;
    int idx_span = 10, idx_ip = 11;
    MYSQL_ROW row = NULL;
    ///出错查到行
    int row_cnt = db_conn->select_first_row(&row, "%s", select_sql);
    if(row_cnt < 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]",
                select_sql, db_conn->get_last_errstr());
        return -1;
    } else if (0 == row_cnt) {
        DEBUG_LOG("=======No special metric alarm info like[192.168.%d.\%]=======", g_grid_segment);
        return 0;
    }

    metric_alarm_t metric_info;
    char pre_ip[17] = "InvalidIP";
    metric_alarm_vec_t *metric_alarm_vec = new metric_alarm_vec_t; 
    if(metric_alarm_vec == NULL) {
        ERROR_LOG("ERROR: new metric_alarm_vec_t failed.");
        return -1;
    }

    for (;NULL != row; row = db_conn->select_next_row(false)) {
        ///除了argument可以为空其他的都不可以
        if(row[0] == NULL || row[1] == NULL || row[3] == NULL || row[4] == NULL || 
                row[5] == NULL || row[6] == NULL || row[7] == NULL || row[8] == NULL ||
                NULL == row[9] || NULL == row[10] || NULL == row[11]) {
            continue;
        }

        memset(&metric_info, 0, sizeof(metric_info));

        if(row[idx_name] != NULL) {
            strncpy(metric_info.metric_name, row[idx_name], sizeof(metric_info.metric_name) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric name is NULL.");
            continue;
        }

        if(NULL != row[idx_arg]) {
            strncpy(metric_info.metric_arg, row[idx_arg], sizeof(metric_info.metric_arg) - 1);
        }

        if(NULL != row[idx_warn]) {
            strncpy(metric_info.alarm_info.warning_val, row[idx_warn], sizeof(metric_info.alarm_info.warning_val) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric warning value is NULL or not correct.");
            continue;
        }

        if(NULL != row[idx_crit]) {
            strncpy(metric_info.alarm_info.critical_val, row[idx_crit],
                    sizeof(metric_info.alarm_info.critical_val) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric critical value is NULL or not correct.");
            continue;
        }

        if(row[idx_opt] && is_integer(row[idx_opt])) {
            switch(atoi(row[idx_opt])) {
            case 1:
                metric_info.alarm_info.op = OP_GE;
                break;
            case 2:
                metric_info.alarm_info.op = OP_LE;
                break;
            case 3:
                metric_info.alarm_info.op = OP_GT;
                break;
            case 4:
                metric_info.alarm_info.op = OP_LT;
                break;
            case 5:
                metric_info.alarm_info.op = OP_EQ;
                break;
            default:
                metric_info.alarm_info.op = OP_GT;
                break;//缺省为大于
            }
        }
        else {
            metric_info.alarm_info.op = OP_GT;//缺省为大于
        }

        if(row[idx_normal] && is_integer(row[idx_normal])) {
            metric_info.alarm_info.normal_interval = atoi(row[idx_normal]);
        }
        else {
            ERROR_LOG("Database ERROR: normal interval is NULL or not correct.");
            continue;
        }

        if(row[idx_retry] && is_integer(row[idx_retry])) {
            metric_info.alarm_info.retry_interval = atoi(row[idx_retry]);
        }
        else {
            ERROR_LOG("Database ERROR: retry interval is NULL or not correct.");
            continue;
        }

        if(row[idx_type] && is_integer(row[idx_type])) {
            metric_info.metric_type = atoi(row[idx_type]);
        }
        else {
            ERROR_LOG("Database ERROR: metric_type is NULL or not correct.");
            continue;
        }

        if(row[idx_max] && is_integer(row[idx_max])) {
            metric_info.alarm_info.max_atc = atoi(row[idx_max]);
        }
        else {
            metric_info.alarm_info.max_atc = 2;
        }

        if(metric_info.alarm_info.max_atc <= 0 || metric_info.alarm_info.normal_interval <= 0 ||
                metric_info.alarm_info.retry_interval <= 0 || 
                metric_info.alarm_info.normal_interval < metric_info.alarm_info.retry_interval) {    
            ERROR_LOG("error alarm config :normal interval[%u],retry interval[%u],max attempt count[%u]", 
                    metric_info.alarm_info.normal_interval, metric_info.alarm_info.retry_interval,
                    metric_info.alarm_info.max_atc);
            continue;
        }   
        metric_info.alarm_info.normal_interval = metric_info.alarm_info.normal_interval / REQUEST_INTERVAL + 1;
        metric_info.alarm_info.retry_interval = metric_info.alarm_info.retry_interval / REQUEST_INTERVAL + 1;

        if(row[idx_span]) {
            strncpy(metric_info.alarm_span, row[idx_span], sizeof(metric_info.alarm_span) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: alarm_span is NULL.");
            continue;
        }

        if(strcmp(pre_ip, row[idx_ip])) {
            if(strcmp(pre_ip, "InvalidIP")) {
                p_metric_map->insert(pair<string, metric_alarm_vec_t*>(pre_ip, metric_alarm_vec));
                metric_alarm_vec = new metric_alarm_vec_t;
                if(metric_alarm_vec == NULL) {
                    ERROR_LOG("ERROR: new metric_alarm_vec_t failed.");
                    return -1;
                }
            }
            strncpy(pre_ip, row[idx_ip], sizeof(pre_ip) - 1);
        }
        metric_alarm_vec->push_back(metric_info);
    }
    p_metric_map->insert(pair<string, metric_alarm_vec_t*>(pre_ip, metric_alarm_vec));

    DEBUG_LOG("================================Special Metric Alarm Info================================");
    metric_alarm_map_t::iterator its = p_metric_map->begin(); 
    while (its != p_metric_map->end()) {
        DEBUG_LOG(" Server IP[%s] metric alarm infos:", (its->first).c_str());
        metric_alarm_vec_t::iterator it = (its->second)->begin();
        while (it != (its->second)->end()) {
            DEBUG_LOG("   |__metric[%s],arg[%s],W[%s],C[%s],norm[%d],retry[%d],max[%d],type[%d],span[%s]",
                    it->metric_name, it->metric_arg, it->alarm_info.warning_val, it->alarm_info.critical_val,
                    it->alarm_info.normal_interval, it->alarm_info.retry_interval,
                    it->alarm_info.max_atc, it->metric_type, it->alarm_span);
            ++it;
        }
        ++its;
    }
    DEBUG_LOG("=========================================================================================\n");

    return 0;
}

/** 
* @brief   从数据库中获取metric的缺省报警相关信息
* @param   p_metric_set 保存metric_alarm_t结构的vector指针
* @param   db_conn      数据库连接
* @return  0-success -1-failed
*/
int db_get_default_metric_alarm_info(c_mysql_iface* db_conn, metric_alarm_vec_t *p_metric_set)
{
    if(NULL == p_metric_set || NULL == db_conn || db_conn->get_conn() == NULL) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    char select_sql[MAX_STR_LEN] = "SELECT default_key, default_value FROM t_alarm_default_info \
                                    WHERE is_default = 1 AND default_key LIKE '%_expr_%'";
    MYSQL_ROW row = NULL;
    if(db_conn->select_first_row(&row, select_sql) <= 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] DB Error is:[%s]",
                select_sql, db_conn->get_last_errstr());
        return -1;
    }

    map<int, string> span_map;//<service_type, span_str>
    int idx_key = 0, idx_span = 1;
    int service_type = -1;
    for (; NULL != row; row = db_conn->select_next_row(false)) {
        if(NULL != row[idx_key]) {
            if (strncmp(row[idx_key], "server_", 7)) {
                service_type = OA_SEVICE_TYPE;
            } else if (strncmp(row[idx_key], "switch_", 7)) {
                service_type = OA_SWITCH_TYPE;
            } else if (strncmp(row[idx_key], "db_", 3)) {
                service_type = OA_MYSQL_TYPE;
            } else if (strncmp(row[idx_key], "photo_", 6)) {
                service_type = OA_PHOTO_TYPE;
            } else {
                ERROR_LOG("Database ERROR: cannot support default_key[%s], take it as OA_ALL_TYPE.", row[idx_key]);
                continue;
            }
        }
        else {
            ERROR_LOG("Database ERROR: default_key in t_alarm_default_info is NULL.");
            continue;
        }

        if(NULL == row[idx_span]) {
            ERROR_LOG("Database ERROR: default_value of [%s] in t_alarm_default_info is NULL.", row[idx_span]);
            continue;
        }

        //添加到span集合中
        span_map.insert(pair<int, string>(service_type, row[idx_span]));
    }


    sprintf(select_sql, "%s", "SELECT metric_name,argument,warning_val,critical_val,operation,normal_interval,\
                   retry_interval, max_attempt, metric_type FROM t_metric_info WHERE metrictype_id&1=1");
    row = NULL;
    if(db_conn->select_first_row(&row, select_sql) <= 0) {
        ERROR_LOG("db_conn->select_first_row failed.SQL:[%s] db error is:[%s]", select_sql, db_conn->get_last_errstr());
        return -1;
    }

    int idx_name = 0, idx_arg = 1, idx_warn = 2, idx_crit = 3, idx_opt = 4;
    int idx_normal = 5, idx_retry = 6, idx_max = 7, idx_type = 8;
    metric_alarm_t metric_info;
    for (;NULL != row; row = db_conn->select_next_row(false)) {
        memset(&metric_info, 0, sizeof(metric_info));

        if(NULL != row[idx_name]) {
            strncpy(metric_info.metric_name, row[idx_name], sizeof(metric_info.metric_name) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric name is NULL.");
            continue;
        }

        if(NULL != row[idx_arg]) {///arg可能为空
            strncpy(metric_info.metric_arg, row[idx_arg], sizeof(metric_info.metric_arg) - 1);
        }

        if(NULL != row[idx_warn]) {
            strncpy(metric_info.alarm_info.warning_val, row[idx_warn], sizeof(metric_info.alarm_info.warning_val) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric warning value is NULL or not correct.");
            continue;
        }

        if(NULL != row[idx_crit]) {
            strncpy(metric_info.alarm_info.critical_val, row[idx_crit],
                    sizeof(metric_info.alarm_info.critical_val) - 1);
        }
        else {
            ERROR_LOG("Database ERROR: metric critical value is NULL or not correct.");
            continue;
        }

        if(row[idx_opt] && is_integer(row[idx_opt])) {
            switch(atoi(row[idx_opt])) {
            case   1: 
                metric_info.alarm_info.op = OP_GE;
                break;
            case   2: 
                metric_info.alarm_info.op = OP_LE;
                break;
            case   3:
                metric_info.alarm_info.op = OP_GT;
                break;
            case   4:
                metric_info.alarm_info.op = OP_LT;
                break;
            case   5:
                metric_info.alarm_info.op = OP_EQ;
                break;
            default :
                metric_info.alarm_info.op = OP_GT;
                break;//缺省为大于
            }
        }
        else {
            metric_info.alarm_info.op = OP_GT;//缺省为大于
        }

        if(row[idx_normal] && is_integer(row[idx_normal])) {
            metric_info.alarm_info.normal_interval = atoi(row[idx_normal]);
        }
        else {
            ERROR_LOG("Database ERROR: normal interval is NULL or not correct.");
            continue;
        }

        if(row[idx_retry] && is_integer(row[idx_retry])) {
            metric_info.alarm_info.retry_interval = atoi(row[idx_retry]);
        }
        else {
            ERROR_LOG("Database ERROR: retry interval is NULL or not correct.");
            continue;
        }

        if(row[idx_max] && is_integer(row[idx_max])) {
            metric_info.alarm_info.max_atc = atoi(row[idx_max]);
        }
        else {
            metric_info.alarm_info.max_atc = 2;
        }

        if(metric_info.alarm_info.max_atc <= 0 || metric_info.alarm_info.normal_interval <= 0 ||
                metric_info.alarm_info.retry_interval <= 0 ||
                metric_info.alarm_info.normal_interval < metric_info.alarm_info.retry_interval) {    
            ERROR_LOG("ERROR alarm config :normal interval[%d],retry interval[%d],max attempt count[%d]", 
                    metric_info.alarm_info.normal_interval, metric_info.alarm_info.retry_interval,
                    metric_info.alarm_info.max_atc);
            continue;
        }   

        if(row[idx_type] && is_integer(row[idx_type])) {
            metric_info.metric_type = atoi(row[idx_type]);
        }
        else {
            ERROR_LOG("Database ERROR: metric_type is NULL or not integer.");
            continue;
        }
        map<int, string>::iterator it = span_map.find(metric_info.metric_type);
        if (it != span_map.end()) {
            strncpy(metric_info.alarm_span, it->second.c_str(), sizeof(metric_info.alarm_span) -1);
        }
        else {
            strcpy(metric_info.alarm_span, "1-3:5;3-10:20;10-0:120");
        }

        metric_info.alarm_info.normal_interval = metric_info.alarm_info.normal_interval / REQUEST_INTERVAL + 1;
        metric_info.alarm_info.retry_interval = metric_info.alarm_info.retry_interval / REQUEST_INTERVAL + 1;
        //添加到metric集合中
        p_metric_set->push_back(metric_info);
    }

    DEBUG_LOG("================================Default Metric Alarm Info================================");
    DEBUG_LOG("The default metric alarm info:");
    metric_alarm_vec_t::iterator itd = p_metric_set->begin();
    while(itd != p_metric_set->end()) {
        DEBUG_LOG(" |__metric[%s],arg[%s],W[%s],C[%s],normal[%d],retry[%d],max[%d],type[%d],span[%s]",
                itd->metric_name, itd->metric_arg, itd->alarm_info.warning_val, itd->alarm_info.critical_val,
                itd->alarm_info.normal_interval, itd->alarm_info.retry_interval,
                itd->alarm_info.max_atc, itd->metric_type, itd->alarm_span);
        ++itd;
    }
    DEBUG_LOG("=========================================================================================\n");

    return 0;
}

