/**
 * =====================================================================================
 *       @file  monitor_process.cpp
 *      @brief
 *
 *     Created  2011-11-24 09:53:35
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include <signal.h>
#include <iostream>
#include <iomanip>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <time.h>

#include "../lib/utils.h"
#include "../lib/log.h"
#include "../lib/queue.h"
#include "../i_config.h"
#include "../proto.h"
#include "../collect-process/collect_process.h"
#include "../network-process/network_process.h"
#include "./update_thread.h"
#include "./monitor_process.h"
#include "../command-process/command_process.h"

#ifdef  likely
#undef  likely
#endif
#define likely(x) __builtin_expect(!!(x), 1)

#ifdef  unlikely
#undef  unlikely
#endif
#define unlikely(x) __builtin_expect(!!(x), 0)

#define OA_COMMAND

#define NO_OUTPUT

#define FORK_ERROR      (-1)
#define FORK_SUCCESS    (1)
#define RUN_ERROR       (-2)
#define RUN_SUCCESS     (2)
#define RESTART_ERROR   (-3)
#define CREATE_ERROR    (-4)

#define ONE_DAY         (3600*24)
//MAX_FORK_TIME秒内，某个进程重启次数超过MAX_FORK_COUNT次，程序会异常退出
#define MAX_FORK_COUNT      (10)        //当某个进程core或异常退出后，最多重启的次数，防止频繁重启某个进程
#define MAX_FORK_TIME       (1800)      //重启统计时间段
#define TAR_LOG_INTERVAL    (3600*6)    //打包log的检测间隔

volatile static sig_atomic_t g_got_sig_term = 0;
volatile static sig_atomic_t g_got_sig_int  = 0;
volatile static sig_atomic_t g_got_sig_quit = 0;
volatile static sig_atomic_t g_got_sig_usr1 = 0;    //nobody进程通知主进程，自己已正常退出
volatile static sig_atomic_t g_got_sig_usr2 = 0;    //root进程通知主进程，自己已正常退出

static bool last_update_failed = false;
static bool del_old_prog = true;
static char log_prefix[NAME_MAX] = {0};        //监控用
static int  n_dmax, n_heart_interval, n_listen_port;
static char n_cluster_name[OA_MAX_STR_LEN];
static char n_parent_ip[OA_MAX_STR_LEN];
static bool n_is_recv_udp;
static bool n_is_in;
static pid_t * p_collect_nobody;
static pid_t * p_collect_root;

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
do{\
     if(p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
         ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
         return false;\
     }\
 } while(0)

typedef struct {
    pid_t process_id;           //进程ID
    const char * process_name;  //进程名
    const char * pid_file;      //进程pid文件
    time_t restart_time;        //上次重启时间
    uint32_t restart_count;     //本次时间段内，重启次数
    int (*run)(int argc, char **argv, void * p_arg);    //进程运行主函数
    void * p_arg;               //进程运行主函数的参数
} process_info_t;

enum {
    OA_PRO_COLLECT_ROOT = 0,
    OA_PRO_COLLECT_NOBODY = 1,
    OA_PRO_COMMAND = 2,
    OA_PRO_NETWORK = 3,
};

static bool save_network_config(i_config * p_config)
{
    if(p_config != NULL) {
        set_recv_udp(p_config, &n_is_recv_udp);
        char s_dmax[10] = {0};
        GET_CONFIG("node_info", "host_dmax", s_dmax);
        n_dmax = atoi(s_dmax);
        n_dmax = n_dmax < 0 ? 0 : n_dmax;

        char time_interval[10] = {0};
        GET_CONFIG("node_info", "heartbeat_interval", time_interval);
        n_heart_interval = atoi(time_interval);

        if(n_is_recv_udp) {
            GET_CONFIG("node_info", "cluster_name", n_cluster_name);
            //GET_CONFIG("node_info", "parent_ip", n_parent_ip);
            if(p_config->get_config("node_info", "parent_ip", n_parent_ip, sizeof(n_parent_ip)) != 0) {
                ERROR_LOG("ERROR: get [%s]:[%s] failed.", "node_info", "parent_ip");
                strcpy(n_parent_ip, DEFAULT_HEAD);
            } else {
                //strcpy(n_parent_ip, DEFAULT_HEAD);
            }
        } else {
            n_cluster_name[0] = '0';
            n_parent_ip[0] = '0';
        }

        char port_str[6] = {0};
        GET_CONFIG("node_info", "listen_port", port_str);
        n_listen_port = atoi(port_str);

        n_is_in = get_listen_type(p_config);
        return true;
    } else {
        ERROR_LOG("in save_network_config p_config == NULL");
        return false;
    }
}

/**
 * @brief 判断network进程是否需要重读配置
 * @param p_config 配置接口实例
 * @return true-需要重读配置，false-不需要重读配置
 */
static bool network_need_reload(i_config* p_config)
{
    if(p_config != NULL) {
        char s_dmax[10] = {0};
        int t_dmax;
        GET_CONFIG("node_info", "host_dmax", s_dmax);
        t_dmax = atoi(s_dmax);
        t_dmax = t_dmax < 0 ? 0 : t_dmax;
        if(t_dmax != n_dmax) {
            DEBUG_LOG("t_dmax[%d] != n_dmax[%d]", t_dmax, n_dmax);
            return true;
        }

        char time_interval[10] = {0};
        int t_heart_interval;
        GET_CONFIG("node_info", "heartbeat_interval", time_interval);
        t_heart_interval = atoi(time_interval);
        if(t_heart_interval != n_heart_interval) {
            DEBUG_LOG("t_heart_interval[%d] != n_heart_interval[%d]", t_heart_interval, n_heart_interval);
            return true;
        }

        if(n_is_recv_udp) {
            char t_cluster_name[OA_MAX_STR_LEN];
            GET_CONFIG("node_info", "cluster_name", t_cluster_name);
            if(strcmp(t_cluster_name, n_cluster_name) != 0) {
                DEBUG_LOG("t_cluster_name[%s] != n_cluster_name[%s]", t_cluster_name, n_cluster_name);
                return true;
            }

            char t_parent_ip[OA_MAX_STR_LEN];
            //GET_CONFIG("node_info", "parent_ip", t_parent_ip);
            if(p_config->get_config("node_info", "parent_ip", t_parent_ip, sizeof(t_parent_ip)) != 0) {
                ERROR_LOG("ERROR: get [%s]:[%s] failed.", "node_info", "parent_ip");
                strcpy(t_parent_ip, DEFAULT_HEAD);
            } else {
                //strcpy(t_parent_ip, DEFAULT_HEAD);
            }
            if(strcmp(t_parent_ip, n_parent_ip) != 0) {
                DEBUG_LOG("t_parent_ip[%s] != n_parent_ip[%s]", t_parent_ip, n_parent_ip);
                return true;
            }
        }

        return false;
    } else {
        ERROR_LOG("in network_need_reload p_config == NULL");
        return false;
    }
}

/**
 * @brief 判断network进程是否需要重启
 * @param p_config 配置接口实例
 * @return true-需要重启，false-不需要重启
 */
static bool network_need_restart(i_config* p_config/*, bool run_network*/)
{
    if(p_config != NULL) {
        bool t_is_in = get_listen_type(p_config);
        if(t_is_in != n_is_in) {
            //return run_network;
            DEBUG_LOG("t_is_in[%s] != n_is_in[%s]", t_is_in?"true":"false", n_is_in?"true":"false");
            return true;
        }
        char port_str[6] = {0};
        int t_listen_port;
        GET_CONFIG("node_info", "listen_port", port_str);
        t_listen_port = atoi(port_str);
        if(n_listen_port != t_listen_port) {
            DEBUG_LOG("n_listen_port[%d] != t_listen_port[%d]", n_listen_port, t_listen_port);
            return true;
        }
        bool t_is_recv_udp;
        set_recv_udp(p_config, &t_is_recv_udp);
        if(t_is_recv_udp != n_is_recv_udp) {
            DEBUG_LOG("t_is_recv_udp[%s] != n_is_recv_udp[%s]", t_is_recv_udp?"true":"false", n_is_recv_udp?"true":"false");
            return true;
        }/* else if(!t_is_recv_udp) {
            if(!run_network) {
                return false;
            }
        }*/
        return false;
    } else {
        ERROR_LOG("in network_need_restart p_config == NULL");
        return false;
    }
}

#undef GET_CONFIG

/**
 * @brief 获得当前时间+delta的日期
 * @return 以uint32_t返回日期，YYYYMMDD
 */
static uint32_t current_date(time_t delta = 0)
{
    struct tm tm;
	time_t now = time(0) + delta;
	localtime_r(&now, &tm);
	return (tm.tm_year+1900)*10000+(tm.tm_mon+1)*100+(tm.tm_mday);
}

static void release_shm(u_shmid_t * shmid)
{
    if(shmid->cmd_shmid >= 0) {
        shmctl(shmid->cmd_shmid, IPC_RMID, NULL);
    }
    if(shmid->ret_shmid >= 0) {
        shmctl(shmid->ret_shmid, IPC_RMID, NULL);
    }
    if(shmid->used_shmid >= 0) {
        shmctl(shmid->used_shmid, IPC_RMID, NULL);
    }
}

/**
 * @brief 初始化日志模块
 * @param p_config 配置接口实例
 * @return 成功返回0，失败返回-1
 */
static int init_log(const i_config *p_config)
{
	if (p_config == NULL) {
		return -1;
	}

	char log_dir[PATH_MAX] = {0};
    char buffer[100] = {0};
	int log_lvl = 0;
	uint32_t log_size = 0;
	int log_count = 0;

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
do{\
     if(p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
         std::cerr << "ERROR : config->get_config(\"" << section << "\", \"" << name << "\", buf, sizeof(buf))." << std::endl;\
         return -1;\
     }\
 }while(0)

    GET_CONFIG("log", "log_dir", log_dir);
	GET_CONFIG("log", "log_prefix", log_prefix);
    GET_CONFIG("log", "log_count", buffer);
    log_count = atol(buffer);
	GET_CONFIG("log", "log_lvl", buffer);
    log_lvl = atol(buffer);
	GET_CONFIG("log", "log_size", buffer);
    log_size = atol(buffer);

#undef GET_CONFIG

	if (log_count == 0) {
		std::cerr << "ERROR: log: log_count." << std::endl;
		return -1;
	}
	if (log_lvl == 0) {
		std::cerr << "ERROR: log: log_lvl." << std::endl;
		return -1;
	}
	if (log_size == 0) {
		std::cerr << "ERROR: log: log_size." << std::endl;
		return -1;
	}
    if (log_size < OA_MIN_LOG_SIZE) {
        log_size = OA_MIN_LOG_SIZE;
    }

	if (log_init(log_dir, (log_lvl_t)log_lvl, log_size, log_count, log_prefix) != 0) {
		std::cerr << "log_init error." << std::endl;
		return -1;
	}
	enable_multi_thread();
	set_log_dest(log_dest_file);

	return 0;
}

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
    case SIGINT:
        g_got_sig_int = 1;
        break;
    case SIGQUIT:
        g_got_sig_quit = 1;
        break;
    case SIGUSR1:
        DEBUG_LOG("got SIGUSER1");
        wait(NULL);
        g_got_sig_usr1 = 1;
        *p_collect_nobody = -1;
        break;
    case SIGUSR2:
        DEBUG_LOG("got SIGUSER1");
        wait(NULL);
        *p_collect_root = -1;
        g_got_sig_usr2 = 1;
        break;
    default:
        ERROR_LOG("ERROR: it should never come here!");
        break;
    }
}

/**
 * @brief 判断进程是否退出，不阻塞
 * @param pid 进程号
 * @return 进程不存在返回true，否则返回false
 */
static bool exited(pid_t pid)
{
    if(pid < 0) {//不处理进程组
        return true;
    }
    pid_t r = waitpid(pid, NULL, WNOHANG);
    return ((r == pid) || (kill(pid, 0) != 0));
}

/**
 * @brief 将文件打包
 * @param tar_file，打包后的tar文件名
 * @param files，欲打包的文件
 * @return 成功返回true，否则返回false
 */
static int tar_package(const char * tar_file, const char * files)
{
    char cmd[OA_MAX_STR_LEN * 2] = {'\0'};
    sprintf(cmd, "tar -Pczvf %s %s", tar_file, files);
    DEBUG_LOG("run cmd : %s", cmd);

    int ret = system(cmd);
    if(ret != 0) {
       ERROR_LOG("tar_package(...) error, call [%s] failed, return [%d].", cmd, ret);
       return -1;
    } else {
        DEBUG_LOG("tar return %d", ret);
        return 0;
    }
}

static int tar_log()
{
    char files[OA_MAX_STR_LEN];
    char tar_file[OA_MAX_STR_LEN];
    char cmd[OA_MAX_STR_LEN];
    uint32_t yesterday = current_date(-ONE_DAY);
    sprintf(files, "%s%s*%u*", OA_LOG_PATH, log_prefix, yesterday);
    sprintf(tar_file, "%s%u.tar.gz", OA_LOG_PATH, yesterday);
    if(access(tar_file, F_OK) == 0) {// tar文件已存在，不用打包了
        DEBUG_LOG("%s exist, rm %s", tar_file, files);
        sprintf(cmd, "rm %s -f", files);
        system(cmd);
        return 0;
    } else if(tar_package(tar_file, files) == 0) {
        sprintf(cmd, "rm %s -f", files);
        system(cmd);
        return 0;
    } else {
        return -1;
    }
}

static void exc_update(const char * client_ip, const char * server_url, c_update_thread * update_thread)
{
    if (strlen(client_ip) > 0 && strlen(server_url) > 0) {
        // 等待程序更新成功, 这样即使itl中未添加配置也不影响服务部署
        bool  output_error = false;
        while(update_thread->exc_update(client_ip, server_url)) {
            if(!output_error) {
                std::cerr << g_red_clr << "ERROR: " << update_thread->get_last_error() << g_end_clr << std::endl;
                output_error = true;
            }
            sleep(10);
        }
    }
}

static void remove_and_copy(const char * del_prog_name, const char * start_prog_name)
{
    if (del_prog_name[0] != 0 && unlink(del_prog_name) != 0) {
        ERROR_LOG("ERROR: unlink(%s).", del_prog_name);
    }
    if (del_prog_name[0] != 0 && access(OA_BIN_FILE, F_OK) == 0) {
        unlink(OA_BIN_FILE);
    }
    if (strcmp(start_prog_name, OA_BIN_FILE) != 0 && link(start_prog_name, OA_BIN_FILE) != 0) {
        ERROR_LOG("ERROR: link(%s, %s).", start_prog_name, OA_BIN_FILE);
    }
}

static void * create_share_mem(u_shmid_t * shmid)
{
    void * p;
    /////1.执行命令队列
    shmid->cmd_shmid = shmget(IPC_PRIVATE, sizeof(c_queue<u_command_t>), IPC_CREAT | 0666 );
    if(shmid->cmd_shmid < 0) {
        ERROR_LOG("create share memory error:%s", strerror(int errno));
        return 0;
    } else {
        DEBUG_LOG("create share memory[%d]", shmid->cmd_shmid);
        p = shmat(shmid->cmd_shmid, 0, 0);
        if(p == (void*)(-1)) {
            ERROR_LOG("link command_queue share memory[%d] failed[%s]", shmid->cmd_shmid, strerror(int errno));
            return 0;
        } else {
            c_queue<u_command_t> * command_q = new c_queue<u_command_t>;
            command_q->clean();
            memcpy(p, command_q, sizeof(c_queue<u_command_t>));
            delete command_q;
        }
    }
    /////2.执行结果队列
    shmid->ret_shmid = shmget(IPC_PRIVATE, sizeof(c_queue<u_return_t>), IPC_CREAT | 0666 );
    if(shmid->ret_shmid < 0) {
        ERROR_LOG("create share memory error:%s", strerror(int errno));
        return 0;
    } else {
        DEBUG_LOG("create share memory[%d]", shmid->ret_shmid);
        p = shmat(shmid->ret_shmid, 0, 0);
        if(p == (void*)(-1)) {
            ERROR_LOG("link return_queue share memory[%d] failed[%s]", shmid->ret_shmid, strerror(int errno));
            return 0;
        } else {
            c_queue<u_return_t> * return_q = new c_queue<u_return_t>;
            return_q->clean();
            memcpy(p, return_q, sizeof(c_queue<u_return_t>));
            delete return_q;
        }
    }
    /////3.共享内存使用的标志位
    shmid->used_shmid = shmget(IPC_PRIVATE, sizeof(u_sh_mem_used_t), IPC_CREAT | 0666 );
    if(shmid->used_shmid < 0) {
        ERROR_LOG("create share memory error:%s", strerror(int errno));
        return 0;
    } else {
        DEBUG_LOG("create share memory[%d]", shmid->used_shmid);
        p = shmat(shmid->used_shmid, 0, 0);
        if(p == (void*)(-1)) {
            ERROR_LOG("link u_sh_mem_used_t share memory[%d] failed[%s]", shmid->used_shmid, strerror(int errno));
            return 0;
        } else {
            u_sh_mem_used_t * used = (u_sh_mem_used_t *)p;
            used->command_t_used = 1;
            used->return_t_used = 1;
        }
    }
    return p;
}

static int dup2null()
{
    int null_fd = open("/dev/null", O_RDWR);
    if (-1 == null_fd) {
        ERROR_LOG("ERROR: open(\"/dev/null\", O_RDWR).");
        return -1;
    }
    dup2(null_fd, 0);
    dup2(null_fd, 1);
    dup2(null_fd, 2);
    close(null_fd);
    return 0;
}

static int fork_process(process_info_t * process_info, int argc, char ** argv)
{
    if((process_info->process_id = fork()) < 0) {
        ERROR_LOG("fork %s failed.", process_info->process_name);
        return FORK_ERROR;
    } else if(process_info->process_id == 0) {  //child
        if(create_file(process_info->pid_file)) {
            //return CREATE_ERROR;
            //创建pid文件失败，程序不退出，保证oa_node正常运行，仅在执行state_oa_node.sh脚本时会有问题
            ERROR_LOG("create_file(%s) error: %s", process_info->pid_file, strerror(errno));
        } else {
            chmod(process_info->pid_file, 0666);
        }
        DEBUG_LOG("fork %s[%u] success.", process_info->process_name, getpid());
        if(process_info->run(argc, argv, process_info->p_arg) < 0) {
            ERROR_LOG("%s return error.", process_info->process_name);
            process_info->process_id = -1;
            return RUN_ERROR;
        } else {
            return RUN_SUCCESS;
        }
    } else {
        return FORK_SUCCESS;
    }
}

static int refork_process(process_info_t * process_info, int argc, char ** argv)
{
    if((time(NULL) - process_info->restart_time) <= MAX_FORK_TIME) { //在MAX_FORK_TIME时间内，重启次数大于MAX_FORK_COUNT，认为有异常，oa_node会退出
        if(process_info->restart_count >= MAX_FORK_COUNT) {
            ERROR_LOG("%s restart too many times. ready to exit...", process_info->process_name);
            return RESTART_ERROR;
        } else {
            process_info->restart_count++;
        }
    } else {
        process_info->restart_time = time(NULL);
        process_info->restart_count = 0;
    }
    ERROR_LOG("%s[%u] exit. ready to restart...", process_info->process_name, process_info->process_id);
    return fork_process(process_info, argc, argv);
}

/**
 * @brief monitor进程的主函数
 * @param 标准main函数的参数
 * @return 成功返回0，否则返回-1
 */
int monitor_run(int argc, char **argv)
{
    int reboot = 0;
    int c;
    //bool run_network = true;
    char client_ip[16] = {0};
    char server_url[OA_MAX_STR_LEN] = {0};
    char del_prog_name[PATH_MAX] = {0};
    char start_prog_name[PATH_MAX];
    strcpy(start_prog_name, argv[0]);
    bool create_shm = true;
    // 共享内存的id和存储对象
    u_shmid_t shmid;
    shmid.cmd_shmid = -1;
    shmid.ret_shmid = -1;
    shmid.used_shmid = -1;

    while (-1 != (c = getopt(argc, argv, "c:s:p:d:rn:m:"))) {
        switch (c) {
            case 'r':                           // 通过自动更新重启的程序
                reboot = 1;
                break;
            case 'c':                           // 唯一标识客户端机器的ip地址
                strcpy(client_ip, optarg);
                break;
            case 's':                           // 更新服务器的url地址
                strcpy(server_url, optarg);
                break;
            case 'p':                           // 启动这个更新程序的父进程的进程id
                break;
            case 'd':                           // 启动这个更新程序的父进程的程序名
                strcpy(del_prog_name, optarg);
                break;
            default:
                break;
        }
    }
    optarg = NULL;
    optind = 0;
    opterr = 0;
    optopt = 0;
    //设置进程名
    set_proc_name(argc, argv, "%s_monitor", OA_BIN_FILE_SAMPLE);
    // 处理信号
    mysignal(SIGTERM, signal_handler);
    mysignal(SIGINT, signal_handler);
    mysignal(SIGQUIT, signal_handler);
    mysignal(SIGUSR1, signal_handler);
    mysignal(SIGUSR2, signal_handler);

    //bool remain_network = true;    //network进程在更新时，是否保留

    time_t start;
    update_info_t update;
    bool restart = false;
    bool need_update = false;
    c_update_thread update_thread(&need_update);
    uint32_t tar_log_interval = 0;
    u_sh_mem_used_t * p_used = 0;
    // 有更新服务器的url,则会通过更新服务器下载所有的程序
    exc_update(client_ip, server_url, &update_thread);
    // 创建配置文件对象
    i_config *p_config = NULL;
    if (create_and_init_config_instance(&p_config) != 0) {
        if (p_config) {
            p_config->release();
            p_config = NULL;
        }
        return -1;
    }
    save_network_config(p_config);
    // 初始化日志模块
    if (init_log(p_config) != 0) {
        ERROR_LOG("ERROR: init_log(p_config).");
        return -1;
    }

    collect_arg_t collect_arg[] = {
        {&p_config, OA_USER_ROOT},
        {&p_config, OA_USER_NOBODY}
    };

    process_info_t process_info[] = {
        {-1, "collect_process for root", OA_COLLECT_R_FILE, time(NULL), 0, collect_run, (void *)&collect_arg[OA_USER_ROOT]},
        {-1, "collect_process for nobody", OA_COLLECT_N_FILE, time(NULL), 0, collect_run, (void *)&collect_arg[OA_USER_NOBODY]},
        {-1, "command_process", OA_COMMAND_FILE, time(NULL), 0, command_run, (void *)&shmid},
        {-1, "network_process", OA_NETWORK_FILE, time(NULL), 0, network_run, (void *)&shmid}
    };
    p_collect_nobody = &process_info[OA_PRO_COLLECT_NOBODY].process_id;
    p_collect_root = &process_info[OA_PRO_COLLECT_ROOT].process_id;

    if(create_file(OA_PID_FILE)) {
        goto exit;
    }
    write_pid_to_file(OA_PID_FILE);

    if(update_thread.init(p_config, pthread_self()) != 0) {
        ERROR_LOG("ERROR: update thread init().");
        goto exit;
    }

    if (reboot) {
        // 删除老的可执行程序,并将新的程序拷贝一份成oa_node
        remove_and_copy(del_prog_name, start_prog_name);
        // 通知update线程上次更新的状态
        update_thread.set_update_status(true);
        reboot = 0;
    }
    //初始化共享内存
    if(create_shm) {
        if((p_used = (u_sh_mem_used_t *)create_share_mem(&shmid)) == 0) {
            goto release_exit;
        }
    } else {
        p_used = (u_sh_mem_used_t *)shmat(shmid.used_shmid, 0, 0);
        if(p_used == (void*)(-1)) {
            ERROR_LOG("link u_sh_mem_used_t share memory[%d] failed[%s]", shmid.used_shmid, strerror(int errno));
            goto release_exit;
        }
    }
    // 更新失败会返回这里，重新执行
again:
    if (last_update_failed) { /**<自动更新失败会执行到这,通知update线程上次更新失败*/
        update_thread.set_update_status(false);
        last_update_failed = false;
    } else {
        std::cout << std::setw(70) << std::left << "OA_NODE START SUCCESSFULLY, VERSION: " << OA_VERSION
            << g_grn_clr << "[ ok ]" << g_end_clr << std::endl;
    }
    DEBUG_LOG("SUCCESS: OA_NODE STARTED, VERSION:%s.", OA_VERSION);
#ifdef NO_OUTPUT
    // 重定向标准输入输出到/dev/null
    if(dup2null()) {
        ERROR_LOG("dup2null error");
        goto release_exit;
    }
#endif

    del_old_prog = true;
    restart = false;
    g_got_sig_term = 0;
    g_got_sig_usr1 = 0;
    g_got_sig_usr2 = 0;

#define FORK_PROCESS(process) \
do{\
    int ret = fork_process(&process_info[process], argc, argv);\
    if(ret == FORK_ERROR) {\
        process_info[process].process_id = -1;\
        goto error_exit;\
    } else if(ret == FORK_SUCCESS) {\
        sleep(1);\
    } else if(ret == RUN_ERROR) {\
        return -1;\
    } else if(ret == RUN_SUCCESS) {\
        return 0;\
    }\
}\
while(0)
    // fork collect进程，root权限
    g_got_sig_usr2 = 0;
    FORK_PROCESS(OA_PRO_COLLECT_ROOT);
#ifdef OA_COLLECT_NOBODY
    // fork collect进程，nobody权限
    g_got_sig_usr1 = 0;
    FORK_PROCESS(OA_PRO_COLLECT_NOBODY);
#endif
#ifdef OA_COMMAND
    // fork command进程
    FORK_PROCESS(OA_PRO_COMMAND);
#endif
/*
#ifdef RESTART_NETWORK
#undef RESTART_NETWORK
    kill(process_info[OA_PRO_NETWORK].process_id, SIGTERM);
    sleep(1);
    wait(NULL);
    process_info[OA_PRO_NETWORK].process_id = -1;
#endif
*/
    //DEBUG_LOG("network process id = %d, command process id = %d", process_info[OA_PRO_NETWORK].process_id, process_info[OA_PRO_COMMAND].process_id);
    //if(process_info[OA_PRO_NETWORK].process_id <=0  //network进程没有启动
    //    && (process_info[OA_PRO_COMMAND].process_id > 0 || n_is_recv_udp)) {//是发送主机或者需要执行远程指令
    //    DEBUG_LOG("begin to fork network");
    //    FORK_PROCESS(OA_PRO_NETWORK);
    //}
    //DEBUG_LOG("network process id = %d, command process id = %d", process_info[OA_PRO_NETWORK].process_id, process_info[OA_PRO_COMMAND].process_id);
    FORK_PROCESS(OA_PRO_NETWORK);
#undef FORK_PROCESS

    DEBUG_LOG("monitor process enter main loop.");
    start = time(NULL);

    while (g_got_sig_term == 0) {//没有收到退出信号，则一直执行
        if (g_got_sig_int) {
        }
        if (g_got_sig_quit) {
        }

        update_thread.work_thread_proc(&start);
        //监控项：将前一天的log打包
        if((tar_log_interval++)%TAR_LOG_INTERVAL == 0) {
            tar_log();
        }

#define REFORK_PROCESS(process) \
do {\
    int ret = refork_process(&process_info[process], argc, argv);\
    if(ret == RESTART_ERROR) {\
        goto error_exit;\
    } else if(ret == FORK_ERROR) {\
        goto error_exit;\
    } else if(ret == FORK_SUCCESS) {\
    } else if(ret == RUN_ERROR) {\
        return -1;\
    } else if(ret == RUN_SUCCESS) {\
        return 0;\
    }\
} while(0)

        if(/*run_network && */unlikely(exited(process_info[OA_PRO_NETWORK].process_id))) { //network已退出
            DEBUG_LOG("network[%u] exited", process_info[OA_PRO_NETWORK].process_id);
            REFORK_PROCESS(OA_PRO_NETWORK);
        }
        //检测command是否退出，退出则重启
#ifdef OA_COMMAND
        if(unlikely(exited(process_info[OA_PRO_COMMAND].process_id))) { //command已退出
            DEBUG_LOG("command[%u] exited", process_info[OA_PRO_COMMAND].process_id);
            REFORK_PROCESS(OA_PRO_COMMAND);
        }
#endif
        //检测collect_root是否退出，退出则重启
        if(unlikely(g_got_sig_usr2 == 0 && exited(process_info[OA_PRO_COLLECT_ROOT].process_id))) { //collect_root已退出
            DEBUG_LOG("%d collect_root[%u] exited", g_got_sig_usr2, process_info[OA_PRO_COLLECT_ROOT].process_id);
            REFORK_PROCESS(OA_PRO_COLLECT_ROOT);
        }
#ifdef OA_COLLECT_NOBODY
        //检测collect_nobody是否退出，退出则重启
        if(unlikely(g_got_sig_usr1 == 0 && exited(process_info[OA_PRO_COLLECT_NOBODY].process_id))) { //collect_nobody已退出
            DEBUG_LOG("%d collect_nobody[%u] exited", g_got_sig_usr1, process_info[OA_PRO_COLLECT_NOBODY].process_id);
            REFORK_PROCESS(OA_PRO_COLLECT_NOBODY);
        }
#endif
#undef REFORK_PROCESS
        if(unlikely(need_update)) { //收到更新信号
            need_update = false;
            //remain_network = true;
            update_thread.get_update_info(&update);
            if(update.exec_updated == 1) {  //bin文件更新，重启oa_node
                restart = true;
                goto restart;
            }
            if(update.config_updated == 1) {    //config文件更新，重启oa_node
                del_old_prog = false;   //配置文件改变，不需要删除旧的bin文件
                update.so_updated_root = 1;
                update.so_updated_nobody = 1;
                if(uninit_and_release_config_instance(p_config) != 0) {
                    //remain_network = true;
                    ERROR_LOG("uninit_and_release_config_instance error");
                }
                if (create_and_init_config_instance(&p_config) != 0) {
                    if (p_config) {
                        p_config->release();
                        p_config = NULL;
                    }
                    //remain_network = true; //读配置错误，不重启network
                    ERROR_LOG("create_and_init_config_instance error");
                } else {
                    if(network_need_restart(p_config/*, run_network*/)) {
                        //remain_network = false;  //重启network
                        pid_t network_pid = process_info[OA_PRO_NETWORK].process_id;
                        if(network_pid > 0) {
                            p_used->return_t_used = 0;
                            kill(network_pid, SIGTERM);
                            while(!exited(network_pid)) {
                                sleep(1);   //等待network退出
                            }
                        }
                        network_pid = fork();
                        process_info[OA_PRO_NETWORK].process_id = network_pid;
                        if(network_pid < 0) {
                            ERROR_LOG("update fork network-process failed.");
                            del_update_and_recover_old(update.config_updated == 1 ? true : false,
                                update.so_updated_root + update.so_updated_nobody + update.so_updated_command >=1 ? true : false, NULL);
                            last_update_failed = true;
                            p_used->command_t_used = 0;
                            goto error_exit;
                        } else if(network_pid == 0) {//child
                            DEBUG_LOG("[%u] update fork network-process[%u] success.", getppid(), getpid());
                            last_update_failed = false;
                            if(network_run(argc, argv, (void *)&shmid) < 0) {
                                ERROR_LOG("update command-process run return error.");
                                uninit_and_release_config_instance(p_config);
                                return -1;
                            } else {
                                uninit_and_release_config_instance(p_config);
                                return 0;
                            }
                        } else {
                            //waitpid(network_pid, NULL, WNOHANG);
                        }
                    } else if(network_need_reload(p_config)) {
                        kill(process_info[OA_PRO_NETWORK].process_id, SIGUSR1); //通知network，重读配置
                        DEBUG_LOG("network-process[%u] should reload config.", process_info[OA_PRO_NETWORK].process_id);
                    }
                    //uninit_and_release_config_instance(p_config);
                    //p_config = NULL;
                }
            }
            if(update.so_updated_root == 1) {   //root使用的so文件更新，重启collect进程
                g_got_sig_usr2 = 0;
                pid_t collect_root = process_info[OA_PRO_COLLECT_ROOT].process_id;
                if(collect_root > 0) {
                    kill(collect_root, SIGTERM);
                    while(!exited(collect_root)) {
                        sleep(1);   //等待root退出
                    }
                }
                collect_root = fork();
                process_info[OA_PRO_COLLECT_ROOT].process_id = collect_root;
                if(collect_root < 0) {
                    ERROR_LOG("update fork collect-process for root failed.");
                    del_update_and_recover_old(update.config_updated == 1 ? true : false,
                        update.so_updated_root + update.so_updated_nobody + update.so_updated_command >=1 ? true : false, NULL);
                    last_update_failed = true;
                    goto error_exit;
                } else if(collect_root == 0) {//child
                    //update_thread.uninit();
                    DEBUG_LOG("update fork collect-process[%u] for root success.", getpid());
                    last_update_failed = false;
                    if(collect_run(argc, argv, process_info[OA_PRO_COLLECT_ROOT].p_arg) < 0) {
                        ERROR_LOG("update collect-process for root run return error.");
                        return -1;
                    } else {
                        DEBUG_LOG("update collect-process for root run return 0.");
                        return 0;
                    }
                } else {
                    //waitpid(collect_root, NULL, WNOHANG);
                }
            }
            if(update.so_updated_command == 1) {   //command使用的so文件更新，重启command进程
                pid_t command_pid = process_info[OA_PRO_COMMAND].process_id;
                if(command_pid > 0) {
                    p_used->command_t_used = 0;
                    kill(command_pid, SIGTERM);
                    while(!exited(command_pid)) {
                        sleep(1);   //等待command退出
                    }
                }
                command_pid = fork();
                process_info[OA_PRO_COMMAND].process_id = command_pid;
                if(command_pid < 0) {
                    ERROR_LOG("update fork command-process failed.");
                    del_update_and_recover_old(update.config_updated == 1 ? true : false,
                        update.so_updated_root + update.so_updated_nobody + update.so_updated_command >=1 ? true : false, NULL);
                    last_update_failed = true;
                    p_used->return_t_used = 0;
                    goto error_exit;
                } else if(command_pid == 0) {//child
                    DEBUG_LOG("update fork command-process[%u] success.", getpid());
                    last_update_failed = false;
                    if(command_run(argc, argv, (void *)&shmid) < 0) {
                        ERROR_LOG("update command-process run return error.");
                        return -1;
                    } else {
                        return 0;
                    }
                }else {
                    //waitpid(command_pid, NULL, WNOHANG);
                }
            }
            if(update.script_updated == 1) {
                //do nothing
            }
        }

        sleep(1);
    }
    DEBUG_LOG("monitor process leave main loop.");

error_exit: //异常退出：给所有进程发送退出信号
    //remain_network = false;
    p_used->command_t_used = 0;
    p_used->return_t_used = 0;
restart:
    update_thread.uninit();

    if(process_info[OA_PRO_COLLECT_ROOT].process_id > 0) {
        kill(process_info[OA_PRO_COLLECT_ROOT].process_id, SIGTERM);
    }
    if(process_info[OA_PRO_NETWORK].process_id > 0) {
        //if(!remain_network) {
            kill(process_info[OA_PRO_NETWORK].process_id, SIGTERM);
        //}
    } else {
        p_used->command_t_used = 0;
    }
#ifdef OA_COLLECT_NOBODY
    if(process_info[OA_PRO_COLLECT_NOBODY].process_id > 0) {
        kill(process_info[OA_PRO_COLLECT_NOBODY].process_id, SIGTERM);
    }
#endif
#ifdef OA_COMMAND
    if(process_info[OA_PRO_COMMAND].process_id > 0) {
        kill(process_info[OA_PRO_COMMAND].process_id, SIGTERM);
    } else {
        p_used->return_t_used = 0;
    }
#endif
    //等待程序退出
    while(1) {
        if(exited(process_info[OA_PRO_COLLECT_ROOT].process_id)
            && exited(process_info[OA_PRO_NETWORK].process_id)
#ifdef OA_COLLECT_NOBODY
            && exited(process_info[OA_PRO_COLLECT_NOBODY].process_id)
#endif
#ifdef OA_COMMAND
            && exited(process_info[OA_PRO_COMMAND].process_id)
#endif
        ) {
            break;
        } else {
            sleep(1);
        }
    }
    //释放共享内存
release_exit:
    //if(!remain_network) {
        release_shm(&shmid);
    //}

exit:
    if(restart) {   //需要重启oa_node，fork一个进程启动新的oa_node
        DEBUG_LOG("program has updated.");
        pid_t pid;
        if ((pid = fork()) > 0) {   // parent
            for (int i = 0; i != 30; ++i) {
                if (g_got_sig_usr1) { /**<新程序执行成功，退出程序*/
                    DEBUG_LOG("updated program send kill signal.");
                    //wait(NULL); /**<XXX 新产生的子进程daemon后就会返回*/
                    uninit_and_release_config_instance(p_config);
                    log_fini();
                    return 0;
                }
                sleep(1);
            }
            // 30秒没收到SIGUSR1信号,则认为运行新程序失败
            ERROR_LOG("ERROR: UPDATE FAILED");
            //wait(NULL);
            // 还原老的文件
            del_update_and_recover_old(update.config_updated == 1 ? true : false,
                                        update.so_updated_root + update.so_updated_nobody + update.so_updated_command >=1 ? true : false,
                                        NULL);
            last_update_failed = true;
            create_shm = true;//remain_network ? false : true;
            goto again;
        } else if (0 == pid) {  // child
            char path[PATH_MAX] = {0};
            snprintf(path, sizeof(path) - 1, "./%s", update.program_name);
            char pid_str[OA_MAX_STR_LEN] = {0};
            snprintf(pid_str, sizeof(pid_str) - 1, "%d", getppid());

            if(false/*remain_network*/) {    //network进程不退出，向新进程报告network的pid，共享内存的shmid
                if(process_info[OA_PRO_NETWORK].process_id > 0) {
                    char shmid_str[OA_MAX_STR_LEN] = {0};
                    char s_network[OA_MAX_STR_LEN] = {0};
                    sprintf(shmid_str, "%u;%u;%u", shmid.cmd_shmid, shmid.ret_shmid, shmid.used_shmid);
                    sprintf(s_network, "%u", process_info[OA_PRO_NETWORK].process_id);
                    DEBUG_LOG("execl new program:%s -r -p %s -d %s -n %s -m %s", update.program_name, pid_str, del_old_prog?start_prog_name:"", s_network, shmid_str);
                    execl(path, update.program_name, "-r", "-p", pid_str, "-d", del_old_prog?start_prog_name:"", "-n", s_network, "-m", shmid_str, (char *)0);
                } else {
                    char shmid_str[OA_MAX_STR_LEN] = {0};
                    sprintf(shmid_str, "%u;%u;%u", shmid.cmd_shmid, shmid.ret_shmid, shmid.used_shmid);
                    DEBUG_LOG("execl new program:%s -r -p %s -d %s -m %s", update.program_name, pid_str, del_old_prog?start_prog_name:"", shmid_str);
                    execl(path, update.program_name, "-r", "-p", pid_str, "-d", del_old_prog?start_prog_name:"", "-m", shmid_str, (char *)0);
                }
            } else {
                DEBUG_LOG("execl new program:%s -r -p %s -d %s", update.program_name, pid_str, del_old_prog?start_prog_name:"");
                execl(path, update.program_name, "-r", "-p", pid_str, "-d", del_old_prog?start_prog_name:"", (char *)0);
            }
            return 0;
        } else {
            ERROR_LOG("ERROR: fork error.");
            del_update_and_recover_old(update.config_updated == 1 ? true : false,
                                        update.so_updated_root + update.so_updated_nobody + update.so_updated_command >=1 ? true : false,
                                        NULL);
            last_update_failed = true;
            create_shm = true;/*remain_network ? false : true;*/
            goto again;
        }
    }

    uninit_and_release_config_instance(p_config);
    log_fini();
    return 0;
}
