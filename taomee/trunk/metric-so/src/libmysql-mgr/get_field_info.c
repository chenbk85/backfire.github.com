#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "mysql_mgr.h"

typedef struct {
    uint8_t    head;   //必须是0xFF开头
    char       host[180];
    char       db[192];
    char       user[48];
    char       table[192];
    char       column[192];
    uint32_t   timestamp;
    uint8_t    priv;
} __attribute__((__packed__)) field_priv_t;

static char priv_str[][16] = {"Select", "Insert", "Update", "References"};
static int  priv_count = sizeof(priv_str)/sizeof(priv_str[0]);

int get_field_info(const char * file, name_t * field, int b_len)
{
    FILE * fd = fopen(file, "rb");
    uint32_t num;
    uint16_t seek, id = 3;
    uint8_t len = 0xf;
    char buf[256];
    int n = 0;
    if(fd == 0) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -4;
    } else {
        DEBUG_LOG("open %s", file);
    }
    if(fseek(fd, 0xa, SEEK_SET) == 0) {
        READ_NUM(seek, fd);
        seek -= 0xeb0;
        fseek(fd, seek, SEEK_SET);
        seek = 0;
        while(1) {
            if(n >= b_len) {
                n = -n-100;
                break;
            }
            READ_NUM(seek, fd);
            if(seek >= 5 && seek != id+1) {
                READ_NUM(num, fd);
                READ_NUM(len, fd);
                if(len == 0) {
                    break;
                }
                READ_STR(buf, len, fd);
                READ_NUM(id, fd);
                READ_NUM(len, fd);
                READ_STR(buf, len, fd);
                buf[len] = '\0';
                strcpy(field[n].name, buf);
            } else {
                id = seek;
                READ_NUM(len, fd);
                READ_STR(buf, len, fd);
                buf[len] = '\0';
                strcpy(field[n].name, buf);
            }
            n++;
        }
    } else {
        n = -1;
        ERROR_LOG("fseek error");
    }
    fclose(fd);
    return n;
}

int get_field_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv)
{
    FILE * fd = fopen(file, "rb");
    if(fd == 0) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -4;
    } else {
        DEBUG_LOG("open %s", file);
    }
    field_priv_t field_priv;
    int n = 0;
    while(fread(&field_priv, sizeof(field_priv_t), 1, fd) == 1) {
        if(n >= len) {
            n = -n-100;
            break;
        }
        if(field_priv.head != 0xFF) {
            ERROR_LOG("%d field_priv.head[0x%02X] != 0xFF.", n, field_priv.head);
            continue;
        }
        TRIM(field_priv.host);
        TRIM(field_priv.db);
        TRIM(field_priv.user);
        TRIM(field_priv.table);
        TRIM(field_priv.column);
#if 0
        if(strlen(field_priv.host) == 0
            || strlen(field_priv.db) == 0
            || strlen(field_priv.user) == 0
            || strlen(field_priv.table) == 0
            || strlen(field_priv.column) == 0) {
            continue;
        }
#endif
        if((upriv->user[0] != '\0' && upriv->host[0] != '\0' )
                && (strcmp(upriv->host, field_priv.host) != 0 || strcmp(upriv->user, field_priv.user) != 0)) {
            //DEBUG_LOG("user");
            continue;
        }
        if((upriv->db[0] != '\0' && strcmp(upriv->db, field_priv.db) != 0)) {
            //DEBUG_LOG("db");
            continue;
        }
        if((upriv->table[0] != '\0' && strcmp(upriv->table, field_priv.table) != 0)) {
            //DEBUG_LOG("table");
            continue;
        }
        if((upriv->field[0] != '\0' && strcmp(upriv->field, field_priv.column) != 0)) {
            //DEBUG_LOG("field");
            continue;
        }
        strcpy(priv[n].host_ip, field_priv.host);
        strcpy(priv[n].user_name, field_priv.user);
        strcpy(priv[n].db_name, field_priv.db);
        strcpy(priv[n].table_name, field_priv.table);
        strcpy(priv[n].column_name, field_priv.column);
        priv[n].priv = field_priv.priv;
        n++;
    }
    fclose(fd);
    return n;
}

void print_field_priv(int priv)
{
    for(int i=0; i<priv_count; i++) {
        if(priv & (1<<i)) {
            DEBUG_LOG("%s;", priv_str[i]);
        }
    }
}

char * get_field_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf, const char * column)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        if(new_priv&(1<<i)) {
            sprintf(buf, "%s%s(%s),", buf, priv_str[i], column);
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}

char * get_field_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        if(new_priv&(1<<i)) {
            sprintf(buf, "%s%s,", buf, priv_str[i]);
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}

char * get_field_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        if(new_priv&(1<<i)) {
            sprintf(buf, "%s%s,", buf, priv_str[i]);
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}
