#!/bin/bash

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

if ! test -e ./bin/daemon.pid; then
	printf "$red_clr%50s$end_clr\n" "OA_NODE is not running"
	exit -1
fi

d_pid=`cat ./bin/daemon.pid`
result=`ps -p $d_pid | wc -l`
if test $result -le 1; then
	printf "$red_clr%50s$end_clr\n" "OA_NODE is not running"
	exit -1
fi

ps -fp $d_pid

if  test -e ./bin/collect_r.pid; then
    n_pid=`cat ./bin/collect_r.pid`
    ps -fp $n_pid | grep $n_pid | grep -v grep
fi

if  test -e ./bin/collect_n.pid; then
    r_pid=`cat ./bin/collect_n.pid`
    ps -fp $r_pid | grep $r_pid | grep -v grep
fi

if  test -e ./bin/command.pid; then
    c_pid=`cat ./bin/command.pid`
    ps -fp $c_pid | grep $c_pid | grep -v grep
fi

if  test -e ./bin/network.pid; then
    w_pid=`cat ./bin/network.pid`
    ps -fp $w_pid | grep $w_pid | grep -v grep
fi
