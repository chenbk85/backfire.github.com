#!/bin/bash

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

if ! test -e ./bin/daemon.pid; then
	printf "$red_clr%50s$end_clr\n" "OA_NODE is not running"
	exit -1
fi

pid=`cat ./bin/daemon.pid`
result=`ps -p $pid | wc -l`

if test $result -le 1; then
	printf "$red_clr%50s$end_clr\n" "OA_NODE is not running"
	exit -1
fi

result=`ps -p $pid | wc -l`
while ! test $result -le 1; do
	kill -15 $pid >> /dev/null
	sleep 1
	result=`ps -p $pid | wc -l`
done

printf "$grn_clr%50s$end_clr\n" "OA_NODE has been stopped"

