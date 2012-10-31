#ifndef  MYSQL_CONNECT_AUTO_PTR_H
#define  MYSQL_CONNECT_AUTO_PTR_H

#include <mysql/mysql.h>
#include <stdint.h>

class c_mysql_connect_auto_ptr
{
public:
    c_mysql_connect_auto_ptr(const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned int client_flag);
    ~c_mysql_connect_auto_ptr();

    int query(const char *query);
    const char * m_error();
    uint32_t m_errno();

private:
    MYSQL *mysql;
};


#endif  /*MYSQL_CONNECT_AUTO_PTR_H*/
