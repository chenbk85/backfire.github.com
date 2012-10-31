CC		= g++

FLAGS  	= -Wall -fPIC -MMD


SOFLAGS = -shared

LIBPATH = ../../itl-common/

GEN_PROTO_PATH = ../../gen_proto/

INCLUDE = -I$(LIBPATH) -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/ -I/usr/include/libxml2/


LIBS	= -litl_common -L$(LIBPATH) -ltaomee++ -ltaomee -lrrd

BINS	= ../bin/librrd.so

SUBDIRS	 = proto
SRCS 	= $(wildcard *.cpp $(foreach n, $(SUBDIRS), $(n)/*.cpp))
OBJS	= $(patsubst %.cpp, %.o, $(SRCS))

all: dev




dev: FLAGS += -ggdb -DENABLE_TRACE_LOG
rel: FLAGS += -O2



dev rel: $(OBJS) 
	$(CC) $(FLAGS) $(SOFLAGS) $(OBJS) $(LIBS) -o $(BINS)
	@-rm -rf $(OBJS)

%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDE) -c $< -o $@
	@-rm -f $*.d



.PHONY : all clean

clean:
	@-rm -f *.o *.oxx *.po *.so *.d .dep.* $(BINS) ../core

rebuild: clean all

c: clean

r: rebuild

gen:
	$(GEN_PROTO_PATH)/getxml.sh itl_rrd
	cp $(GEN_PROTO_PATH)/proto/itl.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl.cpp ./proto
	rm -f ./proto/itl_rrd*.*
	cp $(GEN_PROTO_PATH)/proto/itl_rrd*.h ./proto
	cp $(GEN_PROTO_PATH)/proto/itl_rrd.cpp ./proto

g: gen

a: g r
