#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mysql_mgr.h"
#include "mysql_connect_auto_ptr.h"
#include "proto_handler.h"
#include "lib.h"
#include "error_code.h"

#define REAPIR 0

char hostip[2][16];
char mysql_socket[1024];

int check_package(head_t * head)
{
    char buf[128];
    char md5[32];
    int r;
    sprintf(buf, "%s%u%u%u", PRIV_KEY, head->cmd_id, head->timestamp, head->serial_no1);
    if((r = str2md5(buf, md5)) != 0) {
        ERROR_LOG("calculate MD5 for %s return error[%d]", buf, r);
        return RESULT_ECALUCMD5;
    }
    if((r = memcmp(md5, head->veri_code, sizeof(md5))) != 0) {
        ERROR_LOG("check veri code error %s", buf);
        ERROR_LOG("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                head->veri_code[0], head->veri_code[1], head->veri_code[2], head->veri_code[3],
                head->veri_code[4], head->veri_code[5], head->veri_code[6], head->veri_code[7],
                head->veri_code[8], head->veri_code[9], head->veri_code[10], head->veri_code[11],
                head->veri_code[12], head->veri_code[13], head->veri_code[14], head->veri_code[15],
                head->veri_code[16], head->veri_code[17], head->veri_code[18], head->veri_code[19],
                head->veri_code[20], head->veri_code[21], head->veri_code[22], head->veri_code[23],
                head->veri_code[24], head->veri_code[25], head->veri_code[26], head->veri_code[27],
                head->veri_code[28], head->veri_code[29], head->veri_code[30], head->veri_code[31]);
        ERROR_LOG("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
                md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15],
                md5[16], md5[17], md5[18], md5[19], md5[20], md5[21], md5[22], md5[23],
                md5[24], md5[25], md5[26], md5[27], md5[28], md5[29], md5[30], md5[31]);
        return RESULT_ENODECHECKMD5;
    }
    return RESULT_OK;
}

int check_sqlip(uint32_t sqlip)
{
    char ip[16];
    convert_ip(sqlip, ip);
    if(strcmp(hostip[OA_INSIDE_IP], "error") != 0) {
        if(strcmp(ip, hostip[OA_INSIDE_IP]) == 0) {
            return RESULT_OK;
        } else {
            ERROR_LOG("sqlip[%s] != OA_INSIDE_IP[%s]", ip, hostip[OA_INSIDE_IP]);
        }
    }
    if(strcmp(hostip[OA_OUTSIDE_IP], "error") != 0) {
        if(strcmp(ip, hostip[OA_OUTSIDE_IP]) == 0) {
            return RESULT_OK;
        } else {
            ERROR_LOG("sqlip[%s] != OA_OUTSIDE_IP[%s]", ip, hostip[OA_OUTSIDE_IP]);
        }
    } else {
        return RESULT_EGETHOSTIP;
    }
    return RESULT_EHOSTIP;
}

void process_query_info(uint8_t * buf, uint32_t buf_len)
{
    int ret;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    head_t * head = (head_t *)buf;
    query_recv_t * query = (query_recv_t *)(buf+sizeof(head_t));
    query_info_send_t * query_info_send = (query_info_send_t *)(buf+sizeof(head_t));
    uint32_t len = buf_len - ((uint8_t *)(query_info_send->name) - buf);

    print_query_recv(query);

    strcpy(upriv.user, query->user_name);
    convert_ip(query->host_ip, upriv.host);
    strcpy(upriv.db, query->db_name);
    strcpy(upriv.table, query->table_name);
    strcpy(upriv.field, query->column_name);

    if(query->db_name[0] == '\0') {
        DEBUG_LOG("database~~~");
        ret = get_mysql_info(GET_DATABASE_INFO, query->sql_port, &upriv, (uint8_t *)query_info_send->name, len);
    } else if(query->table_name[0] == '\0') {
        DEBUG_LOG("table~~~");
        ret = get_mysql_info(GET_TABLE_INFO, query->sql_port, &upriv, (uint8_t *)query_info_send->name, len);
    } else if(query->column_name[0] == '\0') {
        DEBUG_LOG("column~~~");
        ret = get_mysql_info(GET_FIELD_INFO, query->sql_port, &upriv, (uint8_t *)query_info_send->name, len);
    } else {
        ret = RESULT_EGETINFO_PAR;
    }
    if(ret >= 0) {
        head->result = RESULT_OK;
        head->pkg_len = sizeof(head_t) + sizeof(name_t)*ret + sizeof(uint32_t);
        query_info_send->count = ret;
        DEBUG_LOG("OK.");
    } else if(ret == -2) {
        head->result = RESULT_EGETINFO_DB;
        head->pkg_len = sizeof(head_t);
        ERROR_LOG("error %d.", ret);
    } else if(ret == -3) {
        head->result = RESULT_EGETINFO_TAB;
        head->pkg_len = sizeof(head_t);
        ERROR_LOG("error %d.", ret);
    } else if(ret == -4) {
        head->result = RESULT_EGETINFO_COL;
        head->pkg_len = sizeof(head_t);
        ERROR_LOG("error %d.", ret);
    } else {
        head->result = RESULT_EGETINFO_NOBUF;
        head->pkg_len = sizeof(head_t);
        ERROR_LOG("error %d.", ret);
    }

    DEBUG_LOG("return : ");
    print_head(head);
    if(head->pkg_len > sizeof(head_t)) {
        print_query_info_send(query_info_send);
    }
}

void process_check_user(uint8_t * buf, uint32_t buf_len)
{
    uint32_t len = buf_len - sizeof(head_t);
    int ret;
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    check_user_t * check_user = (check_user_t*)(buf + sizeof(head_t));
    print_check_user_t(check_user);

    check_user_send_t * send = (check_user_send_t *)(buf + sizeof(head_t));

    strcpy(upriv.user, check_user->user.name);
    convert_ip(check_user->user.host_ip, upriv.host);

    ret = get_mysql_info(GET_USER_INFO, check_user->head.sql_port, &upriv, buf + sizeof(head_t), len);
    if(ret == 1) {
        send->exists = 1;
        head->result = RESULT_OK;
        head->pkg_len = sizeof(head_t) + sizeof(check_user_send_t);
    } else if(ret == 0) {
        send->exists = 0;
        head->result = RESULT_OK;
        head->pkg_len = sizeof(head_t) + sizeof(check_user_send_t);
    } else if(ret == -1) {
        head->result = RESULT_ECHKUSER_FILE;
        head->pkg_len = sizeof(head_t);
    } else if(ret < 0) {
        head->result = RESULT_ECHKUSER_NOBUF;
        head->pkg_len = sizeof(head_t);
    } else {
        head->result = RESULT_ECHKUSER_MORE;
        head->pkg_len = sizeof(head_t);
    }
    DEBUG_LOG("return : ");
    print_head(head);
    print_chech_user_send(send);
}

#define GLOBAL (0)
#define TOTAL (1)
#define EXACT (2)
#define TABLE (3)
#define COLUMN (4)
#define MAX_SQL_BUF_LEN (4096)

void process_priv_info(uint8_t * buf, uint32_t buf_len)
{
    int ret;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    head_t * head = (head_t *)buf;
    query_recv_t * query = (query_recv_t *)(buf+sizeof(head_t));
    print_query_recv(query);
    query_priv_send_t * send = (query_priv_send_t*)(buf+sizeof(head_t));
    uint32_t len = buf_len - ((uint8_t *)(send->priv) - buf);

    strcpy(upriv.user, query->user_name);
    convert_ip(query->host_ip, upriv.host);
    strcpy(upriv.db, query->db_name);
    strcpy(upriv.table, query->table_name);
    strcpy(upriv.field, query->column_name);

    if(query->level == GLOBAL) {
        DEBUG_LOG("global priv ~~~~~~~");
        ret = get_mysql_info(GET_GLOBAL_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
    } else if(query->level == TOTAL) {
        if(query->db_name[0] == '\0') {
            DEBUG_LOG("database priv ~~~~~~~");
            ret = get_mysql_info(GET_DATABASE_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        } else if(query->table_name[0] == '\0') {
            DEBUG_LOG("table priv ~~~~~");
            ret = get_mysql_info(GET_TABLE_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        } else if(query->column_name[0] == '\0') {
            DEBUG_LOG("column priv ~~~~~");
            ret = get_mysql_info(GET_FIELD_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        } else {
            ret = -5;
        }

    } else if(query->level == EXACT) {
        if(query->db_name[0] == '\0') {
            head->result = 1;
            head->pkg_len = sizeof(head_t);
        } else if(query->table_name[0] == '\0') {
            DEBUG_LOG("database priv ~~~~~~~");
            ret = get_mysql_info(GET_DATABASE_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        }  else if(query->column_name[0] == '\0') {
            DEBUG_LOG("table priv ~~~~~");
            ret = get_mysql_info(GET_TABLE_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        } else {
            DEBUG_LOG("column priv ~~~~~");
            ret = get_mysql_info(GET_FIELD_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
        }
    } else if(query->level == TABLE) {
        DEBUG_LOG("all table priv ~~~~~");
        ret = get_mysql_info(GET_TABLE_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
    } else if(query->level == COLUMN) {
        DEBUG_LOG("all column priv ~~~~~");
        ret = get_mysql_info(GET_FIELD_PRIV, query->sql_port, &upriv, (uint8_t *)send->priv, len);
    } else {
        ret = -6;
    }
    if(ret >= 0) {
        send->count = ret;
        head->result = RESULT_OK;
        head->pkg_len = sizeof(head_t) + sizeof(send->count) + sizeof(send->priv[0]) * send->count;
    } else if(ret == -1) {
        head->result = RESULT_EGETPRIV_GLO;
        head->pkg_len = sizeof(head_t);
    } else if(ret == -2) {
        head->result = RESULT_EGETPRIV_GLO;
        head->pkg_len = sizeof(head_t);
    } else if(ret == -3) {
        head->result = RESULT_EGETPRIV_GLO;
        head->pkg_len = sizeof(head_t);
    } else if(ret == -4) {
        head->result = RESULT_EGETPRIV_GLO;
        head->pkg_len = sizeof(head_t);
    } else if(ret == -5) {
        head->result = RESULT_EGETPRIV_PAR;
        head->pkg_len = sizeof(head_t);
    } else if(ret == -6) {
        head->result = RESULT_EGETPRIV_LEVPAR;
        head->pkg_len = sizeof(head_t);
    } else {
        head->result = RESULT_EGETPRIV_NOBUF;
        head->pkg_len = sizeof(head_t);
    }
    DEBUG_LOG("result :");
    print_head(head);
    if(head->pkg_len > sizeof(head_t)) {
        print_query_priv_send(send);
    }
}

void process_query_user(uint8_t * buf, uint32_t buf_len)
{
    int ret;
    head_t * head = (head_t *)buf;

    query_user_info_t * query_user_info = (query_user_info_t *)(buf + sizeof(head_t));
    print_query_user_info(query_user_info);
    query_user_send_t * send = (query_user_send_t*)(buf + sizeof(head_t));
    uint32_t len = buf_len - ((uint8_t *)(send->user) - buf);

    ret = get_mysql_info(GET_USER_INFO, query_user_info->sql_port, NULL, (uint8_t *)send->user, len);
    if(ret >= 0) {
        head->result = RESULT_OK;
        send->count = ret;
        head->pkg_len = sizeof(head_t) + sizeof(send->user[0])*send->count + sizeof(send->count);
    } else if(ret == -1) {
        head->result = RESULT_EGETUSER_FILE;
        head->pkg_len = sizeof(head_t);
    } else {
        head->result = RESULT_EGETUSER_NOBUF;
        head->pkg_len = sizeof(head_t);
    }
    DEBUG_LOG("return :");
    print_head(head);
    if(head->pkg_len > sizeof(head_t)) {
        print_query_user_send(send);
    }
}

#define SET_HTTP_RETURN(errno, sql, errstr) \
    do {\
        http_return.err_no = errno;\
        put_into_http_return(&http_return, sql, errstr);\
        ret_len = put_into_buf(p_http_return + save_len, &http_return, left_len);\
        save_len += ret_len;\
        left_len = ret_len >= left_len ? 0 : left_len - ret_len;\
        sql_count++;\
    } while(0)

void process_add_user(uint8_t * buf, uint32_t buf_len)
{
    int ret, n = 0;
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    //uint16_t * sql_count = (uint16_t *)(buf + sizeof(head_t));
    uint16_t sql_count = 0;
    //bool dosql_error = false;    
    http_return_t http_return;
    uint32_t save_len = 0;
    uint32_t left_len = buf_len - (sizeof(head_t) + sizeof(sql_count));
    uint32_t ret_len = 0;
    char sql_buf[MAX_SQL_BUF_LEN];
    uint8_t * p_http_return = (uint8_t *)malloc(left_len);

    add_user_t * add = (add_user_t *)(buf + sizeof(head_t));
    print_add_user(add);

    change_record_t change_record;
    sql_str_t sql_str;

    c_mysql_connect_auto_ptr mysql("localhost", add->dba.dba_name,
            (char *)decode((const uint8_t *)add->dba.dba_passwd, (uint8_t *)add->dba.dba_passwd, sizeof(add->dba.dba_passwd)),
            "mysql", add->head.sql_port, mysql_socket, CLIENT_INTERACTIVE);

    strcpy(upriv.user, add->new_user);
    strcpy(change_record.user, add->new_user);
    strcpy(change_record.new_passwd, add->new_passwd);
    for(uint32_t i=0; i<add->host_count; i++) {
        convert_ip(add->host[i].host_ip, upriv.host);
        strcpy(change_record.host, upriv.host);
        memset(change_record.old_passwd, 0, sizeof(change_record.old_passwd));

        change_record.serial_no1 = head->serial_no1;
        change_record.serial_no2 = add->host[i].add_no;
        http_return.value_id = add->host[i].add_no;
        change_record.sql_no = 1;   //自增
        if((ret = get_mysql_info(GET_USER_PWD, add->head.sql_port, &upriv, (uint8_t *)change_record.old_passwd, sizeof(change_record.old_passwd))) == 1) {
            DEBUG_LOG("change pwd.");
            change_record.op_type = OP_CHANGE_PWD;
        } else if(ret == 0) {
            DEBUG_LOG("not find.");
            change_record.op_type = OP_CREATE_USER;
        } else if(ret == -1) {
            SET_HTTP_RETURN(RESULT_EADDUSER_FILE, "", "RESULT_EADDUSER_FILE");
            //print_http_return(&http_return);
            goto out;
        } else if(ret < 0 ) {
            SET_HTTP_RETURN(RESULT_EADDUSER_NOBUF, "", "RESULT_EADDUSER_NOBUF");
            //print_http_return(&http_return);
            goto out;
        } else {
            SET_HTTP_RETURN(RESULT_EADDUSER_MORE, "", "RESULT_EADDUSER_MORE");
            //print_http_return(&http_return);
            goto out;
        }

        if((n = get_sql_str(&change_record, &sql_str)) < 0) {
            SET_HTTP_RETURN(RESULT_EGETSQL, "", "RESULT_EGETSQL");
            //print_http_return(&http_return);
            goto out;
        }
        sql_buf[0] = '\0';
        http_return.sql_id = n;
        for(int j=0; j<n; j++) {
            INFO_LOG("%u %u %u %s", head->serial_no1, add->host[i].add_no, j+1, sql_str.sql_str[j]);                        
            sprintf(sql_buf, "%s%s;", sql_buf, sql_str.sql_str[j]);

            if(mysql.query(sql_str.sql_str[j]) == 0) {
                DEBUG_LOG("do sql [%s] success!!!", sql_str.sql_str[j]);
                http_return.err_no = 0;
                http_return.err_len = 0;
            } else {
                ERROR_LOG("do sql [%s] error!!!", sql_str.sql_str[j]);
                int e = mysql.m_errno();
                http_return.err_no = e==-1 ? RESULT_EMYSQL_CONNECT : e+RESULT_EMYSQL;
                SET_HTTP_RETURN(http_return.err_no, sql_buf, mysql.m_error());
                //put_into_http_return(&http_return, sql_buf, mysql.m_error());
                //http_return.sql_len = strlen(sql_buf) + 1;
                //strcpy(http_return.str, sql_buf);
                //strcpy(http_return.str + http_return.sql_len, mysql.m_error());
                //http_return.err_len = strlen(http_return.str + http_return.sql_len) + 1;
                //ret_len = put_into_buf(p_http_return + save_len, &http_return, left_len);
                //save_len += ret_len;
                //left_len = ret_len >= left_len ? 0 : left_len - ret_len;
                //sql_count++;
                //print_http_return(&http_return);
                goto out;
            }
        }
        SET_HTTP_RETURN(http_return.err_no, sql_buf, "");
        //sql_count++;
        //put_into_http_return(&http_return, sql_buf, "");
        //http_return.sql_len = strlen(sql_buf) + 1;
        //strcpy(http_return.str, sql_buf);
        //ret_len = put_into_buf(p_http_return + save_len, &http_return, left_len);
        //save_len += ret_len;
        //left_len = ret_len >= left_len ? 0 : left_len - ret_len;
        //print_http_return(&http_return);
    }

out:
#if REAPIR
    mysql.query("REPAIR TABLE user");
    mysql.query("FLUSH TABLE user");
    INFO_LOG("REPAIR TABLE user");
    INFO_LOG("FLUSH TABLE user");
#endif
    head->result = RESULT_OK;
    head->pkg_len = sizeof(head_t) + save_len + sizeof(sql_count);
    *(uint16_t *)(buf + sizeof(head_t)) = sql_count;
    memcpy(buf + sizeof(head_t) + sizeof(sql_count), p_http_return, save_len);

    free(p_http_return);
    DEBUG_LOG("result :");
    print_head(head);
    print_http_return((uint8_t *)(buf + sizeof(head_t)));
}

void process_del_user(uint8_t * buf, uint32_t buf_len)
{
    int ret, n;
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    uint16_t sql_count = 0;
    http_return_t http_return;
    uint32_t save_len = 0;
    uint32_t left_len = buf_len - (sizeof(head_t) + sizeof(sql_count));
    uint32_t ret_len = 0;
    char sql_buf[MAX_SQL_BUF_LEN];
    uint8_t * p_http_return = (uint8_t *)malloc(left_len);

    del_user_t * del = (del_user_t *)(buf + sizeof(head_t));
    print_del_user(del);

    change_record_t change_record;
    sql_str_t sql_str;

    c_mysql_connect_auto_ptr mysql("localhost", del->dba.dba_name,
            (char *)decode((const uint8_t *)del->dba.dba_passwd, (uint8_t *)del->dba.dba_passwd, sizeof(del->dba.dba_passwd)),
            "mysql", del->head.sql_port, mysql_socket, CLIENT_INTERACTIVE);

    for(uint8_t i=0; i<del->del_count; i++) {
        strcpy(change_record.user, del->del_user[i].name);
        strcpy(upriv.user, del->del_user[i].name);
        convert_ip(del->del_user[i].host_ip, upriv.host);
        strcpy(change_record.host, upriv.host);
        memset(change_record.old_passwd, 0, sizeof(change_record.old_passwd));

        change_record.serial_no1 = head->serial_no1;
        change_record.serial_no2 = del->del_user[i].del_no;
        http_return.value_id = del->del_user[i].del_no;
        change_record.sql_no = 1;   //自增
        if((ret = get_mysql_info(GET_USER_PWD, del->head.sql_port, &upriv, (uint8_t *)change_record.old_passwd, sizeof(change_record.old_passwd))) == 1) {
            DEBUG_LOG("delete user.");
            change_record.op_type = OP_DELETE_USER;
        } else if(ret == 0) {
            ERROR_LOG("not find.");
            SET_HTTP_RETURN(RESULT_EDELUSER_NOFIND, "", "RESULT_EDELUSER_NOFIND");
            //print_http_return(&http_return);
            goto out;
        } else if(ret == -1) {
            ERROR_LOG("RESULT_EDELUSER_FILE");
            SET_HTTP_RETURN(RESULT_EDELUSER_FILE, "", "RESULT_EDELUSER_FILE");
            //print_http_return(&http_return);
            goto out;
        } else if(ret < 0 ) {
            ERROR_LOG("RESULT_EDELUSER_NOBUF");
            SET_HTTP_RETURN(RESULT_EDELUSER_NOBUF, "", "RESULT_EDELUSER_NOBUF");
            //print_http_return(&http_return);
            goto out;
        } else {
            ERROR_LOG("RESULT_EDELUSER_MORE");
            SET_HTTP_RETURN(RESULT_EDELUSER_MORE, "", "RESULT_EDELUSER_MORE");
            //print_http_return(&http_return);
            goto out;
        }
        if((n = get_sql_str(&change_record, &sql_str)) < 0) {
            ERROR_LOG("RESULT_EGETSQL");
            SET_HTTP_RETURN(RESULT_EGETSQL, "", "RESULT_EGETSQL");
            //print_http_return(&http_return);
            goto out;
        }
        DEBUG_LOG("%d", n);
        sql_buf[0] = '\0';
        http_return.sql_id = n;
        for(int j=0; j<n; j++) {
            INFO_LOG("%u %u %u %s", head->serial_no1, del->del_user[i].del_no, j+1, sql_str.sql_str[j]);            
            sprintf(sql_buf, "%s%s;", sql_buf, sql_str.sql_str[j]);
            if( mysql.query(sql_str.sql_str[j]) == 0) {
                DEBUG_LOG("do sql [%s] success!!!", sql_str.sql_str[j]);
                http_return.err_no = 0;
                http_return.err_len = 0;
            } else {
                ERROR_LOG("do sql [%s] error!!!", sql_str.sql_str[j]);
                int e = mysql.m_errno();
                http_return.err_no = e==-1 ? RESULT_EMYSQL_CONNECT : e+RESULT_EMYSQL;
                SET_HTTP_RETURN(http_return.err_no, sql_buf, mysql.m_error());
                //print_http_return(&http_return);
                goto out;
            }
        }
        SET_HTTP_RETURN(http_return.err_no, sql_buf, "");
        //print_http_return(&http_return);
    }

out:
#if REAPIR
    mysql.query("REPAIR TABLE user");
    mysql.query("FLUSH TABLE user");
    INFO_LOG("REPAIR TABLE user");
    INFO_LOG("FLUSH TABLE user");
#endif
    head->result = RESULT_OK;
    head->pkg_len = sizeof(head_t) + save_len + sizeof(sql_count);
    *(uint16_t *)(buf + sizeof(head_t)) = sql_count;
    memcpy(buf + sizeof(head_t) + sizeof(sql_count), p_http_return, save_len);

    free(p_http_return);
    DEBUG_LOG("result :");
    print_head(head);
    print_http_return((uint8_t *)(buf + sizeof(head_t)));
}

void process_add_priv(uint8_t * buf, uint32_t buf_len)
{
    int ret, n;
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));
    priv_t rpriv;

    uint16_t sql_count = 0;
    http_return_t http_return;
    uint32_t save_len = 0;
    uint32_t left_len = buf_len - (sizeof(head_t) + sizeof(sql_count));
    uint32_t ret_len = 0;
    char sql_buf[MAX_SQL_BUF_LEN];
    uint8_t * p_http_return = (uint8_t *)malloc(left_len);

    edit_priv_t * edit_priv = (edit_priv_t *)(buf + sizeof(head_t));
    //total_priv_t * priv = (total_priv_t *)(edit_priv->host_ip + edit_priv->host_count);
    //total_priv_t * priv = (total_priv_t *)((uint8_t *)(edit_priv->host_ip) + sizeof(edit_priv->host_ip[0]) * edit_priv->host_count);
    print_edit_priv(edit_priv);
    //print_total_priv(priv);

    change_record_t change_record;
    sql_str_t sql_str;
    strcpy(change_record.user, edit_priv->user);
    strcpy(upriv.user, edit_priv->user);

    c_mysql_connect_auto_ptr mysql("localhost", edit_priv->dba.dba_name,
            (char *)decode((const uint8_t *)edit_priv->dba.dba_passwd, (uint8_t *)edit_priv->dba.dba_passwd, sizeof(edit_priv->dba.dba_passwd)),
            "mysql", edit_priv->head.sql_port, mysql_socket, CLIENT_INTERACTIVE);

    for(uint32_t j=0; j<edit_priv->host_count; j++) {
        //填change_record的各个字段
        change_record.serial_no1 = head->serial_no1;
        change_record.serial_no2 = edit_priv->priv[j].priv_no;
        http_return.value_id = edit_priv->priv[j].priv_no;
        change_record.sql_no = 1;//自增

        upriv.priv = edit_priv->priv[j].priv;
        change_record.new_priv = edit_priv->priv[j].priv;

        strcpy(upriv.db, edit_priv->priv[j].db_name);
        strcpy(upriv.table, edit_priv->priv[j].table_name);        
        strcpy(upriv.field, edit_priv->priv[j].column_name);        

        //for(uint8_t i=0; i<edit_priv->host_count; i++) {
        convert_ip(edit_priv->priv[j].host_ip, change_record.host);
        strcpy(upriv.host, change_record.host);
        //查找该权限:有则错，无则加
        if(edit_priv->priv[j].db_name[0] == '\0') { //全局权限
            ret = get_mysql_info(GET_GLOBAL_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret < -100) {
                //找到，权限已存在，出错
                ERROR_LOG("exits priv : *.* for %s@%s too many user", upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_GTM, "", "RESULT_EADDPRIV_GTM");
                //print_http_return(&http_return);
                goto out;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : *.* for %s@%s", upriv.user, upriv.host);
            } else if(ret == 1) {
                if(rpriv.priv != 0) {
                    ERROR_LOG("exits priv : *.* for %s@%s", upriv.user, upriv.host);
                    SET_HTTP_RETURN(RESULT_EADDPRIV_GEXIST, "", "RESULT_EADDPRIV_GEXIST");
                    //print_http_return(&http_return);
                    goto out;
                }
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : *.* for %s@%s", ret, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_GFILE, "", "RESULT_EADDPRIV_GFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else if(edit_priv->priv[j].table_name[0] == '\0') {  //库权限
            ret = get_mysql_info(GET_DATABASE_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，出错
                ERROR_LOG("exits priv : %s.* for %s@%s", upriv.db, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_DEXIST, "", "RESULT_EADDPRIV_DEXIST");
                //print_http_return(&http_return);
                goto out;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : %s.* for %s@%s", upriv.db, upriv.user, upriv.host);
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.* for %s@%s", ret, upriv.db, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_DFILE, "", "RESULT_EADDPRIV_DFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else if(edit_priv->priv[j].column_name[0] == '\0') { //表权限
            ret = get_mysql_info(GET_TABLE_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，出错
                ERROR_LOG("exits priv : %s.%s for %s@%s", upriv.db, upriv.table, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_TEXIST, "", "RESULT_EADDPRIV_TEXIST");
                //print_http_return(&http_return);
                goto out;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : %s.%s for %s@%s", upriv.db, upriv.table, upriv.user, upriv.host);
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.%s for %s@%s", ret, upriv.db, upriv.table, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_TFILE, "", "RESULT_EADDPRIV_TFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else { //字段权限
            ret = get_mysql_info(GET_FIELD_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {                    
                //找到，权限已存在，出错
                ERROR_LOG("exits priv : %s.%s.%s for %s@%s", upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_CEXIST, "", "RESULT_EADDPRIV_CEXIST");
                //print_http_return(&http_return);
                goto out;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : %s.%s.%s for %s@%s", upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.%s.%s for %s@%s", ret, upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EADDPRIV_CFILE, "", "RESULT_EADDPRIV_CFILE");
                //print_http_return(&http_return);
                goto out;
            }
        }
        change_record.old_priv = rpriv.priv;
        change_record.op_type = OP_ADD_PRIV_CR;
        strcpy(change_record.db, upriv.db);
        strcpy(change_record.table, upriv.table);
        strcpy(change_record.column, upriv.field);
        //DEBUG_LOG("db = %s table = %s column = %s", edit_priv->priv[j].db_name, edit_priv->priv[j].table_name, edit_priv->priv[j].column_name);
        //DEBUG_LOG("db = %s table = %s column = %s", change_record.db, change_record.table, change_record.column);
        if((n = get_sql_str(&change_record, &sql_str)) < 0) {
            ERROR_LOG("RESULT_EGETSQL");
            SET_HTTP_RETURN(RESULT_EGETSQL, "", "RESULT_EGETSQL");
            //print_http_return(&http_return);
            goto out;
        }
        DEBUG_LOG("%d", n);
        sql_buf[0] = '\0';
        http_return.sql_id = n;
        for(int k=0; k<n; k++) {
            INFO_LOG("%u %u %u %s", head->serial_no1, edit_priv->priv[j].priv_no, k+1, sql_str.sql_str[k]);                            
            sprintf(sql_buf, "%s%s;", sql_buf, sql_str.sql_str[k]);

            if( mysql.query(sql_str.sql_str[k]) == 0) {
                DEBUG_LOG("do sql [%s] success!!!", sql_str.sql_str[k]);
                http_return.err_no = 0;
                http_return.err_len = 0;
            } else {
                ERROR_LOG("do sql [%s] error!!!", sql_str.sql_str[k]);
                int e = mysql.m_errno();
                http_return.err_no = e==-1 ? RESULT_EMYSQL_CONNECT : e+RESULT_EMYSQL;
                SET_HTTP_RETURN(http_return.err_no, sql_buf, mysql.m_error());
                //print_http_return(&http_return);
                goto out;
            }
        }
        SET_HTTP_RETURN(http_return.err_no, sql_buf, "");
        //print_http_return(&http_return);
    }

out:
#if REAPIR
    mysql.query("REPAIR TABLE user");
    mysql.query("REPAIR TABLE db");
    mysql.query("REPAIR TABLE tables_priv");
    mysql.query("REPAIR TABLE columns_priv");
    mysql.query("FLUSH TABLE user");
    mysql.query("FLUSH TABLE db");
    mysql.query("FLUSH TABLE tables_priv");
    mysql.query("FLUSH TABLE columns_priv");
    INFO_LOG("REPAIR TABLE user");
    INFO_LOG("REPAIR TABLE db");
    INFO_LOG("REPAIR TABLE tables_priv");
    INFO_LOG("REPAIR TABLE columns_priv");
    INFO_LOG("FLUSH TABLE user");
    INFO_LOG("FLUSH TABLE db");
    INFO_LOG("FLUSH TABLE tables_priv");
    INFO_LOG("FLUSH TABLE columns_priv");
#endif
    head->result = RESULT_OK;
    head->pkg_len = sizeof(head_t) + save_len + sizeof(sql_count);
    *(uint16_t *)(buf + sizeof(head_t)) = sql_count;
    memcpy(buf + sizeof(head_t) + sizeof(sql_count), p_http_return, save_len);

    free(p_http_return);
    DEBUG_LOG("result :");
    print_head(head);
    print_http_return((uint8_t *)(buf + sizeof(head_t)));
}

void process_edit_priv(uint8_t * buf, uint32_t buf_len)
{
    int ret, n;
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));
    priv_t rpriv;
    uint16_t sql_count = 0;
    //bool dosql_error = false;
    http_return_t http_return;
    uint32_t save_len = 0;
    uint32_t left_len = buf_len - (sizeof(head_t) + sizeof(sql_count));
    uint32_t ret_len = 0;
    char sql_buf[MAX_SQL_BUF_LEN];
    uint8_t * p_http_return = (uint8_t *)malloc(left_len);

    edit_priv_t * edit_priv = (edit_priv_t *)(buf + sizeof(head_t));
    //total_priv_t * priv = (total_priv_t *)(edit_priv->host_ip + edit_priv->host_count);
    //total_priv_t * priv = (total_priv_t *)((uint8_t *)(edit_priv->host_ip) + sizeof(edit_priv->host_ip[0]) * edit_priv->host_count);
    print_edit_priv(edit_priv);
    //print_total_priv(priv);

    change_record_t change_record;
    sql_str_t sql_str;
    strcpy(change_record.user, edit_priv->user);
    strcpy(upriv.user, edit_priv->user);

    c_mysql_connect_auto_ptr mysql("localhost", edit_priv->dba.dba_name,
            (char *)decode((const uint8_t *)edit_priv->dba.dba_passwd, (uint8_t *)edit_priv->dba.dba_passwd, sizeof(edit_priv->dba.dba_passwd)),
            "mysql", edit_priv->head.sql_port, mysql_socket, CLIENT_INTERACTIVE);

    for(uint32_t j=0; j<edit_priv->host_count; j++) {
        //填change_record的各个字段
        change_record.serial_no1 = head->serial_no1;
        change_record.serial_no2 = edit_priv->priv[j].priv_no;
        http_return.value_id = edit_priv->priv[j].priv_no;
        change_record.sql_no = 1;//自增

        upriv.priv = edit_priv->priv[j].priv;
        change_record.new_priv = edit_priv->priv[j].priv;

        strcpy(upriv.db, edit_priv->priv[j].db_name);
        strcpy(change_record.db, edit_priv->priv[j].db_name);

        strcpy(upriv.table, edit_priv->priv[j].table_name);
        strcpy(change_record.table, edit_priv->priv[j].table_name);

        strcpy(upriv.field, edit_priv->priv[j].column_name);
        strcpy(change_record.column, edit_priv->priv[j].column_name);

        convert_ip(edit_priv->priv[j].host_ip, change_record.host);
        strcpy(upriv.host, change_record.host);
        //查找该权限:有则改，无则加
        if(edit_priv->priv[j].db_name[0] == '\0') { //全局权限
            ret = get_mysql_info(GET_GLOBAL_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，修改
                ERROR_LOG("exits priv : *.* for %s@%s", upriv.user, upriv.host);
                change_record.op_type = OP_CHANGE_PRIV;
            } else if(ret == 0) {
                DEBUG_LOG("create priv : *.* for %s@%s", upriv.user, upriv.host);
                //没找到，增加权限
                change_record.op_type = OP_ADD_PRIV;
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : *.* for %s@%s", ret, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EEDITPRIV_GFILE, sql_buf, "RESULT_EEDITPRIV_GFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else if(edit_priv->priv[j].table_name[0] == '\0') {  //库权限
            ret = get_mysql_info(GET_DATABASE_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，修改
                DEBUG_LOG("exits priv : %s.* for %s@%s", upriv.db, upriv.user, upriv.host);
                change_record.op_type = OP_CHANGE_PRIV;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : %s.* for %s@%s", upriv.db, upriv.user, upriv.host);
                change_record.op_type = OP_ADD_PRIV;
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.* for %s@%s", ret, upriv.db, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EEDITPRIV_DFILE, sql_buf, "RESULT_EEDITPRIV_DFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else if(edit_priv->priv[j].column_name[0] == '\0') { //表权限
            ret = get_mysql_info(GET_TABLE_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            ret = get_mysql_info(GET_TABLE_TC_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&change_record.old_tc_priv, sizeof(change_record.old_tc_priv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，修改
                DEBUG_LOG("exits priv : %s.%s for %s@%s", upriv.db, upriv.table, upriv.user, upriv.host);
                change_record.op_type = OP_CHANGE_PRIV;
            } else if(ret == 0) {
                //没找到，增加权限
                ret = get_mysql_info(GET_TABLE_TC_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&change_record.old_tc_priv, sizeof(change_record.old_tc_priv));
                if(ret == 1) {
                    DEBUG_LOG("exits priv : %s.%s for %s@%s", upriv.db, upriv.table, upriv.user, upriv.host);
                    change_record.op_type = OP_CHANGE_PRIV;
                } else {
                    DEBUG_LOG("create priv : %s.%s for %s@%s", upriv.db, upriv.table, upriv.user, upriv.host);
                    change_record.op_type = OP_ADD_PRIV;
                }
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.%s for %s@%s", ret, upriv.db, upriv.table, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EEDITPRIV_TFILE, sql_buf, "RESULT_EEDITPRIV_TFILE");
                //print_http_return(&http_return);
                goto out;
            }
        } else { //字段权限
            ret = get_mysql_info(GET_TABLE_TC_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&change_record.old_tc_priv, sizeof(change_record.old_tc_priv));
            ret = get_mysql_info(GET_FIELD_PRIV, edit_priv->head.sql_port, &upriv, (uint8_t *)&rpriv, sizeof(rpriv));
            if(ret == 1 || ret < -100) {
                //找到，权限已存在，修改
                DEBUG_LOG("exits priv : %s.%s.%s for %s@%s", upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
                change_record.new_tc_priv = get_new_field_priv(edit_priv->head.sql_port, upriv.db, upriv.table, upriv.field, upriv.user, upriv.host) | change_record.new_priv;
                change_record.op_type = OP_CHANGE_PRIV;
            } else if(ret == 0) {
                //没找到，增加权限
                DEBUG_LOG("create priv : %s.%s.%s for %s@%s", upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
                change_record.op_type = OP_ADD_PRIV;
            } else {
                //查找出错
                ERROR_LOG("find priv error[%d] : %s.%s.%s for %s@%s", ret, upriv.db, upriv.table, upriv.field, upriv.user, upriv.host);
                SET_HTTP_RETURN(RESULT_EEDITPRIV_CFILE, sql_buf, "RESULT_EEDITPRIV_CFILE");
                //print_http_return(&http_return);
                goto out;
            }
        }
        change_record.old_priv = rpriv.priv;
        DEBUG_LOG("db = %s table = %s column = %s", change_record.db, change_record.table, change_record.column);
        if((n = get_sql_str(&change_record, &sql_str)) < 0) {
            ERROR_LOG("RESULT_EGETSQL");
            SET_HTTP_RETURN(RESULT_EGETSQL, sql_buf, "RESULT_EGETSQL");
            //print_http_return(&http_return);
            goto out;
        }
        DEBUG_LOG("%d", n);
        sql_buf[0] = '\0';
        http_return.value_id = edit_priv->priv[j].priv_no;
        http_return.sql_id = n;
        for(int k=0; k<n; k++) {
            INFO_LOG("%u %u %u %s", head->serial_no1, edit_priv->priv[j].priv_no, k+1, sql_str.sql_str[k]);            
            sprintf(sql_buf, "%s%s;", sql_buf, sql_str.sql_str[k]);
            if( mysql.query(sql_str.sql_str[k]) == 0) {
                DEBUG_LOG("do sql [%s] success!!!", sql_str.sql_str[k]);
                http_return.err_no = 0;
                http_return.err_len = 0;
            } else {
                ERROR_LOG("do sql [%s] error!!!", sql_str.sql_str[k]);
                int e = mysql.m_errno();
                http_return.err_no = e==-1 ? RESULT_EMYSQL_CONNECT : e+RESULT_EMYSQL;
                SET_HTTP_RETURN(http_return.err_no, sql_buf, mysql.m_error());
                //print_http_return(&http_return);
                goto out;
            }
        }
        SET_HTTP_RETURN(http_return.err_no, sql_buf, "");
        //print_http_return(&http_return);
    }

out:
#if REAPIR
    mysql.query("REPAIR TABLE user");
    mysql.query("REPAIR TABLE db");
    mysql.query("REPAIR TABLE tables_priv");
    mysql.query("REPAIR TABLE columns_priv");
    mysql.query("FLUSH TABLE user");
    mysql.query("FLUSH TABLE db");
    mysql.query("FLUSH TABLE tables_priv");
    mysql.query("FLUSH TABLE columns_priv");
#endif

    head->result = RESULT_OK;
    head->pkg_len = sizeof(head_t) + save_len + sizeof(sql_count);
    *(uint16_t *)(buf + sizeof(head_t)) = sql_count;
    memcpy(buf + sizeof(head_t) + sizeof(sql_count), p_http_return, save_len);

    free(p_http_return);
    DEBUG_LOG("result :");
    print_head(head);
    print_http_return((uint8_t *)(buf + sizeof(head_t)));
}

void process_check_passwd(uint8_t * buf, uint32_t buf_len)
{
    int ret;
    char passwd[64];
    head_t * head = (head_t *)buf;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    check_passwd_t * check_passwd = (check_passwd_t*)(buf + sizeof(head_t));
    print_check_passwd(check_passwd);

    check_passwd_send_t * send = (check_passwd_send_t *)(buf + sizeof(head_t));

    strcpy(upriv.user, check_passwd->user);
    convert_ip(check_passwd->host_ip, upriv.host);

    ret = get_mysql_info(GET_USER_PWD, check_passwd->head.sql_port, &upriv, (uint8_t *)passwd, sizeof(passwd));
    if(ret == 1) {
        DEBUG_LOG("%s %lu", check_passwd->passwd, strlen(check_passwd->passwd));
        DEBUG_LOG("%s %lu", passwd, strlen(passwd));
        if(strcmp(passwd, check_passwd->passwd) == 0) {
            DEBUG_LOG("check passwd OK");
            send->result = 0;
        } else {
            DEBUG_LOG("check passwd failed");
            send->result = 1;
        }
    } else if(ret == 0) {
        send->result = 2;
    } else if(ret == -1) {
        ret = RESULT_ECHKPWD_FILE;
        goto error;
    } else if(ret < 100) {
        ret = RESULT_ECHKPWD_NOBUF;
        goto error;
    } else {
        ret = RESULT_ECHKPWD_MORE;
        goto error;
    }

    head->result = RESULT_OK;
    head->pkg_len = sizeof(head_t) + sizeof(check_passwd_send_t);
    DEBUG_LOG("return : ");
    print_head(head);
    print_check_passwd_send(send);
    return ;

error:
    head->result = ret;
    head->pkg_len = sizeof(head_t);
    DEBUG_LOG("result :");
    print_head(head);
}

#undef GLOBAL
#undef TOTAL
#undef EXACT
#undef TABLE
#undef COLUMN
#undef MAX_SQL_BUF_LEN
#undef REAPIR
