/** 
 * ========================================================================
 * @file column.cpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-10-24
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#include "column.h"


const char * g_column_priv[] = 
{
#define DB_MGR_COLUMN_PRIV(name) #name,
#include "column_priv_define.h"
#undef DB_MGR_COLUMN_PRIV
};


int parse_column_priv(const char * str, uint32_t * p_priv)
{
    if (0 == strcmp("ALL PRIVILEGES", str))
    {
        for (uint32_t i = 0; i < array_elem_num(g_column_priv); i++)
        {
            *p_priv |= (1<<i);
        }
        return 0;
    }
    for (uint32_t i = 0; i < array_elem_num(g_column_priv); i++)
    {
        if (0 == strcasecmp(str, g_column_priv[i]))
        {
            *p_priv |= (1<<i);
        }

    }

    return 0;
}



int gen_column_priv_string(const db_priv_t * p_priv, char * buf, uint32_t len)
{
    buf[0] = 0;
    for (uint32_t i = 0; i < array_elem_num(g_column_priv); i++)
    {
        if (p_priv->priv & (1<<i))
        {
            uint32_t buf_len = strlen(buf);
            snprintf(buf + buf_len, len - buf_len, "%s (%s), ", g_column_priv[i], p_priv->column);
        }

    }
    if (buf[0])
    {
        buf[strlen(buf) - 2] = 0;
    }


    return 0;
}



// int get_column_priv(uint32_t port, const db_priv_t * p_priv, std::vector< db_priv_t > & ret_vec)
// {
    // GEN_SQLSTR(g_sql, "SELECT Host, Db, User, Table_name, Column_name, Column_priv FROM mysql.columns_priv WHERE Host = '%s'", p_priv->host);


    // if (p_priv->user[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND User = '%s'", p_priv->user);
    // }

    // if (p_priv->db[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND Db = '%s'", p_priv->db);
    // }

    // if (p_priv->table[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND Table_name = '%s'", p_priv->table);
    // }

    // if (p_priv->column[0])
    // {
        // APPEND_SQLSTR(g_sql, " AND Column_name = '%s'", p_priv->column);
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
    // int idx_table = 3;
    // int idx_column = 4;
    // int idx_priv = 5;
    // while (NULL != row)
    // {
        // db_priv_t info;
        // DB_MGR_STRCPY(info.user, row[idx_user]);
        // DB_MGR_STRCPY(info.db, row[idx_db]);
        // DB_MGR_STRCPY(info.host, row[idx_host]);
        // DB_MGR_STRCPY(info.table, row[idx_table]);
        // DB_MGR_STRCPY(info.column, row[idx_column]);

        // char priv_buf[512] = {0};
        // DB_MGR_STRCPY(priv_buf, row[idx_priv]);


        // char * str = priv_buf;
        // char * save_ptr = NULL;
        // char * token = NULL;
        // while (NULL != (token = strtok_r(str, ",", &save_ptr)))
        // {
            // str = NULL;

            // for (uint32_t i = 0; i < array_elem_num(g_column_priv); i++)
            // {
                // if (0 == strncmp(g_column_priv[i], token, strlen(g_column_priv[i])))
                // {
                    // info.priv |= (1<<i);
                    // break;
                // }
            // }
        // }

        // ret_vec.push_back(info);

        // row = g_mysql->select_next_row(true);
    // }
    // return 0;
// }



// int add_column_priv(uint32_t port, const db_priv_t * p_priv)
// {
    // if (p_priv->priv == 0)
    // {
        // return 0;
    // }

    // GEN_SQLSTR(g_sql, "GRANT ");
    // for (uint32_t i = 0; i < sizeof(g_column_priv) / sizeof(g_column_priv[0]); i++)
    // {
        // if (p_priv->priv & (1<<i))
        // {
            // APPEND_SQLSTR(g_sql, "%s(%s), ", g_column_priv[i], p_priv->column);
        // }
    // }

    // g_sql[strlen(g_sql) - 2] = 0;


    // APPEND_SQLSTR(g_sql, " %s.%s TO '%s'@'%s'", 
            // p_priv->db, 
            // p_priv->table,
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
// int update_column_priv(uint32_t port, const db_priv_t * p_priv)
// {
    // return 0;
// }
