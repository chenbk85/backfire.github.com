/* vim: set tabstop=4 softtabstop=4 shiftwidth=4: */
/**
 * @file utils.h
 * @author richard <richard@taomee.com>
 * @date 2010-03-09
 */

#ifndef UTILS_H_2010_03_09
#define UTILS_H_2010_03_09

#include <stdint.h>
#include "../proto.h"
#include "../defines.h"

/**
 * @brief 设置进程gid和uid为指定用户的
 * @param username 用户名
 * @return 成功返回0，失败返回-1
 */
int set_user(const char *username);

/**
 * @brief 为指定的信号安装信号处理函数
 * @param sig 要处理的信号
 * @param signal_handler 信号处理函数
 * @return 成功返回0，失败返回-1
 */
int mysignal(int sig, void(*signal_handler)(int));

/**
 * @brief 判断程序是否已经有实例正在运行
 * @return 如果已经有实例正在运行返回1，否则返回0，出错时返回-1
 */
int already_running(); 

/**
 * @brief 初始化设置proc标题
 * @param argc main函数的第一个参数
 * @param argv main函数的第二个参数
 * @return 无
 */
void init_proc_title(int argc, char *argv[]);

/**
 * @brief 设置proc标题
 * @param fmt 标题格式
 * @param ... 可变参数
 * @return 无
 */
void set_proc_title(const char *fmt, ...);

/**
 * @brief 反初始化设置proc标题
 * @return 无
 */
void uninit_proc_title();

inline int addstring(char *strings, int *edge, const char *s) 
{
   int e = *edge;
   int end = e + strlen(s) + 1;

   if (e > HEAD_FRAMESIZE || end > HEAD_FRAMESIZE)
   {   
      return -1; 
   }   

   strcpy(strings + e, s); 
   *edge = end;
   return e;
}
inline char *getfield(char* buf, short int index)
{
    if(index < 0)
        return NULL;
    return (char*)buf + index;
}


inline slope_t cstr_to_slope(const char* str)
{
    if(str == NULL) 
	{
        return SLOPE_UNSPECIFIED;
    }   
    
    if(!strcasecmp(str, "zero")) 
	{
        return SLOPE_ZERO;
    }   
    
    if(!strcasecmp(str, "positive")) 
	{
        return SLOPE_POSITIVE;
    }   
    
    if(!strcasecmp(str, "negative")) 
	{
        return SLOPE_NEGATIVE;
    }   
    
    if(!strcasecmp(str, "both")) 
	{
        return SLOPE_BOTH;
    }   

    return SLOPE_UNSPECIFIED;
}

struct  op_tag
{
    op_t op;
    char op_name[16];
};

inline char *op2str(op_t op)
{
    static struct op_tag op_tags[] = 
    {
        {OP_EQ, "="},
        {OP_GT, ">"},
        {OP_LT, "<"},
        {OP_GE, ">="},
        {OP_LE,  "<="}
    };

    if(op == OP_UNKNOW) {
        return NULL;
    }

    for(unsigned int i = 0; i < sizeof(op_tags)/sizeof(op_tags[0]); i++) {
        if(op_tags[i].op == op)
            return op_tags[i].op_name;
    }
    return NULL;
}  

inline op_t str2op(const char* str)
{
    if(str == NULL) {
        return OP_UNKNOW;
    }   
    
    if(!strcasecmp(str, "EQ")) {
        return OP_EQ;
    }   
    
    if(!strcasecmp(str, "GT")) {
        return OP_GT;
    }   
    
    if(!strcasecmp(str, "LT")) {
        return OP_LT;
    }   
    
    if(!strcasecmp(str, "GE")) {
        return OP_GE;
    }   

    if(!strcasecmp(str, "LE")) {
        return OP_LE;
    }   

    return OP_UNKNOW;
}

/* Zeroes out every metric value in a summary hash table. */
inline int zero_out_summary(datum_t *key, datum_t *val, void *arg)
{
    /* Note that we get the actual value bytes here, not a copy. */
    metric_t *metric = (metric_t*)val->data;
    memset(&metric->val, 0, sizeof(metric->val));
    metric->num = 0;
    return 0;
}


/** 
 * @brief   类型判断函数
 * @param   const char*  要传入的数字符串
 * @return  false = no, true = yes 
 */
bool is_integer(const char *); 
bool is_intpos(const char *); 
bool is_intneg(const char *); 
bool is_intnonneg(const char *); 
bool is_intpercent(const char *); 

bool is_numeric(const char *); 
bool is_positive(const char *); 
bool is_negative(const char *); 
bool is_nonnegative(const char *); 
bool is_percentage(const char *); 
bool is_option(const char *); 
#define is_inet_addr(addr) resolve_host_or_addr(addr, AF_INET)

bool resolve_host_or_addr(const char *address, int family);

int get_local_ip_str(char *buf, unsigned int len);

int hostname_to_s_addr(const char *host_name);
/** 
 * @brief   sleep和usleep的封装
 * @param   usec  微妙数
 * @return  0=success,-1=failed
 */
int my_sleep(unsigned int usec);

/**
 * @brief  分隔一个字符串
 * @param   src_str      要分隔的字符串头指针
 * @param   delimiter    分隔符
 * @param   fields       保存分隔开的每个字段的头指针
 * @param   field_count  值结果参数传入最大个数，传出实际个数
 * @return  -1-failed 0 = success
 */
int split(char *src_str, int delimiter, char **fields, int *field_count);

/**
 * @brief  将时间戳转换为日期字符串
 * @param   timestamp 待转换的时间戳
 * @param   p_buff 转换后的字符串
 * @param   buff_len 缓存大小
 * @return  转换后的字符串
 */
char *tm2str(time_t timestamp, char *p_buff, int buff_len);

/**
 * @brief  求字符串的md5值
 * @param   p_str 源字符串
 * @param   p_md5 保存返回的md5值
 * @return  -1-failed, 0-success
 */
int str2md5(const char *p_str, char *p_md5);

/**
 * @brief  获取进程运行状态
 * @param   proc_name 进程名
 * @param   p_is_running 进程运行状态
 * @return  -1-failed, 0-success
 */
int get_process_status(const char *proc_name, bool *p_is_running);

/** 
 * @brief 过滤一个字符串中的space字符
 * @param   str  要过滤的字符串头指针
 * @return  void 
 */
void str_trim(char *str);

/** 
 * @brief 过滤一个字符串中的space字符
 * @param   src_ip 整型IP
 * @param   dst_ip IP字符串
 * @return  NULL-fail, dst_ip-succ 
 */
char *long2ip(uint32_t src_ip, char *dst_ip);

/**
 * @brief  将时间戳转换为日期字符串
 * @param   timestamp 待转换的时间戳
 * @param   p_str 转换后的字符串
 * @return  转换后的字符串
 */
char *time2str(uint32_t timestamp, char *p_str);

/**
 * @brief  将时间戳转换为日期字符串
 * @param   timestamp 待转换的时间戳
 * @param   p_str 转换后的字符串
 * @return  转换后的字符串
 */
char *tm2str(time_t timestamp, char *p_str);

void print_bytes(uint8_t * buf, uint32_t len);
int get_host_ip(int ip_type, char *host_ip);

/**
 * @brief  URL特殊字符转义
 * @param  encode:待转义字符串
 * @param  decode:转换后的字符串
 * @return  转换后的字符串
 */
char * urlencode(const char * encode, char * decode);

/** 
 * @brief  进程初始化日志
 * @param  p_log_prefix: 日志前缀
 * @return  -1: failed,  0: success
 */
int proc_log_init(const char *p_log_prefix);

#endif //UTILS_H

