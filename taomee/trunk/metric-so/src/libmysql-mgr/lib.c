#include <openssl/md5.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include<unistd.h>

#include <libtaomee/log.h>
#include "mysql_connect_auto_ptr.h"
#include "lib.h"

/**
 * @brief  通过网卡名获取机器IP地址
 * @param  eth_name: 网卡类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip_by_name(const char *eth_name, char *host_ip)
{

    int ret_code = -1;
    int sockfd = -1;

    do {
        if (NULL == eth_name || NULL == host_ip) {
            ERROR_LOG("ERROR: Parameter cannot be NULL.");
            break;
        }
        if (0 == strlen(eth_name)) {
            ERROR_LOG("ERROR: eth_name length cannot be zero.");
            break;
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(-1 == sockfd) {
            ERROR_LOG("ERROR: socket(AF_INET, SOCK_DGRAM, 0) failed: %s", strerror(errno));
            break;
        }

        struct ifreq ifr;
        strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) -1] = 0;

        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
            ERROR_LOG("ERROR: ioctl(%d, SIOCGIFADDR, %s) failed: %s", sockfd, eth_name, strerror(errno));
            break;
        }
        strcpy(host_ip, inet_ntoa(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr));
        ret_code = 0;
    } while (false);

    if (sockfd >= 0) {
        close(sockfd);
    }

    return ret_code;
}


char *trim(char *src)
{
    char *start;
    start = src;
    while (isspace(*start)) {
        if ( uint32_t(start - src) == (strlen(src) -1) ) {
            src[0] = '\0';
            return src;
        }
        start++;
    }
    char *end;
    end = src + strlen(src) - 1;
    while (isspace(*end)) {
        if (end == src) {
            src[0] = '\0';
        }
        end--;
    }

    char *p;
    p = src;
    while (start <= end) {
        *p = *start;
        p++;
        start++;
    }
    *p = '\0';
    return src;
}

/**
 * @brief  获取机器IP地址
 * @param  ip_type: IP类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip(int ip_type, char *host_ip)
{
    if (NULL == host_ip) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }
    if (OA_OUTSIDE_IP == ip_type) {
        if (0 == get_host_ip_by_name("eth0", host_ip)) {///一般情况，外网是eth0
            return 0;
        }
    } else {
        if (0 == get_host_ip_by_name("eth1", host_ip)) {///一般情况，内网是eth1
            return 0;
        }
    }
    ///特殊情况
    char bond_ip[16] = {0};
    char eth2_ip[16] = {0};
    char eth3_ip[16] = {0};
    get_host_ip_by_name("bond0", bond_ip);
    get_host_ip_by_name("eth2", eth2_ip);
    get_host_ip_by_name("eth3", eth3_ip);

    if (OA_OUTSIDE_IP == ip_type) {//外网
        if (strlen(bond_ip) >= 7 || strncmp(bond_ip, "192.168.", 8)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (strlen(eth3_ip) >= 7 || strncmp(eth3_ip, "192.168.", 8)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (strlen(eth2_ip) >= 7 || strncmp(eth2_ip, "192.168.", 8)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }
    else {//内网
        if (!strncmp(bond_ip, "192.168.", 8)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (!strncmp(eth3_ip, "192.168.", 8)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (!strncmp(eth2_ip, "192.168.", 8)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }

    return -1;
}

/**
 * @brief  求字符串的md5值
 * @param   p_str 源字符串
 * @param   p_md5 保存返回的md5值
 * @return  -1-failed, 0-success
 */
int str2md5(const char *p_str, char *p_md5)
{
    if (NULL == p_str || NULL == p_md5) {
        return -1;
    }

    unsigned char md5[16] = {0};
    char *p_pos = p_md5;
    MD5((const unsigned char *)p_str, strlen(p_str), md5);
    for (int i = 0; i < 16; i++) {
        sprintf(p_pos, "%02x", md5[i]);
        p_pos += 2;
    }

    return 0;
}

uint8_t * decode(const uint8_t * code, uint8_t * buf, uint8_t len)
{
    uint8_t code_len = ((code[len-2] ^ (0x91)) - 0x30) * 10 + (code[len-1] ^ (0x91)) - 0x30;
    code_len = code_len > len ? len-1 : code_len;
    for(uint8_t i=0; i<code_len; i++) {
        buf[i] = code[i] ^ (0x91);
    }
    buf[code_len] = '\0';
    //DEBUG_LOG("dba passwd %s", buf);
    return buf;
}
/*
int do_sql(MYSQL * mysql, const char * sql, MYSQL_RES ** sql_res)
{
	//检查连接
	if(mysql_ping(mysql))
	{
		ERROR_LOG("mysql_ping() ERROR, %s!", mysql_error(mysql));
		return -1;
	}
	//查询数据
	if(mysql_query(mysql, sql))
	{
		ERROR_LOG("mysql_query() ERROR, %s!, \nSQL[%s].", mysql_error(mysql), sql);
		return -1;
	}
	//存储结果
	if(sql_res != NULL) {
		*sql_res = mysql_store_result(mysql);
		if(NULL == sql_res)
		{
			ERROR_LOG("mysql_store_result() ERROR, %s!\nSQL[%s]", mysql_error(mysql), sql);
			return -1;
		}
	}
	return 0;
}
*/
int flush_table(const char * table, int port)
{
    //return 0;
    c_mysql_connect_auto_ptr mysql("localhost", "oaadmin", "oaadmin@tm", "mysql", port, mysql_socket, CLIENT_INTERACTIVE);
    char buf[1024];
    sprintf(buf, "REPAIR TABLE %s", table);
    INFO_LOG("%s", buf);
    int r = mysql.query(buf);
    if(r != 0) {
        ERROR_LOG("REPAIR TABLE %s error[%s] %d", table, mysql.m_error(), mysql.m_errno());
    }
    return r;
}
