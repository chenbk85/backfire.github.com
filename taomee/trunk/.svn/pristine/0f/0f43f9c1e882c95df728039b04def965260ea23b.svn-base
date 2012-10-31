/**
 * =====================================================================================
 *       @file  c_rrd_handler.cpp
 *      @brief  
 *
 *      封装的rrd操作函数类实现
 *
 *   @internal
 *     Created  2010-10-18 11:13:42
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <unistd.h>
#include <rrd.h>
#include <time.h>

#include "rrd_handler.h"

/** 
 * @brief  构造函数
 * @param   
 * @return  
 */
c_rrd_handler::c_rrd_handler()
{
}


/** 
 * @brief  析构函数
 * @param   
 * @return  
 */
c_rrd_handler::~c_rrd_handler()
{
}

/** 
 * @brief  反初始化函数
 * @return  0 成功， -1 失败
 */
int c_rrd_handler::uninit()
{
    pthread_mutex_destroy(&m_rrd_fil_mutex);
    m_rrd_root_dir = NULL;
    m_grid_id = 0;
    return 0;
}

/** 
 * @brief  初始化函数
 * @param   p_config config对象指针
 * @return  0 成功， -1 失败
 */
int c_rrd_handler::init(const char *p_rrd_dir, int grid_id)
{
    if(p_rrd_dir == NULL || grid_id < 0) {
        return -1;
    }

    m_rrd_root_dir = p_rrd_dir;
    m_grid_id = grid_id;
    pthread_mutex_init(&m_rrd_fil_mutex, NULL);

    return 0;
}


/** 
 * @brief   更新RRD数据库
 * @param   rrd  rrd数据库路径
 * @param   sum  数值
 * @param   num  个数
 * @param   process_time 更新时间戳
 * @return  成功返回0，失败返回-1
 */
int c_rrd_handler::RRD_update(const char *rrd, const char *sum, const char *num, unsigned int process_time)
{
    const char *argv[4] = {NULL};
    int         argc = 3;
    char        val[128] = {'\0'};

    if(rrd == NULL || sum == NULL) {
        ERROR_LOG("RRD path OR sum is NULL.");
        return -1;
    }

    if(access(rrd, F_OK | R_OK | W_OK) != 0)
    {
        ERROR_LOG("Can not access rrd file:[%s],sys error:[%s]", rrd, strerror(errno));
        return -1;
    }

    if(num)
    {
        snprintf(val, sizeof(val) - 1, "N:%s:%s", sum, num);
        //snprintf(val, sizeof(val) - 1, "%u:%s:%s", process_time, sum, num);
    }
    else
    {
        snprintf(val, sizeof(val) - 1, "N:%s", sum);
        //snprintf(val, sizeof(val) - 1, "%u:%s", process_time, sum);
    }

    argv[0] = "dummy";
    argv[1] = rrd;
    argv[2] = val; 

    optind = 0;
    optopt = 0;
    opterr = 0;
    optarg = NULL;
    rrd_clear_error();

    rrd_update(argc, (char**)argv);
    if(rrd_test_error())
    {
        ERROR_LOG("ERROR:RRD_update(%s):%s", rrd, rrd_get_error());
        return -1;
    } 

    return 0;
}

/** 
 * @brief  创建RRD数据库
 * @param   rrd     rrd数据库路径
 * @param   summary 标志位是否summary
 * @param   step    步长
 * @param   process_time 更新时间戳
 * @param   slope   增长率，决定数据源的类型
 * @return  成功返回0，失败返回-1
 */
int c_rrd_handler::RRD_create(const char *rrd, int summary, unsigned int step, unsigned int process_time, slope_t slope)
{
    /* Warning: RRD_create will overwrite a RRdb if it already exists */
    const char *data_source_type = "GAUGE";
    const char *argv[128] = {NULL};
    int  argc = 0;
    int  heartbeat = 30;
    char s[16] = {'\0'}, start[64] = {'\0'};
    char sum[64] = {'\0'};
    char num[64] = {'\0'};

    /* Our heartbeat is twice the step interval. */
    heartbeat = 2 * step;

    switch(slope) 
    {
        case SLOPE_POSITIVE:
            //data_source_type = "COUNTER";
            //break;
        case SLOPE_ZERO:
        case SLOPE_NEGATIVE:
        case SLOPE_BOTH:
        case SLOPE_UNSPECIFIED:
            data_source_type = "GAUGE";
            break;
    }

    argv[argc++] = "dummy";
    argv[argc++] = rrd;
    argv[argc++] = "--step";
    sprintf(s, "%u", step);
    argv[argc++] = s;
    argv[argc++] = "--start";
    sprintf(start, "%u", process_time - 1);
    argv[argc++] = start;
    sprintf(sum, "DS:sum:%s:%d:U:U", data_source_type, heartbeat);
    argv[argc++] = sum;

    if(summary) 
    {
        sprintf(num, "DS:num:%s:%d:U:U", data_source_type, heartbeat);
        argv[argc++] = num;
    }

    argv[argc++] = "RRA:AVERAGE:0.5:1:240";
    argv[argc++] = "RRA:AVERAGE:0.5:24:240";
    argv[argc++] = "RRA:AVERAGE:0.5:168:240";
    argv[argc++] = "RRA:AVERAGE:0.5:720:240";
    argv[argc++] = "RRA:AVERAGE:0.5:5760:360";

    optind = 0;
    optopt = 0;
    opterr = 0;
    optarg = NULL;
    rrd_clear_error();

    rrd_create(argc, (char**)argv);

    if(rrd_test_error())
    {
        ERROR_LOG("RRD_create error: %s", rrd_get_error());
        return -1;
    }

    DEBUG_LOG("Created rrd %s", rrd);
    return 0;
}

/** 
 * @brief  存储数据到RRD数据库中
 * @param   rrd  rrd数据库路径
 * @param   sum  数值
 * @param   num  个数
 * @param   step 抽取间隔 
 * @param   process_time 更新时间戳
 * @param   slope slope   增长率，决定数据源的类型
 * @return  成功返回0,失败返回-1
 */
int c_rrd_handler::push_data_to_rrd(char *rrd, const char *sum, const char *num, unsigned int step, unsigned int process_time, slope_t slope)
{
    int summary = 0;
    int rval    = -1;
    unsigned int prss_tim = 0;

    pthread_mutex_lock(&m_rrd_fil_mutex);

    //if process_time is undefined or wrong, we set it to the current time
    //process_time = process_time <= 0 ? time(NULL) : process_time;
    //unix_timestamp('2011-1-1') = 1293811200
    prss_tim = process_time <= 1293811200 ? time(NULL) : process_time;

    summary = num != NULL ? 1 : 0;

    if(access(rrd, F_OK) != 0)
    {
        if(RRD_create(rrd, summary, step, prss_tim, slope) != 0)
        {
            pthread_mutex_unlock(&m_rrd_fil_mutex);
            return -1;
        }
    }

    rval =  RRD_update(rrd, sum, num, prss_tim);

    pthread_mutex_unlock(&m_rrd_fil_mutex);
    return rval;
}


/** 
* @brief   写数据到RRD数据库中
* @param   source 数据源名字
* @param   host    host名字
* @param   metric  metric的名字
* @param   sum     数值
* @param   num     个数
* @param   step     抽取间隔
* @param   process_time 更新时间戳
* @param   slope    数据增长率
* @return  成功返回0,失败返回-1
*/
int c_rrd_handler::write_data_to_rrd(const char *source, const char *host, const char *metric, const char *sum, 
const char *num, unsigned int step, unsigned int process_time, slope_t slope)
{
    char      rrd_file_path[PATH_MAX] = {'\0'};
    int       len = 0, remain_len = PATH_MAX - 1;
    char      *write_pos = rrd_file_path;

    //先把根目录拷进来
    len = snprintf(write_pos, remain_len, "%s", m_rrd_root_dir);
    write_pos += len;
    remain_len -= len;

    if(source != NULL && host != NULL)
    {
        len = snprintf(write_pos, remain_len, "/clusters-data");
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);

        len = snprintf(write_pos, remain_len, "/%s", host);
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);
    }
    else if(source != NULL && host == NULL)
    {
        len = snprintf(write_pos, remain_len, "/grid_%u", m_grid_id);
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);

        len = snprintf(write_pos, remain_len, "/%s-summary-data", source);
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);
    }
    else if(source == NULL && host == NULL)
    {
        len = snprintf(write_pos, remain_len, "/grid_%u", m_grid_id);
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);

        len = snprintf(write_pos, remain_len, "/self-summary-data");
        write_pos += len;
        remain_len -= len;
        my_mkdir(rrd_file_path);
    }
    else
    {
        //unlikely to come to here
        return -1;
    }

    len = snprintf(write_pos, remain_len, "/%s.rrd", metric);

    return push_data_to_rrd(rrd_file_path, sum, num, step, process_time, slope);
}
