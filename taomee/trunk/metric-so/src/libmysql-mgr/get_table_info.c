#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "mysql_mgr.h"

typedef struct {
    uint8_t  head;//0xFF
    char     host[180];
    char     db[192];
    char     user[48];
    char     table_name[192];
    char     grantor[231];
    uint32_t timestamp;
    uint16_t table_priv;
    uint8_t  column_priv;
} __attribute__((__packed__)) table_priv_t;

static char priv_str[][32] = {"Select", "Insert", "Update", "Delete", "Create", "Drop", "Grant", "References", "Index", "Alter", "Create View", "Show view"};
static int priv_count = sizeof(priv_str)/sizeof(priv_str[0]);

/**
 *     @fn  get_table_info
 *  @brief  获得表信息
 *
 *  @param  const char * path  数据库对应的文件夹
 *  @param  char * table       获得的表信息
 * @return  >=0-count of table, <0-failed
 */

int get_table_info(const char * path, name_t * table, int len)
{
    struct dirent* ent = NULL;
    DIR *pdir;
    pdir = opendir(path);
    if(pdir == NULL) {
        ERROR_LOG("open %s error %u:%s", path, errno, strerror(errno));
        return -3;
    } else {
        DEBUG_LOG("open %s", path);
    }
    int n = 0;
    char * end = 0;
    while((ent = readdir(pdir)) != NULL) {
        if(n >= len) {
            n = -n-100;
            break;
        }
        if(ent->d_type == 8 //是文件
                && strcmp(ent->d_name, "db.opt") != 0   //不是db.opt
                && (end = strstr(ent->d_name, ".frm")) != 0) {  //以frm为后缀名
            ent->d_name[end - ent->d_name] = 0;     //文件名是表名，去除.frm
            strcpy(table[n++].name, ent->d_name);
        }
    }
    closedir(pdir);
    return n;
}

int get_table_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv)
{
    FILE * fd = fopen(file, "rb");
    if(fd == 0) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -3;
    } else {
        DEBUG_LOG("open %s", file);
    }
#if 0
    if(upriv->db[0] == '\0') {
        ERROR_LOG("database cannot be NULL.");
        return -2;
    }
#endif
    int n = 0;
    int r;
    table_priv_t table_priv;
    while((r = fread(&table_priv, sizeof(table_priv_t), 1, fd)) == 1) {        
        if(table_priv.head != 0xFF) {
            DEBUG_LOG("table_priv.head[0x%02x] != 0xFF.", table_priv.head);
            continue;
        }
        TRIM(table_priv.host);
        TRIM(table_priv.db);
        TRIM(table_priv.user);
        TRIM(table_priv.table_name);
#if 0
        if(strlen(table_priv.host) == 0
                || strlen(table_priv.db) == 0
                || strlen(table_priv.user) == 0
                || strlen(table_priv.table_name) == 0) {
            continue;
        }
#endif
        if((upriv->host[0] != '\0' && upriv->user[0] != '\0')
                && (strcmp(upriv->host, table_priv.host) != 0 || strcmp(upriv->user, table_priv.user) != 0)) {
            continue;
        }
        if((upriv->db[0] != '\0' && strcmp(upriv->db, table_priv.db) != 0)) {
            continue;
        }
        if((upriv->table[0] != '\0' && strcmp(upriv->table, table_priv.table_name) != 0)) {
            continue;
        }
        if(table_priv.table_priv != 0) {
            if(n >= len) {
                n = -n-100;
                break;
            }
            strcpy(priv[n].host_ip, table_priv.host);
            strcpy(priv[n].user_name, table_priv.user);
            strcpy(priv[n].db_name, table_priv.db);
            strcpy(priv[n].table_name, table_priv.table_name);
            strcpy(priv[n].column_name, "");
            priv[n].priv = table_priv.table_priv;
            n++;
        }
    }
    fclose(fd);
    return n;
}

int get_table_tc_priv(const char * file, uint32_t * priv, const user_priv_t * upriv)
{
    *priv = ~0;
    FILE * fd = fopen(file, "rb");
    if(fd == 0) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -1;
    } else {
        DEBUG_LOG("open %s", file);
    }

    if(upriv->db[0] == '\0') {
        ERROR_LOG("database cannot be NULL.");
        return -2;
    }
    if(upriv->table[0] == '\0') {
        ERROR_LOG("table cannot be NULL.");
        return -2;
    }
    if(upriv->host[0] == '\0') {
        ERROR_LOG("host cannot be NULL.");
        return -2;
    }
    if(upriv->user[0] == '\0') {
        ERROR_LOG("user cannot be NULL.");
        return -2;
    }

    int n = 0;
    int r;
    table_priv_t table_priv;
    while((r = fread(&table_priv, sizeof(table_priv_t), 1, fd)) == 1) {
        if(table_priv.head != 0xFF) {
            DEBUG_LOG("table_priv.head[0x%02x] != 0xFF.", table_priv.head);
            continue;
        }
        TRIM(table_priv.host);
        TRIM(table_priv.db);
        TRIM(table_priv.user);
        TRIM(table_priv.table_name);
#if 0
        if(strlen(table_priv.host) == 0
                || strlen(table_priv.db) == 0
                || strlen(table_priv.user) == 0
                || strlen(table_priv.table_name) == 0) {
            continue;
        }
#endif
        if(strcmp(upriv->host, table_priv.host) != 0 || strcmp(upriv->user, table_priv.user) != 0) {
            continue;
        }
        if(strcmp(upriv->db, table_priv.db) != 0) {
            continue;
        }
        if(strcmp(upriv->table, table_priv.table_name) != 0) {
            continue;
        }
        *priv = table_priv.column_priv;
        n++;
        break;
    }
    fclose(fd);
    return n;
}

void print_table_priv(int priv)
{
    for(int i=0; i<priv_count; i++) {
        if(priv & (1<<i)) {
            DEBUG_LOG("%s;", priv_str[i]);
        }
    }
}

char * get_table_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf)
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

char * get_table_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf)
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

char * get_table_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf)
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
