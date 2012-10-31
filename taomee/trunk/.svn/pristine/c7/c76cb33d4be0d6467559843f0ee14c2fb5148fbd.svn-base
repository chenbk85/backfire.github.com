/** 
 * ========================================================================
 * @file database.cpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-10-24
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */


#include "database.h"



const char * g_db_priv[] = 
{
#define DB_MGR_DB_PRIV(name) #name,
#include "db_priv_define.h"
#undef DB_MGR_DB_PRIV
};



int parse_db_priv(char * str, uint32_t * p_priv)
{
    if (0 == strcmp("ALL PRIVILEGES", str))
    {
        for (uint32_t i = 0; i < array_elem_num(g_db_priv); i++)
        {
            *p_priv |= (1<<i);
        }
        return 0;
    }
    char * p = str;
    char * token = NULL;
    char * save_ptr = NULL;
    while (NULL != (token = strtok_r(p, ",", &save_ptr)))
    {
        p = NULL;
        while (' ' == token[0])
        {
            // 跳过起头的空格
            token++;
        }

        for (uint32_t i = 0; i < array_elem_num(g_db_priv); i++)
        {
            if (0 == strcasecmp(token, g_db_priv[i]))
            {
                *p_priv |= (1<<i);
            }

        }

    }

    return 0;
}


int gen_db_priv_string(uint32_t priv, char * buf, uint32_t len)
{
    buf[0] = 0;
    for (uint32_t i = 0; i < array_elem_num(g_db_priv); i++)
    {
        if (priv & (1<<i))
        {
            uint32_t buf_len = strlen(buf);
            snprintf(buf + buf_len, len - buf_len, "%s, ", g_db_priv[i]);
        }

    }
    if (buf[0])
    {
        buf[strlen(buf) - 2] = 0;
    }


    return 0;
}



// int get_db_priv(uint32_t port, const db_priv_t * p_priv, std::vector< db_priv_t > & ret_vec)
// {
    // GEN_SQLSTR(g_sql, "SELECT Host, Db, User");
    // for (uint32_t i = 0; i < array_elem_num(g_db_priv); i++)
    // {
        // APPEND_SQLSTR(g_sql, ", %s", g_db_priv[i]);
    // }

    // APPEND_SQLSTR(g_sql, " FROM mysql.db WHERE Host = '%s'", p_priv->host);

    // if (p_priv->user[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND User = '%s'", p_priv->user);
    // }

    // if (p_priv->db[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND Db = '%s'", p_priv->db);
    // }

    // if (0 != g_mysql->init(DB_MGR_HOST, port, DB_MGR_NAME, DB_MGR_USER, DB_MGR_PASS, "utf8"))
    // {
        // return DB_MGR_ERR_CONNECT;
    // }

    // MYSQL_ROW row;
    // int ret = g_mysql->select_first_row(&row, "%s", g_sql);
    // if (ret < 0)
    // {
        // return g_mysql->get_last_errno();
    // }

    // int idx_host = 0;
    // int idx_db = 1;
    // int idx_user = 2;
    // int priv_start = 3;
    // while (NULL != row)
    // {
        // db_priv_t info;
        // DB_MGR_STRCPY(info.user, row[idx_user]);
        // DB_MGR_STRCPY(info.db, row[idx_db]);
        // DB_MGR_STRCPY(info.host, row[idx_host]);


        // for (uint32_t i = 0; i < sizeof(g_db_priv) / sizeof(g_db_priv[0]); i++)
        // {
            // if (row[priv_start + i] && 0 == strncasecmp(row[priv_start + i], "Y", 1))
            // {
                // info.priv |= (1<<i);
            // }

        // }

        // ret_vec.push_back(info);

        // row = g_mysql->select_next_row(true);
    // }

    // return 0;
// }

// int add_db_priv(uint32_t port, const db_priv_t * p_priv)
// {
    // if (p_priv->priv == 0)
    // {
        // return 0;
    // }

    // GEN_SQLSTR(g_sql, "GRANT ");
    // for (uint32_t i = 0; i < sizeof(g_db_priv) / sizeof(g_db_priv[0]); i++)
    // {
        // if (p_priv->priv & (1<<i))
        // {
            // APPEND_SQLSTR(g_sql, "%s, ", g_db_priv_name[i]);
        // }
    // }

    // g_sql[strlen(g_sql) - 2] = 0;


    // APPEND_SQLSTR(g_sql, " %s.* TO '%s'@'%s'", 
            // p_priv->db, 
            // p_priv->user, 
            // p_priv->host);

    // int ret = g_mysql->execsql("%s", g_sql);
    // if (ret < 0)
    // {
        // return g_mysql->get_last_errno();
    // }
    // else
    // {
        // return 0;
    // }



// }



// int update_db_priv(uint32_t port, const db_priv_t * p_priv)
// {
    
    // GEN_SQLSTR(g_sql, "UPDATE mysql.db SET ");

    // for (uint32_t i = 0; i < sizeof(g_db_priv) / sizeof(g_db_priv[0]); i++)
    // {

        // APPEND_SQLSTR(g_sql, "%s = '%s', ", 
                // g_db_priv_name[i],
                // p_priv->priv & (1<<i) ? "Y" : "N");
    // }

    // g_sql[strlen(g_sql) - 2] = 0;

    // APPEND_SQLSTR(g_sql, " WHERE User = '%s' AND Host = '%s' AND Db = '%s'", 
            // p_priv->user, 
            // p_priv->host, 
            // p_priv->db);


    // int ret = g_mysql->execsql("%s", g_sql);
    // if (ret < 0)
    // {
        // return g_mysql->get_last_errno();
    // }
    // else
    // {
        // return 0;
    // }

// }
