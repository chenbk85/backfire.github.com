/**
 * =====================================================================================
 *       @file  xml_parser.cpp
 *      @brief  
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <openssl/md5.h>   
#include <netdb.h>
#include <arpa/inet.h>

#include "../lib/xml_hash.h"
#include "../lib/http_transfer.h"
#include "../lib/type_hash.h"
#include "../lib/check_hostalive.h"
#include "../defines.h"
#include "../db_operator.h"
#include "./alarm_thread.h"
#include "./parser.h"
#include "./xml_parser.h"

using namespace std;
static const char *host_alarm_type[4] = {"", "agent_down", "host_unknown", "host_down"};

/** 
 * @brief  ���캯��
 * @return  
 */
xml_parser::xml_parser(): m_inited(false), m_p_root(NULL), m_p_queue(NULL), m_rrd_handler(NULL), m_xml_parser(NULL)
{
}

/** 
 * @brief  ��������
 * @return  
 */
xml_parser::~xml_parser()
{
    uninit();
}

/** 
 * @brief  ����ʼ��
 * @return  0success 
 */
int xml_parser::uninit()
{
    if(!m_inited) {
        return -1;
    }

    m_p_root        = NULL;
    m_p_queue       = NULL;
    m_rrd_handler   = NULL;

    if(m_xml_parser != NULL) {
        XML_ParserFree(m_xml_parser);
        m_xml_parser = NULL;
    }

    m_inited  = false;
    return 0;
}

/** 
 * @brief  ��ʼ������,Ҫôinit�ɹ���Ҫôʧ�ܻ�uninit�Ѿ�init�ı���
 * @param   p_root   ���汾������Դ��Ϣ�ĸ��ڵ�ָ��
 * @param   p_queue  ������е�ָ��
 * @param   p_rrd    rrd���������ָ��
 * @param   collect_interval    �ռ����
 * @return  0:success -1:failed 
 */
int xml_parser::init(source_t *p_root,
        i_ring_queue *p_queue,
        c_rrd_handler *p_rrd,
        unsigned int collect_interval,
        const metric_alarm_vec_t *default_metric_alarm_info,
        const metric_alarm_map_t *specified_metric_alarm_info)
{
    if(m_inited) {
        return -1;
    }

    if(p_root == NULL || p_rrd == NULL || p_queue == NULL) {
        ERROR_LOG("ERROR: Parament cannot be NULL.");
        return -1;
    }

    m_p_root  = p_root;
    m_p_queue = p_queue;
    m_rrd_handler = p_rrd; 
    m_collect_interval = collect_interval; 
    m_p_default_metric_alarm_info = default_metric_alarm_info;
    m_p_specified_metric_alarm_info = specified_metric_alarm_info;

    memset(&m_xml_data, 0, sizeof(m_xml_data)); 
    m_xml_data.host_change = 1;

    m_xml_parser = XML_ParserCreate("UTF-8");
    if(!m_xml_parser) {
        ERROR_LOG("Process XML: unable to create XML parser.");
        m_p_root        = NULL; 
        m_p_queue       = NULL;
        m_rrd_handler   = NULL; 
        return -1;
    }

    m_inited = true;
    return 0;
}

/** 
 * @brief   ���һ��metric
 * @param   attr   �����ַ�������
 * @param   metric metric_tָ�� 
 * @param   type   ��ʾ���͵��ַ��� 
 * @return  void
 */
void xml_parser::fillmetric(const char** attr, metric_t *metric, const char* type)
{
    int              i;
    int              edge = 0;
    struct type_tag *tt = NULL;
    struct xml_tag  *xt = NULL;
    char   *metricval = NULL, *p = NULL;

    metric->slope = -1;

    for(i = 0; attr[i]; i += 2)
    {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if(!xt)
        {
            continue;
        }
        switch(xt->tag)
        {
            case SUM_TAG:
            case VAL_TAG:
                metricval = (char*)attr[i+1];
                tt = in_type_list(type, strlen(type));
                if(!tt)
                {
                    return;
                }
                switch(tt->type)
                {
                    case INT:
                    case TIMESTAMP:
                    case UINT:
                    case FLOAT:
                        metric->val.d = (double)strtod(metricval, (char**)NULL);
                        p = strrchr(metricval, '.');
                        //���ø������ľ���
                        if(p) 
                        {
                            metric->precision = (short int)strlen(p + 1);
                            break;
                        }
                    case STRING:
                        /* We store string values in the 'valstr' field. */
                        break;
                    default:
                        break;
                }
                /**���������͵�val�ֶζ����浽strings*/
                metric->valstr = addstring(metric->strings, &edge, metricval);
                break;
            case TYPE_TAG:
                metric->type = addstring(metric->strings, &edge, attr[i+1]);
                break;
            case UNITS_TAG:
                metric->units = addstring(metric->strings, &edge, attr[i+1]);
                break;
            case ARG_TAG:
                metric->arg = addstring(metric->strings, &edge, attr[i+1]);
                break;
            case TN_TAG:
                metric->tn = atoi(attr[i+1]);
                break;
            case ALARM_TYPE_TAG:
                metric->alarm_type = addstring(metric->strings, &edge, attr[i+1]);
                break;
            case TMAX_TAG:
                metric->tmax = atoi(attr[i+1]);
                break;
            case DMAX_TAG:
                metric->dmax = atoi(attr[i+1]);
                break;
            case SLOPE_TAG:
                metric->slope = addstring(metric->strings, &edge, attr[i+1]);
                break;
            case NUM_TAG:
                metric->num = atoi(attr[i+1]);
                break;
            default:
                break;
        }
    }
    metric->stringslen = edge;
    return;
}

/** 
 * @brief   ���һ��metric_status_info_t�ṹ
 * @param   formular  ��ʽ�ַ���,��ʽ�ַ����ĸ�ʽ��:warning_val;critical_val;operation;normal_interval;retry_interval;max_attempt_count
 * @param   metric_status metric_status_info_tָ�� 
 * @return  0 - success -1 - failed 
 */
int xml_parser::fill_metric_status(metric_status_info_t *metric_status, const char* formular, const char *arg_md5)
{
//    if(metric_status == NULL || formular == NULL)
//    {
//        return -1;
//    }
//
//    char           tmp_formular[MAX_STR_LEN] = {'\0'};
//    char          *fields[MAX_FORMULAR_FIELD] = {0};
//    char          *start = tmp_formular;
//    char          *cur = NULL;
//    unsigned int   i = 0, field_count = 0;
//
//    strncpy(tmp_formular, formular, sizeof(tmp_formular));
//    str_trim(tmp_formular);
//
//    while(*start != '\0' && i < MAX_FORMULAR_FIELD)
//    {
//        cur = start;
//        fields[i++] = cur;
//
//        while(*cur != ';' && *cur != '\0')
//        {
//            cur++;
//        }
//
//        if(*cur == '\0')
//        {
//            break;
//        }
//        *cur = '\0';
//
//        start = cur + 1;
//    }
//    field_count  = i;
//
//    if(field_count < 6)
//    {
//        ERROR_LOG("Too fewer field for <METRIC>'s <MF> attribute.the source formular is :[%s]", formular);
//        return -1;
//    }
//
//    double warning_val = 0, critical_val = 0;
//    if(!is_numeric(fields[0]))
//    {
//        if(is_expresion(fields[0]))
//        {
//            string expr_war("");
//            replace_var_name_by_var_val(fields[0], &expr_war, arg_md5);
//            //DEBUG_LOG("The expr string is:[%s]", expr_war.c_str());
//            if(parse_expr(expr_war.c_str(), &warning_val) != 0)
//            {
//                ERROR_LOG("Parse the warning value failed.the source string is :[%s]", fields[0]);
//                expr_war.clear();
//                return -1;
//            }
//            expr_war.clear();
//        }
//        else
//        {
//            ERROR_LOG("Invalid warning value expresion.the source string is :[%s]", fields[0]);
//            return -1;
//        }
//    }
//    else
//    {
//        warning_val = strtod(fields[0], NULL);
//    }
//
//    if(!is_numeric(fields[1]))
//    {
//        if(is_expresion(fields[1]))
//        {
//            string expr_ctl("");
//            replace_var_name_by_var_val(fields[1], &expr_ctl, arg_md5);
//            //DEBUG_LOG("The expr string is:[%s]", expr_ctl.c_str());
//            if(parse_expr(expr_ctl.c_str(), &critical_val) != 0)
//            {
//                expr_ctl.clear();
//                return -1;
//            }
//            expr_ctl.clear();
//        }
//        else
//        {
//            ERROR_LOG("Invalid critical value expresion.the source string is :[%s]", fields[0]);
//            return -1;
//        }
//    }
//    else
//    {
//        critical_val = strtod(fields[1], NULL);
//    }
//
//    if(!is_intpos(fields[3]) || !is_intpos(fields[4]) || !is_intpos(fields[5]))
//    {
//        ERROR_LOG("formular format error,the source formular sring is :%s", tmp_formular);
//        return -1;
//    }
//    unsigned int step = m_xml_data.ds->step;
//
//    metric_status->wrn_val = warning_val;
//    metric_status->crtcl_val = critical_val;
//    metric_status->op = str2op(fields[2]);
//    metric_status->normal_interval = atoi(fields[3]) / step + 1;
//    metric_status->retry_interval = atoi(fields[4]) / step + 1;
//    metric_status->max_atc = atoi(fields[5]);
//
//    if(metric_status->normal_interval < metric_status->retry_interval)
//    {
//        ERROR_LOG("error alarm config :normal interval[%u],retry interval[%u],max attempt count[%u]", 
//                metric_status->normal_interval, metric_status->retry_interval, metric_status->max_atc);
//        return -1;
//    }
//
//    if(metric_status->op == OP_UNKNOW)
//    {
//        ERROR_LOG("unknow operation:[%s],or operation is NULL", fields[2]);
//        return -1;
//    }
//
//    if(metric_status->wrn_val == 0 && metric_status->crtcl_val == 0)
//    {
//        ERROR_LOG("wrong threshold values:[warning=0 and critical=0].");
//        return -1;
//    }
//
//    if((metric_status->op == OP_GT || metric_status->op == OP_GE) && 
//            metric_status->wrn_val > metric_status->crtcl_val)
//    {
//        ERROR_LOG("wrong threshold values:[warning=%0.2f,critical=%0.2f,operation=\">|>=\"]",
//                metric_status->wrn_val, metric_status->crtcl_val);
//        return -1;
//    }
//
//    if((metric_status->op == OP_LT || metric_status->op == OP_LE) &&
//            metric_status->wrn_val < metric_status->crtcl_val)
//    {
//        ERROR_LOG("wrong threshold values:[warning=%0.2f,critical=%0.2f,operation=\"<|<=\"]",
//                metric_status->wrn_val, metric_status->crtcl_val);
//        return -1;
//    }

    //if(field_count == MAX_FORMULAR_FIELD)
    //{
    //    metric_status->wrn_level = atoi(fields[6]);
    //    metric_status->crtcl_level = atoi(fields[7]);
    //    if(metric_status->wrn_level <= 0 || metric_status->crtcl_level <= 0)
    //    {
    //        ERROR_LOG("wrong alarm levels:[warning level=%u,critical level=%u]",
    //                metric_status->wrn_level, metric_status->crtcl_level);
    //        return -1;
    //    }
    //    strncpy(metric_status->expr, fields[8], MAX_STR_LEN);
    //}

    return 0;
}

/** 
 * @brief   �ж�һ���ַ����ǲ��ǹ�ʽ
 * @param   str     �ַ���
 * @return  true-yes false-no  
 */
bool xml_parser::is_expresion(const char *str)
{
    if(index(str, '$')) {//Ŀǰ��ֻͨ���ַ������Ƿ����'$'���ж�
        return true;
    }

    return false;
}

/** 
 * @brief   �滻һ����ʽ�еı���Ϊֵ
 * @param   raw_expr ԭʼ��ʽ 
 * @param   expr     �滻�õĹ�ʽ 
 * @param   arg_md5      ����
 * @return  void  
 */
void xml_parser::replace_var_name_by_var_val(const char *raw_expr, string *expr, const char *arg_md5)
{
    if(raw_expr == NULL) {
        ERROR_LOG("ERROR: parameter raw_expr cannot be NULL.");
        return;
    }

    ///$inodes_total_by_mount*0.1
    const char *start = raw_expr;
    while(*start != '\0') {
        if(*start != '$') {//���еı�������$��ʼ
            expr->append(1, *start);
            start++;
        } else {
            char var_name[256 + 32] = {0};//�������ֳ��ȼ���һ��md5��ĳ���
            char var_val[64] = {0};
            const char *var_start = start + 1;
            unsigned int idx = 0;
            ///��������²�Ӧ���������֮ǰ����$��
            while(*var_start != '\0' && *var_start != '+' && *var_start != '-' && 
                    *var_start != '/' && *var_start != '*' && *var_start != '$') {
                ///����Ų��¾Ͳ�Ҫ���ˣ�����var_startָ��Ҫ��������,��֤����ı����ܹ���ȷ����
                if(idx < sizeof(var_name)) {
                    var_name[idx++] = *var_start;
                }
                var_start++;
            }
            if(strlen(var_name) > 0) {
                if(strlen(arg_md5) > 0 && idx < sizeof(var_name)) {
                    snprintf(&var_name[idx], sizeof(var_name) - idx, "_%s", arg_md5);
                }
                if(get_val_str_by_name(var_name, var_val, sizeof(var_val)) == 0) {
                    expr->append(var_val);
                    if(*var_start == '$') expr->append(1, '*');//����������������ŵ������������������˷�����
                }
            }
            start = var_start;
        }
    }
    return;
}

/** 
 * @brief   ����һ�����ʽ��ֵ
 * @param   expr   ��ʽ 
 * @param   result �����������ָ��
 * @return  0-success -1-failed  
 */
int xml_parser::parse_expr(const char *expr, double *result)
{
    return do_parse(expr, result);
}

/** 
 * @brief   ��ȡһ��metric��ֵ
 * @param   var_name   ������
 * @param   val_str    ����ֵ�ַ�������Ļ�����
 * @param   len        ����������
 * @return  0-success -1-failed  
 */
int xml_parser::get_val_str_by_name(const char *var_name, char *val_str, unsigned int len)
{
    hash_t *metrics = m_xml_data.host.metrics;//ȡ��ǰhost��metrics ��hash��
    if(metrics == NULL || var_name == NULL || val_str == NULL || len == 0) {
        return -1;
    }

    datum_t key = {(void*)var_name, strlen(var_name) + 1};
    datum_t *data = hash_lookup(&key, metrics);
    if(data == NULL) {
        snprintf(val_str, len, "0");//û�ҵ����߲��Ҵ������Ƿ���0
    } else {
        metric_t *metric_val = (metric_t*)data->data;
        snprintf(val_str, len, "%.2lf", metric_val->val.d);
        datum_free(data);
    }

    return 0;
}

/** 
 * @brief  ��ʼ����<CLUSTER>Ԫ��
 * @param   attr �����ַ�������
 * @param   el  xmlԪ���� 
 * @return  0 = success -1 = failed 
 */
int xml_parser::start_element_cluster(const char *el, const char **attr)
{
    struct xml_tag *xt = NULL;
    datum_t *hash_datum = NULL;
    datum_t hashkey;
    const char *name = NULL;
    source_t *source = NULL;
    unsigned int localtime = 0;

    for(int i = 0; attr[i]; i += 2) {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if(!xt) {
            continue;
        }

        switch(xt->tag) {
        case NAME_TAG:
            name = attr[i+1];
            break;
        case LOCALTIME_TAG:
            localtime = strtoul(attr[i+1], (char **)NULL, 10);
            break;
        case HOSTCHANGE_TAG:
            if(!strcasecmp(attr[i+1], "yes")) {
                m_xml_data.host_change = 1;
            }
            break;
        default:
            break;
        }
    }

    source = &(m_xml_data.source);

    m_xml_data.sourcename = (char*)realloc(m_xml_data.sourcename, strlen(name) + 1);
    strcpy(m_xml_data.sourcename, name);
    hashkey.data = (void*)(m_xml_data.sourcename);
    hashkey.size = strlen(m_xml_data.sourcename) + 1;

    hash_datum = hash_lookup(&hashkey, m_xml_data.root);
    if(!hash_datum) {
        memset((void*)source, 0, sizeof(*source));
        source->node_type = CLUSTER_NODE;

        source->children = hash_create(DEFAULT_CLUSTERSIZE);
        if(!source->children) {
            ERROR_LOG("Could not create hash table for cluster %s", m_xml_data.sourcename);
            return -1;
        }
        DEBUG_LOG("Created hash table for cluster %s", m_xml_data.sourcename);
        hash_set_flags(source->children, HASH_FLAG_IGNORE_CASE);

        source->metric_summary = hash_create(DEFAULT_METRICSIZE);
        if(!source->metric_summary) {
            ERROR_LOG("Could not create summary hash for cluster %s", m_xml_data.sourcename);
            return -1;
        }
        DEBUG_LOG("Created metric summary hash table for cluster %s", m_xml_data.sourcename);
        hash_set_flags(source->metric_summary, HASH_FLAG_IGNORE_CASE);

        source->ds = m_xml_data.ds;
    }
    else {
        memcpy(source, hash_datum->data, hash_datum->size);
        datum_free(hash_datum);
        hash_foreach(source->metric_summary, zero_out_summary, NULL);
    }
    source->localtime = localtime;

    return 0;
}

/** 
 * @brief   ��ʼ����<HOST>Ԫ��
 * @param   attr �����ַ�������
 * @param   el   xmlԪ���� 
 * @return  0 = success -1 = failed 
 */
int xml_parser::start_element_host(const char *el, const char **attr)
{
    datum_t    *hash_datum = NULL;
    datum_t    *rdatum = NULL;
    datum_t     hashkey, hashval;
    struct      xml_tag *xt = NULL;
    uint32_t    tn = 0;
    uint32_t    tmax = 0;
    uint32_t    dmax = 0;
    uint32_t    started = 0;
    const char *name = NULL;
    const char *ip   = NULL; 
    int         i;
    host_t     *host  = NULL;
    hash_t     *hosts = NULL;  
    int         prev_host_status;
    bool        prev_up_status;
    bool        up_failed = false;

    for(i = 0; attr[i]; i += 2) {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if(!xt) {
            continue;
        }
        switch(xt->tag) {
        case TN_TAG:
            tn = atoi(attr[i+1]);
            break;
        case TMAX_TAG:
            tmax = atoi(attr[i+1]);
            break;
        case NAME_TAG:
            name = attr[i+1];
            break;
        case IP_TAG:
            ip = attr[i+1];
            break;
        case DMAX_TAG:
            dmax = strtoul(attr[i+1], (char **)NULL, 10);
            break;
        case STARTED_TAG:
            started = strtoul(attr[i+1], (char **)NULL, 10);
            break;
        case UP_FAILED_TAG:
            if(!strcasecmp(attr[i+1], "yes")) {
                up_failed = true;
            }
            break;
        default :
            break;
        }
    }

    host = &(m_xml_data.host);
    m_xml_data.hostname = (char*)realloc(m_xml_data.hostname, strlen(name) + 1);

    //�����ｫ����ȫ��ת��ΪСд
    for(i = 0; name[i] != 0; i++) {
        m_xml_data.hostname[i] = tolower(name[i]);
    }
    m_xml_data.hostname[i] = 0; 

    hashkey.data = (void*)(m_xml_data.hostname);
    hashkey.size = strlen(m_xml_data.hostname) + 1;

    hosts = m_xml_data.source.children;
    hash_datum = hash_lookup(&hashkey, hosts);
    if(!hash_datum) {
        memset((void*)host, 0, sizeof(*host));
        host->node_type = HOST_NODE;
        host->host_status = HOST_STATUS_OK;

        host->metrics = hash_create(DEFAULT_METRICSIZE);
        if(!host->metrics) {
            ERROR_LOG("Could not create metric hash for host %s", m_xml_data.hostname);
            return -1;
        }

        host->metrics_status = hash_create(DEFAULT_METRICSIZE);
        if(!host->metrics_status) {
            ERROR_LOG("Could not create metric status hash for host %s", m_xml_data.hostname);
            return -1;
        }
    }
    else {
        memcpy(host, hash_datum->data, hash_datum->size);
        datum_free(hash_datum);
    }

    //Check if the host is up. we use the host's TN and TMAX attrs.
    //�������4����tmax��û���µ�����
    if(tn >= tmax * 4) {
        int ret = check_hostalive(ip, 5);
        if(ret == -1 || ret == STATE_WARNING) {///�ж�������popenʧ��
            m_xml_data.host_status = HOST_STATUS_HOST_UNKNOWN;
        }
        else if(ret == STATE_OK) {
            m_xml_data.host_status = HOST_STATUS_AGENT_DOWN;
        }
        else  {
            m_xml_data.host_status = HOST_STATUS_HOST_DOWN;
        }
    }
    else {
        m_xml_data.host_status = HOST_STATUS_OK;
    }

    host->tn = tn;
    host->tmax = tmax;
    strncpy(host->ip, ip, 16);
    host->t0 = m_xml_data.now;
    host->t0.tv_sec -= host->tn;
    host->dmax = dmax;
    host->started = started;

    //host��״̬ 0=ok,1=oa_node down,2= host unknow,3 = host down
    //ȡ��ǰһ�ε�host״̬
    prev_host_status = host->host_status;
    int is_mute = 0;//�Ƿ��֪�û�
    switch (m_xml_data.host_status) {
    case HOST_STATUS_OK:
    {
        if(prev_host_status == HOST_STATUS_OK) {
            break;//do nothing
        } else {
            report_alarm_cmd(OA_HOST_RECOVERY, host->ip, "", "", host_alarm_type[prev_host_status]);
            host->host_status = HOST_STATUS_OK;
        }
        break;
    }
    case HOST_STATUS_AGENT_DOWN:
    {
        if(prev_host_status == HOST_STATUS_OK) {
            host->host_status = HOST_STATUS_AGENT_DOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
        if(prev_host_status == HOST_STATUS_AGENT_DOWN) {
            break;
        }
        if(prev_host_status == HOST_STATUS_HOST_UNKNOWN || prev_host_status == HOST_STATUS_HOST_DOWN) {
            report_alarm_cmd(OA_HOST_RECOVERY, host->ip, "", "", host_alarm_type[prev_host_status]);
            host->host_status = HOST_STATUS_AGENT_DOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
    }
    case HOST_STATUS_HOST_UNKNOWN:
    {
        if(prev_host_status == HOST_STATUS_OK) {
            host->host_status = HOST_STATUS_HOST_UNKNOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
        if(prev_host_status == HOST_STATUS_HOST_UNKNOWN) {
            break;
        }
        if(prev_host_status == HOST_STATUS_AGENT_DOWN || prev_host_status == HOST_STATUS_HOST_DOWN) {
            is_mute = (prev_host_status == HOST_STATUS_AGENT_DOWN) ? 1 : 0;
            report_alarm_cmd(OA_HOST_RECOVERY, host->ip, "", "", host_alarm_type[prev_host_status], is_mute);
            host->host_status = HOST_STATUS_HOST_UNKNOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
    }
    case HOST_STATUS_HOST_DOWN:
    {
        if(prev_host_status == HOST_STATUS_OK) {
            host->host_status = HOST_STATUS_HOST_DOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
        if(prev_host_status == HOST_STATUS_HOST_DOWN) {
            break;
        }
        if(prev_host_status == HOST_STATUS_AGENT_DOWN || prev_host_status == HOST_STATUS_HOST_UNKNOWN) {
            report_alarm_cmd(OA_HOST_RECOVERY, host->ip, "", "", host_alarm_type[prev_host_status], 1);
            host->host_status = HOST_STATUS_HOST_DOWN;
            report_alarm_cmd(OA_HOST_ALARM, host->ip, "", "", host_alarm_type[host->host_status]);
            break;
        }
    }
    default:
        break;
    }

    //ȡ��ǰһ�ε�host up_failed״̬
    prev_up_status = host->up_failed;
    //������up_failed����
    if(up_failed) {//����up_failed
        if(prev_up_status) {//ǰ��Ҳ��up_failed
            //do nothing
        } else { //ǰ�β���up_failed
            report_alarm_cmd(OA_UPDATE_FAIL, host->ip, "", "", "up_failed");
            host->up_failed = up_failed;
        }
    } else {//���β���up_failed
        if(prev_up_status) { //ǰ����up_failed
            report_alarm_cmd(OA_UPDATE_RECOVERY, host->ip, "", "", "up_failed");
            host->up_failed = up_failed;
        } else {//ǰ�β���up_failed
            //do nothing
        }
    }

    hashval.size = sizeof(*host);
    hashval.data = (void*)host;

    rdatum = hash_insert(&hashkey, &hashval, hosts);
    if(!rdatum) {
        ERROR_LOG("Could not insert host %s", m_xml_data.hostname);
        return -1;
    }
    return 0;
}

/** 
 * @brief  ��ʼ����<METRIC>Ԫ��
 * @param   attr �����ַ�������
 * @param   el  xmlԪ���� 
 * @return  0 = success -1 = failed 
 */
int xml_parser::start_element_metric(const char *el, const char **attr)
{
    slope_t slope = SLOPE_UNSPECIFIED;
    struct xml_tag *xt = NULL;
    struct type_tag *tt = NULL;
    datum_t *hash_datum = NULL;
    datum_t *rdatum = NULL;
    datum_t hashkey, hashval;
    const char *name = NULL;
    const char *arg = NULL;
    const char *unit = NULL;
    const char *metricval = NULL;
    const char *type = NULL;
    const char *ci = NULL;
    const char *cleaned = NULL;
    const char *formular = NULL;
    int do_summary;
    int i, edge;
    hash_t *summary = NULL;
    metric_t *metric = NULL;

    //add for alarm
    metric_status_info_t     *metric_status = NULL;
    const char               *alarm_type_str = NULL;
    int                       tmp_alarm_type = 0;
    unsigned char             alarm_type = 0;

    //��������Ѿ�down����Ҫ����
    const char *host_ip = m_xml_data.host.ip;
    if(m_xml_data.host_status != HOST_STATUS_OK) {
        return 0;
    }

    for(i = 0; attr[i]; i+= 2) {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if(!xt) {
            continue;
        }
        switch(xt->tag) {
        case NAME_TAG:
            name = attr[i+1];
            hashkey.data = (void*)name;
            hashkey.size = strlen(name) + 1;
            break;
        case ARG_TAG:
            arg = attr[i+1];
            break;
        case VAL_TAG:
            metricval = attr[i+1];
            break;
        case TYPE_TAG:
            type = attr[i+1];
            break;
        case UNITS_TAG:
            unit = attr[i+1];
            break;
        case SLOPE_TAG:
            slope = cstr_to_slope(attr[i+1]);
            break;
        case ALARM_TYPE_TAG:
            alarm_type_str = attr[i+1];
            break;
        case ALARM_VALUE_TAG:
            formular = attr[i+1];
        case CI_TAG:
            ci = attr[i+1];
            break;
        case CLEANED_TAG:
            cleaned = attr[i+1];
            break;
        default:
            break;
        }
    }

    //if (NULL != cleaned && 0 != atoi(cleaned)) {//���ݹ����ѱ�����
    //    DEBUG_LOG("host[%s] metric[%s] arg[%s] val[%s] expired and be cleaned by node",
    //            host_ip, NULL == name ? "" : name, NULL == arg ? "" : arg, NULL == metricval ? "" : metricval);
    //    report_alarm_cmd(OA_HOST_METRIC_CLEANED, host_ip, name, arg, "", 1);
    //    return 0;
    //}


    if(alarm_type_str == NULL || !is_integer(alarm_type_str) || (tmp_alarm_type = atoi(alarm_type_str)) > 255)
    {
        ERROR_LOG("Unknown metric type:[%s]", alarm_type_str);
        return 0;
    }
    alarm_type = (unsigned char)tmp_alarm_type;

    //if((alarm_type & ALARM_METRIC_TYPE) && formular == NULL)///�������͵��Ǳ�����ʽΪ��
    //{
    //    ERROR_LOG("The metric will report alarm but has no alarm formular.");
    //    return 0;
    //}

    metric = &(m_xml_data.metric);
    memset((void*)metric, 0, sizeof(*metric));
    metric->name = metric->valstr = metric->precision = metric->type = 
        metric->alarm_type = metric->units = metric->arg = metric->slope = -1;

    metric_status = &(m_xml_data.metric_status);

    do_summary = 0;
    tt = in_type_list(type, strlen(type));
    if(!tt) {
        return 0;
    }

    /// ��ֻ���ڱ��� ������ֵ���͵�
    if(!(alarm_type == ALARM_METRIC_TYPE) && (tt->type==INT || tt->type==UINT || tt->type==FLOAT)) {
        do_summary = 1;
    }

    ///��ֵд��rrd���ݿ�
    if(do_summary && ! m_xml_data.ds->dead && !m_xml_data.rval) {
        m_xml_data.rval = m_rrd_handler->write_data_to_rrd("clusters-data",
                m_xml_data.hostname, name, metricval, NULL,
                ci == NULL ? m_xml_data.ds->step : (is_integer(ci)? atoi(ci) : m_xml_data.ds->step),
                m_xml_data.source.localtime, slope);
    }

    if(!(alarm_type == ALARM_METRIC_TYPE)) {//ֻ���ڱ�����metric����Ҫд���ڴ���
        //fill the metric
        metric->node_type = METRIC_NODE;
        fillmetric(attr, metric, type);
        edge = metric->stringslen;
        metric->name = addstring(metric->strings, &edge, name);
        metric->stringslen = edge;
        metric->t0 = m_xml_data.now;
        metric->t0.tv_sec -= metric->tn;

        hashval.size = sizeof(*metric) - HEAD_FRAMESIZE + metric->stringslen;
        hashval.data = (void*)metric;

        rdatum = hash_insert(&hashkey, &hashval, m_xml_data.host.metrics);
        if(!rdatum) {
            ERROR_LOG("Could not insert %s metric", name);
        }
    }

    //start to handler alarm
    if(alarm_type & ALARM_METRIC_TYPE) {
        do {
            hash_datum = hash_lookup(&hashkey, m_xml_data.host.metrics_status);
            if(hash_datum == NULL) {
                memset((void*)metric_status, 0, sizeof(*metric_status));
                metric_status ->node_type = METRIC_STATUS_NODE;
                metric_status->cur_atc = 1;
            }
            else {
                memcpy((void*)metric_status, hash_datum->data, hash_datum->size);
                datum_free(hash_datum);
            }

            char arg_md5[33] = {0};
            ///�����argument������MD5
            if(arg && strlen(arg) > 0) {
                str2md5(arg, arg_md5);
            }

            //if(0 != fill_metric_status(metric_status, formular, arg_md5))
            //{
            //    ERROR_LOG("parse the formular string error.the source formular is:[%s]", formular);
            //    break;
            //}

            string alarm_metric_name(name);
            if(strlen(arg_md5) > 0) {
                std::size_t ret;
                if((ret = alarm_metric_name.find(arg_md5, 0)) != string::npos) {
                    alarm_metric_name.replace(ret - 1, strlen(arg_md5) + 1, "");///��MD5ֵȥ��
                }
            }

            alarm_cond_t alarm_info = {{0}};
            if(0 != find_metric_alarm_info(alarm_metric_name.c_str(), 
                    arg ? arg : "", m_xml_data.host.ip, &alarm_info)) {
                ERROR_LOG("find the metric alarm info of metirc:[%s] with arg:[%s]failed.", 
                        alarm_metric_name.c_str(), arg ? arg : "");
                break;
            }

            if(parse_metric_expression(&alarm_info, arg_md5) != 0) {
                ERROR_LOG("parse the metric alarm value of metirc:[%s] with arg:[%s]failed.", 
                        alarm_metric_name.c_str(), arg ? arg : "");
                break;
            }

            //if(arg && strlen(arg) > 0) {
            //    alarm_metric_name.append("\%2b").append(arg);
            //}

            if(0 != handle_metric_alarm(&m_xml_data.host, &alarm_info, metric_status, 
                        alarm_metric_name.c_str(), arg, metricval)) {
                ERROR_LOG("handler the metric alarm error.the metric value is:[%s]", metricval);
                break;
            }
            alarm_metric_name.clear();

            metric_status->last_alarm = time(0);
            hashval.size = sizeof(*metric_status); 
            hashval.data = (void*)metric_status;

            ///Update metric status info in cluster host metrics_status table. 
            rdatum = hash_insert(&hashkey, &hashval, m_xml_data.host.metrics_status);
            if(!rdatum) {
                ERROR_LOG("Could not insert metric [%s]'s metric_status", name);
            }
        }while(0);
    }

    //��������Դ��summary��metric
    if(do_summary)
    {
        summary = m_xml_data.source.metric_summary;
        hash_datum = hash_lookup(&hashkey, summary);
        if(!hash_datum)
        {
            ///�����Ѿ����õ�metric
        }
        else
        {
            memcpy(&m_xml_data.metric, hash_datum->data, hash_datum->size);
            datum_free(hash_datum);
            metric = &(m_xml_data.metric);

            switch(tt->type)
            {
                case INT:
                case UINT:
                case FLOAT:
                    metric->val.d += (double)strtod(metricval, (char**)NULL);
                    break;
                default:
                    break;
            }
        }

        metric->num++;
        metric->t0 = m_xml_data.now;

        hashval.size = sizeof(*metric) - HEAD_FRAMESIZE + metric->stringslen;
        hashval.data = (void*)metric;

        rdatum = hash_insert(&hashkey, &hashval, summary);
        if(!rdatum) 
        {
            ERROR_LOG("Could not insert %s metric", name);
        }
    }
    return 0;
}

/** 
 * @brief  ����һ��metric��arg��host��Ӧ��alarm��Ϣ
 * @param   metric_name metric name
 * @param   metric_arg  metric argument
 * @param   host_name   server tag
 * @param   p_alarm_info  �澯����
 * @return  0-success, -1-failed 
 */
int xml_parser::find_metric_alarm_info(const char *metric_name, const char *metric_arg,
        const char *host_ip, alarm_cond_t *p_alarm_info)
{
    if(NULL == metric_name || NULL == metric_arg || NULL == host_ip || NULL == p_alarm_info) {
        ERROR_LOG("ERROR: Parameter cannot be NULL.");
        return NULL;
    }

    metric_alarm_map_t::const_iterator its = m_p_specified_metric_alarm_info->find(host_ip);
    if(its != m_p_specified_metric_alarm_info->end()) {
        metric_alarm_vec_t::iterator it = (its->second)->begin();
        while(it != (its->second)->end()) {
            if(!strcmp(it->metric_name, metric_name)) {
                ///���ݿ��������ñȽϲ�����
                if (it->metric_type == OA_MYSQL_TYPE || !strcmp(it->metric_arg, metric_arg)) {
                    memcpy(p_alarm_info, &(it->alarm_info), sizeof(alarm_cond_t));
                    return 0;
                }
            }
            ++it;
        }
    }

    metric_alarm_vec_t::const_iterator itd = m_p_default_metric_alarm_info->begin();
    while(itd != m_p_default_metric_alarm_info->end()) {
        if(!strcmp(itd->metric_name, metric_name)) {
            memcpy(p_alarm_info, &(itd->alarm_info), sizeof(alarm_cond_t));
            return 0;
        }
        ++itd;
    }

    return -1;
}

int xml_parser::parse_metric_expression(alarm_cond_t *alarm_info, const char *arg_md5)
{
    if(alarm_info == NULL) {
        ERROR_LOG("ERROR: parameter cannot be NULL.");
        return -1;
    }

    double warning_val = 0, critical_val = 0;
    if(!is_numeric(alarm_info->warning_val)) {
        if(is_expresion(alarm_info->warning_val)) {
            string expr_war("");
            replace_var_name_by_var_val(alarm_info->warning_val, &expr_war, arg_md5);
            if(parse_expr(expr_war.c_str(), &warning_val) != 0) {
                ERROR_LOG("Parse the warning value failed.the source string is :[%s]", alarm_info->warning_val);
                expr_war.clear();
                return -1;
            }
            expr_war.clear();
        } else {
            ERROR_LOG("Invalid warning value expresion.the source string is :[%s]", alarm_info->warning_val);
            return -1;
        }
    } else {
        warning_val = strtod(alarm_info->warning_val, NULL);
    }

    if(!is_numeric(alarm_info->critical_val)) {
        if(is_expresion(alarm_info->critical_val)) {
            string expr_ctl("");
            replace_var_name_by_var_val(alarm_info->critical_val, &expr_ctl, arg_md5);
            if(parse_expr(expr_ctl.c_str(), &critical_val) != 0) {
                expr_ctl.clear();
                return -1;
            }
            expr_ctl.clear();
        } else {
            ERROR_LOG("Invalid critical value expresion.the source string is :[%s]", alarm_info->critical_val);
            return -1;
        }
    } else {
        critical_val = strtod(alarm_info->critical_val, NULL);
    }

    alarm_info->wrn_val = warning_val;
    alarm_info->crt_val = warning_val;
    //if(alarm_info->wrn_val == 0 && alarm_info->crt_val == 0) {
    //    ERROR_LOG("wrong threshold values:[warning=0 and critical=0].");
    //    return -1;
    //}

    if((alarm_info->op == OP_GT || alarm_info->op == OP_GE) && 
            alarm_info->wrn_val > alarm_info->crt_val) {
        ERROR_LOG("wrong threshold values:[warning=%0.2f,critical=%0.2f,operation=\">|>=\"]",
                alarm_info->wrn_val, alarm_info->crt_val);
        return -1;
    }

    if((alarm_info->op == OP_LT || alarm_info->op == OP_LE) &&
            alarm_info->wrn_val < alarm_info->crt_val) {
        ERROR_LOG("wrong threshold values:[warning=%0.2f,critical=%0.2f,operation=\"<|<=\"]",
                alarm_info->wrn_val, alarm_info->crt_val);
        return -1;
    }

    return 0;
}
/** 
 * @brief  ��ʼ����<EXTRA_DATA>Ԫ��
 * @param   attr �����ַ�������
 * @param   el  xmlԪ���� 
 * @return  0 = success -1 = failed 
 */
int xml_parser::start_element_extra_data(const char *el, const char **attr)
{
    return 0;
}

/** 
 * @brief  ��ʼ����<EXTRA_ELEMENT>Ԫ��
 * @param   attr �����ַ�������
 * @param   el   xmlԪ���� 
 * @return  0 = success -1 = failed 
 */
int xml_parser::start_element_extra_element(const char *el, const char **attr)
{
    int    edge = 0;
    struct xml_tag *xt = NULL;
    int    i, name_off = 0, value_off = 0;
    metric_t metric;
    char *name = getfield(m_xml_data.metric.strings, m_xml_data.metric.name);
    datum_t *rdatum = NULL;
    datum_t hashkey, hashval;
    datum_t *hash_datum = NULL;

    if(m_xml_data.host_status != HOST_STATUS_OK) 
    {
        return 0;
    }
    if(!name)
    {
        return 0;
    }

    hashkey.data = (void*)name;
    hashkey.size =  strlen(name) + 1;

    hash_datum = hash_lookup(&hashkey, m_xml_data.host.metrics);
    if(!hash_datum) 
    {
        return 0;
    }

    memcpy(&metric, hash_datum->data, hash_datum->size);
    datum_free(hash_datum);

    if(metric.ednameslen >= MAX_EXTRA_ELEMENTS) 
    {
        DEBUG_LOG("Can not add more extra elements for[%s].Capacity of %d reached.",
                name, MAX_EXTRA_ELEMENTS);
        return 0;
    }

    edge = metric.stringslen;

    name_off = value_off = -1;
    for(i = 0; attr[i]; i+=2)
    {
        xt = in_xml_list(attr[i], strlen(attr[i]));
        if(!xt) 
        {
            continue;
        }
        switch(xt->tag)
        {
            case NAME_TAG:
                name_off = i;
                break;
            case VAL_TAG:
                value_off = i;
                break;
            default:
                break;
        }
    }

    if((name_off >= 0) && (value_off >= 0)) 
    {
        const char *new_name = attr[name_off + 1];
        const char *new_value = attr[value_off + 1];

        metric.ednames[metric.ednameslen++] = addstring(metric.strings, &edge, new_name);
        metric.edvalues[metric.edvalueslen++] = addstring(metric.strings, &edge, new_value);

        metric.stringslen = edge;

        hashkey.data = (void*)name;
        hashkey.size = strlen(name) + 1;

        hashval.size = sizeof(metric) - HEAD_FRAMESIZE + metric.stringslen;
        hashval.data = (void*)&metric;

        rdatum = hash_insert(&hashkey, &hashval, m_xml_data.host.metrics);
        if(!rdatum)
        {
            ERROR_LOG("Could not insert %s metric", name);
        }
        else
        {
            //��������Դ��summary�е�metric
            hash_t *summary = m_xml_data.source.metric_summary;
            metric_t sum_metric;

            hash_datum = hash_lookup(&hashkey, summary);
            if(hash_datum)
            {
                int found = false;

                memcpy(&sum_metric, hash_datum->data, hash_datum->size);
                datum_free(hash_datum);

                for(i = 0; i < sum_metric.ednameslen; i++)
                {
                    char *chk_name = getfield(sum_metric.strings, sum_metric.ednames[i]);
                    char *chk_value = getfield(sum_metric.strings, sum_metric.edvalues[i]);
                    //���Ԫ���Ѿ�������Ҫ���
                    if(!strcasecmp(chk_name, new_name) && !strcasecmp(chk_value, new_value))
                    {
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    edge = sum_metric.stringslen;
                    sum_metric.ednames[sum_metric.ednameslen++] = 
                        addstring(sum_metric.strings, &edge, new_name);
                    sum_metric.edvalues[sum_metric.edvalueslen++] = 
                        addstring(sum_metric.strings, &edge, new_value);
                    sum_metric.stringslen = edge;
                }

                hashval.size = sizeof(sum_metric) - HEAD_FRAMESIZE + sum_metric.stringslen;
                hashval.data = (void*)&sum_metric;

                rdatum = hash_insert(&hashkey, &hashval, summary);
                if(!rdatum)
                {
                    ERROR_LOG("Could not insert summary %s metric", name);
                }
            }
        }
    }
    return 0;
}

/** 
 * @brief  ��ʼ����<OA_XML>Ԫ��
 * @param   attr �����ַ�������
 * @param   el  xmlԪ���� 
 * @return  0  = success -1 = failed 
 */
int  xml_parser::start_element_xml(const char *el, const char **attr)
{
    struct xml_tag *xt = NULL;
    int             i;

    for(i = 0; attr[i]; i+=2)
    {
        if(!(xt = in_xml_list((char *)attr[i], strlen(attr[i]))))
        {
            continue;
        }

        if(xt->tag == VERSION_TAG)
        {

        }
    } 
    return 0;
}


 /** 
 * @brief   XML_Parser��start�ص�����
 * @param   p_data  �û�����ָ��
 * @param   el      xmlԪ���� 
 * @param   attr    �����ַ������� 
 * @return  void  
 */
void  xml_parser::start_parse(void *p_data, const char *el, const char **attr)
{
    struct xml_tag *xt = NULL;
    xml_parser *parser = (xml_parser*)p_data;

    if(el == NULL || !(xt = in_xml_list((char *)el, strlen(el)))) {
        return;
    }

    switch(xt->tag) {
    case CLUSTER_TAG:
        parser->start_element_cluster(el, attr);
        break;
    case HOST_TAG:
        parser->start_element_host(el, attr);
        break;
    case METRIC_TAG:
        parser->start_element_metric(el, attr);
        break;
    case XML_TAG:
        parser->start_element_xml(el, attr);
        break;
    case EXTRA_DATA_TAG:
        parser->start_element_extra_data(el, attr);
        break;
    case EXTRA_ELEMENT_TAG:
        parser->start_element_extra_element(el, attr);
        break;
    default:
        break;
    }
    return;
}

 /** 
 * @brief   Write a metric summary value to the RRD database.
 * @param   key  hash���key
 * @param   val  hash���val
 * @param   arg  �û����� 
 * @return  0 = success -1 = failed  
 */
int xml_parser::finish_processing_source(datum_t *key, datum_t *val, void *arg)
{
    xml_parser *parser  = (xml_parser*)arg;
    xmldata_t *xmldata = &(parser->m_xml_data);
    //rrd_handler *p_rrd_handler = parser->m_rrd_handler;
    char *name = NULL;
    char *type = NULL;
    char sum[256] = {'\0'};
    char num[256] = {'\0'};
    metric_t *metric = NULL;
    struct type_tag *tt = NULL;

    name = (char*)key->data;
    metric = (metric_t*)val->data;
    type = getfield(metric->strings, metric->type);

    if(xmldata->ds->dead) {
        return -1;
    }

    tt = in_type_list(type, strlen(type));
    if(!tt) {
        return -1;
    }

    switch(tt->type) {
    case INT:
    case UINT:
        snprintf(sum, sizeof(sum) - 1, "%.f", metric->val.d);
        break;
    case FLOAT:
        snprintf(sum, sizeof(sum) - 1, "%.*f", (int)metric->precision, metric->val.d);
        break;
    default:
        break;
    }
    snprintf(num, sizeof(num) - 1, "%u", metric->num);

    //�������ݵ�rrd��,�������Դ�Ѿ�down�˻��߽������ݳ�����ô�Ͳ���
    //if(xmldata->ds->dead && !xmldata->rval) 
    //{
    //    char  ds_name[MAX_STR_LEN] = {0};
    //    const char *fmt = NULL;
    //    fmt = xmldata->source.node_type == CLUSTER_NODE ? "cluster_%u" : "grid_%u";
    //    snprintf(ds_name, sizeof(ds_name), fmt, xmldata->ds->ds_id);  
    //    xmldata->rval = p_rrd_handler->write_data_to_rrd(ds_name, NULL, name,
    //            sum, num, xmldata->ds->step, xmldata->source.localtime,
    //            cstr_to_slope(getfield(metric->strings, metric->slope)));
    //}

    return xmldata->rval;
}

/** 
 * @brief   <CLUSTER/>��ǩ��end���� 
 * @param   el  xmlԪ���� 
 * @return  -1 = failed 0 = success  
 */
int  xml_parser::end_element_cluster(const char *el)
{
    datum_t   hashkey, hashval;
    datum_t  *rdatum  = NULL;
    hash_t   *summary = NULL;
    source_t *source  = NULL;

    source  = &m_xml_data.source;
    summary = m_xml_data.source.metric_summary;

    hashkey.data = (void*)m_xml_data.sourcename;
    hashkey.size = strlen(m_xml_data.sourcename) + 1;

    hashval.data = source;
    hashval.size = sizeof(*source);
    //������Դ���뵽hash��
    rdatum = hash_insert(&hashkey, &hashval, m_xml_data.root);
    if(!rdatum) {
        ERROR_LOG("Could not insert data source %s", m_xml_data.sourcename);
        return -1;
    }
    //д���ݵ�rrd��
    hash_foreach(summary, finish_processing_source, this);
    return 0;
}

/** 
 * @brief   <HOST/>��ǩ��end���� 
 * @param   el  xmlԪ���� 
 * @return  void  
 */
int xml_parser::end_element_host(const char *el)
{
    return 0;
}

 /** 
 * @brief  XML_Parser�Ļص�����
 * @param   p_data �û�����ָ��
 * @param   el     xmlԪ���� 
 * @return  void  
 */
void xml_parser::end_parse(void *p_data, const char *el)
{
    struct xml_tag *xt = NULL;
    xml_parser     *parser = (xml_parser*)p_data;

    if(!(xt = in_xml_list((char*)el, strlen(el)))) {
        return;
    }

    switch(xt->tag) {
    case CLUSTER_TAG:
        parser->end_element_cluster(el);
        break;
    case HOST_TAG:
        parser->end_element_host(el);
        break;
    default:
        break;
    }

    return;
}

 /** 
 * @brief   ����xml�ļ�
 * @param   d   ����Դ��Ϣ
 * @param   buf xml�ļ����� 
 * @return  0 = success -1 = failed  
 */
int xml_parser::process_xml(data_source_list_t *ds, const char *buf)
{
    if(NULL == buf || 0 == strlen(buf)) {
        ERROR_LOG("The xml buffer is empty.");
        return -1;
    }
    if(NULL == ds) {
        ERROR_LOG("The data_source_list_t is NULL.");
        return -1;
    }
    if(!m_inited) {
        ERROR_LOG("This xml_parser object has not been inited.");
        return -1;
    }

    int ret = -1;
    m_xml_data.ds = ds;
    m_xml_data.root = m_p_root->children;

    //ȡ����ʼʱ�䣬��Ϊcleanup�̵߳�����
    gettimeofday(&m_xml_data.now, NULL);

    //���ûص�����
    XML_SetElementHandler(m_xml_parser, start_parse, end_parse);

    //�����û�����
    XML_SetUserData(m_xml_parser, this);
    //��ʼ���� 
    ret = XML_Parse(m_xml_parser, buf, strlen(buf), 1);

    //���XML_Parse����0��������ִ���
    if(!ret) {
        XML_Error errcode = XML_GetErrorCode(m_xml_parser);
        ERROR_LOG("Process XML of DS(%u) failed: XML_ParseBuffer() error at line %d:%s",
                ds->ds_id, (int)XML_GetCurrentLineNumber(m_xml_parser), XML_ErrorString(errcode));
        m_xml_data.rval = -1;
    }

    /* Free memory that might have been allocated in xmldata */
    if(m_xml_data.sourcename) {
        free(m_xml_data.sourcename);
    }

    if(m_xml_data.hostname) {
        free(m_xml_data.hostname);
    }

    //��������ɹ��򷵻�0(memset������)
    return m_xml_data.rval;
}


/** 
 * @brief ����һ��host��metric�ı����������
 * @param   host_info      host_t�ṹ�����ڴ���host��ip��Ϣ
 * @param   metric_status  metric_status_info_t�ṹ�����뵱ǰmetirc�ľ���״̬��Ϣ
 * @param   metric_name    metric name string
 * @param   metric_val     metric value string
 * @return  0 = success, -1 = failed 
 */
int xml_parser::handle_metric_alarm(
        const host_t *host_info,
        const alarm_cond_t *alarm_info,
        metric_status_info_t *metric_status,
        const char *metric_name,
        const char *metric_arg,
        const char *metricval)
{
    if(host_info == NULL || alarm_info == NULL || metric_status == NULL || metric_name == NULL || metricval == NULL) {
        return -1;
    }

    if(!is_numeric(metricval)) {
        return -1;
    }

    //first of all add 1 to the check_count
    metric_status->check_count++;        
    //DEBUG_LOG("Start to handler the alarm of metric %s of host %s, current alarm count is:%u.", metric_name, host_info->ip, metric_status->check_count);

    //ÿ��ȡ�̶���"�������"����ô̽�����(atc)��1��Ȼ����ȡ������0
    int retry_interval = metric_status->is_normal ? alarm_info->retry_interval : alarm_info->normal_interval;
    if(metric_status->check_count % retry_interval == 0) {
        metric_status->cur_atc =
            metric_status->cur_atc == (unsigned int)alarm_info->max_atc ?
            alarm_info->max_atc : metric_status->cur_atc + 1;
        metric_status->check_count = 0;
        //DEBUG_LOG("The alarm count reach the interval,start to do alarm,current attemp count is:%u.", metric_status->cur_atc);
    }
    else {
        return 0;
    }

    double   metric_val = atof(metricval);
    state_t  metric_state = STATE_O; //-1 = unknown 0 = ok, 1 = warning, 2 = critical

    //��õ�ǰ��ֵ�ľ���״̬
    metric_state = get_state(alarm_info->wrn_val, alarm_info->crt_val, metric_val, alarm_info->op);
    status_t prev_status = metric_status->cur_status;
    //DEBUG_LOG("STATUS DEBUG: host[%s] metric[%s] arg[%s] pre_status[%d], cur_status[%d].",
                //host_info->ip, metric_name, metric_arg, prev_status, metric_state);
    //��һ�������Ǹ�ֵ�������
    status_change(&metric_status->cur_status, metric_state, 
            metric_status->cur_atc == (unsigned int)alarm_info->max_atc);

    //DEBUG_LOG("The current metric status = %d.", metric_status->cur_status);

    switch(metric_status->cur_status) {
    case  STATUS_OK:
    {
        if(prev_status == STATUS_SW || prev_status == STATUS_SC) {
            metric_status->is_normal = 0;
            metric_status->check_count = 0;
        }

        //��������
        if(prev_status == STATUS_HW) {
            report_alarm_cmd(OA_HOST_METRIC_RECOVERY, host_info->ip, metric_name, metric_arg, "warning");
        }
        if(prev_status == STATUS_HC) {
            report_alarm_cmd(OA_HOST_METRIC_RECOVERY, host_info->ip, metric_name, metric_arg, "critical");
        }

        metric_status->cur_atc = 1;
        break;
    }  
    case  STATUS_SW:
    {
        if(0 == metric_status->is_normal) {
            //�ı�̽��Ƶ�ȼ�������
            metric_status->is_normal = 1;
            metric_status->check_count = 0;
        } 
        break;
    }
    case  STATUS_HW:
    {
        //����ϴ�Ҳ��HW״̬��ô�Ѿ��������ˣ���Ъһ���
        if(prev_status == STATUS_HW) {
            //break;��Ҫbreak---2011��10��24��13:56:57, ����metricֵ
        }

        if(prev_status == STATUS_HC) {
            //�����ϴε�HC������Ȼ��HW
            report_alarm_cmd(OA_HOST_METRIC_RECOVERY, host_info->ip, metric_name, metric_arg, "critical", 1);
        }

        if(prev_status == STATUS_SW || prev_status == STATUS_SC) {
            //�ı�̽��Ƶ��
            metric_status->is_normal = 0;
            metric_status->check_count = 0;
        }

        metric_alarm_info_t ma;
        char val_str[MAX_STR_LEN] = {'\0'};

        if(!strcmp(metricval, "-1")) {//TODO: ����
            ma.op = OP_EQ;
            ma.threshold_val = "-1";
        } else {
            ma.op = alarm_info->op;
            snprintf(val_str, sizeof(val_str) - 1, "%0.2f", alarm_info->wrn_val);
            ma.threshold_val = val_str;
        }
        ma.cur_val = metricval;

        report_alarm_cmd(OA_HOST_METRIC, host_info->ip, metric_name, metric_arg, "warning", 0, &ma);
        break;
    }
    case  STATUS_SC:
    {
        if(metric_status->is_normal == 0) {
            //�ı�̽��Ƶ�ȼ�������
            metric_status->is_normal = 1;
            metric_status->check_count = 0;
        }
        break;
    }
    case  STATUS_HC:
    {
        //����ϴ�Ҳ��HC״̬��ô�Ѿ��������ˣ���Ъһ���
        if(prev_status == STATUS_HC) {
            //break;��Ҫbreak--2011��10��24��13:58:49,����metricֵ
        }

        if(prev_status == STATUS_HW) {
            //�����ϴε�HW������Ȼ��HC
            report_alarm_cmd(OA_HOST_METRIC_RECOVERY, host_info->ip, metric_name, metric_arg, "warning", 1);
        }

        if(prev_status == STATUS_SW || prev_status == STATUS_SC) {
            //�ı�̽��Ƶ��
            metric_status->is_normal = 0;
            metric_status->check_count = 0;
        }

        metric_alarm_info_t ma;
        char val_str[MAX_STR_LEN] = {'\0'};
        if(!strcmp(metricval, "-1")) {
            ma.op = OP_EQ;
            ma.threshold_val = "-1";
        } else {
            ma.op = alarm_info->op;
            snprintf(val_str, sizeof(val_str) - 1, "%0.2f", alarm_info->crt_val);
            ma.threshold_val = val_str;
        }
        ma.cur_val = metricval;

        report_alarm_cmd(OA_HOST_METRIC, host_info->ip, metric_name, metric_arg, "critical", 0, &ma);
        break;
    }
    default:
        break;
    }

    return 0;
}

/** 
 * @brief ����ǰһ��metric��״̬�͵�ǰmetric�ľ���״̬�����µ�״̬
 * @param   old_status metric��ǰһ��״̬(����һ��ֵ-�����͵Ĳ���)
 * @param   cur_state  ��ǰmetric�ľ���״̬
 * @param   flag       ��־λ��ʾ̽������Ƿ�ﵽ����
 * @return  void 
 */
void xml_parser::status_change(status_t *old_status, const state_t cur_state, bool flag)
{
    static status_change_t status_table[]=
    {
        {STATUS_OK, STATE_O, STATUS_OK, false},
        {STATUS_OK, STATE_O, STATUS_OK, true},

        {STATUS_OK, STATE_W, STATUS_SW, false},
        {STATUS_OK, STATE_W, STATUS_SW, true},

        {STATUS_OK, STATE_C, STATUS_SC, false},
        {STATUS_OK, STATE_C, STATUS_SC, true},

        {STATUS_OK, STATE_U, STATUS_SC, false},//����unknown״̬
        {STATUS_OK, STATE_U, STATUS_SC, true},//����unknown״̬

        {STATUS_SW, STATE_O, STATUS_OK, false},
        {STATUS_SW, STATE_O, STATUS_OK, true},

        //����������ת��״̬(��һ�ֵ�cur_atc<max_atc����,�ڶ��ֵ�cur_atc=max_atc����)
        {STATUS_SW, STATE_W, STATUS_SW, false},
        {STATUS_SW, STATE_W, STATUS_HW, true},

        //����������ת��״̬(��һ�ֵ�cur_atc<max_atc����,�ڶ��ֵ�cur_atc=max_atc����)
        {STATUS_SW, STATE_C, STATUS_SC, false},
        {STATUS_SW, STATE_C, STATUS_HC, true},

        {STATUS_SW, STATE_U, STATUS_SC, false},//����unknown״̬
        {STATUS_SW, STATE_U, STATUS_HC, true}, //����unknown״̬

        {STATUS_SC, STATE_O, STATUS_OK, false},
        {STATUS_SC, STATE_O, STATUS_OK, true},

        //����������ת��״̬(��һ�ֵ�cur_atc<max_atc����,�ڶ��ֵ�cur_atc=max_atc����)
        {STATUS_SC, STATE_W, STATUS_SW, false},
        {STATUS_SC, STATE_W, STATUS_HC, true},

        //����������ת��״̬(��һ�ֵ�cur_atc<max_atc����,�ڶ��ֵ�cur_atc=max_atc����)
        {STATUS_SC, STATE_C, STATUS_SC, false},
        {STATUS_SC, STATE_C, STATUS_HC, true},

        {STATUS_SC, STATE_U, STATUS_SC, false},//����unknown״̬
        {STATUS_SC, STATE_U, STATUS_HC, true}, //����unknown״̬

        {STATUS_HW, STATE_O, STATUS_OK, false},
        {STATUS_HW, STATE_O, STATUS_OK, true},

        {STATUS_HW, STATE_W, STATUS_HW, false},
        {STATUS_HW, STATE_W, STATUS_HW, true},

        {STATUS_HW, STATE_C, STATUS_HC, false},
        {STATUS_HW, STATE_C, STATUS_HC, true},

        {STATUS_HW, STATE_U, STATUS_HC, false},//����unknown״̬
        {STATUS_HW, STATE_U, STATUS_HC, true},//����unknown״̬

        {STATUS_HC, STATE_O, STATUS_OK, false},
        {STATUS_HC, STATE_O, STATUS_OK, true},

        {STATUS_HC, STATE_W, STATUS_HW, false},
        {STATUS_HC, STATE_W, STATUS_HW, true},

        {STATUS_HC, STATE_C, STATUS_HC, false},
        {STATUS_HC, STATE_C, STATUS_HC, true},

        {STATUS_HC, STATE_U, STATUS_HC, false},//����unknown״̬
        {STATUS_HC, STATE_U, STATUS_HC, true}//����unknown״̬
    };

    for(unsigned int i = 0; i < sizeof(status_table) / sizeof(status_table[0]); i++) {
        if(*old_status == status_table[i].old_status
                && cur_state == status_table[i].cur_state 
                && flag == status_table[i].flag) {
            *old_status = status_table[i].ret_status;
            break;
        }
    }
    return;
}

/** 
 * @brief   ȡ�õ�ǰ�ľ���״̬ 
 * @param   wrn_val    warning value for a metric
 * @param   crtcl_val  critical value for a metric
 * @param   cur_val    current value for of metric
 * @param   op         ������ʽ(=,<,>,>=,<=)
 * @return  state_t 
 */
state_t xml_parser::get_state(const double wrn_val, const double crtcl_val, const double cur_val, op_t op)
{
    state_t   ret;

    //�����ǰֵΪ-1����Unknown״̬
    if(cur_val > -1.00000000001 && cur_val < -0.99999999999) {
        return STATE_U;
    }

    switch(op)
    {
    case OP_EQ:
    {
        if(cur_val > wrn_val - 0.0000000001 && cur_val < wrn_val + 0.0000000001 ) {
            ret = STATE_W; 
        } else if(cur_val > crtcl_val - 0.0000000001 && cur_val < crtcl_val + 0.0000000001 ) {
            ret = STATE_C; 
        } else {
            ret = STATE_O;
        }
        break;
    }
    case OP_GT:
    {
        if(cur_val > wrn_val && cur_val <= crtcl_val) {
            ret = STATE_W; 
        } else if(cur_val > crtcl_val) {
            ret = STATE_C; 
        } else {
            ret = STATE_O;
        }
        break;
    }
    case OP_LT:
    {
        if(cur_val < crtcl_val) {
            ret = STATE_C; 
        } else if(cur_val >= crtcl_val && cur_val < wrn_val) {
            ret = STATE_W; 
        } else {
            ret = STATE_O;
        }
        break;
    }
    case OP_GE:
    {
        if(cur_val >= wrn_val && cur_val < crtcl_val) {
            ret = STATE_W; 
        } else if(cur_val >= crtcl_val) {
            ret = STATE_C; 
        } else {
            ret = STATE_O;
        }
        break;
    }
    case OP_LE:
    {
        if(cur_val <= crtcl_val) {
            ret = STATE_C; 
        } else if(cur_val > crtcl_val && cur_val <= wrn_val)
        {
            ret = STATE_W; 
        } else {
            ret = STATE_O;
        }
        break;
    }
    default :
        ret = STATE_O;
        break;
    }
    return ret;
}

/** 
 * @brief   ����web���������� 
 * @param   cmd_id      �澯�����
 * @param   host_ip     host ip
 * @param   metric_name  metric name/host status
 * @param   alarm_level ��������(warning or critical)
 * @param   alarm_info  metric_alarm_info_t
 * @return  void 
 */
void xml_parser::report_alarm_cmd(int cmd_id, const char *host_ip, const char *metric_name,
        const char *metric_arg, const char *alarm_level, int is_mute, 
        const metric_alarm_info_t *alarm_info)
{
    if(NULL == host_ip || NULL == metric_name || NULL == metric_arg || NULL == alarm_level) {
        ERROR_LOG("ERROR: Parameter wrong.");
        return;
    }
    if (OA_HOST_METRIC == cmd_id && NULL == alarm_info) {
        ERROR_LOG("ERROR: Parameter wrong, cmd_id[%d] but alarm_level or alarm_info is NULL.", cmd_id);
        return;
    }
    if (0 == strcmp(host_ip, "0.0.0.0") || NULL != strstr(host_ip, "255")) {
        ERROR_LOG("ERROR: cmd_id[%d] alarm_level[%s] invalid IP[%s].", cmd_id, alarm_level, host_ip);
        return;
    }

    alarm_event_t event_info = {0};
    event_info.cmd_id = cmd_id;
    strncpy(event_info.host_ip, host_ip, sizeof(event_info.host_ip) - 1);
    switch (cmd_id) {
    case OA_HOST_METRIC:
    {
        snprintf(event_info.cmd_data, sizeof(event_info.cmd_data) - 1, 
                "cmd=%d&host=%s&metric=%s&arg=%s&alarm=%s;%s;%s;%s&start_time=%lu&mute=%d",
                cmd_id, host_ip, metric_name, metric_arg,
                alarm_info->cur_val, alarm_info->threshold_val, op2str(alarm_info->op),
                alarm_level, time(NULL), is_mute);
        break;
    }
    case OA_HOST_METRIC_RECOVERY:
    case OA_HOST_METRIC_CLEANED:
    case OA_HOST_ALARM:
    case OA_HOST_RECOVERY:
    case OA_UPDATE_FAIL:
    case OA_UPDATE_RECOVERY:
    {
        snprintf(event_info.cmd_data, sizeof(event_info.cmd_data) - 1, 
            "cmd=%d&host=%s&metric=%s&arg=%s&alarm=%s&start_time=%lu&mute=%d",
            cmd_id, host_ip, metric_name, metric_arg, alarm_level, time(NULL), is_mute);
        break;
    }
    default:
        ERROR_LOG("ERROR: cmd_id[%d] wrong.", cmd_id);
        return;
    }

    char key[MAX_STR_LEN] = {0};
    sprintf(key, "%s;%s", metric_name, metric_arg);
    str2md5(key, event_info.key);

    event_info.data_len = sizeof(event_info) - sizeof(event_info.cmd_data) + strlen(event_info.cmd_data);
    int ret = m_p_queue->push_data((char *)&event_info, event_info.data_len, 1);
    DEBUG_LOG("ALARM INFO: push alarm data:[%s]", event_info.cmd_data);
    DEBUG_LOG("ALARM INFO: push data ret:[%d], error str:[%s]", ret, m_p_queue->get_last_errstr());
    return;
}
