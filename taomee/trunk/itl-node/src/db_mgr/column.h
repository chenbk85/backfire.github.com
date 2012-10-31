/** 
 * ========================================================================
 * @file column.h
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-10-24
 * Modify $Date: 2012-10-30 14:02:40 +0800 (二, 30 10月 2012) $
 * Modify $Author: smyang $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#ifndef H_DB_MGR_COLUMN_H_2012_10_24
#define H_DB_MGR_COLUMN_H_2012_10_24

#include "db_mgr_common.h"


int parse_column_priv(const char * str, uint32_t * p_priv);

int gen_column_priv_string(const db_priv_t * p_priv, char * buf, uint32_t len);

// int get_column_priv(uint32_t port, const db_priv_t * priv, std::vector< db_priv_t > & ret_vec);


// int add_column_priv(uint32_t port, const db_priv_t * p_priv);

// int update_column_priv(uint32_t port, const db_priv_t * p_priv);

#endif
