/* vim: set tabstop=4 softtabstop=4 shiftwidth=4: */
/**
 * @file utils.cpp
 * @author richard <richard@taomee.com>
 * @date 2010-03-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <ifaddrs.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/mman.h>
#include <openssl/md5.h>
#include <pwd.h>

#include "./log.h"
#include "../proto.h"
#include "../i_config.h"
#include "utils.h"

int mysignal(int sig, void(*signal_handler)(int))
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    return sigaction(sig, &act, 0);
}

int write_pid_to_file(const char * filename)
{
    int fd = -1;
    char buf[16] = {0};
    fd = open(filename, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

    if (ftruncate(fd, 0) != 0) {
        close(fd);
        return -1;
    }
    sprintf(buf, "%d", (int)getpid());
    if (write(fd, buf, strlen(buf)+1) == -1) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int create_file(const char * filename)
{
    int fd;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        ERROR_LOG("ERROR %s: create %s", strerror(errno), filename);
        return -1;
    } else {
        close(fd);
        return 0;
    }
}

extern char **environ;
static int prog_argc = -1;
static char **pp_prog_argv = NULL;
static char *p_prog_last_argv = NULL;

void init_proc_title(int argc, char *argv[])
{
	char **old_environ = environ;

	int environ_count = 0;
	while (old_environ[environ_count] != NULL) {
		++environ_count;
	}

	int i = 0;
	char **p = NULL;
	if ((p = (char **) malloc((environ_count + 1) * sizeof(char *))) != NULL) {
		for (i = 0; old_environ[i] != NULL; i++) {
			size_t envp_len = strlen(old_environ[i]);

			p[i] = (char *)malloc(envp_len + 1);
			if (p[i] != NULL) {
				strncpy(p[i], old_environ[i], envp_len + 1);
			}
		}

		p[i] = NULL;
		environ = p;
	}

	pp_prog_argv = argv;
	prog_argc = argc;

	for (i = 0; i < prog_argc; ++i) {
		if (!i || (p_prog_last_argv + 1 == argv[i])) {
			p_prog_last_argv = argv[i] + strlen(argv[i]);
		}
	}

	for (i = 0; old_environ[i] != NULL; ++i) {
		if ((p_prog_last_argv + 1) == old_environ[i]) {
			p_prog_last_argv = old_environ[i] + strlen(old_environ[i]);
		}
	}
}

void uninit_proc_title()
{
	if (environ) {
		unsigned int i;

		for (i = 0; environ[i] != NULL; ++i) {
			free(environ[i]);
		}
		free(environ);
		environ = NULL;
	}
}

void set_proc_title(const char *fmt, ...)
{
	int maxlen = (p_prog_last_argv - pp_prog_argv[0]) - 2;

    printf("maxlen: %d\n", maxlen);

	if (fmt == NULL) {
		return;
	}

	va_list msg;
	va_start(msg, fmt);
	memset(pp_prog_argv[0], 0, maxlen);
	vsnprintf(pp_prog_argv[0], maxlen, fmt, msg);
    printf("pp_prog_argv:%s", pp_prog_argv[0]);
	va_end(msg);

	int i = 0;
	for (i = 1; i < prog_argc; ++i) {
		pp_prog_argv[i] = NULL;
	}
}

int publish_data(int sockfd, const char *data, const u_int length, const char *peer_ip, const int peer_port)
{
    if (data == NULL || length == 0) {
        return 0;
    }

    struct sockaddr_in peer_addr;
    //设置对端IP等信息
    memset(&peer_addr, 0, sizeof(struct sockaddr_in));
    peer_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr) <= 0) {
        ERROR_LOG("Inet_pton(%s) failed: %s", peer_ip, strerror(errno));
        return -1;
    }
    if (peer_port > 1024) {
        peer_addr.sin_port = htons(peer_port);
    } else {
        peer_addr.sin_port = htons(7878);
    }

    if (sendto(sockfd, data, length, 0, (struct sockaddr*)&peer_addr, sizeof(struct sockaddr_in)) < 0) {
        ERROR_LOG("send data to [%s:%d] failed!", peer_ip, peer_port);
        return -1;
    }

    return 0;
}

int del_update_and_recover_old(bool config_updated, bool so_updated, const char *prog_name)
{
    // 还原备份的配置文件
    if (config_updated) {
        DEBUG_LOG("backup conf.");
        if (unlink(OA_CONF_FILE)) {
            ERROR_LOG("ERROR: unlink %s, reason:%s", OA_CONF_FILE, strerror(errno));
            return -1;
        }
        if (link(OA_CONF_FILE_BAK, OA_CONF_FILE)) {
            ERROR_LOG("ERROR: link %s %s, reason:%s", OA_CONF_FILE_BAK, OA_CONF_FILE, strerror(errno));
            return -1;
        }
    }

    if (so_updated) {
        DEBUG_LOG("backup so");
        DIR *dir = opendir(OA_SO_PATH);
        if(NULL == dir) {
            ERROR_LOG("open so directory failed,Error:%s", strerror(errno));
            return -1;
        }
        // 先删除更新后的so
        char so_flag[] = ".so";
        struct dirent *de = NULL;
        while ((de = readdir(dir)) != NULL) {
            if (!strcmp(de->d_name,".") || !strcmp(de->d_name, "..")) {
                continue;
            }

            if (!strcmp(de->d_name + strlen(de->d_name) - strlen(so_flag), so_flag)) {
                char temp_name[PATH_MAX] = {0};
                strcpy(temp_name, OA_SO_PATH);
                strcat(temp_name, de->d_name);
                if (unlink(temp_name)) {
                    ERROR_LOG("ERROR: unlink so:%s, reason:%s.", temp_name, strerror(errno));
                    return -1;
                }
            }
        }
        // 还原备份的so
        dir = opendir(OA_SO_BAK_PATH);
        while ((de = readdir(dir)) != NULL) {
            if (!strcmp(de->d_name,".") || !strcmp(de->d_name, "..")) {
                continue;
            }

            if (!strcmp(de->d_name + strlen(de->d_name) - strlen(so_flag), so_flag)) {
                char backup_name[PATH_MAX] = {0};
                strcpy(backup_name, OA_SO_BAK_PATH);
                strcat(backup_name, de->d_name);

                char recover_name[PATH_MAX] = {0};
                strcpy(recover_name, OA_SO_PATH);
                strcat(recover_name, de->d_name);

                if (link(backup_name, recover_name)) {
                    ERROR_LOG("ERROR: link %s %s, reason:%s.", backup_name, recover_name, strerror(errno));
                    return -1;
                }
            }
        }
    }

    if (prog_name != NULL && prog_name[0] != 0) {
        DEBUG_LOG("unlink %s", prog_name);
        if (unlink(prog_name)) {
            ERROR_LOG("ERROR: unlink %s, reason:%s.", prog_name, strerror(errno));
            return -1;
        }
    }

    return 0;
}

/**
 * @brief  目录不存在则创建访问权限为mode的dir，否则更新dir的访问权限为mode
 * @param  dir: 待更新目录
 * @param  mode: 更新后目录的访问权限,为-1则用默认权限
 * @return 0-success, -1-failed
 */
int oa_change_dir(const char * dir, int mode)
{
    if (NULL == dir || mode <= 0) {
        return -1;
    }
    ///检查目录是否存在
    int ret_code = 0;

    mode_t old_mode = umask(0);
    do {
        if (0 != access(dir, F_OK)) {///目录不存在则创建
            if (0 != mkdir(dir, mode)) {
                ERROR_LOG("mkdir(%s, %o) failed: %s", dir, mode, strerror(errno));
                ret_code = -1;
                break;
            }
        } else {///目录存在则修改访问权限
            if (0 != chmod(dir, mode)) {
                ERROR_LOG("chmod(%s, %o) failed: %s", dir, mode, strerror(errno));
                ret_code = -1;
                break;
            }
        }
    } while (false);

    umask(old_mode);
    return ret_code;
}

/**
 * @brief  通过网卡名获取机器IP地址
 * @param  eth_name: 网卡类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip_by_name(const char *eth_name, char *host_ip)
{

    int ret_code = -1;
    int sockfd = -1;

    do {
        if (NULL == eth_name || NULL == host_ip) {
            ERROR_LOG("ERROR: Parameter cannot be NULL.");
            break;
        }
        if (0 == strlen(eth_name)) {
            ERROR_LOG("ERROR: eth_name length cannot be zero.");
            break;
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(-1 == sockfd) {
            ERROR_LOG("ERROR: socket(AF_INET, SOCK_DGRAM, 0) failed: %s", strerror(errno));
            break;
        }

        struct ifreq ifr;
        strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name));
        ifr.ifr_name[sizeof(ifr.ifr_name) -1] = 0;

        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
            ERROR_LOG("ERROR: ioctl(%d, SIOCGIFADDR, %s) failed: %s", sockfd, eth_name, strerror(errno));
            break;
        }
        strcpy(host_ip, inet_ntoa(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr));
        ret_code = 0;
    } while (false);

    if (sockfd >= 0) {
        close(sockfd);
    }

    return ret_code;
}

/**
 * @brief  获取机器IP地址
 * @param  ip_type: IP类型
 * @param  host_ip: 保存获取的IP地址
 * @return 0-success, -1-failed
 */
int get_host_ip(int ip_type, char *host_ip)
{
    if (NULL == host_ip) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return -1;
    }
    if (OA_OUTSIDE_IP == ip_type) {
        if (0 == get_host_ip_by_name("eth0", host_ip)) {///一般情况，外网是eth0
            return 0;
        }
    } else {
        if (0 == get_host_ip_by_name("eth1", host_ip)) {///一般情况，内网是eth1
            return 0;
        }
    }
    ///特殊情况
    char bond_ip[16] = {0};
    char eth2_ip[16] = {0};
    char eth3_ip[16] = {0};
    get_host_ip_by_name("bond0", bond_ip);
    get_host_ip_by_name("eth2", eth2_ip);
    get_host_ip_by_name("eth3", eth3_ip);

    if (OA_OUTSIDE_IP == ip_type) {//外网
        if (strlen(bond_ip) >= 7 || strncmp(bond_ip, "192.168.", 8)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (strlen(eth3_ip) >= 7 || strncmp(eth3_ip, "192.168.", 8)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (strlen(eth2_ip) >= 7 || strncmp(eth2_ip, "192.168.", 8)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }
    else {//内网
        if (!strncmp(bond_ip, "192.168.", 8)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (!strncmp(eth3_ip, "192.168.", 8)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (!strncmp(eth2_ip, "192.168.", 8)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }

    return -1;
}

void set_proc_name(int argc, char ** argv, const char *fmt, ...)
{
    int maxlen = 0;
    for(int i=0; i<argc; i++) {
        maxlen += strlen(argv[i]);
        maxlen++;
    }
    va_list msg;
	va_start(msg, fmt);
	memset(argv[0], 0, maxlen);
	vsnprintf(argv[0], maxlen-1, fmt, msg);
	va_end(msg);

	for(int i=strlen(argv[0]); i<maxlen; i++) {
	    argv[0][i] = 0;
	}
}

/**
 * @brief  计算文件md5值,文件不存在则返回空
 * @param  file_path: 文件路径
 * @param  md5: 计算出来的md5值
 * @return 0-success, -1-failed
 */
int get_file_md5(const char *file_name, char *md5)
{
    if (file_name == NULL || md5 == NULL) {
        ERROR_LOG("Parameter error, cannot be NULL.");
        return -1;
    }

    int ret_code = -1;
    int fd = -1;
    do {
        fd = open(file_name, O_RDONLY);
        if (fd < 0) {
            if (errno == ENOENT) {
                DEBUG_LOG("Cannot access %s: %s", file_name, strerror(errno));
                sprintf(md5, "%032d", 0);
                md5[32] = '\0';
                ret_code = 0;
            } else {
                ERROR_LOG("open(%s, O_RDONLY) failed: %s", file_name, strerror(errno));
            }
            break;
        }

        struct stat st;
        if (fstat(fd, &st) != 0) {
            ERROR_LOG("fstat(%s) failed:%s.", file_name,strerror(errno));
            break;
        }

        void *mfp = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE , fd, 0);
        if (mfp == MAP_FAILED) {
            ERROR_LOG("mmap(%s, %u) failed: %s", file_name, (u_int)st.st_size, strerror(errno));
            break;
        }

        unsigned char md5_buf[16] = {0};
        DEBUG_LOG("%zu", st.st_size);
        MD5((const unsigned char *)mfp, st.st_size, md5_buf);
        char *p_md5 = md5;
        for (int i = 0; i < 16; i++) {
            sprintf(p_md5, "%02x", md5_buf[i]);
            p_md5 += 2;
        }
        md5[32] = '\0';

        if (munmap(mfp, st.st_size) != 0) {
            ERROR_LOG("munmap(%s, %u) failed: %s", file_name, (u_int)st.st_size, strerror(errno));
            break;
        }
        ret_code = 0;
    } while (false);

    if (fd >= 0) {
        close(fd);
    }
    return ret_code;
}


/**
 * @brief 创建并初始化配置实例
 * @param config_file_list 配置文件列表
 * @param config_file_count 配置文件个数
 * @param pp_config 配置接口
 * @return
 */
int create_and_init_config_instance(i_config **pp_config)
{
    if (g_config_file_list == NULL || pp_config == NULL) {
        return -1;
    }
    //创建配置接口的实例
    if (create_config_instance(pp_config) != 0) {
        return -1;
    }
    // 初始化配置接口
    if ((*pp_config)->init(g_config_file_list, g_config_file_count) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief 反初始化并释放配置接口实例
 * @param p_config 指向配置接口实例的指针
 * @return 成功返回0，失败返回-1
 */
int uninit_and_release_config_instance(i_config *p_config)
{
	if (p_config == NULL) {
		return -1;
	}

	if (p_config->uninit() != 0) {
		ERROR_LOG("ERROR: p_config->uninit().");
		return -1;
	}

	if (p_config->release() != 0) {
		ERROR_LOG("ERROR: p_config->release().");
		return -1;
	}

	return 0;
}

/**
 * @brief  setuid到配置文件指定的用户
 * @return 0:success, -1:failed
 */
int change_user(const char * user)
{
    struct passwd *pw = getpwnam(user);
    if (!pw) {
        ERROR_LOG("ERROR: getpwnam(\"%s\") failed.", user);
        return -1;
    }

    uid_t rval = getuid();
    if (pw->pw_uid != rval) {
        if (rval != 0) {
            ERROR_LOG("ERROR: must be root to setuid to (\"%s\").", user);
            return -1;
        }
        // 在change用户之前，把文件和目录的所有者修改为新用户
        if (chown(OA_BIN_PATH, pw->pw_uid, 0) || chown(OA_LOG_PATH, pw->pw_uid, 0) ||
                chown(OA_CONF_PATH, pw->pw_uid, 0) || chown(OA_SO_PATH, pw->pw_uid, 0) ||
                chown(OA_BAK_PATH, pw->pw_uid, 0) || chown(OA_SO_BAK_PATH, pw->pw_uid, 0) ||
                chown(OA_PID_FILE, pw->pw_uid, 0)) {
            ERROR_LOG("ERROR: chown corresponding dir to %s failed, reason: %s", user, strerror(errno));
            return -1;
        }

        rval = setuid(pw->pw_uid);
        if (rval != 0) {
            ERROR_LOG("ERROR: setuid to (\"%s\"), reason: %s", user, strerror(errno));
            return -1;
        }
    }

    return 0;
}

/**
 * @brief  判断是否是接受udp数据的node
 * @param  p_config 配置类的指针
 * @return 0:success, -1:failed
 */
int set_recv_udp(i_config *p_config, bool * is_recv_udp)
{
    if (NULL == p_config) {
        return -1;
    }

    char buffer[10] = {0};
    if (p_config->get_config("node_info", "recv_udp", buffer, sizeof(buffer))) {
        ERROR_LOG("ERROR: get [node_info]:[recv_udp] failed.");
        return -1;
    }

    char *start = buffer;
    if (isblank(*start)) {
        ++start;
    }

    char ch = tolower(*start);
    if (ch != 'y' && ch != 'n') {
        ERROR_LOG("recv_udp config error it should be: [y(yes)|n(no)]");
        return -1;
    }

    if ('n' == ch) {
        *is_recv_udp = false;
    } else {
        *is_recv_udp = true;
    }
    return 0;
}

/**
 * @brief  判断是否是内网主机
 * @param  p_config 配置类的指针
 * @return true:内网主机, false:外网主机
 */
bool get_listen_type(i_config * p_config)
{
    if(p_config == NULL) {
        return true;
    }
    char buffer[10] = {0};
    if (p_config->get_config("node_info", "listen_type", buffer, sizeof(buffer))) {
        //ERROR_LOG("ERROR: get [node_info]:[listen_type] failed.");
        return true;
    } else {
        if(strcmp(buffer, "OUT") == 0) {
            return false;
        } else {
            return true;
        }
    }
}

/**
 * @brief  根据配置的网络类型获得本机ip
 * @param  p_config 配置类的指针
 * @param  inside_ip 保存ip地址的内存指针
 * @return inside_ip:成功, NULL:失败
 */
char * get_ip(i_config * p_config, char * inside_ip)
{
    if(p_config == NULL || inside_ip == NULL) {
        return NULL;
    }
    bool is_inside = get_listen_type(p_config);
    if (0 != get_host_ip(is_inside?OA_INSIDE_IP:OA_OUTSIDE_IP, inside_ip)) {
        ERROR_LOG("ERROR: get_host_ip(%s) failed.", is_inside?"OA_INSIDE_IP":"OA_OUTSIDE_IP");
        if (0 != get_host_ip(!is_inside?OA_INSIDE_IP:OA_OUTSIDE_IP, inside_ip)) {
            ERROR_LOG("ERROR: get_host_ip(%s) failed.", !is_inside?"OA_INSIDE_IP":"OA_OUTSIDE_IP");
            return NULL;
        } else {
            is_inside = !is_inside;
        }
    }
    DEBUG_LOG("get %s : %s", is_inside?"OA_INSIDE_IP":"OA_OUTSIDE_IP", inside_ip);
    return inside_ip;
}
