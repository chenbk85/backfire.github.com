/* vim: set tabstop=4 softtabstop=4 shiftwidth=4: */
/**
 * @file utils.cpp
 * @author mason <mason@taomee.com>
 * @date 2010-11-09
 * @author tonyliu <tonyliu@taomee.com>
 * @date 2012-01-09
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <netdb.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "../defines.h"
#include "utils.h"
#include "log.h"

#define LOCKFILE "./daemon.pid"

int set_user(const char *username)
{
    struct passwd *pw = getpwnam(username);
    if(NULL == pw) {
        return -1;
    }

    int rval = getuid();
    if(rval != pw->pw_uid) {
        if(0 != rval) {
            return  -1; 
        }
        //设置effective gid如果调用者是root那么也将设置real gid
        setgid(pw->pw_gid);
        //设置effective uid如果调用者是root那么也将设置real uid
        if(0 != setuid(pw->pw_uid)) {
            return -1; 
        }
    }

    return  0;
}

int mysignal(int sig, void(*signal_handler)(int))
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	return sigaction(sig, &act, 0);
}

static int lockfile(int fd)
{
	struct flock fl = {0};

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	return fcntl(fd, F_SETLK, &fl);
}

int already_running()
{
	int fd = -1;
	char buf[16] = {0};

	fd = open(LOCKFILE, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (fd < 0) {
		return -1;
	}
	if (lockfile(fd) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			close(fd);
			return 1;
		}
		return -1;
	}
	if (0 != ftruncate(fd, 0)) {
		return -1;
	}
	sprintf(buf, "%d", (int)getpid());
	if (-1 == write(fd, buf, strlen(buf) + 1)) {
		return -1;
	}
	
	return 0;
}

/**
 * @brief  获取进程运行状态
 * @param   proc_name 进程名
 * @param   p_is_running 进程运行状态
 * @return  -1-failed, 0-success
 */
int get_process_status(const char *proc_name, bool *p_is_running)
{
    if (NULL == proc_name || NULL == p_is_running) {
        return -1;
    }

    char cmd_str[MAX_STR_LEN] = {0};
    sprintf(cmd_str, "ps -ef | grep %s | grep -v grep", proc_name);
    FILE *fp = popen(cmd_str, "r");
    if (NULL == fp) {
        return -1;
    }

    char result[MAX_STR_LEN] = {0};
    if (NULL != fgets(result, sizeof(result), fp)) {
        if (strstr(result, proc_name)) {
            *p_is_running = true;
        } else {
            *p_is_running = false;
        }
    }

    pclose(fp);
    return 0;
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

	if (fmt == NULL) {
		return;
	}

	va_list msg;
	va_start(msg, fmt);
	memset(pp_prog_argv[0], 0, maxlen);
	vsnprintf(pp_prog_argv[0], maxlen, fmt, msg);
	va_end(msg);

	int i = 0;
	for (i = 1; i < prog_argc; ++i) {
		pp_prog_argv[i] = NULL;
	}
}

int get_proc_title(char *buf, size_t bufsz) 
{
	if (buf == NULL || bufsz == 0) {
		return strlen(pp_prog_argv[0]);
	}

	strncpy(buf, pp_prog_argv[0], bufsz);
	
	return strlen(buf);
}

/**
 * @brief   类型判断函数
 * @param   const char*  要传入的数字符串
 * @return  false = no, true = yes 
 */
bool is_numeric(const char *number)
{
    char   tmp[1];
    double x;

    if(!number) {
        return false;
    } else if(sscanf(number, "%lf%c", &x, tmp) == 1) {
        return true;
    } else {
        return false;
    }
}

bool is_positive(const char *number)
{
    if (is_numeric(number) && atof(number) > 0.0) {
        return true;
    } 

    return false;
}

bool is_negative(const char *number)
{
    if (is_numeric(number) && atof(number) < 0.0) {
        return true;
    }

    return false;
}
bool is_nonnegative(const char *number)
{
    if (is_numeric(number) && atof(number) >= 0.0) {
        return true;
    }

    return false;
}

bool is_percentage(const char *number)
{
    int x;
    if(is_numeric(number) && (x = atof(number)) >= 0 && x <= 100) {
        return true;
    }

    return false;
}

bool is_integer(const char *number)
{
    if(number == NULL || strlen(number) == 0) {
        return false;
    }

    long int n;
    if(!number || (strspn(number, "-0123456789 ") != strlen(number))) {
        return false;
    }

    n = strtol(number, NULL, 10);

    if(errno != ERANGE && n >= LONG_MIN && n <= LONG_MAX)
        return true;
    else
        return false;
}
bool is_intpos(const char *number)
{
    if(is_integer(number) && atoi(number) > 0)
        return true;
    else
        return false;
}

bool is_intneg(const char *number)
{
    if(is_integer(number) && atoi(number) < 0)
        return true;
    else
        return false;
}

bool is_intnonneg(const char *number)
{
    if(is_integer(number) && atoi(number) >= 0)
        return true;
    else
        return false;
}

bool is_intpercent(const char *number)
{
    int i;
    if(is_integer(number) && (i = atoi(number)) >= 0 && i <= 100)
        return true;
    else
        return false;
}
bool is_option(const char *str)
{
    if(!str)
        return false;
    else if(strspn(str, "-") == 1 || strspn(str, "-") == 2)
        return true;
    else
        return false;
}
/** 
 * @brief   解析一个host或网络地址
 * @param   address  地址
 * @param   af       地址族
 * @return  true or false 
 */
bool resolve_host_or_addr(const char *address, int family)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int    retval;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    retval = getaddrinfo(address, NULL, &hints, &res);

    if(retval != 0)
        return false;
    else 
    {   
        freeaddrinfo(res);
        return true;
    }   
}
/** 
 * @brief   获得当前机器的ip
 * @param   buf  缓存
 * @param   len  缓存长度
 * @return  0=success,-1=failed
 */

int get_local_ip_str(char *buf, unsigned int len)
{
    char   buffer[4096] = {0};
    char   host_name[MAX_STR_LEN] = {0};
    struct hostent host_ent, *hp;
    int    error_num = 0;

    if(0 != gethostname(host_name, sizeof(host_name)))
    {   
        //fprintf(stderr, "gethostname error.\n");
        return -1; 
    }   

    if(0 != gethostbyname_r(host_name, &host_ent, buffer, sizeof(buffer), &hp, &error_num))
    {   
        //fprintf(stderr, "gethostbyname_r failed,error:%s\n", hstrerror(error_num));
        return -1; 
    }   

    strncpy(buf, inet_ntoa(*(struct in_addr*)(host_ent.h_addr)), len);
    return 0;
}

/** 
 * @brief   根据hostname或者ip地址获取ip网络地址
 * @param   host_name  host name
 * @return  0=success,-1=failed
 */
int hostname_to_s_addr(const char *host_name)
{
    if(host_name == NULL)
    {
        return -1;
    }

    struct hostent host_ent, *hp;
    char   buf[4096] = {0};
    int    error_num = 0;

    if(0 != gethostbyname_r(host_name, &host_ent, buf, sizeof(buf), &hp, &error_num))
    {   
        //ERROR_LOG("gethostbyname_r failed,error:%s", hstrerror(error_num));
        return -1;
    }   
    return ((struct in_addr*)host_ent.h_addr)->s_addr;
}

/** 
 * @brief   sleep和usleep的封装
 * @param   usec  微妙数
 * @return  0=success,-1=failed
 */
int my_sleep(unsigned int usec)
{
    if(usec == 0)
        return 0;
    unsigned int sec = (unsigned int)(usec / 1000000);
    if(sec != 0)
        while((sec = sleep(sec)) != 0)
        {
            continue;
        }
    return usleep(usec % 1000000);
}

/**
 * @brief  分隔一个字符串
 * @param   src_str      要分隔的字符串头指针
 * @param   delimiter    分隔符
 * @param   fields       保存分隔开的每个字段的头指针
 * @param   field_count  值结果参数传入最大个数，传出实际个数
 * @return  -1-failed 0 = success
 */
int split(char *src_str, int delimiter, char **fields, int *field_count)
{
    if(src_str == NULL || fields == NULL || field_count == NULL) return -1;

    int max_field = *field_count;
    *field_count = 0;
    char *start = src_str;
    char *seperater = NULL;
    int i = 0;
    while((seperater = index(start, delimiter)) != NULL && i < max_field) {
        fields[i++] = start;
        start = seperater + 1;
        *seperater = '\0';
    }
    fields[i++] = start;
    *field_count = i;
    return 0;
}

/**
 * @brief  将时间戳转换为日期字符串
 * @param   timestamp 待转换的时间戳
 * @param   p_buff 转换后的字符串
 * @param   buff_len 缓存大小
 * @return  转换后的字符串
 */
char *tm2str(time_t timestamp, char *p_buff, int buff_len)
{
    assert(NULL != p_buff && buff_len >= 20);
    
    struct tm *dt;
    dt = localtime(&timestamp);
    if (NULL != dt) {
        strftime(p_buff, buff_len, "%Y-%m-%d %H:%M:%S", dt);
    }

    return p_buff;
}

/**
 * @brief  将时间戳转换为日期字符串
 * @param   timestamp 待转换的时间戳
 * @param   p_str 转换后的字符串
 * @return  转换后的字符串
 */
char *time2str(uint32_t timestamp, char *p_str)
{
    assert(NULL != p_str);

    struct date_time {
        short dt_year;
        short dt_month;
        short dt_day;
        short dt_hour;
        short dt_minute;
        short dt_second;
    } dt;
    short *walker = (short *)&dt;
    unsigned long mask = 1 << 31;
    //YYYY YYYm mmmd dddd HHHH HMMM MMMS SSSS
    short bits_per_field[6] = {7, 4, 5, 5, 6, 5};
    int accum = 0;
    for (int i = 0; i < 6; i++) {
        accum = 0;
        for (int j = 0; j < bits_per_field[i]; j++) {
            int bit = (timestamp & mask) ? 1 : 0;
            if (bit) {
                accum += pow(2, bits_per_field[i] - 1 - j);
            }
            mask = mask >> 1;
        }
        *walker = accum;
        walker++;
    }
    //注意最右边一位在从日期转换到时间戳的时候砍掉了，因此我们秒这一字段要在最右端加一个补充的0
    dt.dt_second <<= 1;

    sprintf(p_str, "%4d-%02d-%02d %02d:%02d:%02d", dt.dt_year + 1980, dt.dt_month,
            dt.dt_day, dt.dt_hour, dt.dt_minute, dt.dt_second);

    return p_str;
}


/**
 * @brief  求字符串的md5值
 * @param   p_str 源字符串
 * @param   p_md5 保存返回的md5值
 * @return  -1-failed, 0-success
 */
int str2md5(const char *p_str, char *p_md5)
{
    assert(NULL != p_str && NULL != p_md5);

    unsigned char md5[16] = {0};
    char dst_md5[33] = {0};
    char *p_pos = dst_md5;
    MD5((const unsigned char *)p_str, strlen(p_str), md5);
    for (int i = 0; i < 16; i++) {
        sprintf(p_pos, "%02x", md5[i]);
        p_pos += 2;
    }
    memcpy(p_md5, dst_md5, 32);

    return 0;
}

/** 
 * @brief 过滤一个字符串中的space字符
 * @param   str  要过滤的字符串头指针
 * @return  void 
 */
void str_trim(char *str)
{
    if(NULL == str) {   
        return;
    }   

    char *out = str;
    char *in = str;

    while(*in != '\0') {   
        if(*in == ' ' || *in == '\t' || *in == '\r' || *in == '\n') { 
            in++;
        }   
        else {   
            *out++ = *in++;
        }   
    }   
    *out = '\0';

    return;
}

char *long2ip(uint32_t src_ip, char *dst_ip)
{
    if (NULL == dst_ip) {
        return NULL;
    }
    uint8_t ip_arr[4] = {0};
    for (int i = 0; i < 4; ++i) {
        ip_arr[i] = src_ip & 0x00FF;
        src_ip >>= 8;
    }
    sprintf(dst_ip, "%u.%u.%u.%u", ip_arr[3], ip_arr[2], ip_arr[1], ip_arr[0]);
    return dst_ip;
}

void print_bytes(uint8_t * buf, uint32_t len)
{
    uint32_t l = len>>4;
    uint8_t c;
    for(uint32_t i=0; i<l; i++) {
        for(uint32_t j=0; j<16; j++) {
            printf("%02X ", buf[(i<<4)+j]);
        }
        for(uint32_t j=0; j<16; j++) {
            c = buf[(i<<4)+j];
            if(c>=32 && c<=127) {
                printf("%c", c);
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    for(uint32_t i=0; i<len-(l<<4); i++) {
        printf("%02X ", buf[(l<<4)+i]);
    }
    for(uint32_t i=0; i<16-(len-(l<<4)); i++) {
        printf("   ");
    }
    for(uint32_t i=0; i<len-(l<<4); i++) {
        c = buf[(l<<4)+i];
        if(c>=32 && c<=127) {
            printf("%c", c);
        } else {
            printf(" ");
        }
    }
    printf("\n");
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
            ERROR_LOG("ERROR: ioctl(%d, SIOCGIFADDR, ifr) failed: %s", sockfd, strerror(errno));
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
    
    if (NET_OUTSIDE_TYPE == ip_type) {
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

    if (NET_OUTSIDE_TYPE == ip_type) {//外网
        if (strlen(bond_ip) >= 7 && strncmp(bond_ip, "192.168.", 8) && strncmp(bond_ip, "10.", 3)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (strlen(eth3_ip) >= 7 && strncmp(eth3_ip, "192.168.", 8) && strncmp(eth3_ip, "10.", 3)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (strlen(eth2_ip) >= 7 && strncmp(eth2_ip, "192.168.", 8) && strncmp(eth2_ip, "10.", 3)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }
    else {//内网:10.0.0.0 - 10.255.255.255  172.16.0.0 - 172.31.255.255 192.168.0.0 - 192.168.255.255
        if (!strncmp(bond_ip, "192.168.", 8) || !strncmp(bond_ip, "10.", 3) || !strncmp(bond_ip, "172.", 4)) {
            strcpy(host_ip, bond_ip);
            return 0;
        }
        if (!strncmp(eth3_ip, "192.168.", 8) || !strncmp(eth3_ip, "10.", 3) || !strncmp(eth3_ip, "172.", 4)) {
            strcpy(host_ip, eth3_ip);
            return 0;
        }
        if (!strncmp(eth2_ip, "192.168.", 8) || !strncmp(eth2_ip, "10.", 3) || !strncmp(eth2_ip, "172.", 4)) {
            strcpy(host_ip, eth2_ip);
            return 0;
        }
    }

    return -1;
}

char * urlencode(const char * encode, char * decode)
{
    uint32_t index = 0;
    for(uint32_t i = 0; i< strlen(encode); i++)
    {
        uint8_t ch = (uint8_t)encode[i];
        if(ch == ' ') {
            decode[index++] = '+';
        }else if(ch >= 'A' && ch <= 'Z'){
            decode[index++]= ch;
        }else if(ch >= 'a' && ch <= 'z'){
            decode[index++]= ch;
        }else if(ch >= '0' && ch <= '9'){
            decode[index++]= ch;
        }else if(ch == '-' || ch == '-' || ch == '.'){
            decode[index++]= ch;
        }else{
            decode[index++]= '%';
            index += sprintf(&decode[index], "%X", ch);
        }

    }

    return decode;
}

/** 
 * @brief  进程初始化日志
 * @param  p_log_prefix: 日志前缀
 * @return  -1: failed,  0: success
 */
int proc_log_init(const char *p_log_prefix)
{
    assert(NULL != p_log_prefix);
    log_conf_t log_cfg = {10,  7, 10240000, "oa_head_", "../log"};
    ///如果目录不存在则试图创建
    if(access(log_cfg.log_dir, F_OK) != 0 && 
            mkdir(log_cfg.log_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
        return -1;
    } else {
        chmod(log_cfg.log_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }

    //把log目录交给nobody
    //struct passwd *pw = NULL;
    //pw = getpwnam("nobody");
    //if(NULL != pw) {
    //    chown(log_cfg.log_dir, pw->pw_uid, pw->pw_gid);
    //}

    if(0 != log_init(log_cfg.log_dir, (log_lvl_t)log_cfg.log_lvl,
            log_cfg.log_size, log_cfg.log_count, p_log_prefix)) {
        fprintf(stderr, "log_init error.");
        return -1;
    }

    enable_multi_thread();
    set_log_dest(log_dest_file);
    return 0;
}
