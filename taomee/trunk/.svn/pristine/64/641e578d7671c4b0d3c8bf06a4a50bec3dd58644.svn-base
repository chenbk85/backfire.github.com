/**
 * =====================================================================================
 *       @file  get_mysql_info.h
 *      @brief  
 *
 *     Created  2012-01-19 14:30:51
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#ifndef  GET_MYSQL_INFO_H
#define  GET_MYSQL_INFO_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>

#include <libtaomee/log.h>

#define PRIV_KEY "e38a9ddb81cd9118389e27903b3e8695"
#define GRANT_GLO_INDEX (10)
#define GRANT_DB_INDEX  (6)
#define OA_MAX_SQL_LEN  (1024)

#define READ_NUM(num, fd) \
do {\
    fread(&num, sizeof(num), 1, fd);\
} while(0)

#define READ_STR(str, len, fd) \
do {\
    fread(str, len, 1, fd);\
} while(0)

#define TRIM(str) \
do {\
    int i = 0;\
    while(str[i] != 0x20) {\
        i++;\
    }\
    str[i] = '\0';\
} while(0)

typedef struct {
    uint32_t  pkg_len;
    uint32_t  version;
    uint32_t  cmd_id;
    uint32_t  result;
    char      veri_code[32];
    uint32_t  timestamp;
    uint32_t  serial_no1;
    uint32_t  serial_no2;
} __attribute__((__packed__)) head_t;

void print_head(head_t * head);

typedef struct {
    uint32_t  sql_ip;
    uint16_t  sql_port;
    uint8_t   level;
    char      user_name[64];
    uint32_t  host_ip;
    char      db_name[64];
    char      table_name[64];
    char      column_name[64];
} __attribute__((__packed__)) query_recv_t;

void print_query_recv(query_recv_t * query);

typedef struct {
    char     name[64];
} __attribute__((__packed__)) name_t;

typedef struct {
    uint32_t  count;
    name_t    name[1024];
} __attribute__((__packed__)) query_info_send_t;

void print_query_info_send(query_info_send_t * query);

typedef struct {
    char      name[64];
    uint32_t  priv;
} __attribute__((__packed__)) sample_priv_t;

typedef struct {
    char     user_name[64];
    char     host_ip[16];
    char     db_name[64];
    char     table_name[64];
    char     column_name[64];
    uint32_t priv;
} __attribute__((__packed__)) priv_t;

typedef struct {
    uint32_t   count;
    priv_t    priv[1024];
} __attribute__((__packed__)) query_priv_send_t;

void print_query_priv_send(query_priv_send_t * query);

typedef struct {
    uint32_t  sql_ip;
    uint16_t  sql_port;
} __attribute__((__packed__)) query_user_info_t;

void print_query_user_info(query_user_info_t * query);

typedef struct  {
    uint32_t  sql_ip;
    uint16_t  sql_port;
    uint8_t   do_sql;
} __attribute__((__packed__)) body_head_t;

void print_body_head(body_head_t * body);

typedef struct {
    char     name[64];
    uint32_t host_ip;
} __attribute__((__packed__)) user_t;

void print_user_t(user_t * user);

typedef struct {
    body_head_t head;
    user_t      user;
} __attribute__((__packed__)) check_user_t;

void print_check_user_t(check_user_t * user);

typedef struct {
    uint8_t exists;
} __attribute__((__packed__)) check_user_send_t;

void print_chech_user_send(check_user_send_t * send);

typedef struct {
    char name[64];
    char host_ip[16];
} __attribute__((__packed__)) query_user_t;

typedef struct {
    uint32_t count;
    query_user_t user[1024];
} __attribute__((__packed__)) query_user_send_t;

void print_query_user_send(query_user_send_t * send);

typedef struct {
    char  dba_name[64];
    char  dba_passwd[64];
} __attribute__((__packed__)) dba_t;

void print_dba(dba_t * dba);

typedef struct {
    uint32_t  add_no;
    uint32_t  host_ip;
} __attribute__((__packed__)) add_host_t;

typedef struct {
    body_head_t  head;
    dba_t        dba;
    char         new_user[64];
    char         new_passwd[64];
    uint8_t      host_count;
    add_host_t   host[1024];
} __attribute__((__packed__)) add_user_t;

void print_add_user(add_user_t * add_user);

typedef struct {
    uint8_t     del_no;
    char        name[64];
    uint32_t    host_ip;
} __attribute__((__packed__)) duser_t;

typedef struct {
    body_head_t  head;
    dba_t        dba;
    uint8_t      del_count;
    duser_t      del_user[1024];
} __attribute__((__packed__)) del_user_t;

void print_del_user(del_user_t * del_user);

typedef struct {
    uint32_t   priv_no;
    uint32_t  host_ip;
    uint32_t  priv;    
    char      db_name[64];
    char      table_name[64];
    char      column_name[64];
} __attribute__((__packed__)) priv_record_t;

typedef struct {
    body_head_t  head;
    dba_t        dba;
    char         user[64];
    uint32_t      host_count;
    priv_record_t  priv[256];
} __attribute__((__packed__)) edit_priv_t;

void print_edit_priv(edit_priv_t * edit);

typedef struct {
    body_head_t  head;
    char         user[64];
    uint32_t     host_ip;
    char         passwd[64];
} __attribute__((__packed__)) check_passwd_t;

void print_check_passwd(check_passwd_t * check);

typedef struct {
    uint8_t  result;
}  __attribute__((__packed__)) check_passwd_send_t;

void print_check_passwd_send(check_passwd_send_t * send);

typedef struct {
//    uint16_t  sql_count;
    uint32_t  value_id;
    uint32_t  sql_id;
    uint32_t  err_no;
    uint16_t  sql_len;
    uint16_t  err_len;
    char      str[10240];
} __attribute__((__packed__)) http_return_t;

void print_http_return(uint8_t * buf);
//void print_http_return(http_return_t * http);

uint32_t ip_to_num(const char * ip);

typedef enum {
    GET_USER_INFO = 0,
    GET_USER_PWD = 1,
    GET_DATABASE_INFO = 2,
    GET_TABLE_INFO = 3,
    GET_FIELD_INFO = 4,
    GET_ALL_INFO = 5,
    GET_GLOBAL_PRIV = 10,
    GET_DATABASE_PRIV = 11,
    GET_TABLE_PRIV = 12,
    GET_FIELD_PRIV = 13,
    GET_ALL_PRIV = 14,
    GET_TABLE_TC_PRIV = 15,
} e_get_mysql_info;

typedef struct {
    char     user[64];
    char     host[64];
    char     db[64];
    char     table[64];
    char     field[64];
    uint32_t priv;
} __attribute__((__packed__)) user_priv_t;

char * convert_ip(uint32_t ip, char * buf);

typedef struct {
    uint32_t  serial_no1;
    uint32_t  serial_no2;
    uint32_t  sql_no;
    uint32_t  op_type;

    char user[64];
    char host[16];
    char db[64];
    char table[64];
    char column[64];
    uint32_t old_priv;
    uint32_t new_priv;
    uint32_t old_tc_priv;   //对于table和column的权限，涉及到2张表的2个权限字段
    uint32_t new_tc_priv;
    char new_passwd[64];
    char old_passwd[64];
} change_record_t;

typedef struct {
    char  sql_str[64][1024];
} sql_str_t;

typedef enum {
    OP_CREATE_USER = 0, //添加用户
    OP_CHANGE_PWD  = 1, //修改密码
    OP_DELETE_USER = 2, //删除用户
    OP_ADD_PRIV_CR = 3, //添加用户时添加权限
    OP_ADD_PRIV    = 4, //添加权限
    OP_CHANGE_PRIV = 5, //修改权限
//    OP_DELETE_PRIV = 6, //删除权限
} e_operation_cmd;

int get_sql_str(change_record_t * change_record, sql_str_t * sql_str);

//int get_mysql_info(int type, char * info, int port, const char * database, const char * table);
int get_mysql_info(int type, int port, user_priv_t * upriv, uint8_t * buf, uint32_t buf_len);
//int get_mysql_info(int type, const char * ip, int port, user_priv_t * upriv, uint8_t * buf, uint32_t buf_len);

int get_user_info(const char * file, query_user_t * user, int len, const user_priv_t * c_user);
int get_user_pwd(const char * file, char * pwd, const user_priv_t * c_user);
int get_database_info(const char * datadir, name_t * name, int len);
int get_table_info(const char * path, name_t * name, int len);
int get_field_info(const char * file, name_t * name, int len);

int get_global_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv);
int get_database_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv);
int get_table_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv);
int get_field_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv);
int get_table_tc_priv(const char * file, uint32_t * priv, const user_priv_t * upriv);
/*
int _get_user_info(MYSQL * mysql, query_user_t * user, int len, const user_priv_t * c_user);
int _get_user_pwd(MYSQL * mysql, char * pwd, const user_priv_t * c_user);
int _get_database_info(MYSQL * mysql, name_t * name, int len);
int _get_table_info(MYSQL * mysql, name_t * name, int len);
int _get_field_info(MYSQL * mysql, name_t * name, int len);

int _get_global_priv(MYSQL * mysql, priv_t * priv, int len, const user_priv_t * upriv);
int _get_database_priv(MYSQL * mysql, priv_t * priv, int len, const user_priv_t * upriv);
int _get_table_priv(MYSQL * mysql, priv_t * priv, int len, const user_priv_t * upriv);
int _get_field_priv(MYSQL * mysql, priv_t * priv, int len, const user_priv_t * upriv);
int _get_table_tc_priv(MYSQL * mysql, uint32_t * priv, const user_priv_t * upriv);
*/
char * get_global_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_database_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_table_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_field_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf);

char * get_global_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_database_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_table_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_field_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf, const char * field);

char * get_global_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_database_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_table_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf);
char * get_field_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf);

uint32_t get_new_field_priv(int port, const char * db, const char * table, const char * column, const char * user, const char * host);
//uint32_t _get_new_field_priv(const char * ip, int port, const char * db, const char * table, const char * column, const char * user, const char * host);

void print_global_priv(int priv);
void print_database_priv(int priv);
void print_table_priv(int priv);
void print_field_priv(int priv);

uint32_t put_into_buf(uint8_t * buf, http_return_t * http_return, uint32_t left_len);
http_return_t * put_into_http_return(http_return_t * http_return, const char * sql, const char * errstr);

#endif  /*GET_MYSQL_INFO_H*/
