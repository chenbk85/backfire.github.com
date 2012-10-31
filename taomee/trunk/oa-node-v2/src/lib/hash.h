/**
 * =====================================================================================
 *       @file  hash.h
 *      @brief
 *
 *     Created  2011-10-31 11:15:54
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef HASH_AC_H
#define HASH_AC_H

#include <stdint.h>
#include "rdwr.h"

#define HASH_KEY_NUM (3)
#define DEFAULT_BUF_LENGTH (128)

class c_hash_table;

/*
 * c_data 空间分配规则：
 * default_buf 够用的话，使用default_buf；
 * 否则malloc一块内存使用
 */
class c_data
{
    friend class c_hash_table;
    private :
        uint32_t data_length;
        uint32_t buf_length;
        void *   pointer;
        //char     default_buf[DEFAULT_BUF_LENGTH];
        //bool     malloced;

        int set_value(const uint32_t l, const void * p);

    public :
        c_data();
        c_data(uint32_t l, void * p);
        c_data(const c_data & d);
        c_data & operator=(const c_data& d);
        ~c_data();
        void * get_value();
        uint32_t get_length();
        bool not_null();
        bool null();
};

/*=======================================================
 * hash_table结构示意图
 * hash_table_node:
 *                  +------------------------------+
 *                  | key_string * ----------------+---> buf_key  //本级hash的key的保存空间
 *                  |                              |
 *                  | key[0]                       |     经hash计算后的32位key值
 *                  | ...                          |
 *                  | key[HASH_KEY_NUM]            |
 *                  |                              |
 *                  | value : +-----------------+  |     保存value的struct
 *                  |         | data_length     |  |
 *                  |         | buf_length      |  |
 *                  |         | default_buf --+-+--+---> buf_value  //本级hash的value的保存空间，优先使用default_buf
 *                  |         | pointer * ----+ |  |                //空间不够的话再pointer = malloc()
 *                  |         +-----------------+  |
 *                  |                              |
 *                  | next_hash_table * -----------+---> hash_table_node  //下级hash的指针，支持多级hash用
 *                  |                              |
 *                  | rwlock                       |     控制此<key, value>的读写锁
 *                  +------------------------------+
 *
 *======================================================
 */

//回调函数原型
typedef int (*RECALL)(const char * key, uint32_t value_length, const void * value_pointer, uint8_t level, void * args);

class c_hash_table
{
    private :
        typedef struct {
            char * key_string;
            c_pthread_rdwr rwlock;
            c_data value;
            uint32_t key[HASH_KEY_NUM];
            c_hash_table * next_hash_table;
        } hash_node_t;

        hash_node_t * hash_table;
        uint32_t hash_length;
        uint32_t hash_node_count;
        c_pthread_rdwr hash_lock;
        uint8_t  level;

    public :
        c_hash_table();
        c_hash_table(uint32_t l);
        ~c_hash_table();

        int create(uint32_t l);
        int insert(uint32_t value_length, void * value_pointer, const char * key);
        int insert(uint32_t value_length, void * value_pointer, uint32_t key_num, const char ** key);
        int update(uint32_t value_length, void * value_pointer, const char * key);
        int update(uint32_t value_length, void * value_pointer, uint32_t key_num, const char ** key);
        int remove(const char * key);
        int remove(uint32_t key_num, const char ** key);
        c_data search(const char * key);
        c_data search(uint32_t key_num, const char ** key);
        int foreach(RECALL fun_before, void * args_before, RECALL fun_after, void * args_after);

    private :
        int create(uint32_t l, uint8_t v);
        uint32_t fix_size(uint32_t l);
        uint32_t hash_string(const char * string, uint32_t type);
        bool key_equal(const uint32_t * hash_key1, const uint32_t * hash_key2);
        int destory();
        int resize();

};

#endif //HASH_AC_H
