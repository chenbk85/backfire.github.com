#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mysql_mgr.h"

static char priv_str_i[][32] = {"Select_priv", "Insert_priv", "Update_priv", "Delete_priv", "Create_priv", "Drop_priv", "Reload_priv", "Shutdown_priv", "Process_priv", "File_priv", "Grant_priv", "References_priv", "Index_priv", "Alter_priv", "Show_db_priv", "Super_priv", "Create_tmp_table_priv", "Lock_tables_priv", "Execute_priv", "Repl_slave_priv", "Repl_client_priv", "Create_view_priv", "Show_view_priv", "Create_routine_priv", "Alter_routine_priv", "Create_user_priv"};
static char priv_str_g[][32] = {"Select", "Insert", "Update", "Delete", "Create", "Drop", "Reload", "Shutdown", "Process", "File", "Grant", "References", "Index", "Alter", "Show databases", "Super", "Create temporary tables", "Lock tables", "Execute", "Replication slave", "Replication client", "Create view", "Show view", "Create routine", "Alter routine", "Create user"};
static int  priv_count = sizeof(priv_str_g)/sizeof(priv_str_g[0]);

int get_user_info(const char * file, query_user_t * userp, int len, const user_priv_t * c_user) {
    FILE * fd = fopen(file, "rb");
    //FILE * fd = fopen("/home/ping/user.MYD", "rb");
    if(fd == NULL) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -1;
    } else {
        DEBUG_LOG("open %s", file);
    }
    int n = 0;
    uint16_t head;//must be 0x0003 or 0x0001
    uint8_t  rec_len;
    uint8_t  tail_len;
    uint16_t skip;//must be 0x01FF
    uint8_t  host_len;
    char     host[64];
    uint8_t  user_len;
    char     user[64];
    //uint8_t  pwd_len;
    //char     pwd[64];
    //uint8_t  priv[26];
    //uint8_t  ssl;
    //uint8_t  tail[64];
    char     buf[256];
    //int i;
    while(fread(&head, sizeof(head), 1, fd) == 1) {
        if(head == 0x0000) { //skip delete info
            READ_NUM(rec_len, fd);
            READ_NUM(tail_len, fd);
            READ_STR(buf, tail_len-4, fd);
            continue;
        }
        if(head != 0x0003 && head != 0x0001) {
            ERROR_LOG("%d head[0x%04X] != 0x0003 or 0x0001.", n, head);
            break;
        }
        READ_NUM(rec_len, fd);
        if(head == 0x0003) {
            READ_NUM(tail_len, fd);
        } else if(head == 0x0001) {
            tail_len = 0;
        } else {
            READ_NUM(tail_len, fd);
            READ_STR(buf, tail_len+rec_len, fd);
            continue;
        }
        READ_NUM(skip, fd);
        if(skip != 0x01FF && skip != 0x03FB) {
            ERROR_LOG("%d skip[0x%04X] != 0x01FF or 0x03FB.", n, skip);
            //continue;
        }
        READ_NUM(host_len, fd);
        READ_STR(host, host_len, fd);
        READ_NUM(user_len, fd);
        READ_STR(user, user_len, fd);
        host[host_len] = '\0';
        user[user_len] = '\0';
        if(c_user != NULL
                && c_user->user[0] != '\0'
                && strcmp(c_user->user, user) == 0
                && c_user->host[0] != '\0'
                && strcmp(c_user->host, host) == 0) { //找到特定用户
            strcpy(userp[n].host_ip, host);
            strcpy(userp[n].name, user);
            n++;
            DEBUG_LOG("user=%s,host_ip=%s", user, host);
            break;
        } else if(c_user == NULL
                || c_user->user[0] == '\0'
                || c_user->host[0] == '\0') {
            if(n >= len) {
                n = -n-100;
                break;
            }
            strcpy(userp[n].host_ip, host);
            strcpy(userp[n].name, user);
            DEBUG_LOG("user=%s,host_ip=%s", user, host);
            n++;
            READ_STR(buf, rec_len - 4 - host_len - user_len + tail_len, fd);
        } else {
            READ_STR(buf, rec_len - 4 - host_len - user_len + tail_len, fd);
        }
    }
    fclose(fd);
    return n;
}

int get_user_pwd(const char * file, char * pwd, const user_priv_t * c_user) {
    if(c_user == NULL || c_user->user[0] == '\0' || c_user->host[0] == '\0') {
        return -1;
    }
    FILE * fd = fopen(file, "rb");
    if(fd == NULL) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -1;
    } else {
        DEBUG_LOG("open %s", file);
    }
    int n = 0;
    uint16_t head;//must be 0x0003 or 0x0001
    uint8_t  rec_len;
    uint8_t  tail_len;
    uint16_t skip;//must be 0x01FF
    uint8_t  host_len;
    char     host[64];
    uint8_t  user_len;
    char     user[64];
    uint8_t  pwd_len;
    //char     pwd[64];
    //uint8_t  priv[26];
    //uint8_t  ssl;
    //uint8_t  tail[64];
    char     buf[256];
    //int i;
    while(fread(&head, sizeof(head), 1, fd) == 1) {
        if(head == 0x0000) {
            READ_NUM(rec_len, fd);
            READ_NUM(tail_len, fd);
            READ_STR(buf, tail_len-4, fd);
            DEBUG_LOG("head == 0x0000 skip %02X bytes.", tail_len);
            continue;
        }
        if(head != 0x0003 && head != 0x0001) {
            ERROR_LOG("%d head[0x%04X] != 0x0003 or 0x0001.", n, head);
            break;
        }
        READ_NUM(rec_len, fd);
        if(head == 0x0003) {
            READ_NUM(tail_len, fd);
        } else {
            tail_len = 0;
        }
        READ_NUM(skip, fd);
        if(skip != 0x01FF && skip != 0x03FB) {
            ERROR_LOG("%d skip[0x%04X] != 0x01FF or 0x03FB.", n, skip);
            //continue;
        }
        READ_NUM(host_len, fd);
        READ_STR(host, host_len, fd);
        READ_NUM(user_len, fd);
        READ_STR(user, user_len, fd);
        host[host_len] = '\0';
        user[user_len] = '\0';
        if(strcmp(c_user->user, user) == 0
                && strcmp(c_user->host, host) == 0) { //找到特定用户
            n++;
            READ_NUM(pwd_len, fd);
            if(pwd_len == 0) {
                //pwd_len = 0;
            } else if(pwd_len == 41) {
                READ_STR(pwd, pwd_len, fd);
            } else {
                pwd[0] = pwd_len;
                READ_STR(&pwd[1], 40, fd);
                pwd_len = 41;
            }
            pwd[pwd_len] = '\0';
            break;
        }
        READ_STR(buf, rec_len - 4 - host_len - user_len + tail_len, fd);
    }
    fclose(fd);
    return n;
}

int get_global_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv)
{
    FILE * fd = fopen(file, "rb");
    if(fd == NULL) {
        DEBUG_LOG("open %s error %u, %s", file, errno, strerror(errno));
        return -1;
    } else {
        DEBUG_LOG("open %s", file);
    }
    int n = 0;
    uint16_t head;//must be 0x0003
    uint8_t  rec_len;
    uint8_t  tail_len;
    uint16_t skip;//must be 0x01FF
    uint8_t  host_len;
    char     host[64];
    uint8_t  user_len;
    char     user[64];
    uint8_t  pwd_len;
    char     pwd[64];
    uint8_t  privs[26];
    uint8_t  ssl;
    uint8_t  tail[64];
    char     buf[256];
    uint8_t  empty_privs[26];
    memset(empty_privs, 0, sizeof(empty_privs));
    while(fread(&head, sizeof(head), 1, fd) == 1) {
        if(head == 0x0000) {
            READ_NUM(rec_len, fd);
            READ_NUM(tail_len, fd);
            READ_STR(buf, tail_len-4, fd);
            DEBUG_LOG("head == 0x0000 skip %02X bytes.", tail_len);
            continue;
        }
        if(head != 0x0003 && head != 0x0001) {
            ERROR_LOG("%d head[0x%04X] != 0x0003 or 0x0001.", n, head);
            break;
        }
        READ_NUM(rec_len, fd);
        if(head == 0x0003) {
            READ_NUM(tail_len, fd);
        } else {
            tail_len = 0;
        }
        READ_NUM(skip, fd);
        if(skip != 0x01FF && skip != 0x03FB) {
            ERROR_LOG("%d skip[0x%04X] != 0x01FF or 0x03FB.", n, head);
            //continue;
        }
        READ_NUM(host_len, fd);
        READ_STR(host, host_len, fd);
        READ_NUM(user_len, fd);
        READ_STR(user, user_len, fd);
        host[host_len] = '\0';
        user[user_len] = '\0';
        if((upriv->user[0] != '\0' && upriv->host[0] != 0)
                && (strcmp(upriv->user, user) != 0 || strcmp(upriv->host, host) != 0)) {
            READ_STR(buf, rec_len - 4 - host_len - user_len + tail_len, fd);
        } else {
            READ_NUM(pwd_len, fd);
            if(pwd_len == 0) {
            } if(pwd_len == 41) {
                READ_STR(pwd, pwd_len, fd);
            } else {
                READ_STR(pwd, 40, fd);
            }
            READ_STR(privs, sizeof(privs), fd);
            READ_NUM(ssl, fd);
            READ_STR(tail, tail_len, fd);
            if(memcmp(empty_privs, privs, sizeof(empty_privs)) != 0) {
                if(n >= len) {
                    n = -n-100;
                    break;
                }
                priv[n].priv = 0;
                for(uint32_t i=0; i<sizeof(privs); i++) {
                    if(privs[i] == 2) {
                        priv[n].priv |= (1<<i);
                    }
                }
                if(priv[n].priv != 0) {
                    strcpy(priv[n].host_ip, host);
                    strcpy(priv[n].user_name, user);
                    strcpy(priv[n].db_name, "");
                    strcpy(priv[n].table_name, "");
                    strcpy(priv[n].column_name, "");
                    n++;
                }
            }
        }
    }
    fclose(fd);
    return n;
}

void print_global_priv(int priv)
{
    for(int i=0; i<priv_count; i++) {
        if(priv & (1<<i)) {
            DEBUG_LOG("%s;", priv_str_g[i]);
        }
    }
}

char * get_global_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        if(i != GRANT_GLO_INDEX) {
            if(new_priv&(1<<i)) {
                sprintf(buf, "%s%s,", buf, priv_str_g[i]);
            }
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}

char * get_global_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    uint32_t df = old_priv ^ new_priv;
    for(int i=0; i<priv_count; i++) {
        if(df & (1<<i)) {
            sprintf(buf, "%s%s='%c',", buf, priv_str_i[i], new_priv&(1<<i) ? 'Y' : 'N');
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}

char * get_global_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        sprintf(buf, "%s'%c',", buf, new_priv&(1<<i) ? 'Y' : 'N');
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}
