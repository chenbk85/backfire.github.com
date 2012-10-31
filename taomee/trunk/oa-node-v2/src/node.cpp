/** 
 * ========================================================================
 * @file node.cpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-03
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "node.hpp"

GHashTable * all_nodes;



c_node * alloc_node(const char * server_tag)
{
    c_node * p = reinterpret_cast<c_node *>(g_slice_alloc0(sizeof(c_node)));

    strncpy(p->m_server_tag, server_tag, 20);


    g_hash_table_insert(all_nodes, p->m_server_tag, p);
    return p;
}

void dealloc_node(c_node * p)
{
    g_hash_table_remove(all_nodes, p->m_server_tag);
}


c_node * get_node(const char * server_tag)
{
    return reinterpret_cast<c_node *>(g_hash_table_lookup(all_nodes, server_tag));
}


void free_server_tag(void * p_server_tag)
{
    // printf("%p\n", p_server_tag);
}


void free_node(void * p_node)
{
    if (NULL != p_node)
    {
        g_slice_free1(sizeof(c_node), p_node);
    }
    
}

int init_nodes()
{
    all_nodes = g_hash_table_new_full(g_str_hash, g_str_equal, free_server_tag, free_node);
    return 0;
}


int uninit_nodes()
{
    g_hash_table_destroy(all_nodes);
    return 0;
}

