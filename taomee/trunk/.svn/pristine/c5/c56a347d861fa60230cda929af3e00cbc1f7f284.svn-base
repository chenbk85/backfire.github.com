/**
 * =====================================================================================
 *       @file  defines.h
 *      @brief
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  10/20/2010 09:10:53 PM
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
#ifndef H_DEFINES_H_2010_10_20
#define H_DEFINES_H_2010_10_20

//#define __TEST_NOT_UPDATE       //定义此项，不会更新

//#define RESTART_NETWORK           //定义此项，会重启network进程

#define OA_VERSION              "2.0.0"
#define OA_UPDATE_VERSION       "v2"

#define OA_COMMAND

//#define OA_COLLECT_NOBODY

/**< pid文件 */
#define OA_PID_FILE             "daemon.pid"
#define OA_COLLECT_R_FILE       "collect_r.pid"
#define OA_COLLECT_N_FILE       "collect_n.pid"
#define OA_COMMAND_FILE         "command.pid"
#define OA_NETWORK_FILE         "network.pid"

#define DEFAULT_HEAD            "192.168.6.51"

/**< 常量 */
#define OA_MAX_UDP_MESSAGE_LEN  (512)               /**< 给每次发送的UDP数据分配的缓冲区大小 */
#define OA_MAX_GROUP_NUM        (30)                /**< 每个collection_group里面最多的metric的个数 */
#define OA_MAX_METRICS_PER_SO   (100)               /**< 每个so最多处理的metric的个数 */
#define OA_MAX_STR_LEN          (512)
#define OA_MIN_LOG_SIZE         (3200000)           /**< 日志文件的最小大小 */
#define OA_MAX_BUF_LEN          (1024 * 1024)
#define MAX_G_STRING_SIZE       (256)               /**< 收集的metric的值的联合里字符串的大小 */

/**< metric类型 */
#define OA_MIN_METRIC_ID        (-3)
#define OA_CLEANUP_TYPE         (-3)                /**<通知listen线程清理保存的机器信息*/
#define OA_UP_FAIL_TYPE         (-2)
#define OA_HEART_BEAT_TYPE      (-1)
#define OA_CUSTOMER_TYPE        (0)
#define OA_BUILD_IN_TYPE        (1)

/**<IP类型 */
#define OA_INSIDE_IP            (1)
#define OA_OUTSIDE_IP           (2)

/**OA命名编码 */
#define OA_CMD_GET_METRIC_INFO  1000
#define OA_CMD_MYSQL_MGR        0x1000

/**<目录名必须以反斜杠结尾 */
#define OA_BIN_FILE_SAMPLE      "oa_node"
#define OA_BIN_PATH             "../bin/"
#define OA_BIN_FILE             "../bin/oa_node"
#define OA_BIN_FILE_BAK         "../backup/oa_node"

#define OA_CONF_PATH            "../conf/"
#define OA_CONF_FILE            "../conf/oa_node.ini"
#define OA_CONF_FILE_BAK        "../backup/oa_node.ini"

#define OA_SO_PATH              "../so/"
#define OA_SO_BAK_PATH          "../backup/so-bak/"

#define OA_SCRIPT_PATH          "../script/"
#define OA_SCRIPT_BAK_PATH      "../backup/script-bak/"

#define OA_BAK_PATH             "../backup/"
#define OA_LOG_PATH             "../log/"

#endif /* H_DEFINES_H_2010_10_20 */
