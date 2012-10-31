/**
 * =====================================================================================
 *       @file  proto.h
 *      @brief  
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  09/13/2010 04:30:22 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason(张龙龙), mason@taomee.com
 *     @author  tonyliu(LCT), tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef PROTO_H
#define PROTO_H

#include <vector>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
//#include <strings.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "lib/hash.h"
#include "defines.h"

//日志结构
typedef struct {
    //[log]
    unsigned int   log_count;              
    unsigned int   log_lvl;
    unsigned int   log_size;
    char           log_prefix[MAX_STR_LEN];
    char           log_dir[PATH_MAX];
} log_conf_t;

//网络进程需要的配置项
typedef struct {
    //[network]
    char           listen_ip[16];
    unsigned short listen_port;
    char           trust_host[COM_STR_LEN];
    char           noti_url[MAX_URL_LEN];
    unsigned short network_thread_cnt;
} network_conf_t;

//数据采集进程需要的配置项
typedef struct {
    //[collect]
    unsigned int   grid_id;
    char           grid_name[32];
    unsigned int   summary_interval;
    unsigned int   queue_len;
    unsigned int   alarm_interval;
    char           alarm_server_url[MAX_URL_LEN];
    char           rrd_dir[PATH_MAX];
} collect_conf_t;

//网络地址
typedef struct {
    int server_ip;//s_addr
    char server_inside_ip[16];//保存内网网络地址
    unsigned short server_port;
} server_addr_t;

const unsigned int MAX_DISABLE_COUNT = 6;
//数据源信息
typedef struct {
    unsigned int ds_id;                  //数据库里的segment_id
    unsigned int step;                   //拉取间隔(s)
    int          last_good_idx;          //一个数据源内最后一次连接成功的主机的idx
    unsigned int dead;                   //这个数据源的所有主机都连不上则置为1说此数据源down,否则置0
    unsigned int disable_count;          //连续不可用的计数
    unsigned int host_num;               //数据源的主机个数
    server_addr_t  host_list[MAX_HOST_NUM];//一个数据源的发送主机
} data_source_list_t;
typedef std::map<unsigned int, data_source_list_t> ds_t;

//数据库信息
typedef struct {
    char           db_host[16];
    unsigned short db_port;
    char           db_name[256];
    char           db_user[256];
    char           db_pass[256];
} db_conf_t;

//metric数据类型
typedef enum {
    ERROR_TYPE = 0,
    INT,
    UINT,
    FLOAT,
    TIMESTAMP,
    STRING
} metric_type_t;

//类型字符串到类型枚举的映射
struct type_tag {
    const char   *name;
    metric_type_t type;
};

//数据增长率
typedef enum {
    SLOPE_ZERO = 0,
    SLOPE_POSITIVE,
    SLOPE_NEGATIVE,
    SLOPE_BOTH,
    SLOPE_UNSPECIFIED
} slope_t;

///xml的元素的名字枚举
typedef enum {
    ERROR_XML_TAG = 0,
    XML_TAG,
    GRID_TAG,
    CLUSTER_TAG,
    HOST_TAG,
    NAME_TAG,
    METRIC_TAG,
    ARG_TAG,
    TN_TAG,
    TMAX_TAG,
    DMAX_TAG,
    VAL_TAG,
    TYPE_TAG,
    CI_TAG,
    CLEANED_TAG,
    ALARM_TYPE_TAG,
    ALARM_VALUE_TAG,
    SLOPE_TAG,
    SOURCE_TAG,
    VERSION_TAG,
    LOCALTIME_TAG,
    HOSTCHANGE_TAG,
    IP_TAG,
    UP_FAILED_TAG,
    STARTED_TAG,
    UNITS_TAG,
    HOSTS_TAG,
    UP_TAG,
    DOWN_TAG,
    METRICS_TAG,
    SUM_TAG,
    NUM_TAG,
    EXTRA_DATA_TAG,
    EXTRA_ELEMENT_TAG
} xml_tag_t;

///元素名字字符串到元素名字枚举的映射
struct xml_tag {
    const char *name;
    xml_tag_t   tag;
};

///节点类型
typedef enum {
    ROOT_NODE,
    GRID_NODE,
    CLUSTER_NODE,
    HOST_NODE,
    METRIC_NODE,
    METRIC_STATUS_NODE
}node_type_t;

///metric数据
typedef union {
    double d;
    int    str;
} metric_val_t;

///传参结构
typedef struct {
    int   fd;
    void *instance;
} cb_arg_t;

///数据源结构，数据源包括cluster和grid
typedef struct {
    node_type_t node_type; //节点类型
    data_source_list_t *ds;//节点对应的数据源
    hash_t *children;      //cluster节点下的host节点的hash表,对于grid节点这个hash表是空的
    hash_t *metric_summary;//保存数据源的summary信息的hash表
    uint32_t localtime;   //数据源报告时间
} source_t;

typedef enum {
    HOST_STATUS_OK = 0,
    HOST_STATUS_AGENT_DOWN,
    HOST_STATUS_HOST_UNKNOWN,
    HOST_STATUS_HOST_DOWN
} host_status_t;

///host结构
typedef struct {
    node_type_t       node_type;      //节点类型
    hash_t           *metrics;        //保存host下的所有metric的hash表
    hash_t           *metrics_status; //保存host下的所有metric的报警状态的hash表
    struct timeval    t0;             //拉取到这个host信息的时间
    char              ip[16];         //host的ipv4地址
    uint32_t          tn;             //tn
    uint32_t          tmax;           //tmax
    uint32_t          dmax;           //dmax
    uint32_t          started;        //host上的node启动的时间
    int               host_status;    //host的状态 0=ok,1=oa_node down,2= host unknow,3 = host down
//    int               host_unok_times;//not in HOST_STATUS_OK times, must be continous
    bool              up_failed;      //host是否更新失败，初始为0，即更新成功
} host_t;

///警报等级
typedef enum {
    STATUS_OK = 0,
    STATUS_SW,
    STATUS_HW,
    STATUS_SC,
    STATUS_HC
} status_t;

///metric警报级别,四种基本状态ok，warning，critical
typedef enum {
    STATE_U = -1,
    STATE_O = 0,
    STATE_W,
    STATE_C
} state_t;

///比较操作符枚举
typedef enum {
    OP_UNKNOW = 0,
    OP_EQ, 
    OP_GT,
    OP_LT,
    OP_GE,
    OP_LE 
} op_t;

///metric警报状态结构
typedef struct {
    //meta data info
    node_type_t   node_type;    
    //alarm info
    unsigned int  cur_atc;           //current attempt count
    unsigned int  is_normal;         //is use the normal attempt interval(0 = yes,1=no)
    unsigned int  check_count;       //the check count 其实就是拉取数据源次数的计数
    status_t      cur_status;        //current metric status
    unsigned int  last_alarm;
} metric_status_info_t;

///告警条件结构
typedef struct {
    char           warning_val[512];  //警戒阀值
    double         wrn_val;
    char           critical_val[512]; //严重警戒阀值
    double         crt_val;
    op_t           op;                //阀值算子
    int            normal_interval;   //正常探测间隔
    int            retry_interval;    //非正常探测间隔
    int            max_atc;           //最大探测次数
} alarm_cond_t;

///metric_alarm 信息
typedef struct {
    int metric_type;  /**<metric类型:1-服务器, 2-交换机, 3-数据库*/
    char metric_name[MAX_NAME_SIZE]; /**<metic名称*/
    char metric_arg[MAX_NAME_SIZE];  /**<metic参数*/
    alarm_cond_t   alarm_info; /**<报警条件信息*/
    char alarm_span[OA_MAX_STR_LEN]; /**<metric告警时间规则,1-5:5;6-0:10*/
} metric_alarm_t;
typedef std::vector<metric_alarm_t> metric_alarm_vec_t;
typedef std::map<std::string, metric_alarm_vec_t*> metric_alarm_map_t;

///告警通知时间间隔
typedef struct {
    int metric_type; /**<metric类型:1-服务器, 2-交换机, 3-数据库, 4-图片服务*/
    char metric_name[MAX_NAME_SIZE]; /**<metric名称, 空:表示其他*/
    char alarm_span[OA_MAX_STR_LEN]; /**<metric告警时间规则,1-5:5;6-0:10*/
} alarm_span_t;
typedef std::vector<alarm_span_t> alarm_span_vec_t;
typedef std::map<std::string, alarm_span_vec_t*> alarm_span_map_t;/**<<IP,alarm_span_t数组>键值对*/

typedef struct {
    metric_alarm_map_t span_map;
    alarm_span_map_t span_info;
} alarm_config_t;

///metric结构
typedef struct {
    node_type_t      node_type;
    struct timeval   t0;
    metric_val_t     val;
    uint32_t         tn;
    uint32_t         tmax;
    uint32_t         dmax;
    uint32_t         num;
    short            name;
    short            valstr;     
    short            precision; 
    short            type;
    short            alarm_type;
    short            units;
    short            arg;
    short            slope;
    short            ednameslen;
    short            edvalueslen;
    short            ednames[MAX_EXTRA_ELEMENTS];
    short            edvalues[MAX_EXTRA_ELEMENTS];
    short            stringslen;
    char             strings[HEAD_FRAMESIZE];
} metric_t;

const unsigned int GET_SUMMARY_INFO = 1000;

/**
 * @struct head与node的命令的格式
 **/
typedef struct {
    uint32_t msg_len;
    uint32_t version;
    uint32_t msg_id;
    char     cmd[0];
}__attribute__((__packed__)) oa_cmd_t;

typedef struct {
    uint8_t listen_type;/**<1-内网，2-外网*/
    uint16_t listen_port;
    uint32_t outside_ip;
} ip_port_t;
typedef std::map<uint32_t, ip_port_t> ip_port_map_t;

typedef struct {
    network_conf_t *p_config;
    ip_port_map_t *p_ip_port;
} network_arg_t;

typedef struct {
    ds_t *p_ds;
    collect_conf_t *p_config;
    db_conf_t *p_db_conf;
    metric_alarm_vec_t *p_default_alarm_info;
    metric_alarm_map_t *p_special_alarm_info;
} collect_arg_t;

typedef struct {
    int type;
    int pid;
    bool is_update;
    bool is_running;
    char proc_name[64];
    time_t last_create_time;
} proc_status_t;

#endif  
