#!/bin/bash

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

if ! test -e ./bin/daemon.pid; then
	printf "$red_clr%50s$end_clr\n" "OA_HEAD is not running"
	exit -1
fi


proc_names=`cat ./bin/daemon.pid | grep -v process | awk '{print $1}'`
pids=`cat ./bin/daemon.pid | awk '{print $2}'`

for proc_name in $proc_names;do
    result=`ps -ef | grep $proc_name | grep -v grep| wc -l`
    if [ "$result" -lt "1" ]; then
    	printf "$grn_clr%50s$end_clr\n" "$proc_name is not running"
    else
    	printf "$grn_clr%50s$end_clr\n" "$proc_name is running"
        pid=`ps -ef | grep $proc_name | grep -v grep | awk '{print $2}'` 
    	printf "$grn_clr%50s$end_clr\n" "kill $pid."
        kill $pid
    	printf "$grn_clr%50s$end_clr\n" "kill $proc_name succ."
        sleep 1
    fi
done
exit;

