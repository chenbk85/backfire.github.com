#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "mysql_mgr.h"
#include "lib.h"

#define DEFAULT_CNF "/etc/mysql/my.cnf"
#define DEFAULT_PORT (3306)

#define OA_DB_NAME  "monitor"
#define OA_DB_PWD   "monitor@tmpwd"

int get_datadir_by_port(const char * file, int port, char * datadir, char * socket)
{
    if(port == 0) {
        port = DEFAULT_PORT;
    }
    FILE * fd = fopen(file[0] == 0 ? DEFAULT_CNF : file, "r");
    if(fd == 0) {
        ERROR_LOG("open %s failed %d:%s", file, errno, strerror(errno));
        return -1;
    } else {
        DEBUG_LOG("open %s", file[0] == 0 ? DEFAULT_CNF : file);
    }
    char buf[1024];
    int found = 0;
    while(fgets(buf, sizeof(buf), fd)) {
        trim(buf);
        if(buf[0] == '#') {
            continue;
        } else if(buf[0] == '[') {
            datadir[0] = 0;
            socket[0] = 0;
            found = 0;
        } else if(strncmp(buf, "port", 4) == 0) {
            trim(&buf[4]);
            if(buf[4] != '=') {
                goto error;
            } else {
                trim(&buf[5]);
                if(port == atoi(&buf[5])) {
                    DEBUG_LOG("port = %u", port);
                    if(found == 2) {
                        goto success;
                    } else {
                        found++;
                    }
                    //if(datadir[0] != 0) {
                    //    goto success;
                    //} else {
                    //    found = 1;
                    //}
                } else {
                    found = 0;
                }
            }
        } else if(strncmp(buf, "datadir", 7) == 0) {
            trim(&buf[7]);
            if(buf[7] != '=') {
                goto error;
            } else {
                trim(&buf[8]);
                strcpy(datadir, &buf[8]);
                DEBUG_LOG("datadir = %s", datadir);
                if(found == 2) {
                    goto success;
                } else {
                    found++;
                }
                //if(found) {
                //    goto success;
                //}
            }
        } else if(strncmp(buf, "socket", 6) == 0) {
            trim(&buf[6]);
            if(buf[6] != '=') {
                goto error;
            } else {
                trim(&buf[7]);
                strcpy(socket, &buf[7]);
                DEBUG_LOG("socket = %s", socket);
                if(found == 2) {
                    goto success;
                } else {
                    found++;
                }
                //if(found) {
                //    goto success;
                //}
            }
        }
    }
error:
    datadir[0] = 0;
    socket[0] = 0;
    fclose(fd);
    return -1;
success:
    fclose(fd);
    DEBUG_LOG("port = %u datadir = %s socket = %s", port, datadir, socket);
    return 0;
}

int get_mysql_info(int type, int port, user_priv_t * upriv, uint8_t * buf, uint32_t buf_len)
//int get_mysql_info(int type, const char * ip, int port, user_priv_t * upriv, uint8_t * buf, uint32_t buf_len)
{
    int r = 0, len = 0;
    priv_t * priv = (priv_t *)buf;
    name_t * name = (name_t *)buf;
    query_user_t * user = (query_user_t *)buf;
    memset(buf, 0, buf_len);
    char datadir[1024];
    if(get_datadir_by_port(DEFAULT_CNF, port, datadir, mysql_socket) != 0) {
        ERROR_LOG("can not get datadir by %s %d.", DEFAULT_CNF, port);
        char buf[1024];
        sprintf(buf, "/db_bak/%u/my.cnf", port);
        if(get_datadir_by_port(buf, port, datadir, mysql_socket) != 0) {
            ERROR_LOG("can not get datadir by %s %d.", buf, port);
            return -1;
        }
    }
    /*
    MYSQL * mysql = mysql_init(NULL);
    if(NULL == mysql_real_connect(mysql, ip, OA_DB_NAME, OA_DB_PWD, "mysql", port, NULL, CLIENT_INTERACTIVE)) {
		ERROR_LOG("mysql_real_connect(...) failed! %s.", mysql_error(mysql));
		ERROR_LOG("Database server:%s, port:%u, db name:%s, user:%s\n",	ip, port, "mysql", OA_DB_NAME);
		return -1;
	}
	my_bool is_auto_reconnect = 1;
	if(mysql_options(mysql, MYSQL_OPT_RECONNECT,&is_auto_reconnect))
	{
		ERROR_LOG("mysql_options(...) failed! %s.", mysql_error(mysql));
		r = -1;
		goto out;
	}
	if (mysql_set_character_set(mysql, "utf8")) 
	{
		ERROR_LOG("mysql_set_character_set(...) failed! %s.", mysql_error(mysql));
		r = -1;
		goto out;
	}
	*/
    switch(type) {
        case GET_USER_INFO:
            r = flush_table("user", port);
            if(r != 0) {
                ERROR_LOG("REPAIR TABLE user error");
            }
            len = buf_len/sizeof(query_user_t);
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "user");
            r = get_user_info(datadir, user, len, upriv);
            //r = _get_user_info(mysql, user, len, upriv);
#if 0
            if(r <= len && r > 0) {
                char ip[16];
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s@%s", user[i].name, user[i].host_ip);
                }
                //memcpy(buf, user, sizeof(query_user_t)*r);
            }
#endif
            break;
        case GET_USER_PWD:
            r = flush_table("user", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE user error");
            }
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "user");
            r = get_user_pwd(datadir, (char*)buf, upriv);
            //r = _get_user_pwd(mysql, (char*)buf, upriv);
            break;
        case GET_GLOBAL_PRIV:
            r = flush_table("user", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE user error");
            }
            len = buf_len/sizeof(priv_t);
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "user");
            r = get_global_priv(datadir, priv, len, upriv);
            //r = _get_global_priv(mysql, priv, len, upriv);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s@%s : ", priv[i].user_name, priv[i].host_ip);
                    print_global_priv(priv[i].priv);
                    DEBUG_LOG("");
                }
                //memcpy(buf, priv, sizeof(user_priv_t)*r);
            }
#endif
            break;
        case GET_DATABASE_INFO:
            len = buf_len/sizeof(name_t);
            r = get_database_info(datadir, name, len);
            //r = _get_database_info(mysql, name, len);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s", name[i].name);
                }
                //memcpy(buf, name, sizeof(name_t)*r);
            }
#endif
            break;
        case GET_DATABASE_PRIV:
            r = flush_table("db", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE db error");
            }
            len = buf_len/sizeof(priv_t);
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "db");
            r = get_database_priv(datadir, priv, len, upriv);
            //r = _get_database_priv(mysql, priv, len, upriv);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s@%s %s.* : ", priv[i].user_name, priv[i].host_ip, priv[i].db_name);
                    print_database_priv(priv[i].priv);
                }
                //memcpy(buf, priv, sizeof(user_priv_t)*r);
            }
#endif
            break;
        case GET_TABLE_INFO:            
            if(upriv->db[0] == '\0') {
                DEBUG_LOG("which database ?");
                r = -1;
                break;
            }
            len = buf_len/sizeof(name_t);
            sprintf(datadir, "%s/%s", datadir, upriv->db);
            r = get_table_info(datadir, name, len);
            //r = _get_table_info(mysql, name, len);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s", name[i].name);
                }
                //memcpy(buf, name, sizeof(name_t)*r);
            }
#endif
            break;
        case GET_TABLE_PRIV:
            r = flush_table("tables_priv", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE tables_priv error");
            }
            len = buf_len/sizeof(priv_t);
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "tables_priv");
            r = get_table_priv(datadir, priv, len, upriv);
            //r = _get_table_priv(mysql, priv, len, upriv);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s@%s %s.%s : ", priv[i].user_name, priv[i].host_ip, priv[i].db_name, priv[i].table_name);
                    print_table_priv(priv[i].priv);
                }
                //memcpy(buf, priv, sizeof(user_priv_t)*r);
            }
#endif
            break;
        case GET_TABLE_TC_PRIV:
            r = flush_table("tables_priv", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE tables_priv error");
            }
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "tables_priv");
            r = get_table_tc_priv(datadir, (uint32_t *)buf, upriv);
            //r = _get_table_tc_priv(mysql, (uint32_t *)buf, upriv);
            break;
        case GET_FIELD_INFO:
            if(upriv->db[0] == '\0') {
                DEBUG_LOG("which database ?");
                r = -1;
                break;
            }
            if(upriv->table[0] == '\0') {
                DEBUG_LOG("which table ?");
                r = -1;
                break;
            }
            len = buf_len/sizeof(name_t);
            sprintf(datadir, "%s/%s/%s.frm", datadir, upriv->db, upriv->table);
            r = get_field_info(datadir, name, len);
            //r = _get_field_info(mysql, name, len);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s", name[i].name);
                }
                //memcpy(buf, name, sizeof(name_t)*r);
            }
#endif
            break;
        case GET_FIELD_PRIV:
            r = flush_table("columns_priv", port);
            if(r != 0) {
                ERROR_LOG("FLUSH TABLE columns_priv error");
            }
            len = buf_len/sizeof(priv_t);
            sprintf(datadir, "%s/%s/%s.MYD", datadir, "mysql", "columns_priv");
            r = get_field_priv(datadir, priv, len, upriv);
            //r = _get_field_priv(mysql, priv, len, upriv);
#if 0
            if(r <= len && r > 0) {
                for(int i=0; i<r; i++) {
                    DEBUG_LOG("%s@%s %s.%s.%s : ", priv[i].user_name, priv[i].host_ip, priv[i].db_name, priv[i].table_name, priv[i].column_name);
                    print_field_priv(priv[i].priv);
                }
                //memcpy(buf, priv, sizeof(user_priv_t)*r);
            }
#endif
            break;
        default:
            ERROR_LOG("unknown type %d.", type);
            r = -1;
            break;
    }
    //mysql_close(mysql);
    return r;
}

void print_head(head_t * head)
{
    DEBUG_LOG("pkg_len = %u", head->pkg_len);
    DEBUG_LOG("version = %u", head->version);
    DEBUG_LOG("cmd_id = 0x%08X", head->cmd_id);
    DEBUG_LOG("result = %u", head->result);
    DEBUG_LOG("veri_code = %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                head->veri_code[0], head->veri_code[1], head->veri_code[2], head->veri_code[3],
                head->veri_code[4], head->veri_code[5], head->veri_code[6], head->veri_code[7],
                head->veri_code[8], head->veri_code[9], head->veri_code[10], head->veri_code[11],
                head->veri_code[12], head->veri_code[13], head->veri_code[14], head->veri_code[15],
                head->veri_code[16], head->veri_code[17], head->veri_code[18], head->veri_code[19],
                head->veri_code[20], head->veri_code[21], head->veri_code[22], head->veri_code[23],
                head->veri_code[24], head->veri_code[25], head->veri_code[26], head->veri_code[27],
                head->veri_code[28], head->veri_code[29], head->veri_code[30], head->veri_code[31]);
    DEBUG_LOG("timestamp = %u", head->timestamp);
    DEBUG_LOG("serial_no1 = %u", head->serial_no1);
    DEBUG_LOG("serial_no2 = %u", head->serial_no2);
}

void print_query_recv(query_recv_t * query)
{
    char ip[16];
    DEBUG_LOG("sql_ip = %u", query-> sql_ip);
    DEBUG_LOG("sql_port = %u", query-> sql_port);
    DEBUG_LOG("level = %u", query-> level);
    DEBUG_LOG("user_name = %s", query-> user_name);
    DEBUG_LOG("host_ip = %s", convert_ip(query-> host_ip, ip));
    DEBUG_LOG("db_name = %s", query-> db_name);
    DEBUG_LOG("table_name = %s", query-> table_name);
    DEBUG_LOG("column_name = %s", query-> column_name);
}

void print_query_info_send(query_info_send_t * query) {
    DEBUG_LOG("count = %u", query->count);
    for(uint32_t i=0; i<query->count;i++) {
        DEBUG_LOG("  name[%u] = %s", i, query->name[i].name);
    }
}

void print_query_priv_send(query_priv_send_t * query) {
    DEBUG_LOG("count = %u", query->count);
    for(uint32_t i=0; i<query->count; i++) {
        DEBUG_LOG("  user_name = %s", query->priv[i].user_name);
        DEBUG_LOG("  host_ip[%u] = %s", i, query->priv[i].host_ip);
        DEBUG_LOG("  db_name = %s", query->priv[i].db_name);
        DEBUG_LOG("  table_name = %s", query->priv[i].table_name);
        DEBUG_LOG("  column_name = %s", query->priv[i].column_name);
        DEBUG_LOG("  priv[%u] = 0x%08X", i, query->priv[i].priv);
    }
}

void print_query_user_info(query_user_info_t * query)
{
    char ip[16];
    DEBUG_LOG("sql_ip = %s", convert_ip(query->sql_ip, ip));
    DEBUG_LOG("sql_port = %u", query->sql_port);
}

void print_body_head(body_head_t * body)
{
    char ip[16];
    DEBUG_LOG("sql_ip = %s", convert_ip(body->sql_ip, ip));
    DEBUG_LOG("sql_port = %u", body->sql_port);
    DEBUG_LOG("do_sql = %u", body->do_sql);
}

void print_user_t(user_t * user)
{
    char ip[16];
    DEBUG_LOG("user_name = %s", user->name);
    DEBUG_LOG("host_ip = %s %u", convert_ip(user->host_ip, ip), user->host_ip);
}

void print_check_user_t(check_user_t * user)
{
    print_body_head(&user->head);
    print_user_t(&user->user);
}

void print_chech_user_send(check_user_send_t * send)
{
    DEBUG_LOG("exists = %u", send->exists);
}

void print_query_user_send(query_user_send_t * send)
{
    DEBUG_LOG("count = %u", send->count);
    for(uint32_t i=0; i<send->count; i++) {
        DEBUG_LOG("  user_name = %s", send->user[i].name);
        DEBUG_LOG("  host_ip = %s", send->user[i].host_ip);
    }
}

void print_dba(dba_t * dba)
{
    DEBUG_LOG("dba_name = %s", dba->dba_name);
    DEBUG_LOG("dba_passwd = XXXXXX");
}

void print_add_user(add_user_t * add_user)
{
    char ip[16];
    print_body_head(&add_user->head);
    print_dba(&add_user->dba);
    DEBUG_LOG("new_user = %s", add_user->new_user);
    DEBUG_LOG("new_passwd = %s", add_user->new_passwd);
    DEBUG_LOG("host_count = %u", add_user->host_count);
    for(uint8_t i=0; i<add_user->host_count; i++) {
        DEBUG_LOG("  add_no[%u] = %u", i, add_user->host[i].add_no);
        DEBUG_LOG("  host_ip[%u] = %s %u", i, convert_ip(add_user->host[i].host_ip, ip), add_user->host[i].host_ip);
    }
}

void print_del_user(del_user_t * del_user)
{
    char ip[16];
    print_body_head(&del_user->head);
    print_dba(&del_user->dba);
    DEBUG_LOG("del_count = %u", del_user->del_count);
    for(uint8_t i=0; i<del_user->del_count; i++) {
        DEBUG_LOG("  del_no[%u] = %u", i, del_user->del_user[i].del_no);
        DEBUG_LOG("  del_name[%u] = %s", i, del_user->del_user[i].name);
        DEBUG_LOG("  del_host[%u] = %s", i, convert_ip(del_user->del_user[i].host_ip, ip));
    }
}

void print_edit_priv(edit_priv_t * edit)
{
    char ip[16];
    print_body_head(&edit->head);
    print_dba(&edit->dba);
    DEBUG_LOG("user = %s", edit->user);
    DEBUG_LOG("host_count = %u", edit->host_count);
    for(uint32_t i=0; i<edit->host_count; i++) {
        DEBUG_LOG("  priv_no = %u", edit->priv[i].priv_no);
        DEBUG_LOG("  host_ip = %s", convert_ip(edit->priv[i].host_ip, ip));
        DEBUG_LOG("  priv = 0x%08X", edit->priv[i].priv);
        DEBUG_LOG("  db_name = %s", edit->priv[i].db_name);
        DEBUG_LOG("  table_name = %s", edit->priv[i].table_name);
        DEBUG_LOG("  column_name = %s", edit->priv[i].column_name);
    }
}

void print_check_passwd(check_passwd_t * check)
{
    char ip[16];
    print_body_head(&check->head);
    DEBUG_LOG("user = %s", check->user);
    DEBUG_LOG("host_ip = %s", convert_ip(check->host_ip, ip));
    DEBUG_LOG("passwd = %s", check->passwd);
}


void print_check_passwd_send(check_passwd_send_t * send)
{
    DEBUG_LOG("result = %u", send->result);
}

void print_http_return(uint8_t * buf)//http_return_t * http)
{
    uint16_t count = *((uint16_t *)(buf));
    uint32_t len = sizeof(count);
    http_return_t * http;
    DEBUG_LOG("sql_count = %u", count);
    for(uint16_t i=0; i<count; i++) {
        http = (http_return_t *)(buf + len);
        DEBUG_LOG("value_id = %u", http->value_id);
        DEBUG_LOG("sql_id = %u", http->sql_id);
        DEBUG_LOG("err_no = %u", http->err_no);
        DEBUG_LOG("sql_len = %u", http->sql_len);
        DEBUG_LOG("err_len = %u", http->err_len);
        DEBUG_LOG("sql = %s", http->sql_len != 0 ? http->str : "");
        DEBUG_LOG("err = %s", http->err_len != 0 ? &http->str[http->sql_len] : "");
        len += sizeof(http_return_t) - sizeof(http->str) + http->sql_len + http->err_len;
    }
}

uint32_t ip_to_num(const char * ip)
{
    if(strcmp(ip, "localhost") == 0) {
        return 0;
    } else if(strcmp(ip, "%") == 0){
        return ~0;
    } else {
        char cip[16];
        strcpy(cip, ip);
        uint8_t aip[4];
        int index = 1;
        aip[0] = 0;
        for(uint32_t i=0; i<sizeof(cip); i++) {
            if(cip[i] == '.') {
                aip[index++] = i+1;
                cip[i] = 0;
            }
        }
        for(uint32_t i=0; i<sizeof(aip)/sizeof(aip[0]); i++) {
            if(cip[aip[i]] == '%') {
                aip[i] = 255;
            } else {
                aip[i] = atoi(&cip[aip[i]]);
            }
        }
        return (aip[3]<<24)+(aip[2]<<16)+(aip[1]<<8)+aip[0];
    }
}

char * utoa(uint8_t num, char * buf)
{
    sprintf(buf, "%u", num);
    return buf;
}

char * convert_ip(uint32_t ip, char * buf)
{
    if(ip == 0) {
        strcpy(buf, "localhost");
        return buf;
    } else if(ip == uint32_t(~0)) {
        strcpy(buf, "%");
        return buf;
    }
    uint32_t ip_seg[4];
    uint32_t mask = 0xFF;
    for(int i = 0; i < 4; i++) {
        ip_seg[i] = ip & mask;
        ip = ip >> 8;
    }
    char cip[4][4];
    sprintf(buf, "%s.%s.%s.%s",
            ip_seg[3] == 255 ? "%" : utoa(ip_seg[3], cip[3]),
            ip_seg[2] == 255 ? "%" : utoa(ip_seg[2], cip[2]),
            ip_seg[1] == 255 ? "%" : utoa(ip_seg[1], cip[1]),
            ip_seg[0] == 255 ? "%" : utoa(ip_seg[0], cip[0]));
    return buf;
}

int get_sql_str(change_record_t * change_record, sql_str_t * sql_str)
{
    int n = 0;
    char buf[OA_MAX_SQL_LEN];
    uint32_t a, b, c, d;
    switch(change_record->op_type) {
    case OP_CREATE_USER:
        DEBUG_LOG("add user %s.", change_record->new_passwd);
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                "GRANT USAGE ON *.* TO '%s'@'%s' IDENTIFIED BY PASSWORD '%s'",
                change_record->user, change_record->host, change_record->new_passwd);
        n++;
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
        n++;
            break;
    case OP_CHANGE_PWD:
        DEBUG_LOG("change pwd.");
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                "SET PASSWORD FOR %s@%s = '%s'",
                change_record->user, change_record->host, change_record->new_passwd);
        n++;
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
        n++;
            break;
    case OP_DELETE_USER:
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                "DROP USER %s@%s",
                change_record->user, change_record->host);
        n++;
        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
        n++;
            break;
    case OP_ADD_PRIV_CR:
        DEBUG_LOG("db = %s table = %s column = %s", change_record->db, change_record->table, change_record->column);
        if(change_record->db[0] == '\0') { //全局权限
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "GRANT %s ON *.* TO %s@%s%s",
                        get_global_priv_grant(0, change_record->new_priv, buf),
                        change_record->user, change_record->host,
                        (change_record->new_priv & (1<<GRANT_GLO_INDEX)) == 0 ? "" : " WITH GRANT OPTION");
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else if(change_record->table[0] == '\0') { //库权限
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "GRANT %s ON %s.* TO %s@%s%s",
                        get_database_priv_grant(0, change_record->new_priv, buf),
                        change_record->db, change_record->user, change_record->host,
                        (change_record->new_priv & (1<<GRANT_DB_INDEX)) == 0 ? "" : " WITH GRANT OPTION");
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else if(change_record->column[0] == '\0') { //表权限
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "GRANT %s ON %s.%s TO %s@%s",
                        get_table_priv_grant(0, change_record->new_priv, buf),
                        change_record->db, change_record->table, change_record->user, change_record->host);
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else {
            if(change_record->old_priv != change_record->new_priv) { //字段权限
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "GRANT %s ON %s.%s TO %s@%s",
                        get_field_priv_grant(0, change_record->new_priv, buf, change_record->column),
                        change_record->db, change_record->table, change_record->user, change_record->host);
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        }
        break;
    case OP_ADD_PRIV:
        if(change_record->db[0] == '\0') {//全局权限没有增加的协议
            break;
        } else if(change_record->table[0] == '\0') {//库权限
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "INSERT INTO mysql.db VALUES ('%s','%s','%s',%s)",
                        change_record->host, change_record->db, change_record->user,
                        get_database_priv_insert(0, change_record->new_priv, buf));
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else if(change_record->column[0] == '\0') {//表权限
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "INSERT INTO mysql.tables_priv VALUES ('%s','%s','%s','%s',default,now(),'%s','')",
                        change_record->host, change_record->db, change_record->user, change_record->table,
                        get_table_priv_insert(0, change_record->new_priv, buf));
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else {//字段权限
            if(change_record->old_priv != change_record->new_priv) {
                if(change_record->old_tc_priv == uint32_t(~0)) {  //tables_priv中无此记录，则insert
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "INSERT INTO mysql.tables_priv VALUES ('%s','%s','%s','%s',default,now(),'','%s')",
                            change_record->host, change_record->db, change_record->user, change_record->table,
                            get_field_priv_insert(0, change_record->new_priv, buf));
                    n++;
                } else {    //有此记录，则update
                    if(change_record->old_tc_priv != (change_record->old_tc_priv | change_record->new_priv)) {
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "UPDATE mysql.tables_priv SET Column_priv='%s' where Host='%s' and Db='%s' and User='%s' and Table_name='%s'",
                                get_field_priv_update(change_record->old_tc_priv, change_record->old_tc_priv | change_record->new_priv, buf),
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                    }
                }
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "INSERT INTO mysql.columns_priv VALUES ('%s','%s','%s','%s','%s',now(),'%s')",
                        change_record->host, change_record->db, change_record->user, change_record->table, change_record->column,
                        get_field_priv_insert(0, change_record->new_priv, buf));
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        }
        break;
    case OP_CHANGE_PRIV:
        if(change_record->db[0] == '\0') {//全局
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "UPDATE mysql.user SET %s WHERE user='%s' and host='%s'",
                        get_global_priv_update(change_record->old_priv, change_record->new_priv, buf),
                        change_record->user, change_record->host);
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else if(change_record->table[0] == '\0') {//库
            if(change_record->old_priv != change_record->new_priv) {
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                        "UPDATE mysql.db SET %s WHERE Db='%s' and user='%s' and host='%s'",
                        get_database_priv_update(change_record->old_priv, change_record->new_priv, buf),
                        change_record->db, change_record->user, change_record->host);
                n++;
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        } else if(change_record->column[0] == '\0') {//表
            if(change_record->old_priv != change_record->new_priv) {
                a = change_record->old_priv & 0x87;
                b = change_record->new_priv & 0x87;
                c = (a ^ b) & (~b);
                d = change_record->old_tc_priv;
                c = (c >= 0x80 ? ((c & 0xF) | 0x8) : c);
                d = d & (~c);
                if(change_record->new_priv == 0 && d == 0) {//删除此条记录
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "DELETE FROM mysql.tables_priv WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s'",
                            change_record->host, change_record->db, change_record->user, change_record->table);
                    n++;
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "DELETE FROM mysql.columns_priv WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s'",
                            change_record->host, change_record->db, change_record->user, change_record->table);
                    n++;
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                    n++;
                } else {
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "UPDATE mysql.tables_priv SET Table_priv='%s' WHERE Host='%s' and Db ='%s' and User='%s' and Table_name='%s'",
                            get_table_priv_update(change_record->old_priv, change_record->new_priv, buf),
                            change_record->host, change_record->db, change_record->user, change_record->table);
                    n++;
                    if(c != 0 && d != change_record->old_tc_priv) { //删除字段权限
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "UPDATE mysql.tables_priv SET Column_priv='%s' WHERE Host='%s' and Db ='%s' and User='%s' and Table_name='%s'",
                                get_field_priv_update(change_record->old_tc_priv, d, buf),
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "UPDATE mysql.columns_priv SET Column_priv=Column_priv&(~%d) WHERE Host='%s' and Db ='%s' and User='%s' and Table_name='%s'",
                                c, change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "DELETE FROM mysql.columns_priv WHERE Host='%s' and Db ='%s' and User='%s' and Table_name='%s' and Column_priv=0",
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                    }
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                    n++;
                }
            }
        } else { //字段
            if(change_record->old_priv != change_record->new_priv) {
                if(change_record->new_priv == 0) { //删除
                    if(change_record->old_tc_priv == uint32_t(~0)) {
                        DEBUG_LOG("delete columns_priv.Column_priv and not find record in tables_priv");
                        return -1;
                    }
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "DELETE FROM mysql.columns_priv WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s' and Column_name='%s'",
                            change_record->host, change_record->db, change_record->user, change_record->table, change_record->column);
                    n++;
                    if(change_record->new_tc_priv != change_record->old_tc_priv) {
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "UPDATE mysql.tables_priv SET Column_priv='%s' WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s'",
                                get_field_priv_update(change_record->old_tc_priv, change_record->new_tc_priv, buf),
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                    }
                    if(change_record->new_tc_priv == 0) {
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "DELETE FROM mysql.tables_priv WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s' and Table_priv=0 and Column_priv=0",
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                    }
                } else { //修改
                    if(change_record->old_tc_priv == uint32_t(~0)) {  //tables_priv中无此记录，则insert
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "INSERT INTO mysql.tables_priv VALUES ('%s','%s','%s','%s',default,now(),'','%s')",
                                change_record->host, change_record->db, change_record->user, change_record->table,
                                get_field_priv_update(change_record->old_priv, change_record->new_priv, buf));
                        n++;
                    } else {    //有此记录，则update
                        snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                                "UPDATE mysql.tables_priv SET Column_priv='%s' WHERE Host='%s' and Db='%s' and User='%s' and Table_name='%s'",
                                get_field_priv_update(change_record->old_tc_priv, change_record->old_tc_priv | change_record->new_priv, buf),
                                change_record->host, change_record->db, change_record->user, change_record->table);
                        n++;
                    }
                    snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]),
                            "UPDATE mysql.columns_priv SET Column_priv='%s' WHERE Host='%s' and Db ='%s' and User='%s' and Table_name='%s' and Column_name='%s'",
                            get_field_priv_update(change_record->old_priv, change_record->new_priv, buf),
                            change_record->host, change_record->db, change_record->user, change_record->table, change_record->column);
                    n++;
                }
                snprintf(sql_str->sql_str[n], sizeof(sql_str->sql_str[n]), "FLUSH PRIVILEGES");
                n++;
            }
        }
        break;
    //case OP_DELETE_PRIV:    //删除字段级权限？？
    //    break;
    default:
        n = -1;
        break;
    }
    return n;
}

uint32_t get_new_field_priv(int port, const char * db, const char * table, const char * column, const char * user, const char * host)
{
    uint32_t ret = 0;
    user_priv_t upriv;
    memset(&upriv, 0, sizeof(upriv));

    strcpy(upriv.user, user);
    strcpy(upriv.host, host);
    strcpy(upriv.db, db);
    strcpy(upriv.table, table);

    priv_t priv[4096];
    int r = get_mysql_info(GET_FIELD_PRIV, port, &upriv, (uint8_t *)priv, sizeof(priv));
    if(r < 0) {
        return 0;
    } else {
        for(int i=0; i<r; i++) {
            if(strcmp(column, priv[i].column_name) != 0) {
                ret |= priv[i].priv;
            }
        }
    }
    return ret;
}

uint32_t put_into_buf(uint8_t * buf, http_return_t * http_return, uint32_t left_len)
{
    if(left_len == 0) {
        return 0;
    }
    uint32_t r = sizeof(http_return_t) - sizeof(http_return->str) + http_return->sql_len + http_return->err_len;
    r = r > left_len ? left_len : r;
    memcpy(buf, http_return, r);
    return r;
}

http_return_t * put_into_http_return(http_return_t * http_return, const char * sql, const char * err)
{
    if(http_return == NULL) {
        return NULL;
    }
    if(sql == NULL || strlen(sql) == 0) {
        http_return->sql_len = 0;
    } else {
        http_return->sql_len = strlen(sql) + 1;
        strcpy(http_return->str, sql);
    }
    if(err == NULL || strlen(err) == 0) {
        http_return->err_len = 0;
    } else {
        http_return->err_len = strlen(err) + 1;
        strcpy(&http_return->str[http_return->sql_len], err);
    }
    return http_return;
}
