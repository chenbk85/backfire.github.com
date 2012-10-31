#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "mysql_mgr.h"

typedef struct {
    uint8_t    head;   //必须是0xFF开头
    char       host[180];
    char       db[192];
    char       user[48];
    uint8_t    priv[17];
} __attribute__((__packed__)) database_priv_t;

static char priv_str_i[][32] = {"Select_priv", "Insert_priv", "Update_priv", "Delete_priv", "Create_priv", "Drop_priv", "Grant_priv", "References_priv", "Index_priv", "Alter_priv", "Create_tmp_table_priv", "Lock_tables_priv", "Create_view_priv", "Show_view_priv", "Create_routine_priv", "Alter_routine_priv", "Execute_priv"};
static char priv_str_g[][32] = {"Select", "Insert", "Update", "Delete", "Create", "Drop", "Grant", "References", "Index", "Alter", "Create temporary tables", "Lock tables", "Create view", "Show view", "Create routine", "Alter routine", "Execute"};
static int  priv_count = sizeof(priv_str_g)/sizeof(priv_str_g[0]);

int get_database_info(const char * datadir, name_t * database, int len)
{
    struct dirent* ent = NULL;
    DIR *pdir;
    pdir = opendir(datadir);
    if(pdir == NULL) {
        ERROR_LOG("open %s error %u:%s", datadir, errno, strerror(errno));
        return -2;
    } else {
        DEBUG_LOG("open %s", datadir);
    }
    int n = 0;
    while((ent = readdir(pdir)) != NULL) {
        if(n >= len) {
            n = -n-100;
            break;
        } 
        if(ent->d_type == 4
                && strcmp(ent->d_name, ".") != 0
                && strcmp(ent->d_name, "..") != 0) {
            strcpy(database[n++].name, ent->d_name);
        }
    }
    closedir(pdir);
    return n;
}

int get_database_priv(const char * file, priv_t * priv, int len, const user_priv_t * upriv)
{
    FILE * fd = fopen(file, "rb");
    if(fd == 0) {
        ERROR_LOG("open %s error %u:%s", file, errno, strerror(errno));
        return -2;
    } else {
        DEBUG_LOG("open %s", file);
    }
    database_priv_t database_priv;
    int n = 0;
    int r;
    while((r = fread(&database_priv, sizeof(database_priv_t), 1, fd)) == 1) {
        if(n >= len) {
            n = -n-100;
            break;
        }
        if(database_priv.head != 0xFF) {
            ERROR_LOG("database_priv.head[0x%02X] != 0xFF.", database_priv.head);
            continue;
        }
        TRIM(database_priv.host);
        TRIM(database_priv.db);
        TRIM(database_priv.user);
#if 0
        if(strlen(database_priv.host) == 0
            || strlen(database_priv.db) == 0
            || strlen(database_priv.user) == 0) {
            continue;
        }
#endif
        if((upriv->user[0] != '\0' && upriv->host[0] != '\0' )
                && (strcmp(upriv->host, database_priv.host) != 0 || strcmp(upriv->user, database_priv.user) != 0)) {
            continue;
        }
        if((upriv->db[0] != '\0' && strcmp(upriv->db, database_priv.db) != 0)) {
            continue;
        }
        strcpy(priv[n].host_ip, database_priv.host);
        strcpy(priv[n].user_name, database_priv.user);
        strcpy(priv[n].db_name, database_priv.db);
        strcpy(priv[n].table_name, "");
        strcpy(priv[n].column_name, "");
        priv[n].priv = 0;
        for(int i=0; i<priv_count; i++) {
            if(database_priv.priv[i] == 2) {
                priv[n].priv |= (1<<i);
            }
        }
        n++;
    }
    fclose(fd);
    return n;
}

void print_database_priv(int priv)
{
    for(int i=0; i<priv_count; i++) {
        if(priv & (1<<i)) {
            DEBUG_LOG("%s;", priv_str_g[i]);
        }
    }
}

char * get_database_priv_grant(uint32_t old_priv, uint32_t new_priv, char * buf)
{
    if(old_priv == new_priv) {
        buf[0] = '\0';
        return buf;
    }
    buf[0] = '\0';
    for(int i=0; i<priv_count; i++) {
        if(i != GRANT_DB_INDEX) {
            if(new_priv&(1<<i)) {
                sprintf(buf, "%s%s,", buf, priv_str_g[i]);
            }
        }
    }
    buf[strlen(buf)-1] = '\0';
    return buf;
}

char * get_database_priv_insert(uint32_t old_priv, uint32_t new_priv, char * buf)
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

char * get_database_priv_update(uint32_t old_priv, uint32_t new_priv, char * buf)
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
