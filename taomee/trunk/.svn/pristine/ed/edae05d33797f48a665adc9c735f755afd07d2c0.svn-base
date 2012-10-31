/**
 * =====================================================================================
 *       @file  command_process.cpp
 *      @brief
 *
 *     Created  2011-11-30 08:41:29
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include <sys/shm.h>
#include <signal.h>
#include <errno.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <unistd.h>

#include "../proto.h"
#include "../defines.h"
#include "../lib/log.h"
#include "../lib/queue.h"
#include "../lib/utils.h"
#include "../monitor-process/monitor_process.h"
#include "command_process.h"
#include "proto_handler.h"

#define MAX_PTHREAD_COUNT (5)

volatile static sig_atomic_t g_got_sig_term = 0;
static command_so_t last_command_so;
static u_command_t * p_command;
static u_return_t * p_return;

/**
 * @brief 信号处理函数
 * @param sig 触发的信号
 */
static void signal_handler(int sig)
{
    switch (sig) {
        case SIGTERM:
            g_got_sig_term = 1;
            break;
        default:
            ERROR_LOG("ERROR: it should never come here!");
            break;
    }
}

static int do_buildin_cmd(const char * cmd_str)
{
    return 0;
}

static int do_so(char * cmd, uint32_t command_id, uint32_t buf_len)
{
    if(cmd == NULL) {
        ERROR_LOG("cmd=%p", cmd);
        return -1;
    }
    //根据command_id 确定要加载的so和fun
    char so_path[] = "../so/libmysqlmgr.so";
    char func[] = "proto_handler";

    char * error = NULL;
    void * handle = last_command_so.so_handler;
    int (*command_handler)(uint8_t *, uint32_t) = 0;
    int (*command_init)(void *) = 0;
    int (*command_fini)(void *) = 0;

    char md5[33];
    get_file_md5(so_path, md5);
    if(strcmp(md5, last_command_so.md5) != 0) { //需要关闭上次的so，并打开新的so
        if(last_command_so.so_handler != 0) {
            command_fini = (int (*)(void *))dlsym(last_command_so.so_handler, "fini");
            if((error = dlerror()) != NULL) {
                ERROR_LOG("error[%s] when dlsym[%s] in [%s]", error, "fini", last_command_so.so_path);
                return OA_CMD_RET_SO_LOAD_FUNC_ERROR;
            }
            command_fini(NULL);
            dlclose(last_command_so.so_handler);
            if((error = dlerror()) != NULL) {
                ERROR_LOG("error[%s] when close[%s]", error, last_command_so.so_path);
            }
            last_command_so.md5[0] = 0;
            last_command_so.so_handler = 0;
            last_command_so.so_path[0] = 0;
        }
    } else if(last_command_so.so_handler != 0) {
        goto load_func;
    }
    //检测能否读取so文件
    if(access(so_path, F_OK) == -1) {
        ERROR_LOG("[%s] not exist", so_path);
        return OA_CMD_RET_SO_NOT_EXIT;
    } else if(access(so_path, R_OK) == -1) {
        ERROR_LOG("[%s] need read premission", so_path);
        return OA_CMD_RET_SO_NOT_READ;
    }

    //打开so文件
    handle = dlopen(so_path, RTLD_NOW);
    if((error = dlerror()) != NULL) {
        ERROR_LOG("error[%s] when open[%s]", error, so_path);
        last_command_so.md5[0] = 0;
        last_command_so.so_handler = 0;
        last_command_so.so_path[0] = 0;
        return OA_CMD_RET_SO_LOAD_ERROR;
    } else {
        get_file_md5(so_path, last_command_so.md5);
        strcpy(last_command_so.so_path, so_path);
        last_command_so.so_handler = handle;
        command_init = (int (*)(void *))dlsym(handle, "init");
        if((error = dlerror()) != NULL) {
            ERROR_LOG("error[%s] when dlsym[%s] in [%s]", error, "init", so_path);
            return OA_CMD_RET_SO_LOAD_FUNC_ERROR;
        }
        command_init(NULL);
    }

load_func:
    //载入调用函数
    command_handler = (int (*)(uint8_t *, uint32_t))dlsym(handle, func);
    if((error = dlerror()) != NULL) {
        ERROR_LOG("error[%s] when dlsym[%s] in [%s]",error, func, so_path);
        return OA_CMD_RET_SO_LOAD_FUNC_ERROR;
    }

    //调用函数
    ERROR_LOG("run %s.%s", so_path, func);
    int r = command_handler((uint8_t *)cmd, buf_len);//cmd_args, cmd_argv);
    ERROR_LOG("run %s.%s", so_path, func);

    ERROR_LOG("0x%08x %s", command_id, strerror(r));
    return r;
}

static int do_oa_cmd(const char * cmd_str)
{
    if(cmd_str == NULL) {
        ERROR_LOG("cmd_str=%p", cmd_str);
        return -1;
    }
    int r = system(cmd_str);
    if(r == -1) {
        ERROR_LOG("system(%s) return -1, failed when fork process", cmd_str);
        return OA_CMD_RET_INIT_ERROR;     //system在fork子进程时出错，返回初始化错误
    } else if(WIFEXITED(r)) {
        if(WEXITSTATUS(r) == 127) {
            ERROR_LOG("system(%s) return 127, cmd not exist", cmd_str);
            return OA_CMD_RET_NOT_EXIST;  //命令不存在
        } else if(WEXITSTATUS(r) == 126) {
            ERROR_LOG("system(%s) return 126, need more permission", cmd_str);
            return OA_CMD_RET_PER_DENY;   //权限不够
        } else {
            ERROR_LOG("system(%s) return %u", cmd_str, WEXITSTATUS(r));
            return WEXITSTATUS(r);        //正常结束，返回命令的执行结果
        }
    } else {
        ERROR_LOG("system(%s) exit abnormal", cmd_str);
        return OA_CMD_RET_RUN_EXIT_ABN;   //执行时异常终止
    }
}

static int do_scripts(const char * cmd_str) //脚本直接调用system执行
{
    if(cmd_str == NULL) {
        ERROR_LOG("cmd_str=%p", cmd_str);
        return -1;
    }
    return do_oa_cmd(cmd_str);
}

static void do_command(u_command_t * p_command_t, u_return_t * p_return_t)
{
    if(p_command_t == NULL || p_return_t == NULL) {
        ERROR_LOG("p_command_t=%p p_return_t=%p", p_command_t, p_return_t);
        return ;
    }

    p_return_t->command_id = p_command_t->command_id;
    p_return_t->send_fd = p_command_t->send_fd;
    DEBUG_LOG("ready to run %u", p_command_t->command_id);

    switch(p_command_t->command_type) {
        case OA_BUILDIN_CMD:
            do_buildin_cmd(p_command_t->command_data);
            break;
        case OA_SO:
            do_so(p_command_t->command_data, p_command_t->command_id, sizeof(p_command_t->command_data));
            break;
        case OA_SCRIPTS:
            do_scripts(p_command_t->command_data);
            break;
        case OA_CMD:
            do_oa_cmd(p_command_t->command_data);
            break;
        default:
            ERROR_LOG("ERROR command type[%u:%u]", p_command_t->command_id, p_command_t->command_type);
            //p_return_t->return_val = OA_CMD_RET_ERROR_CMD_TYPE;
            return ;
    }
    memcpy(p_return_t->return_val, p_command_t->command_data, sizeof(p_return_t->return_val));
}

static int put_into_queue(u_return_t * return_t, c_queue<u_return_t>* return_q)
{
    if(return_t == NULL || return_q == NULL) {
        ERROR_LOG("return_t=%p return_q=%p", return_t, return_q);
        return -1;
    }
    int r;

    if(return_q->full()) {
        ERROR_LOG("queue full and throw away[%u]", return_t->command_id);
        return -3;
    } else {
        r = return_q->push(return_t);
        if(r == 0) {
            DEBUG_LOG("put[%08X] into queue.", return_t->command_id);
            return 2;
        } else {
            ERROR_LOG("put into queue failed. throw away[%u]", return_t->command_id);
            return -2;
        }
    }
}

int command_run(int argc, char **argv, void * p_arg)
{
    if(p_arg == NULL) {
        ERROR_LOG("p_arg=%p", p_arg);
        return -1;
    }
    u_shmid_t * p_shmid = (u_shmid_t *)p_arg;
    if(p_shmid->cmd_shmid < 0 || p_shmid->ret_shmid < 0 || p_shmid->used_shmid < 0) {
        ERROR_LOG("cmd_shmid=%d[<0] ret_shmid=%d[<0] used_shmid=%d[<0]", p_shmid->cmd_shmid, p_shmid->ret_shmid, p_shmid->used_shmid);
        return -1;
    }

    set_proc_name(argc, argv, "%s_command", OA_BIN_FILE_SAMPLE);
    write_pid_to_file(OA_COMMAND_FILE);

    mysignal(SIGTERM, signal_handler);

    c_queue<u_command_t>* p_command_q;
    c_queue<u_return_t>* p_return_q;
    u_sh_mem_used_t * p_used;

    last_command_so.md5[0] = 0;
    last_command_so.so_handler = 0;
    last_command_so.so_path[0] = 0;

    p_command_q = (c_queue<u_command_t>*)shmat(p_shmid->cmd_shmid, 0, 0);
    if(p_command_q == (void*)(-1)) {
        ERROR_LOG("link command_queue share memory[%d] failed[%s]", p_shmid->cmd_shmid, strerror(int errno));
        return -1;
    }

    p_return_q = (c_queue<u_return_t>*)shmat(p_shmid->ret_shmid, 0, 0);
    if(p_return_q == (void*)(-1)) {
        ERROR_LOG("link return_queue share memory[%d] failed[%s]", p_shmid->ret_shmid, strerror(int errno));
        return -1;
    }

    p_used = (u_sh_mem_used_t*)shmat(p_shmid->used_shmid, 0, 0);
    if(p_used == (void*)(-1)) {
        ERROR_LOG("link pid share memory[%d] failed[%s]", p_shmid->used_shmid, strerror(int errno));
        return -1;
    }
    p_used->return_t_used = 1;

    int r;
    u_command_t command_t;
    u_return_t return_t;
    p_command = &command_t;
    p_return = &return_t;
    while(g_got_sig_term == 0) {
        if(!p_command_q->empty()) {
            ////1.从command_q中取一条命令
            r = p_command_q->pop(p_command);
            if(r == 0) {
                ////2.执行命令 a.shell脚本或命令行命令 b.so程序
                do_command(p_command, p_return);
                ////3.将执行结果写入return_q中
                put_into_queue(p_return, p_return_q);
                printf("put result to queue 0x%08X\n", p_command->command_id);
            } else {
                ERROR_LOG("get command from queue error %d", r);
            }
        } else {
            sleep(1);
        }
    }
    //收到退出命令，等待接受所有的命令
    int max = 10;
    int n = 0;
    while(p_used->command_t_used != 0) {
        if(!p_command_q->empty()) {
            ////1.从command_q中取一条命令
            r = p_command_q->pop(p_command);
            if(r == 0) {
                ////2.执行命令 a.shell脚本或命令行命令 b.so程序，需要fork进程执行还是新建线程执行？
                do_command(p_command, p_return);
                ////3.将执行结果写入return_q中
                put_into_queue(p_return, p_return_q);
            } else {
                ERROR_LOG("get command from queue error %d", r);
            }
        } else {
            if(n++ >=max) {
                break;
            } else {
                sleep(1);
            }
        }
    }
    //通知network进程，所有的执行结果都放到队列中了
    p_used->return_t_used = 0;
    //如果打开的so没关闭，退出前需要关闭

    char * error;
    if(last_command_so.so_handler != 0) {
        int (*command_fini)(void *) = 0;
        command_fini = (int (*)(void *))dlsym(last_command_so.so_handler, "fini");
        if((error = dlerror()) != NULL) {
            ERROR_LOG("error[%s] when dlsym[%s] in [%s]", error, "fini", last_command_so.so_path);
            return OA_CMD_RET_SO_LOAD_FUNC_ERROR;
        }
        command_fini(NULL);
        dlclose(last_command_so.so_handler);
        if((error = dlerror()) != NULL) {
            ERROR_LOG("error[%s] when close[%s]", error, last_command_so.so_path);
        }
        last_command_so.md5[0] = 0;
        last_command_so.so_handler = 0;
        last_command_so.so_path[0] = 0;
    }

    return 0;
}
