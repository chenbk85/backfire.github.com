/**
 * =====================================================================================
 *      @file  network_proto.h
 *      @brief network_process进程用到的协议结构定义
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  2/9/2012 11:23:12 AM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2012, TaoMee.Inc, ShangHai.
 *
 *     @author  tonyliu (LCT), tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef H_NETWORK_PROTO_H_20120209
#define H_NETWORK_PROTO_H_20120209

#define NETWORK_PROCESS_VERSION 1
#define NETWORK_MAX_SEND_TIME 8
#define NETWORK_MAX_RECV_TIME 60

/**协议类型*/
enum {
    PROTO_DB  = 0x1000,
    PROTO_SYS = 0x2000
};

/**通知类型*/
typedef enum {
    NOTI_SOCK  = 1,
    NOTI_HTTP  = 2,
} notice_type_e;

/**返回包错误码*/
enum {
    RESULT_OK  = 0,
    RESULT_ESYSTEM    = 0x1001, /**<系统错误*/
    RESULT_ECOMMAND   = 0x1002, /**<cmd_id无效*/
    RESULT_EUNCONNECT = 0x1003, /**<node连接不上*/
    RESULT_ETIMEOUT   = 0x1004, /**<node处理超时*/
    RESULT_ENOMEMORY  = 0x1005, /**<node返回包过长*/
    RESULT_ENODEHEAD  = 0x1006, /**<node返回包头错误*/
};

/**请求协议包的包头*/
typedef struct {
    uint32_t pkg_len;
    uint32_t version;
    uint32_t cmd_id;/**<高四位:命令执行完的通知方式*/
    uint32_t result;
    char     auth_code[32];/**<命令的md5值:security_code+cmd_id+send_time+serial_no1的md5值*/
    uint32_t send_time;
    uint32_t serial_no1;
    uint32_t serial_no2;
}__attribute__((__packed__)) pkg_head_t;


/**上层head发送的命令的包体*/
//数据库的
typedef struct {
    uint32_t dst_ip;
    char     other[0];
}__attribute__((__packed__)) db_body_t;

/**head和node之间的协议包头*/
typedef pkg_head_t node_head_t;

/**head和node之间的协议包体*/
typedef struct {
    uint16_t sql_cnt;
}__attribute__((__packed__)) node_db_body_t;
//数据库包体中不变的部分
typedef struct {
    uint32_t serial_id;
    uint32_t sql_no;
    int32_t err_no;
    uint16_t sql_len;
    uint16_t err_len;
}__attribute__((__packed__)) db_body_fix_t;

#endif
