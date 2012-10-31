#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <time.h>
#include <arpa/inet.h>

#include "mysql_mgr.h"
#include "mysql_connect_auto_ptr.h"
#include "proto_handler.h"
#include "lib.h"
#include "error_code.h"

void process_query_info(uint8_t *, uint32_t);
void process_query_user(uint8_t *, uint32_t);
void process_check_user(uint8_t *, uint32_t);
void process_check_passwd(uint8_t *, uint32_t);
void process_priv_info(uint8_t *, uint32_t);

void process_add_user(uint8_t *, uint32_t);
void process_del_user(uint8_t *, uint32_t);

void process_add_priv(uint8_t *, uint32_t);
void process_edit_priv(uint8_t *, uint32_t);

int check_package(head_t * head);
int check_sqlip(uint32_t sqlip);

extern char hostip[2][16];

int init(void * par)
{
    log_init("../log/", (log_lvl_t)7, 32000000, 7, "oa_mysql_");

    if(get_host_ip(OA_INSIDE_IP, hostip[OA_INSIDE_IP]) != 0) {
        strcpy(hostip[OA_INSIDE_IP], "error");
        ERROR_LOG("get OA_INSIDE_IP error");
    }
    if(get_host_ip(OA_OUTSIDE_IP, hostip[OA_OUTSIDE_IP]) != 0) {
        strcpy(hostip[OA_OUTSIDE_IP], "error");
        ERROR_LOG("get OA_OUTSIDE_IP error");
    }

    return 0;
}

int fini(void * par)
{
    memset(hostip[0], 0, sizeof(hostip[0]));
    memset(hostip[1], 0, sizeof(hostip[1]));
    log_fini();
    return 0;
}

int proto_handler(uint8_t * buf, uint32_t buf_len)
{
    head_t * head = (head_t *)buf;
    int r;
    print_head(head);
    uint32_t ip = *((uint32_t *)(buf + sizeof(head_t)));
    uint32_t cmd_id = head->cmd_id & 0x0FFFFFFF; //屏蔽高位的返回类型

    uint32_t ddd = 0;
    ddd++;

    if((r = check_package(head)) != RESULT_OK) {
        head->pkg_len = sizeof(head_t);
        head->result = r;
        ERROR_LOG("check head error");
        DEBUG_LOG("return : ");
        print_head(head);
        goto out;
    }

    if((r = check_sqlip(ip)) != RESULT_OK) {
        head->pkg_len = sizeof(head_t);
        head->result = r;
        ERROR_LOG("check sqlip error");
        DEBUG_LOG("return : ");
        print_head(head);
        goto out;
    }

    switch(cmd_id) {
        case 0x1001://查询库、表、字段信息
            process_query_info(buf, buf_len);
            break;
        case 0x1002:
            process_priv_info(buf, buf_len);
            break;
        case 0x1011:
            process_add_user(buf, buf_len);
            break;
        case 0x1012:
            process_del_user(buf, buf_len);
            break;
        case 0x1013://查询用户是否存在
            process_check_user(buf, buf_len);
            break;
        case 0x1014:
            process_query_user(buf, buf_len);
            break;
        case 0x1015:
            process_check_passwd(buf, sizeof(buf));
            break;
        case 0x1021:
            process_edit_priv(buf, buf_len);
            break;
        case 0x1022:
            process_add_priv(buf, buf_len);
            break;
        default:
            head->pkg_len = sizeof(head_t);
            head->result = RESULT_ECOMMAND;
            break;
    }

out:
    head->timestamp = time(NULL);
    char md5buf[128];
    char md5[33];
    sprintf(md5buf, "%s%u%u%u", PRIV_KEY, head->cmd_id, head->timestamp, head->serial_no1);
    if((r = str2md5(md5buf, md5)) != 0) {
        ERROR_LOG("calculate MD5 for %s return error[%d]", md5buf, r);
    }
    memcpy(head->veri_code, md5, 32);
    if(head->result == RESULT_ECOMMAND) {
        ERROR_LOG("error cmdid 0x%08X", head->cmd_id);
        DEBUG_LOG("return : ");
        print_head(head);
    }
    return 0;
}
