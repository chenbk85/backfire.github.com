/**
 * =====================================================================================
 *       @file  main.cpp
 *      @brief   
 *
 *  Detailed description starts here.
 *
 *   @internal
 *     Created  09/03/2010 06:31:41 PM 
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason , mason@taomee.com
 *     @author  tonyliu , tonyliu@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <iomanip>
#include <pwd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "proto.h"
#include "db_operator.h"
#include "lib/log.h"
#include "lib/utils.h"
#include "lib/oa_popen.h"
#include "lib/c_mysql_iface.h"
#include "data-process/data_process.h"
#include "network-process/network_process.h"

using namespace std;

#define OPEN_SUID_CORE_DUMP   system("/sbin/sysctl -e -q -w fs.suid_dumpable=1")
#define CLOSE_SUID_CORE_DUMP  system("/sbin/sysctl -e -q -w fs.suid_dumpable=0")
const char* g_red_clr = "\e[1m\e[31m";
const char* g_grn_clr = "\e[1m\e[32m";
const char* g_ylw_clr = "\e[1m\e[33m";
const char* g_end_clr = "\e[m";

///标示这个grid的id
int g_grid_id = -1;
int g_grid_segment = -1;
static proc_status_t g_monitor_status = {PROC_MONITOR, -1, true, true, "", 0};
static proc_status_t g_network_status = {PROC_NETWORK, -1, true, true, "", 0};
static proc_status_t g_collect_status = {PROC_COLLECT, -1, true, true, "", 0};
///SIGTERM
volatile static sig_atomic_t g_got_sig_term = 0;

/**
 * @brief  记录pid到文件中
 * @param   first_open 是否第一次打开
 * @return  -1-failed, 0-success
 */
int record_process_id()
{
    int flag = O_RDWR | O_CREAT | O_TRUNC;
    int ret_code = -1;
    int fd = -1;
    do {
        fd = open(DAEMON_FILE, flag, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
        if (fd < 0) {
            break;
        }

        char record[MAX_STR_LEN] = "#process name\t\t\tpid\t\tcreate time\n";
        if (-1 == write(fd, record, strlen(record))) {
            break;
        }
        char mtime[32] = {0};
        char ntime[32] = {0};
        char ctime[32] = {0};
        tm2str(g_monitor_status.last_create_time, mtime, sizeof(mtime));
        tm2str(g_network_status.last_create_time, ntime, sizeof(ntime));
        tm2str(g_collect_status.last_create_time, ctime, sizeof(ctime));
        snprintf(record, sizeof(record), "%s\t%d\t%s\n%s\t%d\t%s\n%s\t%d\t%s",
                g_monitor_status.proc_name, g_monitor_status.pid, mtime,
                g_network_status.proc_name, g_network_status.pid, ntime,
                g_collect_status.proc_name, g_collect_status.pid, ctime);
        if (-1 == write(fd, record, strlen(record))) {
            break;
        }
        ret_code = 0;
    } while (false);
	
    if (fd >= 0) {
        close(fd);
    }

    return ret_code;
}

static int wait_proc_stop(int fd, proc_status_t *p_proc_status)
{
    assert(NULL != p_proc_status);
    if (p_proc_status->pid < 0 || fd < 0) {
        ERROR_LOG("ERROR: wrong fd[%d] or pid[%d] of process[%s], take it as stopped.",
                fd, p_proc_status->pid, p_proc_status->proc_name);
        return 0;
    }

    pid_t wait_res = -1;
    int status = -1;
    char buff[64] = {0};
    sprintf(buff, "%d", PROC_NEED_STOP);
    DEBUG_LOG("Notice process[%s][%d] to stop[%s]", p_proc_status->proc_name, p_proc_status->pid, buff);
    if (write(fd, buff, strlen(buff)) < 0) {
        ERROR_LOG("ERROR: write(PROC_NEED_STOP) to process[%s][%d] failed[%s].",
                p_proc_status->proc_name, p_proc_status->pid, strerror(errno));
        return -1;
    }
    do {
        wait_res = waitpid(p_proc_status->pid, &status, 0);
    } while (-1 == wait_res && EINTR == errno);
    DEBUG_LOG("Process[%s][%d] exit status[%d].", p_proc_status->proc_name, p_proc_status->pid, status);
    p_proc_status->pid = -1;

    return 0;
}

static int fork_process(int *proc_fd, proc_status_t *p_proc_status, void *p_arg)
{
    assert(NULL != proc_fd && NULL != p_proc_status && NULL != p_arg);
    ///fork process
    char buff[MAX_STR_LEN] = {0};
    //新建前先关闭之前的socket
    FD_CLOSE(proc_fd[0]);
    FD_CLOSE(proc_fd[1]);
    if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, proc_fd)) {
        ERROR_LOG("ERROR: socketpair(...) for process[%s] failed[%s].", p_proc_status->proc_name, strerror(errno));
        return -1;
    }

    int pid = fork();
    if (pid < 0) {
        ERROR_LOG("ERROR: fork() process[%s] failed[%s]", p_proc_status->proc_name, strerror(errno));
        return -1;
    } else if (0 == pid) {//child
        set_proc_title(p_proc_status->proc_name);
        FD_CLOSE(proc_fd[0]);//close parent fd

        switch(p_proc_status->type) {
        case PROC_COLLECT:
        {
            collect_arg_t *p_collect_arg = (collect_arg_t *)p_arg;
            collect_process_run(proc_fd[1], p_collect_arg->p_ds, p_collect_arg->p_config, p_collect_arg->p_db_conf,
                p_collect_arg->p_default_alarm_info, p_collect_arg->p_special_alarm_info);
            break;
        }
        case PROC_NETWORK:
        {
            network_arg_t *p_network_arg = (network_arg_t *)p_arg;
            network_process_run(proc_fd[1], p_network_arg->p_config, p_network_arg->p_ip_port);
            break;
        }
            break;
        default:
            ERROR_LOG("process[%s] type[%d] wrong", p_proc_status->proc_name, p_proc_status->type);
            break;
        }

        ///回收资源
        FD_CLOSE(proc_fd[1]);
        exit(0);
    } else {//parent
        DEBUG_LOG("Monitor process fork [%s][%d] succ.", p_proc_status->proc_name, pid);
        FD_CLOSE(proc_fd[1]);//close parent fd
        p_proc_status->pid = pid;
        p_proc_status->last_create_time = time(NULL);
        int read_len = read(proc_fd[0], buff, sizeof(buff) - 1);
        if (read_len < 0) {
            ERROR_LOG("ERROR: read(...) from child process[%s] failed[%s].",
                    p_proc_status->proc_name, strerror(errno));
            return -1;
        } else if (read_len > 0) {
            buff[read_len] = 0;
            DEBUG_LOG("recv process[%s][%d] cmd_id[%s]", p_proc_status->proc_name, pid, buff);
            int cmd_id = atoi(buff);
            if (PROC_INIT_FAIL == cmd_id) {
                return -1;
            }
        }
    }

    return 0;
}


static void rlimit_reset()
{
    struct rlimit rlim;
    rlim.rlim_cur = RLIM_INFINITY;
    rlim.rlim_max = RLIM_INFINITY;
    //raise open files
    setrlimit(RLIMIT_NOFILE, &rlim);
    //allow core dump
    setrlimit(RLIMIT_CORE, &rlim);
}

static void sigterm_handler(int signo)
{
    g_got_sig_term = 1;
}


static void sighup_handler(int signo)
{
    g_got_sig_term = 1;
}


static void sigchld_handler(int signo, siginfo_t *si, void * p) 
{
    int status;
	while (waitpid (-1, &status, WNOHANG|__WALL) > 0);
#if 0
	char *corename;

	switch (si->si_code) {
		case SI_USER:
		case SI_TKILL:
			DEBUG_LOG("SIGCHLD from pid=%d uid=%d, IGNORED",
					si->si_pid, si->si_uid);
			return; /* someone send use fake SIGCHLD */
		case CLD_KILLED:
			DEBUG_LOG("child %d killed by signal %s",
					si->si_pid, signame[WTERMSIG(si->si_status)]);
			stop = 1;
			if (WTERMSIG(si->si_status) == SIGABRT)
				restart = 1;
			break;
		case CLD_TRAPPED:
			DEBUG_LOG("child %d trapped", si->si_pid);
			return;
		case CLD_STOPPED:
			DEBUG_LOG("child %d stopped", si->si_pid);
			if(si->si_pid > 1) kill(si->si_pid, SIGCONT);
			return;
		case CLD_CONTINUED:
			DEBUG_LOG("child %d continued", si->si_pid);
			return;
		case CLD_DUMPED:
			DEBUG_LOG("child %d coredumped by signal %s",
					si->si_pid, signame[WTERMSIG(si->si_status)]);
			chmod("core", 700);
			chown("core", 0, 0);
			corename = alloca(40);
			sprintf(corename, "core.%d", si->si_pid);
			rename("core", corename);
			restart = 1;
			stop = 1;
			break;
	}
#endif
}

static int daemon_start()
{
	struct sigaction sa;
	sigset_t sset;

	rlimit_reset ();

	memset(&sa, 0, sizeof(sa));
	signal(SIGPIPE,SIG_IGN);	

	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	
	sa.sa_handler = sighup_handler;
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_flags = SA_RESTART|SA_SIGINFO;
	sa.sa_sigaction = sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);
/*	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
*/	
	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
    // struct sigaction sa;
    // memset(&sa, 0, sizeof(sa));
    // rlimit_reset();

    // sa.sa_handler = sig_handler_term;
    // sigaction(SIGINT, &sa, NULL);
    // sigaction(SIGTERM, &sa, NULL); 
    // sigaction(SIGQUIT, &sa, NULL);

    // signal(SIGPIPE, SIG_IGN);
    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGHUP, SIG_IGN);

    daemon(1, 1);
    return 0;
}

static int check_rrd_conf(const char* p_rrd_dir)
{
    //因为在得到rrd路径之前已经setuid为nobody了，所以只能检查一下，不能做任何操作(创建，修改权限等)
    if(strlen(p_rrd_dir) <= 0 || access(p_rrd_dir, F_OK | R_OK | W_OK | X_OK) != 0) {
        ERROR_LOG("ERROR: can not access rrd_rootdir[%s],sys error[%s]", p_rrd_dir, strerror(errno));
        return -1;
    }

    return 0;
}

static bool check_update_status(c_mysql_iface* db_conn)
{
    bool is_running = false;
    if(0 != get_process_status(g_collect_status.proc_name, &is_running)) {
        ERROR_LOG("ERROR: get collect process[%s] status failed.", g_collect_status.proc_name);
        g_collect_status.is_running = true;
    } else {
        g_collect_status.is_running = is_running;
    }
    g_collect_status.is_update = db_get_update_status(db_conn, PROC_COLLECT);

    if(0 != get_process_status(g_network_status.proc_name, &is_running)) {
        ERROR_LOG("ERROR: get network process[%s] status failed.", g_network_status.proc_name);
        g_network_status.is_running = true;
    } else {
        g_network_status.is_running = is_running;
    }
    g_network_status.is_update = db_get_update_status(db_conn, PROC_NETWORK);

    //debug log
    if (!g_collect_status.is_running) {
        DEBUG_LOG("Process Status: collect process[%d] is not running", g_collect_status.pid);
    }
    if (!g_network_status.is_running) {
        DEBUG_LOG("Process Status: network process[%d] is not running", g_network_status.pid);
    }
    if (g_collect_status.is_update) {
        DEBUG_LOG("Process Status: collect process[%d] config be updated", g_collect_status.pid);
    }
    if (g_network_status.is_update) {
        DEBUG_LOG("Process Status: network process[%d] config be updated", g_network_status.pid);
    }

    bool final_status = g_collect_status.is_update || !g_collect_status.is_running ||
                        g_network_status.is_update || !g_network_status.is_running;

    return final_status;
}

static int set_update_status(c_mysql_iface* db_conn, int proc_type)
{
    return db_set_update_status(db_conn, proc_type);
}

static int get_segment_info(c_mysql_iface* db_conn)
{
    g_grid_segment = db_get_segment(db_conn);
    return g_grid_segment >= 0 ? 0 : -1;
}

static int get_network_conf(c_mysql_iface* db_conn, network_conf_t *p_network_cfg)
{
    network_conf_t tmp_cfg;
    memcpy(&tmp_cfg, p_network_cfg, sizeof(tmp_cfg));

    int ret = db_get_network_conf(db_conn, &tmp_cfg);
    if(ret == 0) {
        memcpy(p_network_cfg, &tmp_cfg, sizeof(tmp_cfg));
    }

    return ret;
}

static int get_collect_conf(c_mysql_iface* db_conn, collect_conf_t *p_collect_cfg)
{
    collect_conf_t tmp_cfg;
    memcpy(&tmp_cfg, p_collect_cfg, sizeof(tmp_cfg));

    int ret = db_get_collect_conf(db_conn, &tmp_cfg);
    if(ret == 0) {
        memcpy(p_collect_cfg, &tmp_cfg, sizeof(tmp_cfg));
    }

    return ret;
}

static int get_ip_port_info(ip_port_map_t *p_ip_port, c_mysql_iface* db_conn)
{
    ip_port_map_t ip_port;

    int ret = db_get_ip_port(&ip_port, db_conn);
    if(ret == 0) {
        p_ip_port->clear();
        *p_ip_port = ip_port;
    }

    return ret;
}

static int get_data_source(ds_t *p_ds, c_mysql_iface* db_conn)
{
    map<unsigned int, data_source_list_t> tmp_ds;
    int ret = db_get_data_source(&tmp_ds, db_conn);
    if(ret == 0) {
        //清空原来保存数据源的map
        p_ds->clear();
        *(p_ds) = tmp_ds;
    }

    tmp_ds.clear();
    return ret;
}

static int get_metric_alarm_info(
        metric_alarm_vec_t *p_default_metric_alarm_info,
        metric_alarm_map_t *p_special_alarm_info,
        c_mysql_iface *db_conn) 
{
    metric_alarm_vec_t tmp_default_metric_set;
    int ret1 = db_get_default_metric_alarm_info(db_conn, &tmp_default_metric_set);

    metric_alarm_map_t tmp_specified_metric_set;
    int ret2 = db_get_special_metric_alarm_info(db_conn, &tmp_specified_metric_set);

    if (0 == ret1 && 0 == ret2) {   
        ///清空原来保存metric alarm的vector
        p_default_metric_alarm_info->clear();
        *p_default_metric_alarm_info = tmp_default_metric_set;

        ///清空原来的map
        metric_alarm_map_t::iterator it = p_special_alarm_info->begin();
        for(; it != p_special_alarm_info->end(); it++) {
            metric_alarm_vec_t *tmp = it->second;
            if(tmp) {
                tmp->clear();
                delete tmp;
            }
        }
        p_special_alarm_info->clear();
        *p_special_alarm_info = tmp_specified_metric_set;
    }   

    if (0 != ret2) {
        metric_alarm_map_t::iterator it = tmp_specified_metric_set.begin();
        for(; it != tmp_specified_metric_set.end(); it++) {
            metric_alarm_vec_t *tmp = it->second;
            if(tmp) {
                tmp->clear();
                delete tmp;
            }
        }
    }

    tmp_default_metric_set.clear();
    tmp_specified_metric_set.clear();

    return (ret1 | ret2);
}

/**
 * @brief  检查进程是否已经停止
 * @param   proc_name 进程名
 * @return  false-在运行或检查失败, true-不在运行
 */
static bool is_proc_stopped(const char *proc_name)
{
    bool is_running = false;
    if(0 != get_process_status(proc_name, &is_running)) {
        fprintf(stderr, "%sGet process[%s] status.%40s%s\n", g_red_clr, proc_name, "[failed]", g_end_clr);
        return false; 
    }   
    if (is_running) {
        fprintf(stderr, "%sProcess[%s] is still running, pls stop it first.%40s%s\n",
                g_ylw_clr, proc_name, "[Warning]", g_end_clr);
        return false; 
    }

    return true;
}

static int set_running_environment()
{
    //检查进程是否有在运行的
    bool is_all_stop = true;
    char proc_name[MAX_STR_LEN] = {0};
    snprintf(proc_name, sizeof(proc_name), "%s%d", PRE_NAME_MONITOR, g_grid_id);
    is_all_stop = is_proc_stopped(proc_name) && is_all_stop;
    snprintf(proc_name, sizeof(proc_name), "%s%d", PRE_NAME_NETWORK, g_grid_id);
    is_all_stop = is_proc_stopped(proc_name) && is_all_stop;
    snprintf(proc_name, sizeof(proc_name), "%s%d", PRE_NAME_COLLECT, g_grid_id);
    is_all_stop = is_proc_stopped(proc_name) && is_all_stop;
    if (!is_all_stop) {
        return -1; 
    }

    //将所有的环境变量设为本地环境变量
    setlocale(LC_ALL, "");

    //设置日志目录
    char log_dir[MAX_STR_LEN] = "../log";
    if(access(log_dir, F_OK) != 0 && 
            mkdir(log_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        fprintf(stderr, "Log dir[%s] does not exist, but create it failed,sys error:%s.\n",
                log_dir, strerror(errno));
        return -1;
    } else {
        chmod(log_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

    //把bin目录交给nobody,这个工作在安装head程序的时候就已经做过，但是为了可靠性在程序里再做一次
    struct passwd *pw = NULL;
    pw = getpwnam("nobody");
    if(pw) {
        chown("../bin", pw->pw_uid, pw->pw_gid);
        chown("../bin/oa_head", pw->pw_uid, pw->pw_gid);
        chown(log_dir, pw->pw_uid, pw->pw_gid);
    }

    //设置setuid的core dump标志位
    OPEN_SUID_CORE_DUMP;


    //setuid to nobody
    if(0 != set_user("nobody")) {
        fprintf(stderr, "%sSet uid to nobody.%70s%s\n", g_red_clr, "[failed]", g_end_clr);
        //CLOSE_SUID_CORE_DUMP;
        return -1;
    }
    fprintf(stdout, "%sGoing to run as \"nobody\"%70s%s.\n", g_grn_clr, "[succ]", g_end_clr);

    return 0;
}

static int get_and_check_argument(int argc, char *argv[], db_conf_t *p_db_conf)
{
    //配置库的配置项
    int grid_id = -1;
    char listen_ip[16] = {0};
    char db_host[16] = {0};
    char db_name[64] = "db_itl";
    unsigned short db_port = 3306;
    char db_user[256] = {0};
    char db_pass[256] = {0};
    int ch = 0;

    //处理命令行参数
    while(-1 != (ch = getopt(argc, argv, "l:n:d:h:p:u:P:"))) {   
        switch(ch) {   
        case 'l': //listen ip
            if(strlen(optarg) >= sizeof(listen_ip) - 1) {
                fprintf(stderr, "Wrong listen ip[%s].\n", optarg);
                return -1;
            }
            strncpy(listen_ip, optarg, sizeof(listen_ip));
            break;
        case 'n':  //grid id
            if(strlen(optarg) <= 0 || !is_integer(optarg)) {
                fprintf(stderr, "Wrong grid_id[%s].", optarg);
                return -1;
            }
            grid_id = atoi(optarg);
            break;
        case 'd':  //db name 
            strncpy(db_name, optarg, sizeof(db_name) - 1);
            break;
        case 'h':  //db host
            strncpy(db_host, optarg, sizeof(db_host) - 1);
            break;
        case 'P':  //db port
            db_port = atoi(optarg) <= 0 ? 3306 : atoi(optarg);
            break;
        case 'u':  //db user 
            strncpy(db_user, optarg, sizeof(db_user) - 1);
            break;
        case 'p':  //db passwd
            strncpy(db_pass, optarg, sizeof(db_pass) - 1);
            break;
        default:
            break;
        }
    }

    if(0 == strlen(db_user) || 0 == strlen(db_pass) || grid_id < 0) {
        fprintf(stderr, "%sCritical: No %s %s %s argument.%s\n",
                g_red_clr,
                strlen(db_user) == 0 ? "db_user" : "",
                strlen(db_pass) == 0 ? "db_passwd" : "",
                grid_id < 0 ? "grid_id" : "",
                g_end_clr);
        fprintf(stderr, "\n%sUsage:oa_head -u<db_user> -p<db_passwd> -n<grid_id> [-d<db_name>] [-h<db_host>] [-P<db_port>] [-l<listen_ip>]%s\n", g_grn_clr, g_end_clr);
        return -1;
    }

    if(0 == strlen(listen_ip) && 0 != get_local_ip_str(listen_ip, sizeof(listen_ip))) {
        fprintf(stderr, "%sNo listen ip argument, and got local host ip failed%70s%s.\n",
                g_ylw_clr, "[Warning]", g_end_clr);
        return -1;
    }

    //strcpy(p_config->listen_ip, listen_ip);
    //p_config->grid_id = g_grid_id;
    g_grid_id = grid_id;
    strcpy(p_db_conf->db_host, db_host);
    p_db_conf->db_port = db_port;
    strcpy(p_db_conf->db_name, db_name);
    strcpy(p_db_conf->db_user, db_user);
    strcpy(p_db_conf->db_pass, db_pass);

    return 0;
}

int main(int argc, char **argv)
{
    ///配置项结构赋初值
    //[collect]grid_id,grid_name,summary_interval,queue_len,alarm_interval,alarm_server_url,rrd_dir
    collect_conf_t collect_cfg = {0, "", 20, 4096000, 10, "", ""};
    //[network]listen_ip,listen_port,trust_host,noti_url,network_thread_cnt
#ifdef _DEBUG
    network_conf_t network_cfg = {"", 0, "127.0.0.0",  "http://10.1.1.27/violet_project/itl/webroot/interface.php", 4};
#else
    network_conf_t network_cfg = {"", 0, "127.0.0.0",  "http://itl.taomee.com/interface.php", 4};
#endif

    ///数据库配置信息
    db_conf_t  db_cfg;
    memset(&db_cfg, 0, sizeof(db_cfg));
    if (0 != get_and_check_argument(argc, argv, &db_cfg)) {
        return -1;
    }

    // if (0 != set_running_environment()) {
        // return -1;
    // }

    ///初始化日志
    if(0 != proc_log_init(PRE_NAME_MONITOR)) {
        fprintf(stderr, "%sInit log%70s%s\n", g_red_clr, "[failed]", g_end_clr);
        return -1;
    }

    ///初始化popen模块
    if(0 != oa_popen_init()) {
        fprintf(stderr, "%soa_popen_init()%70s%s\n", g_red_clr, "[failed]", g_end_clr);
        return -1;
    }

    fprintf(stdout, "%sOA_HEAD: version: %s build time: %s %s%s\n",
            g_grn_clr, VERSION, __DATE__, __TIME__, g_end_clr);

    daemon_start();

#ifdef _DEBUG
    //Nothing to do
    DEBUG_LOG("TO DEBUG: Need not redirect to \"/dev/null\"");
#else
    ///重定向标准输入输出到/dev/null
    int null_fd = open("/dev/null", O_RDWR);
    if(-1 == null_fd) {
        oa_popen_uninit();
        return -1;
    }
    dup2(null_fd, 0);
    dup2(null_fd, 1);
    dup2(null_fd, 2);
    close(null_fd);
#endif

    c_mysql_iface *p_db_conn = NULL; /**<数据库连接*/
    if(0 != create_mysql_iface_instance(&p_db_conn)) {
        ERROR_LOG("ERROR: Create mysql connect instance failed.");
        oa_popen_uninit();
        return -1;
    }
    if(0 != p_db_conn->init(db_cfg.db_host, db_cfg.db_port, db_cfg.db_name,
                db_cfg.db_user, db_cfg.db_pass, "utf8")) {
        ERROR_LOG("ERROR: Connect to db[%s] of host[%s] failed.", db_cfg.db_name, db_cfg.db_host);
        oa_popen_uninit();
        p_db_conn->release();
        return -1;
    }

    metric_alarm_vec_t default_alarm_info; /**<缺省报警配置*/
    metric_alarm_map_t special_alarm_info; /**<特殊报警配置*/
    ds_t data_sources;
    pid_t pid = -1;
    int fd_net[2] = {-1, -1};
    int fd_data[2] = {-1, -1};
    //初始化设置proc标题
    init_proc_title(argc, argv);

    ip_port_map_t ip_port_ins;
    char buff[MAX_STR_LEN] = {0};
    char grid_name[MAX_STR_LEN] = {0};
    g_monitor_status.pid = getpid();
    g_monitor_status.last_create_time = time(NULL);

    network_arg_t network_arg = {&network_cfg, &ip_port_ins};
    collect_arg_t collect_arg = {&data_sources, &collect_cfg, &db_cfg, &default_alarm_info, &special_alarm_info};

restart:
    //从数据库获取网段信息
    // 目前只是检查g_grid_id是否存在
    if(0 != get_segment_info(p_db_conn)) {
        goto starterror;
    }
    snprintf(grid_name, sizeof(grid_name), "%d_%d", g_grid_id, g_grid_segment);
    //设置主进程名
    snprintf(g_monitor_status.proc_name, sizeof(g_monitor_status.proc_name), "%s%s", PRE_NAME_MONITOR, grid_name);
    set_proc_title(g_monitor_status.proc_name);

    //collect process配置有变更
    if (g_collect_status.is_update) {
        //从数据库获取配置信息
        if(0 != get_collect_conf(p_db_conn, &collect_cfg)) {
            goto starterror;
        }
        //检查rrd路径的有效性
        if(0 != check_rrd_conf(collect_cfg.rrd_dir)) {
            goto starterror;
        }
        //获取数据源
        if(0 != get_data_source(&data_sources, p_db_conn)) {
            goto starterror;
        }
        //获取告警信息
        if(0 != get_metric_alarm_info(&default_alarm_info, &special_alarm_info, p_db_conn)) {
            goto starterror;
        }
        //获取完配置信息后将更新标志位置为false
        if(0 != set_update_status(p_db_conn, PROC_COLLECT)) {
            ERROR_LOG("Set the collect process update status of grid[%s] failed.", grid_name);
        }
        snprintf(g_collect_status.proc_name, sizeof(g_collect_status.proc_name), "%s%s", PRE_NAME_COLLECT, grid_name);
    }

    //network process配置有变更
    if (g_network_status.is_update) {
        if(0 != get_network_conf(p_db_conn, &network_cfg)) {
            goto starterror;
        }
        if(0 != get_ip_port_info(&ip_port_ins, p_db_conn)) {
            goto starterror;
        }
        if(0 != set_update_status(p_db_conn, PROC_NETWORK)) {
            ERROR_LOG("Set the network process update status of grid[%s] failed.", grid_name);
        }
        snprintf(g_network_status.proc_name, sizeof(g_network_status.proc_name), "%s%s", PRE_NAME_NETWORK, grid_name);
    }

    ///fork network process
    if (g_network_status.is_update || !g_network_status.is_running) {
        if (0 != fork_process(fd_net, &g_network_status, (void *)&network_arg)) {
            ERROR_LOG("fork_process(...) for network process failed.");
            goto starterror;
        }
    }

    ///fork data collect process
    if (g_collect_status.is_update || !g_collect_status.is_running) {
        if (0 != fork_process(fd_data, &g_collect_status, (void *)&collect_arg)) {
            ERROR_LOG("fork_process(...) for collect process failed.");
            goto starterror;
        }
    }

    record_process_id();
    ///主进程循环
    while (!g_got_sig_term) {
        unsigned sec = 0;
        while (!g_got_sig_term && sec++ < REINIT_INTERVAL) {
            sleep(1);//等待接收信号中断
        }

        //SIGTERM信号
        if (g_got_sig_term) {
            DEBUG_LOG("Got SIGTERM. Will exiting....");
            break;
        }

        //如果更新标志位为false,继续睡眠,否则重启
        if (!check_update_status(p_db_conn)) {
            continue; //继续睡眠
        } else {
            if (g_collect_status.is_update && 0 != wait_proc_stop(fd_data[0], &g_collect_status)) {
                break;
            }
            if (g_network_status.is_update && 0 != wait_proc_stop(fd_net[0], &g_network_status)) {
                break;
            }
            goto restart;
        }
    }
    wait_proc_stop(fd_net[0], &g_network_status);
    wait_proc_stop(fd_data[0], &g_collect_status);

    //出错或正常退出
starterror:
    if(!data_sources.empty()) {
        data_sources.clear();
    }

    metric_alarm_map_t::iterator it = special_alarm_info.begin();
    for(; it != special_alarm_info.end(); it++) {
        metric_alarm_vec_t *tmp = it->second;
        if(tmp) {
            tmp->clear();
            delete tmp;
        }
    }
    special_alarm_info.clear();
    default_alarm_info.clear();
    ip_port_ins.clear();

    p_db_conn->uninit();
    p_db_conn->release();
    oa_popen_uninit();

    FD_CLOSE(fd_net[0]);
    FD_CLOSE(fd_net[1]);
    FD_CLOSE(fd_data[0]);
    FD_CLOSE(fd_data[1]);

    //反初始化设置proc标题
    uninit_proc_title();
    //CLOSE_SUID_CORE_DUMP;

    return 0;
}
