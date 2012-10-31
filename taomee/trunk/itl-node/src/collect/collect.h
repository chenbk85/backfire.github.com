/** 
 * ========================================================================
 * @file collect.h
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-07-06
 * Modify $Date: 2012-10-25 11:26:03 +0800 (四, 25 10月 2012) $
 * Modify $Author: smyang $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */


#ifndef H_COLLECT_H_2012_07_06
#define H_COLLECT_H_2012_07_06

#include "itl_common.h"
#include "../proto.h"


int init_server_collect(Cmessage * c_out);

int init_mysql_collect(Cmessage * c_out);

int init_collect();

int reload_collect();

int fini_collect();

bool is_collect_inited();

int init_metric_timer();

int fini_metric_timer();

#endif
