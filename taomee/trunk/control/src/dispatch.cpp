/** 
 * ========================================================================
 * @file dispatch.cpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-10
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */



#include "dispatch.h"
#include "proto.h"
#include "node.h"
#include "control.h"
#include "proxy.h"



int dispatch(int fd, const char * buf, uint32_t len)
{
    c_node * p_node = find_node_by_fd(fd);
    if (NULL != p_node)
    {
        // 是来自node的报文
        return dispatch_from_node(p_node, buf, len);
    }


    const control_proto_t * pkg = reinterpret_cast<const control_proto_t *>(buf);

    uint16_t cmd = pkg->cmd;
    if (control_p_node_register_cmd == cmd)
    {
        uint32_t body_len = len - sizeof(control_proto_t);
        return control_p_node_register(fd, pkg->body, body_len);

    }

    if (2 == cmd / 1000)
    {
        const node_proto_t * pkg = reinterpret_cast< const node_proto_t * >(buf);

        // 规定包体的第一个字段是node_ip
        const char * node_ip_str = pkg->body;
        return dispatch_to_node(fd, node_ip_str, buf, len);
    }

    if (1 == cmd / 0x1000)
    {
        // 数据库管理的报文
        // 定位到node_ip
        uint32_t node_ip = ntohl(*(uint32_t *)(buf + 18 + 32 + 4 * 3));
        DEBUG_LOG("dispatch mysql mgr to node: %s", long2ip(node_ip));
        return dispatch_to_node(fd, node_ip, buf, len);
    }

    uint32_t seq = pkg->seq;
    uint32_t body_len = len - sizeof(web_proto_t);

    TRACE_LOG("dispatch[%u] fd=%u, seq=%u, len=%u",
            cmd, fd, seq, len);

    // if (head_p_get_version_cmd == cmd)
    // {
    // return get_version();
    // }
    // else if (head_p_node_register_cmd == cmd)
    // {
    // return head_p_node_register(fd, pkg->body, body_len);
    // }
    // else if (head_p_get_node_data_cmd == cmd)
    // {
    // return head_p_get_node_data(fd, node_id);
    // }
    // else
    // {
    // p_node = find_node(node_id);
    // if (NULL == p_node)
    // {
    // ERROR_LOG("unregistered node, fd: %d, node id: %u", fd, node_id);
    // net_close_cli(fd);
    // return -1;
    // }
    // }


    const cmd_proto_t * p_cmd = find_control_cmd(cmd);
    if (NULL == p_cmd)
    {
        ERROR_LOG("cmdid not existed: %u", cmd);
        return 0;
    }


    bool read_ret = p_cmd->p_in->read_from_buf_ex(pkg->body, body_len);
    if (!read_ret) 
    {
        ERROR_LOG("read_from_buf_ex error cmd=%u", cmd);
        return -1;
    }



    int cmd_ret = p_cmd->func(&fd, p_cmd->p_in, p_cmd->p_out);


    return cmd_ret;
}


int get_version()
{
    return 0;

}
