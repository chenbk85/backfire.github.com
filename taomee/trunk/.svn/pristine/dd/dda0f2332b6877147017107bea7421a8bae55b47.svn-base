/**
 * =====================================================================================
 *       @file  xml_parser.h
 *      @brief  
 *
 *  parse the xml and and then save them into hash
 *
 *   @internal
 *     Created  2010-10-18 11:13:42
 *    Revision  1.0.0.0
 *    Compiler  gcc/g++
 *     Company  TaoMee.Inc, ShangHai.
 *   Copyright  Copyright (c) 2010, TaoMee.Inc, ShangHai.
 *
 *     @author  mason, mason@taomee.com
 * This source code was wrote for TaoMee,Inc. ShangHai CN.
 * =====================================================================================
 */
#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <sys/time.h>
#include <expat.h>
#include "../lib/utils.h"
#include "../lib/i_ring_queue.h"
#include "../lib/c_mysql_iface.h"
#include "./rrd_handler.h"
#include "../defines.h"

/**
*  @struct 用于XMLParser解析的用户数据
*/
typedef struct {
    int                      rval;
    char                    *sourcename;   /**< The current source name. */
    char                    *hostname;     /**< The current host name. */
    data_source_list_t      *ds;
    int                      grid_depth;   /**< The number of nested grids at this point.Will begin at zero. */
    int                      host_status;
    int                      host_change;  /**< True if the hosts of current cluster has changed*/
    source_t                 source;       /**< The current source structure. */
    host_t                   host;         /**< The current host structure. */
    metric_t                 metric;       /**< The current metric structure. */
    metric_status_info_t     metric_status;/**< The current metric status info(for alarm). */
    hash_t                  *root;         /**< The root children table (contains our data sources). */ 
    struct timeval           now;          /**< The time when parsing started. */
} xmldata_t;


/**
*  @struct 用于报警的传参结构
*/
typedef struct {
    const   char          *cur_val;
    const   char          *threshold_val;
    op_t                   op;
} metric_alarm_info_t;

/**
*  @struct 状态转换表结构 
*/
typedef struct {
    const status_t      old_status;
    const state_t       cur_state;
    const status_t      ret_status; 
    bool                flag;
} status_change_t;

class xml_parser
{
public :
    xml_parser();
    ~xml_parser();

    int init(source_t *p_root,
            i_ring_queue* p_queue,
            c_rrd_handler *p_rrd,
            unsigned int collect_interval, 
            const metric_alarm_vec_t *default_metric_alarm_info,
            const metric_alarm_map_t *specified_metric_alarm_info);

    int uninit();
    inline int xml_parser_reset()
    {
        memset(&m_xml_data, 0, sizeof(m_xml_data));
        if(m_xml_parser) {
            if(XML_TRUE != XML_ParserReset(m_xml_parser, NULL)) {
                return -1;
            }
        }
        return 0;
    }

    /** 
     * @brief  处理xml文件的parser函数
     * @param   d 数据源指针
     * @param   buf xml文件缓存 
     * @return  0 success 1 failed  
     */
    int process_xml(data_source_list_t *d, const char *buf);

protected:
    /** 
     * @brief   判断一个字符串是不是公式
     * @param   str     字符串
     * @return  true-yes false-no  
     */
    bool is_expresion(const char *str);

    /** 
     * @brief   替换一个公式中的变量为值
     * @param   raw_expr 原始公式 
     * @param   expr     替换好的公式 
     * @return  void  
     */
    void replace_var_name_by_var_val(const char *raw_expr, std::string *expr, const char *arg);

    /** 
     * @brief   结算一个表达式的值
     * @param   expr   公式 
     * @param   result 保存计算结果的指针
     * @return  0-success -1-failed  
     */
    int parse_expr(const char *expr, double *result);

    /** 
     * @brief   获取一个metric的值
     * @param   var_name   变量名
     * @param   val_str    保存值字符串结果的缓冲区
     * @param   len        缓冲区长度
     * @return  0-success -1-failed  
     */
    int get_val_str_by_name(const char *var_name, char *val_str, unsigned int len);

    /** 
     * @brief   <CLUSTER>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_cluster(const char *el, const char **attr);

    /** 
     * @brief   <HOST>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_host(const char *el, const char **attr);

    /** 
     * @brief   <METRIC>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_metric(const char *el, const char **attr);

    /** 
     * @brief   <EXTRA_DATA>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_extra_data(const char *el, const char **attr);

    /** 
     * @brief   <EXTRA_ELEMENT>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_extra_element(const char *el, const char **attr);

    /** 
     * @brief   <METRICS>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_metrics(const char *el, const char **attr);

    /** 
     * @brief   <OA_XML>标签的start函数 
     * @param   el  xml元素名 
     * @param   attr  属性字符串数组 
     * @return  0=success 1=failed  
     */
    int start_element_xml(const char *el, const char **attr);

    /** 
     * @brief   <GRID/>标签的end函数 
     * @param   el  xml元素名 
     * @return  void  
     */
    int end_element_grid(const char *el);

    /** 
     * @brief   <CLUSTER/>标签的end函数 
     * @param   el  xml元素名 
     * @return  void  
     */
    int end_element_cluster(const char *el);

    /** 
     * @brief   <HOST/>标签的end函数 
     * @param   el  xml元素名 
     * @return  -1 = failed 0 = success  
     */
    int end_element_host(const char *el);

    /** 
     * @brief  判断是否是authority模式
     * @param   xml_data 解析xml的用户数据
     * @return  true or false 
     */
    bool is_nestedgrid(xmldata_t *xmldata);

    /** 
     * @brief  填充一个metric
     * @param   attr 属性字符串数组
     * @param   metric metric_t指针 
     * @param   type   表示类型的字符串 
     * @return  void 
     */
    void fillmetric(const char** attr, metric_t *metric, const char* type);

    /** 
     * @brief  填充一个metric_status_info_t
     * @param   formular     公式字符串
     * @param   metric_status metric_status_info_t指针 
     * @return  0=success -1 = failed 
     */
    int fill_metric_status(metric_status_info_t *metric_status, const char* formular, const char *arg);

    /** 
     * @brief  查找一个metric，arg，host对应的alarm信息
     * @param   metric_name metric name
     * @param   metric_arg  metric argument
     * @param   host_name   server tag
     * @param   p_alarm_info  告警条件
     * @return  0-success, -1-failed 
     */
    int find_metric_alarm_info(const char *metric_name, const char *metric_arg,
            const char *hostname, alarm_cond_t *p_alarm_info);

    int parse_metric_expression(alarm_cond_t *alarm_info, const char *arg_md5);

    /** 
     * @brief   Write a metric summary value to the RRD database.
     * @param   key  hash表的key
     * @param   val  hash表的val
     * @param   arg  参数 
     * @return  0 success 1 failed  
     */
    static int finish_processing_source(datum_t *key, datum_t *val, void *arg);

    /** 
     * @brief  XML_Parser的回调函数
     * @param   p_data 用户数据指针
     * @param   el  xml元素名 
     * @param   attr 属性字符创数组 
     * @return  void  
     */
    static void start_parse(void *p_data, const char *el, const char **attr);

    /** 
     * @brief  XML_Parser的回调函数
     * @param   p_data 用户数据指针
     * @param   el   xml元素名
     * @return  void  
     */
    static void end_parse(void *p_data, const char *el);

    /** 
     * @brief 过滤一个字符串中的space字符
     * @param   str  要过滤的字符串头指针
     * @return  void 
     */
    void str_trim(char *str);

    /** 
     * @brief 处理一个host的metric的报警相关事宜
     * @param   host_info      host_t结构，用于传入host的信息
     * @param   metric_status  metric_status_info_t结构，传入当前metirc的警报状态信息
     * @param   metric_name    metric name string
     * @param   metric_val     metric value string
     * @return  void 
     */

    int handle_metric_alarm(const host_t *host_info, const alarm_cond_t *alarm_info,
            metric_status_info_t *metric_status, const char *metric_name,
            const char *metric_arg, const char *metricval);

    /** 
     * @brief 根据前一个metric的状态和当前metric的警报状态(state)产生新的状态(status)
     * @param   old_status  metric的前一个状态(这是一个值-参数型的参数)
     * @param   cur_state   当前metric的警报状态
     * @param   flag        标志位表示探测次数是否达到上限
     * @return  void 
     */
    void status_change(status_t *old_status, const state_t cur_state, bool flag);

    /** 
     * @brief 取得当前的警报状态 
     * @param   wrn_val    warning value for a metric
     * @param   crtcl_val  critical value for a metric
     * @param   cur_val    current value  of metric
     * @param   op         操作方式(=,<,>,>=,<=)
     * @return  state_t 
     */
    state_t get_state(const double wrn_val, const double crtcl_val, const double cur_val, op_t op);

    /** 
     * @brief   请求web服务器报警 
     * @param   cmd_id      告警命令号
     * @param   host_ip     host ip
     * @param   metric_name metric name/host status
     * @param   metric_arg  metric argument
     * @param   alarm_level 报警级别(warning or critical)
     * @param   alarm_info  metric_alarm_info_t
     * @return  void 
     */
    void report_alarm_cmd(int cmd_id, const char *host_ip, const char *metric_name, const char *metric_arg,
            const char *alarm_level, int is_mute = 0, const metric_alarm_info_t *alarm_info = NULL);
private:
    bool m_inited;
    source_t *m_p_root;        /**<保存本head数据源的根节点*/
    i_ring_queue *m_p_queue;   /**<保存queue对象指针*/
    c_rrd_handler *m_rrd_handler;/**<rrd操作对象*/
    xmldata_t m_xml_data;      /**<XMLPsraer用户数据*/
    XML_Parser m_xml_parser;   /**<XMLPsraer对象指针*/
    unsigned int m_collect_interval;
    const metric_alarm_vec_t *m_p_default_metric_alarm_info;
    const metric_alarm_map_t *m_p_specified_metric_alarm_info;
};

#endif
