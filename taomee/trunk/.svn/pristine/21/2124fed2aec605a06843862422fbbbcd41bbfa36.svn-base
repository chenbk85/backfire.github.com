/*
 * main.c 主控逻辑，无限循环
 *
 *  Created on:	2011-7-4
 *  Author:		singku
 *	Paltform:	Linux Fedora Core 8 x86-32
 *	Compiler:	GCC-4.1.2
 *	Copyright:	TaoMee, Inc. ShangHai CN. All Rights Reserved
 */

/*
 * @brief main control logical of data sampling
 * @param no parameters
 */

#include "init_and_destroy.h"
#include "thread.h"

int main(int argc,char **argv)
{
	initiate(argv[0]);

	while (1) {
		//创建收线程与发线程
		start_thread();
		//等待线程结束
		wait_thread();
	}

	destroy();
	return 0;
}
