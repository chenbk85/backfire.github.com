/**
 * =====================================================================================
 *       @file  proto.h
 *      @brief
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  08/24/2010 01:54:02 PM
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  luis (程龙), luis@taomee.com
 *     @modify  ping, ping@taomee.com   at  2011-11-21 15:11:59
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_PROTO_H_2010_10_18
#define H_PROTO_H_2010_10_18

#include <stdint.h>
#include <limits.h>
#include <string>
#include <map>
#include <vector>
#include "defines.h"
#include "i_config.h"

// 配置文件列表
const static char g_config_file_list[][PATH_MAX] = {
    "../conf/oa_node.ini"
};

// 配置文件个数
const static int g_config_file_count = sizeof(g_config_file_list) / sizeof(*g_config_file_list);

const static std::string g_red_clr = "\e[1m\e[31m";
const static std::string g_grn_clr = "\e[1m\e[32m";
const static std::string g_end_clr = "\e[m";

/**
 * @union value_t
 * @brief 保存采集metric值的联合体
 */
typedef union {
    short value_short;
    unsigned short  value_ushort;
    int value_int;
    unsigned int value_uint;
    long long value_ll;
    unsigned long long value_ull;
    float value_f;
    double value_d;
    char value_str[MAX_G_STRING_SIZE];
} value_t;

typedef enum {
    OA_VALUE_STRING = 0,
    OA_VALUE_UNSIGNED_SHORT = 1,
    OA_VALUE_SHORT = 2,
    OA_VALUE_UNSIGNED_INT = 3,
    OA_VALUE_INT = 4,
    OA_VALUE_FLOAT = 5,
    OA_VALUE_DOUBLE = 6,
    OA_VALUE_UNSIGNED_LONG_LONG = 7,
    OA_VALUE_LONG_LONG = 8
} value_type_t;

enum {
    OA_BUILDIN_CMD = 0,
    OA_SO = 1,
    OA_SCRIPTS = 2,
    OA_CMD = 3,
};

/**<OA报警类型*/
enum {
    OA_LIMIT_MIN = 1,
    OA_ALARM = 1,
    OA_RRD = 2,
    OA_MYSQL = 4,
    OA_BOTH = 3,
    OA_LIMIT_MAX = 7
};

/**<OA_COLLECT运行时用户*/
enum {
    OA_USER_ROOT = 0,
    OA_USER_NOBODY = 1
};

/**<command进程执行结果的返回值*/
enum {
    OA_CMD_RET_SYSTEM_BUSY = 1000,          //系统忙
    OA_CMD_RET_ERROR_CMD_TYPE = 1001,       //错误命令类型
    OA_CMD_RET_QUEUE_FULL = 1002,           //命令太多,队列满
    OA_CMD_RET_QUEUE_OP_ERROR = 1003,       //队列操作错误

    OA_CMD_RET_INIT_ERROR = 1010,           //初始化错误
    OA_CMD_RET_RUN_EXIT_ABN = 1011,         //运行时异常退出
    OA_CMD_RET_NOT_EXIST = 1012,            //脚本或命令不存在
    OA_CMD_RET_PER_DENY = 1013,             //权限不够

    OA_CMD_RET_SO_CMD_ERROR = 1020,         //so命令格式不正确
    OA_CMD_RET_SO_NOT_EXIT = 1021,          //so文件不存在
    OA_CMD_RET_SO_NOT_READ = 1022,          //so文件没有读权限
    OA_CMD_RET_SO_LOAD_ERROR = 1023,        //打开so文件出错
    OA_CMD_RET_SO_LOAD_FUNC_ERROR = 1024,   //加载so文件中的函数出错
    OA_CMD_RET_SO_INIT_ERROR = 1025,        //执行so函数前，初始化错误
};

const char g_user_list[][10] = {"root", "nobody"};

/**
 * @struct metric_info_t
 * @brief  加载so时返回的对so的描述信息
 */
typedef struct {
    const char *name;                      /**< metric的名字 */
    int tmax;                              /**< 超时时间 */
    int dmax;                              /**< 超时时间 */
    value_type_t type;                     /**< 返回值的类型 */
    const char *units;                     /**< 返回值的单位的名称 */
    const char *fmt;                       /**< 返回值显示的格式 */
    const char *desc;                      /**< 对metric的描述 */
    const char *slope;                     /**< 存储在rrd的类型 */
    const char *sum_formula;               /**< 对这个metric进行汇总时的计算公式，默认为加法 */
} metric_info_t;

/**
 * @struct proto_so_t
 * @brief  so的函数接口
 */
typedef struct {
    void *handle;
    int (*proto_init)(i_config *p_config);
    int (*get_proto_info)(metric_info_t *metric_info, int *metric_count, bool *is_custom_so);
    value_t (*proto_handler)(int index, const char *arg);
    int (*proto_uninit)();
} proto_so_t;

/**
 * @struct collect_info_t
 * @brief  对每一条要收集的metric的完整描述
 */
typedef struct {
    metric_info_t metric_info;                        /**< so返回的每一条metric信息 */
    int index;                                        /**< 传递给so的处理函数的参数 */
    value_t (*proto_handler)(int index, const char *arg);   /**< 收集指标的回调函数 */
    int metric_id;                                    /**< -1:心跳,0:自定义指标,>0:内置指标 */
} collect_info_t;

/**
 * @struct metric_t
 * @brief  自定义的metric发送的格式
 */
typedef struct {
    const char *name;                 /**< metric的名字  */
    const char *arg;                  /**< 和name一起生成metric的名字 */
    int dmax;                         /**< metric的硬超时时间 */
    int tmax;                         /**< metric的软超时时间 */
    const char *units;                /**< 返回值的单位的名称 */
    const char *fmt;                  /**< 返回值显示的格式 */
    const char *desc;                 /**< metric的描述信息 */
    const char *slope;                /**< 存储在rrd的类型 */
    const char *sum_formula;          /**< metric进行汇总时的公式 */
    value_type_t type;                /**< 返回值的类型 */
    value_t value;                    /**< metric的值 */
    int metric_type;                  /**< 1为只写rrd,2为只报警，3为即写rrd又报警 */
    const char *alarm_formula;
} metric_t;

/**
 * @struct metric_send_t
 * @brief  UDP多播发送的信息的格式(通过XDR打包)
 */
typedef struct {
    const char *server_tag;                   /**< 标识是哪台机器发送的信息 */
    unsigned host_ip;
    int collect_interval;
    int metric_id;                            /**< 0为自定义的指标，-1为心跳信息，-2表示更新失败，其他为内置指标 */
    union {
        unsigned heartbeat;                   /**< 心跳信息 */
        metric_t metric;                      /**< 用户自定义的指标 */
    };
} metric_send_t;

/**
 * @struct update_status_t
 * @brief  主线程和export线程共享的用来传递更新信息的数据结构
 */
typedef struct {
    int exec_updated;                 /**< 0:程序没有更新，1：程序有更新 */
    int config_updated;               /**< 1:config文件有更新, 0:config文件没有更新 */
    int so_updated_root;              /**< 1:so文件有更新, 0:so文件没有更新 for root*/
    int so_updated_nobody;            /**< 1:so文件有更新, 0:so文件没有更新 for nobody*/
    int so_updated_command;            /**< 1:so文件有更新, 0:so文件没有更新 for command*/
    int script_updated;               /**< 1:script文件有更新, 0:script文件没有更新 */
    char program_name[PATH_MAX];      /**< 需要exec的程序的文件名 */
} update_info_t;

/**
 * @struct metric_collect_t
 * @brief  每个metric的描述信息及要保存的值
 */
typedef struct {
    int metric_type;
    const collect_info_t *p_collect_info;                       /**< 每个metric收集信息的完整描述的索引 */
    char arg[OA_MAX_STR_LEN];                                   /**< 报警的回调函数的参数 */
    char formula[OA_MAX_STR_LEN];                               /**< 报警的公式  */
    float value_threshold;                                      /**< 超过这个阀值了才发送数据 */
} metric_collect_t;

/**
 * @struct collect_group_t
 * @brief  对收集的每一个组的描述信息
 */
typedef struct {
    int collect_interval;                                           /**< 收集的间隔时间 */
    int time_threshold;                                             /**< 在这个阀值之前的随机时间把送数据发送出去 */
    int num;                                                        /**< 这组需要收集的metric的个数 */
    const metric_collect_t *p_metric_collect_arr[OA_MAX_GROUP_NUM]; /**< 这组需要收集的metric的索引，不能超过MAX_GROUP_NUM */
} collect_group_t;

typedef struct {
    char ip[16];
    int port;
} send_addr_t;

typedef std::vector<collect_group_t> collect_group_vec_t;
typedef std::vector<collect_info_t> collect_info_vec_t;
typedef std::vector<send_addr_t> send_addr_vec_t;
typedef std::vector<proto_so_t> proto_vec_t;
typedef std::map<int, metric_info_t *> metric_info_map_t;

#endif //H_PROTO_H_2010_10_18