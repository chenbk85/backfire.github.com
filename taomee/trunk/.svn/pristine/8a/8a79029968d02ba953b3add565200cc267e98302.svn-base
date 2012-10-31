/**
 * =====================================================================================
 *       @file  define.h
 *      @brief  
 *
 * some macro defines
 *
 *   @internal
 *     Created  09/02/2010 03:08:06 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */

#ifndef DEFINES_H
#define DEFINES_H

#define _DEBUG


#include <limits.h>
#include <map>
#include <string>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define FD_CLOSE(a) if ((a) >= 0) {close(a);a = -1;}
#define DAEMON_FILE "./daemon.pid"

const char *const VERSION                         = "2.0.0"; /**<@XML里显示的VERSION */
const char *const HEAD_SECURITY_CODE              = "422cfaac1b203578b63460406a70e5ab"; /**<@head命令验证码*/
const char *const NODE_SECURITY_CODE              = "e38a9ddb81cd9118389e27903b3e8695"; /**<@node命令验证码*/
const char *const SWITCH_CODE                     = "c8758b517083196f05ac29810b924aca"; /**<@http开关验证码*/
const unsigned int RECV_TIMEOUT                   = 5;       /**<@export线程接受请求超时的时间 */
const unsigned int SEND_TIMEOUT                   = 20;      /**<@export线程发送数据超时的时间 */

const unsigned int MAX_EXPORT_THREAD_NUM          = 20;      /**<@最大的export线程个数 */
const unsigned int REINIT_INTERVAL                = 120;     /**<@从数据库更新配置的间隔*/
const unsigned int REQUEST_INTERVAL               = 20;      /**<@拉取数据源的间隔*/
const unsigned int MAX_HOST_NUM                   = 20;      /**<@一个数据源中发送主机的最大个数 */

const unsigned int MAX_STR_LEN                    = 1024;    
const unsigned int COM_STR_LEN                    = 256;     /**<@一般字符串长度*/
const unsigned int MAX_NAME_SIZE                  = 128;    
const unsigned int MAX_URL_LEN                    = 256;     /**<@最大的URL长度*/
const unsigned int OA_MAX_STR_LEN                 = 512;     /**<@最大字符串长度*/
const unsigned int OA_ALARM_CMD_MAX_LEN           = 512;     /**<@最大告警命令长度*/
const unsigned int OA_MAX_BUF_LEN                 = 1024 * 1024;

const double SLEEP_RANDOMIZE                      = 5.0;     
const unsigned int MINIMUM_SLEEP                  = 1;       /**<@最小的睡眠时间*/
const unsigned int MAX_FORMULAR_FIELD             = 9;       /**<@metric里的formular属性字符串';'分隔的字段最大个数*/
const unsigned int MAX_VARIABLE_NUM_PER_FORMULAR  = 10;      /**<@阀值公式里包含的metric名字的个数最大值*/

const unsigned char ALARM_METRIC_TYPE             = 0x01;    /**<@报警类型的metric*/
const unsigned char RRD_METRIC_TYPE               = 0x02;    /**<@RRD类型的metric*/
const unsigned char MYSQL_METRIC_TYPE             = 0x04;    /**<@MYSQL类型的metric*/

const unsigned int DEFAULT_CLUSTERSIZE = 1024;/**<cluster的hash表的缺省大小*/
const unsigned int DEFAULT_GRIDSIZE = 10;     /**<非root grid的hash表的缺省大小*/
const unsigned int DEFAULT_ROOTSIZE = 40;     /**<root grid的hash表的缺省大小*/
const unsigned int DEFAULT_METRICSIZE = 50;   /**</host的hash表的缺省大小*/
const unsigned int HEAD_FRAMESIZE = 1572;
const unsigned int MAX_EXTRA_ELEMENTS = 32;

#define PRE_NAME_MONITOR "oa_head_monitor_"
#define PRE_NAME_NETWORK "oa_head_network_"
#define PRE_NAME_COLLECT "oa_head_collect_"

/*进程类型*/
enum {
    PROC_MONITOR = 0,
    PROC_COLLECT = 1,
    PROC_NETWORK = 2
};

/*告警命令*/
enum {
   OA_HOST_METRIC = 20001,
   OA_HOST_METRIC_RECOVERY = 20002,
   OA_HOST_ALARM = 20003,
   OA_HOST_RECOVERY = 20004,
   OA_UPDATE_FAIL = 20005,
   OA_UPDATE_RECOVERY = 20006,
   OA_DS_DOWN = 20007,
   OA_DS_RECOVERY = 20008,
   OA_HOST_METRIC_CLEANED = 20009
};


/*进程间通信命令号*/
enum {
   PROC_NEED_RESTART    = 1001,
   PROC_START_SUCC      = 1002,
   PROC_NEED_STOP       = 1003,
   PROC_STOP_SUCC       = 1004,
   PROC_INIT_SUCC       = 1005,
   PROC_INIT_FAIL       = 1006
};

///1:服务器硬件 2:交换机 3:mysql 4:图片服务器
enum {
    OA_ALL_TYPE = 0,
    OA_SEVICE_TYPE = 1,
    OA_SWITCH_TYPE,
    OA_MYSQL_TYPE,
    OA_PHOTO_TYPE
};

//网络类型
enum {
    NET_INSIDE_TYPE = 1,
    NET_OUTSIDE_TYPE = 2
};


#endif /* DEFINES_H*/
