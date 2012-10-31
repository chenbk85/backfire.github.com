/** 
 * ========================================================================
 * @file node.hpp
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-03
 * Modify $Date: $
 * Modify $Author: $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#include <stdint.h>



class c_node
{
    public:

        char m_server_tag[20];

        uint32_t m_metric_count;


    private:



};


c_node * alloc_node(const char * server_tag);


void dealloc_node(c_node * p);


c_node * get_node(const char * server_tag);


int init_nodes();


int uninit_nodes();
