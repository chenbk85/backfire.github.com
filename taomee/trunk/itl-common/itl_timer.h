/** 
 * ========================================================================
 * @file itl_timer.h
 * @brief 
 * @author smyang
 * @version 1.0
 * @date 2012-10-11
 * Modify $Date: 2012-10-17 16:13:16 +0800 (三, 17 10月 2012) $
 * Modify $Author: smyang $
 * Copyright: TaoMee, Inc. ShangHai CN. All rights reserved.
 * ========================================================================
 */


#ifndef H_ITL_TIMER_H_2012_10_11
#define H_ITL_TIMER_H_2012_10_11



extern "C"
{
#include <libtaomee/list.h>
#include <libtaomee/timer.h>
}



struct timer_head_t
{
    timer_head_t()
    {
        INIT_LIST_HEAD(&timer_list);
    }
    ~timer_head_t()
    {
        list_del_init(&timer_list);
    }
    list_head_t timer_list;
};




#endif
