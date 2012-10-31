/*
 * thread.c 创建与等待收发线程
 *
 *  Created on:	2011-7-5
 *  Author:		Singku
 *	Paltform:	Linux Fedora Core 8 x86-32
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */

// needed for thread operation
#include <pthread.h>
#include <string.h>
// needed for errno
#include <errno.h>

#include "common.h"
#include "log.h"
#include "collect.h"
#include "send.h"
#include "log.h"
#include "thread.h"

static pthread_t collect_thread = 0;
static pthread_t send_thread = 0;

/*
 * @brief 创建收发线程
 */
int start_thread()
{

	if (collect_thread == 0)
		if (pthread_create(&collect_thread, NULL, data_collect, NULL) != 0) {
			write_log("Create collect thread failed: %s\n", strerror(errno));
			return -1;
		}

	if (send_thread == 0)
		if (pthread_create(&send_thread, NULL, data_send, NULL) != 0) {
			write_log("Create send thread failed: %s\n", strerror(errno));
			return -1;
		}

	return 0;
}

/*
 * @brief 等待收发线程
 */
int wait_thread()
{
	if (collect_thread != 0)
		if (pthread_join(collect_thread, NULL) != 0) {
			write_log("Wait collect thread failed: %s\n", strerror(errno));
			return -1;
		}
	collect_thread = 0;

	if (send_thread != 0)
		if (pthread_join(send_thread, NULL) != 0) {
			write_log("Wait send thread failed: %s\n", strerror(errno));
			return -1;
		}
	send_thread = 0;

	return 0;
}
