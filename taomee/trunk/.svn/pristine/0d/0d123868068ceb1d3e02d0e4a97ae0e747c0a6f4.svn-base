#include <stddef.h>
#include <stdio.h>
#include <libtaomee/log.h>
#include "mysql_connect_auto_ptr.h"

c_mysql_connect_auto_ptr::c_mysql_connect_auto_ptr(const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned int client_flag) {
    mysql = mysql_init(NULL);
    if(mysql != NULL) {
        my_bool is_auto_reconnect = 1;
        mysql_options(mysql, MYSQL_OPT_RECONNECT, &is_auto_reconnect);
        if(mysql_real_connect(mysql, host, user, passwd, db, port, unix_socket, client_flag) != NULL) {
            mysql_set_character_set(mysql, "utf8");
            DEBUG_LOG("connect to mysql");
        } else {
            ERROR_LOG("connect to mysql error");
        }
    }
}

c_mysql_connect_auto_ptr::~c_mysql_connect_auto_ptr()
{
    if(mysql != NULL) {
        mysql_close(mysql);
    }
}

int c_mysql_connect_auto_ptr::query(const char *query)
{
    //return 0;
    if(mysql == NULL || mysql_ping(mysql) != 0) {
        return -1;
    }
    return mysql_query(mysql, query);
}

const char error_str[] = "mysql_headle == NULL.";

const char * c_mysql_connect_auto_ptr::m_error()
{
    if(mysql == NULL) {
        return error_str;
    } else {
        return mysql_error(mysql);
    }
}

uint32_t c_mysql_connect_auto_ptr::m_errno()
{
    if(mysql == NULL) {
        return -1;
    } else {
        return mysql_errno(mysql);
    }
}
