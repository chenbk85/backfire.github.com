#!/bin/bash

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

#判断是否是root用户
if [ `echo $USER` != "root" ]
then
    printf "$red_clr%50sIt must be root to run this shell.$end_clr%s\n"
    exit -1
fi

#本shell所在的目录
path=`dirname $0`
#启动用户当前所在的绝对路径
cwd=`pwd`
cd $cwd && cd $path && cd ./bin

#判断是否已经运行
if test -e ./daemon.pid; then
	pid=`cat ./daemon.pid`
	result=`ps -p $pid | wc -l`
	if test $result -gt 1; then
		printf "$red_clr%50s$end_clr\n" "OA_NODE already running\n"
		exit -1
	fi
fi

#循环查找两个网卡的ip
i=0
while [ $i -lt 2 ]
do
    ipaddr=`ifconfig eth$i | grep "inet addr" | cut -f 2 -d ":" | cut -f 1 -d " "`
#if test `echo $ipaddr | grep "192.168"`
    if test `echo $ipaddr | grep "10.1."`
    then 
    break
    fi
    i=$(($i+1))
done
if [ -z ${ipaddr} ]; then 
    ipaddr=`ifconfig bond0 | grep "inet addr" | cut -f 2 -d ":" | cut -f 1 -d " "`
#echo $ipaddr | grep "192.168"
    echo $ipaddr | grep "10.1."
fi
#更新服务器url
update_server_url='http://10.1.1.63/oa-auto-update/index.php'
#update_server_url='http://itl-update.taomee.com/oa-auto-update/index.php'

./oa_node -c$ipaddr -s$update_server_url

exit 0

