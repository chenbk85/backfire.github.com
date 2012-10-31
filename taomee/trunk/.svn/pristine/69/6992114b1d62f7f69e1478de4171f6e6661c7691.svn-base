#!/bin/bash

./stop_oa_node.sh
svn up
cd src
make clean
make
cd ..
./start_oa_node.sh
