/** 
 * ========================================================================
 * @file database.h
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-10-24
 * Modify $Date: 2012-10-30 14:02:40 +0800 (二, 30 10月 2012) $
 * Modify $Author: smyang $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */

#ifndef H_DB_MGR_DATABASE_H_2012_10_24
#define H_DB_MGR_DATABASE_H_2012_10_24

#include "db_mgr_common.h"


int parse_db_priv(char * str, uint32_t * p_priv);

int gen_db_priv_string(uint32_t priv, char * buf, uint32_t len);

// int get_db_priv(uint32_t port, const db_priv_t * p_priv, std::vector< db_priv_t > & ret_vec);

// int add_db_priv(uint32_t port, const db_priv_t * p_priv);

// int update_db_priv(uint32_t port, const db_priv_t * p_priv);

#endif
