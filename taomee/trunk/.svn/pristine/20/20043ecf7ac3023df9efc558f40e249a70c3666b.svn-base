/** 
 * ========================================================================
 * @file db.cpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-12
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#include <set>

extern "C"
{
#include <libtaomee/inet/inet_utils.h>
}


#include "db.h"
#include "timer.h"
#include "proto.h"
#include "alarm.h"


using namespace taomee;

c_server g_db_server;

c_server * g_db = &g_db_server;


int init_connect_to_db()
{
    g_db->set_auto_reconnect(CONNECT_TO_SERVER_INTERVAL);
    g_db->connect(config_get_strval("db_ip", NULL), config_get_intval("db_port", 0));
    return 0;
}




int dispatch_db(int fd, const char * buf, uint32_t len)
{
    const db_proto_t * pkg = reinterpret_cast<const db_proto_t *>(buf);

    uint16_t cmd = pkg->cmd;
    uint32_t uid = pkg->id;
    uint32_t seq = pkg->seq;


    TRACE_LOG("dispatch db[%u] sender=%u, fd=%u, seq=%u, len=%u",
            cmd, uid, fd, seq, len);

    const cmd_proto_t * p_cmd = find_db_cmd(cmd);
    if (NULL == p_cmd)
    {
        ERROR_LOG("cmdid not existed: %u", cmd);
        return 0;
    }


    uint32_t body_len = len - sizeof(db_proto_t);

    p_cmd->p_out->init();
    bool read_ret = p_cmd->p_out->read_from_buf_ex(pkg->body, body_len);
    if (!read_ret) 
    {
        ERROR_LOG("read_from_buf_ex error cmd=%u, u=%u", cmd, uid);
        return -1;
    }

    int cmd_ret = p_cmd->func(&fd, p_cmd->p_in, p_cmd->p_out);
    if (DB_SUCC == cmd_ret)
    {
    }

    return 0;
}



//======================Interface Define======================


int db_get_update_notice()
{
    db_proto_t head;
    init_db_proto_header(&head, sizeof(head), 0, db_p_get_update_notice_cmd, 0, 0);
    g_db->send(&head, (Cmessage *)NULL);
    return 0;
}


int db_p_get_update_notice_cb(DEFAULT_ARG)
{
    db_p_get_update_notice_out *p_out = P_OUT;
    if (NULL == p_out)
    {
        ERROR_LOG("db_p_get_update_notice_out is NULL");
        return -1;
    }

    std::set<uint32_t> switch_set;
    std::set<uint32_t> node_set;
    std::set<uint32_t> alarm_set;
    vector_for_each(p_out->update_node, node_it)
    {
        if (node_it->collect_update_flag == 1)
        {
            if (node_it->node_type == ITL_TYPE_SWITCH)
            {
                switch_set.insert(node_it->node_id);
            }
            else
            {
                node_set.insert(node_it->node_id);
            }
        }

        if (node_it->alarm_update_flag == 1)
        {
            alarm_set.insert(node_it->node_id);
        }
    }

    set_for_each(node_set, it)
    {
        uint32_t node_id = *it;
        c_node * p_node = find_node(node_id);
        if (NULL == p_node)
        {
            continue;
        }

        node_p_notify_config_change_in in;
        in.config_type = ITL_TYPE_SERVER;
        send_cli_to_itl_node(p_node, node_p_notify_config_change_cmd, &in);
        INFO_LOG("notify collect config update to node[%u: %s]", p_node->m_node_id, p_node->m_node_ip_str);
    }

    set_for_each(alarm_set, it)
    {
        uint32_t node_id = *it;
        alarm_change_node_alarm_config(node_id);
        INFO_LOG("notify alarm config update to node[%u]", node_id);
    }

    return 0;
}


