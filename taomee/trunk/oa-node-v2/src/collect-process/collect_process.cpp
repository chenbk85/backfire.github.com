/**
 * =====================================================================================
 *       @file  collect_process.cpp
 *      @brief
 *
 *     Created  2011-11-24 09:42:28
 *    Revision  1.0.0.0
 *    Compiler  g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2011, TaoMee.Inc, ShangHai.
 *
 *     @author  ping, ping@taomee.com
 * =====================================================================================
 */

#include <errno.h>
#include <dlfcn.h>
#include <utility>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/prctl.h>
#include <sys/types.h>

#include "./collect_thread.h"
#include "./processor_thread.h"
#include "./collect_process.h"
#include "../lib/log.h"
#include "../lib/queue.h"
#include "../proto.h"
#include "../lib/utils.h"

#define WAIT_BEFORE_EXIT (10)

const static time_t g_start_time = time(NULL);
volatile static sig_atomic_t g_got_sig_term = 0;
static bool last_update_failed = false;

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

/**
 * @brief  卸载所有的so，并做相应的清理工作
 * @param  p_proto_vec 保存所有so信息的vector的指针
 * @return 0:success, -1:failed
 */
static int unload_so(proto_vec_t *p_proto_vec)
{
    if(p_proto_vec == NULL) {
        ERROR_LOG("p_proto_vec=%p", p_proto_vec);
        return -1;
    }
    proto_vec_t::iterator iter = p_proto_vec->begin();
    for(; iter != p_proto_vec->end(); iter++) {
        iter->proto_uninit();
        dlclose((iter->handle));
        iter->handle = NULL;
    }

    return 0;
}

/**
 * @brief  获得心跳值的回调函数
 * @param  index: 回调函数定义的接口，这里不需要使用
 * @param  index: 回调函数定义的接口，这里不需要使用
 * @return 程序启动的时间作为心跳值
 */
static value_t heartbeat_func(int index, const char *arg)
{
    value_t val;
    val.value_uint = g_start_time;

    return val;
}

/**
 * @brief  加载指定的so，并保存相应的so信息在map里面
 * @param  so_file: 带相对路径的so文件的文件名
 * @param  p_collect_info_vec: 保存so可以收集的metric信息的vector的指针
 * @param  p_proto_vec: 保存所有so信息的vector的指针
 * @return 0:success, -1:failed
 */
static int register_so(i_config *p_config,
                       const char *so_file,
                       collect_info_vec_t *p_collect_info_vec,
                       proto_vec_t *p_proto_vec,
                       int *p_index,
                       uint32_t user
                      )
{
    if (NULL == p_config || NULL == so_file || NULL == p_collect_info_vec || NULL == p_proto_vec || user >= sizeof(g_user_list)/sizeof(g_user_list[0])) {
        ERROR_LOG("p_config=%p so_file=%p p_collect_info_vec=%p p_proto_vec=%p user=%u[>%lu]", p_config, so_file, p_collect_info_vec, p_proto_vec, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    proto_so_t proto_so;
    char *error = NULL;
    proto_so.handle = dlopen(so_file, RTLD_NOW);
    if(!proto_so.handle) {
        ERROR_LOG("dlopen %s failed:%s", so_file, dlerror());
        return -1;
    }
    dlerror();
    *(void**)(&proto_so.proto_init) = dlsym(proto_so.handle, "proto_init");
    if((error = dlerror()) != NULL) {
        ERROR_LOG("dlsym failed:%s",error);
        return -1;
    }
    *(void**)(&proto_so.proto_uninit) = dlsym(proto_so.handle, "proto_uninit");
    if((error = dlerror()) != NULL) {
        ERROR_LOG("dlsym failed:%s",error);
        return -1;
    }
    *(void**)(&proto_so.proto_handler) = dlsym(proto_so.handle, "proto_handler");
    if((error = dlerror()) != NULL) {
        ERROR_LOG("dlsym failed:%s",error);
        return -1;
    }
    *(void**)(&proto_so.get_proto_info) = dlsym(proto_so.handle, "get_proto_info");
    if((error = dlerror()) != NULL) {
        ERROR_LOG("dlsym failed:%s",error);
        return -1;
    }
    if(proto_so.proto_init(p_config) != 0) {
        ERROR_LOG("so %s inited failed",so_file);
        return -1;
    }
    // 获得so里返回的信息
    metric_info_t metric_info[OA_MAX_METRICS_PER_SO] = {{0}};
    int metric_num = 0;
    bool is_custom_so = 0;
    proto_so.get_proto_info(metric_info, &metric_num, &is_custom_so);
    if (metric_num > OA_MAX_METRICS_PER_SO) {
        ERROR_LOG("ERROR: so %s handle more than %d metrics.", so_file, metric_num);
        return -1;
    }
    if (metric_num <= 0) {
        ERROR_LOG("ERROR: so %s handle %d metrics.", so_file, metric_num);
        return -1;
    }
    p_proto_vec->push_back(proto_so);
    // 把so返回的每一个metric加入m_collect_info_vec里面
    for (int i = 0; i < metric_num; ++i) {
        collect_info_t collect_info = {{0}};

        memcpy(&collect_info.metric_info, &metric_info[i], sizeof(metric_info_t));
        collect_info.index = i;
        collect_info.proto_handler = p_proto_vec->back().proto_handler;

        int metric_id = 0;
        if (!is_custom_so) {
            metric_id = ++(*p_index);
        } else {
            metric_id = 0;
        }
        collect_info.metric_id = metric_id;

        p_collect_info_vec->push_back(collect_info);
    }

    return 0;
}

#ifdef GET_CONFIG
#undef GET_CONFIG
#endif
#define GET_CONFIG(section, name, buf)\
do{\
     if(p_config->get_config(section, name, buf, sizeof(buf)) != 0) {\
         ERROR_LOG("ERROR: get [%s]:[%s] failed.", section, name);\
         return -1;\
     }\
 } while(0)

/**
 * @brief  加载所有需要的so，并做相应的初始化工作
 * @param  p_config: 指向配置类的指针
 * @param  p_collect_info_vec: 保存so可以收集的metric信息的vector的指针
 * @param  p_metric_info_map:  保存每个metric的id到详细信息的对于表
 * @param  p_proto_vec: 保存所有so信息的vector的指针
 * @return 0:success, -1:failed
 */
static int load_proto_so(i_config *p_config,
                         collect_info_vec_t *p_collect_info_vec,
                         metric_info_map_t *p_metric_info_map,
                         proto_vec_t *p_proto_vec,
                         uint32_t user
                        )
{
    if (NULL == p_config || NULL == p_collect_info_vec || NULL == p_metric_info_map || NULL == p_proto_vec || user >= sizeof(g_user_list)/sizeof(g_user_list[0])) {
        ERROR_LOG("p_config=%p p_collect_info_vec=%p p_metric_info_map=%p p_proto_vec=%p user=%u[>%lu]", p_config, p_collect_info_vec, p_metric_info_map, p_proto_vec, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    p_collect_info_vec->clear();
    p_metric_info_map->clear();
    p_proto_vec->clear();

    char so_num_str[10] = {0};
    if(user == OA_USER_ROOT) {
        //GET_CONFIG("node_info", "root_so_num", so_num_str);
        GET_CONFIG("node_info", "so_num", so_num_str);
    } else {
        GET_CONFIG("node_info", "so_num", so_num_str);
    }
    int so_num = atoi(so_num_str);
    if (so_num <= 0) {
        ERROR_LOG("ERROR: so_num: %d", so_num);
        return -1;
    }

    char so_name[PATH_MAX] = {0};
    char so_name_full[PATH_MAX] = {0};
    int index = 0;
    for (int i = 0; i < so_num; ++i) {
        char name[OA_MAX_STR_LEN] = {0};

        if(user == OA_USER_ROOT) {
            //snprintf(name, sizeof(name) - 1, "root_so_name_%d", i + 1);
            snprintf(name, sizeof(name) - 1, "so_name_%d", i + 1);
            //GET_CONFIG("root_so_name", name, so_name);
            GET_CONFIG("so_name", name, so_name);
        } else {
            snprintf(name, sizeof(name) - 1, "so_name_%d", i + 1);
            GET_CONFIG("so_name", name, so_name);
        }

        sprintf(so_name_full, "%s%s", OA_SO_PATH, so_name);

        if (access(so_name_full, F_OK | R_OK) == -1) {
            ERROR_LOG("ERROR: proto name [%s] must exist and have F_OK|R_OK permission, reason: %s.",
                        so_name_full, strerror(errno));
            return -1;
        }

        if (register_so(p_config, so_name_full, p_collect_info_vec, p_proto_vec, &index, user) == -1) {
            ERROR_LOG("ERROR: register_so(%s).", so_name_full);
            return -1;
        }
    }
    // 在收集信息里加入发送心跳信息,其id为-1
    collect_info_t collect_info = {{0}};
    collect_info.metric_info.name = "heartbeat";
    collect_info.metric_id = -1;
    collect_info.metric_info.type = OA_VALUE_UNSIGNED_INT;
    collect_info.proto_handler = heartbeat_func;
    p_collect_info_vec->push_back(collect_info);
    return 0;
}

/**
 * @brief  初始化所有的模块和线程
 * @param  pp_config: 指向配置类的指针
 * @param  p_collect_info_vec: 保存so可以收集的metric信息的vector的指针
 * @param  p_metric_info_map:  保存每个metric的id到详细信息的对于表
 * @param  p_proto_vec: 保存所有so信息的vector的指针
 * @param  p_collect_thread: 指向collect线程实例的指针
 * @param  p_listen_thread: 指向listen线程实例的指针
 * @param  p_export_thread: 指向export线程实例的指针
 * @param  p_update_thread: 指向export线程实例的指针
 * @return 0:success, -1:failed
 */
static int init_all_modules(i_config *p_config,
                            collect_info_vec_t *p_collect_info_vec,
                            metric_info_map_t *p_metric_info_map,
                            proto_vec_t *p_proto_vec,
                            c_collect_thread *p_collect_thread,
                            uint32_t user
                           )
{
    if (NULL == p_config || NULL == p_collect_info_vec || NULL == p_metric_info_map || NULL == p_proto_vec ||
        NULL == p_collect_thread || user >= sizeof(g_user_list)/sizeof(g_user_list[0])) {
        ERROR_LOG("p_config=%p p_collect_info_vec=%p p_metric_info_map=%p p_proto_vec=%p user=%u[>%lu]", p_config, p_collect_info_vec, p_metric_info_map, p_proto_vec, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    if (load_proto_so(p_config, p_collect_info_vec, p_metric_info_map, p_proto_vec, user) != 0) {
        ERROR_LOG("ERROR: load_proto_so() failed.");
        return -1;
    }

    collect_info_vec_t::iterator collect_iter = p_collect_info_vec->begin();
    for (; collect_iter != p_collect_info_vec->end(); ++collect_iter) {
        DEBUG_LOG("metric_id:%d, name:%s", collect_iter->metric_id, collect_iter->metric_info.name);
    }

    DEBUG_LOG("last_update_failed:%s in main", last_update_failed ? "TRUE" : "FALSE");

    char inside_ip[16];
    if(get_ip(p_config, inside_ip) == NULL) {
        unload_so(p_proto_vec);
        ERROR_LOG("get ip error");
        return -1;
    }

    bool is_recv_udp;
    if (set_recv_udp(p_config, &is_recv_udp) != 0) {
        ERROR_LOG("ERROR: set_recv_udp() failed.");
        return -1;
    }

    if (p_collect_thread->init(p_config, inside_ip, p_collect_info_vec, p_metric_info_map, g_start_time, last_update_failed, is_recv_udp, user) != 0) {
        ERROR_LOG("ERROR: collect thread init().");
        unload_so(p_proto_vec);
        return -1;
    }

    return 0;
}

#undef GET_CONFIG

/**
 * @brief  反初始化所有的模块和线程
 * @param  p_config: 指向配置类的指针
 * @param  p_proto_vec: 保存所有so信息的vector的指针
 * @param  p_collect_thread: 指向collect线程实例的指针
 * @param  p_listen_thread: 指向listen线程实例的指针
 * @param  p_export_thread: 指向export线程实例的指针
 * @param  p_update_thread: 指向export线程实例的指针
 * @return 0:success, -1:failed
 */
static inline void uninit_all_modules(proto_vec_t *p_proto_vec,
                                      c_collect_thread *p_collect_thread
                                     )
{
    if(p_proto_vec == NULL || p_collect_thread == NULL) {
        ERROR_LOG("p_proto_vec=%p p_collect_thread=%p", p_proto_vec, p_collect_thread);
        return ;
    }
    p_collect_thread->uninit();
    unload_so(p_proto_vec);
}

int collect_run(int argc, char **argv, void * p_arg)
{
    if(p_arg == NULL) {
        ERROR_LOG("p_arg=%p", p_arg);
        return -1;
    }
    collect_arg_t * arg = (collect_arg_t *)p_arg;
    i_config * p_config = *(arg->config);
    uint32_t user = arg->user;
    if(user >= sizeof(g_user_list)/sizeof(g_user_list[0]) || p_config == NULL) {
        ERROR_LOG("p_config=%p user=%u[>%lu]", p_config, user, sizeof(g_user_list)/sizeof(g_user_list[0]));
        return -1;
    }

    set_proc_name(argc, argv, "%s_collect", OA_BIN_FILE_SAMPLE);
    mysignal(SIGTERM, signal_handler);
    if(user == OA_USER_NOBODY) {
        write_pid_to_file(OA_COLLECT_N_FILE);
        DEBUG_LOG("im nobody");
    } else if(user == OA_USER_ROOT) {
        write_pid_to_file(OA_COLLECT_R_FILE);
        DEBUG_LOG("im root");
    }

    if(user != OA_USER_ROOT && 0 != change_user(g_user_list[user])) {
        ERROR_LOG("ERROR: change to %s failed.", g_user_list[user]);
        return -1;
    }

    c_collect_thread collect_thread;

    collect_info_vec_t collect_info_vec;
    metric_info_map_t metric_info_map;      /**<内置类型<id, base_info>映射表，传递给export线程*/
    proto_vec_t proto_vec;

    int ret = init_all_modules(p_config, &collect_info_vec, &metric_info_map, &proto_vec, &collect_thread, user);
    last_update_failed = false;
    if (ret != 0) {
        ERROR_LOG("ERROR: init all modules.");
        if(user == OA_USER_NOBODY) {
            kill(getppid(), SIGUSR1);
            ERROR_LOG("kill(%u, %u) collect-nobody exit.", getppid(), SIGUSR1);
            sleep(1);
            return 0;
        } else if(user == OA_USER_ROOT) {
            kill(getppid(), SIGUSR2);
            ERROR_LOG("kill(%u, %u) collect-root exit.", getppid(), SIGUSR2);
            sleep(1);
            return 0;
        }
    }

    DEBUG_LOG("init_all_modules return %d", ret);

    while(g_got_sig_term == 0) {//todo  可以处理monitor发过来的信号等
#if 0
        if(user != OA_USER_ROOT && p_command_q != NULL && p_return_q != NULL) {
            sprintf(command.command_str, "./so.o;do_command(%u)", command.command_id);
            ret = p_command_q->push(command);
            if(ret != 0) {
                //返回初始化错误
                return_val.command_id = command.command_id;
                return_val.return_val = OA_CMD_RET_QUEUE_OP_ERROR;
                send_cmd_return_val(&return_val);
                continue;
            }
            command.command_id++;
            sleep(1);
            if(!p_return_q->empty()) {
                p_return_q->pop(&return_val);
                send_cmd_return_val(&return_val);
            }
        }
#endif
       sleep(1);
    }

    DEBUG_LOG("%d ready to exit", getpid());

    collect_thread.send_special_metric_info(OA_CLEANUP_TYPE);
    uninit_all_modules(&proto_vec, &collect_thread);

    return 0;
}

void set_last_update(bool b_last_update)
{
    last_update_failed = b_last_update;
}
