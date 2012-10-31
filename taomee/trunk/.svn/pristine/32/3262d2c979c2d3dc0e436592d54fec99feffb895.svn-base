CC		= g++

FLAGS  	= -Wall -fPIC -MMD -rdynamic `mysql_config --cflags`


SOFLAGS = -shared

LIBPATH = ../../itl-common/

GEN_PROTO_PATH = ../../gen_proto/

INCLUDE = -I$(LIBPATH) -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/ -I/usr/include/libxml2/


LIBS	= -litl_common -L$(LIBPATH) -ltaomee++ -ltaomee -lcurl -lxml2 -lglib-2.0 `mysql_config --libs`

BINS	= ../bin/libnode.so

SUBDIRS	 = proto db_mgr collect
SRCS 	= $(wildcard *.cpp $(foreach n, $(SUBDIRS), $(n)/*.cpp))
OBJS	= $(patsubst %.cpp, %.o, $(SRCS))

all: dev



dev: FLAGS += -ggdb -DENABLE_TRACE_LOG -DINSIDE_ETH0 -DMETRIC_LOG -DSHORT_COLLECT_INTERVAL

rel: FLAGS += -O2 -DSET_PROC_USER




dev rel: $(OBJS) 
	$(CC) $(FLAGS) $(SOFLAGS) $(OBJS) $(LIBS) -o $(BINS).$@
	@-ln -sf $(BINS).$@ $(BINS)
	#@-rm -rf $(OBJS)

%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDE) -c $< -o $@
	@-rm -f $*.d



.PHONY : all clean

clean:
	@-rm -f $(OBJS) *.oxx *.po *.so *.d .dep.* $(BINS) ../core

rebuild: clean all

c: clean

r: rebuild

gen: gen_node gen_db gen_head gen_control


gen_node:
	$(GEN_PROTO_PATH)/getxml.sh itl_node
	cp $(GEN_PROTO_PATH)/proto/itl.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl.cpp ./proto
	rm -f ./proto/itl_node*.*
	cp $(GEN_PROTO_PATH)/proto/itl_node*.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl_node.cpp ./proto

gen_db:
	$(GEN_PROTO_PATH)/getxml.sh itl_db
	rm -f ./proto/itl_db*.*
	cp $(GEN_PROTO_PATH)/proto/itl_db*.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl_db.cpp ./proto


gen_head:
	$(GEN_PROTO_PATH)/getxml.sh itl_head
	rm -f ./proto/itl_head*.*
	cp $(GEN_PROTO_PATH)/proto/itl_head*.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl_head.cpp ./proto

gen_control:
	$(GEN_PROTO_PATH)/getxml.sh itl_control
	rm -f ./proto/itl_control*.*
	cp $(GEN_PROTO_PATH)/proto/itl_control*.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl_control.cpp ./proto


g: gen

a: g r
