/** 
 * ========================================================================
 * @file itl_util.h
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-19
 * Modify $Date: 2012-10-18 16:00:43 +0800 (四, 18 10月 2012) $
 * Modify $Author: tonyliu $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */


#ifndef H_ITL_UTIL_H_2012_07_19
#define H_ITL_UTIL_H_2012_07_19

#include <stdint.h>
#include <string>
#include <algorithm>
#include <fstream>
#include <stdint.h>
#include <libxml/tree.h>
#include <libtaomee++/utils/md5.h>
#include <libtaomee++/utils/tcpip.h>
#include <libtaomee++/conf_parser/xmlparser.hpp>


#define STRNCPY(des, src, len) \
    do { \
        strncpy((des), (src), (len)); \
        *((des) + (len) - 1) = 0; \
    } while (0)

#define STRNCPY_LEN(des, owner, src, len) \
    do { \
        if (0 == ((owner)->_ ## src ## _len)) { \
            *(des) = 0; \
        } else { \
            STRNCPY(des, (owner)->src, len); \
        } \
    } while (0)

#define DAY_SECONDS (24 * 3600)

/**
 * @brief 得到timestamp当天0点0分0秒的时间戳
 *
 * @param timestamp
 *
 * @return 0 失败，非0为时间戳
 */
uint32_t get_day_timestamp(uint32_t timestamp);


/** 
 * @brief 得到文件的md5
 * 
 * @param file
 * @param md5 char md5[33]
 * 
 * @return 0成功，-1失败
 */
int get_file_md5(const char * file, char * md5);

/** 
 * @brief 得到源字符串的md5
 * 
 * @param src_str: 源字符串
 * @param md5 char md5[33]
 * 
 * @return 0成功，-1失败
 */
int str2md5(const char * src_str, char * md5);


/** 
 * @brief 将时间戳转换成可读的字符串
 * 
 * @param timestamp
 * 
 * @return 字符串，不需要手动释放，下一次调用本函数之前均有效
 */
const char * timestamp2str(time_t timestamp);


/** 
 * @brief 转换成紧凑的字符串
 * 
 * @param 
 * 
 * @return YYYYMMDDHHmmSS
 */
const char * timestamp2compact_str(time_t timestamp);

/** 
 * @brief 设置进程的user
 * 
 * @param username
 * 
 * @return 0成功，-1失败
 */
int set_user(const char * username);


/** 
 * @brief 转换成小写字母
 * 
 * @param str
 * 
 * @return 0成功，-1失败
 */
int lower_case(char * str);


/** 
 * @brief 获取文件名，去掉目录
 * 
 * @param file
 * 
 * @return 
 */
const char * get_filename(const char * file);


/**
 * @brief  通过网卡名获取机器IP地址
 * @param  eth_name: 网卡类型
 * @param  ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_ip_by_name(const char * eth_name, char * ip);

/**
 * @brief  获取机器IP地址
 * @param  ip_type: IP类型
 * 0x01 内网
 * 0x02 外网
 * @param  ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_ip(int ip_type, char * ip);


inline bool is_xmlnode_name(xmlNodePtr node, const char * name)
{
    if (NULL == node || NULL == name)
    {
        return false;
    }
    return (0 == xmlStrcmp(node->name, BAD_CAST(name)));
}

template <size_t len>
inline int get_xml_prop_string(char (&val)[len], xmlNodePtr cur, const void * prop)
{
    try
    {
        taomee::get_xml_prop_raw_str(val, cur, prop);
    }
    catch (taomee::XmlParseError & e)
    {
        return -1;
    }
    return 0;
}


template <typename T1>
int get_xml_prop(T1& val, xmlNodePtr cur, const void * prop)
{
    try
    {
        taomee::get_xml_prop(val, cur, prop);
    }
    catch (taomee::XmlParseError & e)
    {
        return -1;
    }

    return 0;
}

#define xmlnode_for_each(parent, child)    \
    for (xmlNodePtr child = (parent)->xmlChildrenNode; child != NULL; child = child->next)

#endif
