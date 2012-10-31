#ifndef LIB_2012_02_23_H
#define LIB_2012_02_23_H

#include <mysql/mysql.h>

/**<IP类型 */
#define OA_INSIDE_IP            (0)
#define OA_OUTSIDE_IP           (1)

extern char mysql_socket[1024];

char *trim(char *src);
int get_host_ip_by_name(const char *eth_name, char *host_ip);
int get_host_ip(int ip_type, char *host_ip);
int str2md5(const char *p_str, char *p_md5);
uint8_t * decode(const uint8_t * code, uint8_t * buf, uint8_t len);
//int do_sql(MYSQL * mysql, const char * sql, MYSQL_RES ** sql_res);
int flush_table(const char * table, int port);

#endif //LIB_2012_02_23_H
