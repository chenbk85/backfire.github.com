#!/bin/bash

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

#判断是否是root用户
if [ `echo $USER` != "root" ]
then
    printf "$red_clr%70sIt must be root to run this shell.$end_clr%s\n"
    exit -1
fi

proc_names=`cat ./bin/daemon.pid | grep -v process | awk '{print $1}'`
pids=`cat ./bin/daemon.pid | awk '{print $2}'`

is_all_stop="true"
for proc_name in $proc_names;do
    result=`ps -ef | grep $proc_name | grep -v grep`
    if [ "$result" != "" ]; then
    	printf "$grn_clr%50s$end_clr\n" "$proc_name is still running"
        is_all_stop="false"
    fi
done

if [ "$is_all_stop" == "false" ];then
    exit -1
fi

#启动用户当前所在的绝对路径
cwd=`pwd`

#本shell所在的目录
path=`dirname $0`

cd $cwd && cd $path && cd ./bin 


./oa_head -h10.1.1.27 -ddb_itl -uroot -pta0mee -nxx
exit 0
