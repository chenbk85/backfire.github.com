/**
 * =====================================================================================
 *       @file  main.cpp
 *      @brief
 *
 *     Created  2011-11-17 14:25:00
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sys/resource.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#include "i_config.h"
#include "proto.h"
#include "defines.h"
#include "lib/utils.h"
#include "lib/log.h"
#include "./monitor-process/monitor_process.h"

/**
 * @brief  程序启动的时候把以前自动更新过来的程序删掉
 * @param  prog_name: 现在正在运行的程序的名字
 * @return 0:success, -1:failed
 */
static int unlink_old_prog(const char *prog_name)
{
    DIR *dir = NULL;
    struct dirent *de = NULL;
    dir = opendir("./");
    if (dir == NULL) {
        ERROR_LOG("Open dir \"./\" failed, Error: %s", strerror(errno));
        return -1;
    }

    while ((de = readdir(dir)) != NULL) {
        if (strstr(prog_name, de->d_name) != NULL || !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }
        if(strstr(de->d_name, "oa_node_") != NULL) {
            ERROR_LOG("delete %s", de->d_name);
            unlink(de->d_name);
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char **argv)
{
    if(g_config_file_list[0] == 0 || strlen(g_config_file_list[0]) == 0) {
        std::cerr << g_red_clr << "ERROR: no config file!\n" << g_end_clr << std::endl;
        return -1;
    }

    int reboot = 0;
    //必须以root身份启动程序
    // if(getuid() != 0) {
        // std::cerr << g_red_clr << "ERROR: must be root to run " << argv[0] << "!\n" << g_end_clr << std::endl;
        // return -1;
    // }
    //要kill老的进程吗？
    int c, pid;
    while (-1 != (c = getopt(argc, argv, "c:s:p:d:rn:m:"))) {
        switch (c) {
            case 'r':                           // 通过自动更新重启的程序
                //ERROR_LOG("r=%s", optarg);
                reboot = 1;
                break;
            case 'c':                           // 唯一标识客户端机器的ip地址
                //ERROR_LOG("c=%s", optarg);
                break;
            case 's':                           // 更新服务器的url地址
                //ERROR_LOG("s=%s", optarg);
                break;
            case 'p':                           // 启动这个更新程序的父进程的进程id
                //ERROR_LOG("p=%s", optarg);
                pid = atoi(optarg);
                if (pid < 0) {
                    return -1;
                } else {
                    kill(pid, SIGUSR1);         // 给老进程发送信号，可以退出了
                    DEBUG_LOG("kill -%d %d", SIGUSR1, pid);
                    sleep(2);
                }
                break;
            case 'd':                           // 启动这个更新程序的父进程的程序名
                //ERROR_LOG("d=%s", optarg);
                break;
            default:
                break;
        }
    }
    optarg = NULL;
    optind = 0;
    opterr = 0;
    optopt = 0;
    // 作为后台程序运行，不改变根路径，不关闭标准终端
    if (daemon(1, 1) != 0) {
        return -1;
    }

    if(reboot == 0) {
        // 删除以前更新留下来的程序
        if (unlink_old_prog(argv[0]) != 0) {
            std::cerr << g_red_clr << "ERROR: unlink update prog before." << g_end_clr << std::endl;
            return -1;
        }
        if (access("../bin/", F_OK) != 0) {
            std::cerr << g_red_clr << "ERROR: program must be put in \"bin/\"." << g_end_clr << std::endl;
            return -1;
        }
        //创建，改变目录权限
        if (oa_change_dir(OA_LOG_PATH, 0777) || oa_change_dir(OA_SO_PATH, 0755) ||
                oa_change_dir(OA_CONF_PATH, 0755) || oa_change_dir(OA_BAK_PATH, 0755) ||
                oa_change_dir(OA_SO_BAK_PATH, 0755)) {
            std::cerr << g_red_clr << "ERROR: create corresponding dir and chmod to 0755 failed." << g_end_clr << std::endl;
            return -1;
        }
        // 创建daemon.pid文件
        int fd;
        if ((fd = open(OA_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
            std::cerr << g_red_clr << "ERROR: create " << OA_PID_FILE << "." << g_end_clr << std::endl;
            return -1;
        } else {
            close(fd);
        }
    }
    // 上调打开文件数的限制
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
        return -1;
    }
    rl.rlim_cur = rl.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &rl) != 0 ) {
        return -1;
    }
    // 允许产生CORE文件
    if (getrlimit(RLIMIT_CORE, &rl) != 0) {
        return -1;
    }
    rl.rlim_cur = rl.rlim_max;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        return -1;
    }
    //fork monitor进程
    pid_t monitor_pid = 0;
    if((monitor_pid = fork()) < 0) {
       std::cerr << g_red_clr << "ERROR: create monitor process. reason " << strerror(errno) << g_end_clr << std::endl;
       return -1;
    } else if(monitor_pid == 0) {
        monitor_run(argc, argv);
    } else {
        sleep(1);
    }

    return 0;
}
