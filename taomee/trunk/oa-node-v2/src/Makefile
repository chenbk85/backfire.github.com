XX      = g++
CFLAGS  = -Wall -ggdb -Wno-non-virtual-dtor -pg
LIBS    = -lpthread -ldl -lcurl
INCLUDE	= -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
TARGET  = ../bin/oa_node

SUBDIRS	= monitor-process network-process collect-process command-process lib config collect-process/collect-thread
SRCS 	= $(wildcard *.cpp $(foreach n, $(SUBDIRS), $(n)/*.cpp))
OBJS	= $(patsubst %.cpp, %.o, $(SRCS))

#SOURCES  = $(wildcard *.cpp)
#OBJS     = $(patsubst %.cpp,%.o,$(SOURCES))

#SUBSOURCES  = $(wildcard ./*/*.cpp ./*/*/*.cpp)
##SUBOBJS  = $(patsubst %.cpp,%.o,$(SUBSOURCES)) ./lib/libcurl.a ./lib/libssh2.a
#SUBOBJS  = $(patsubst %.cpp,%.o,$(SUBSOURCES))

all: dev

dev: CFLAGS += -DENABLE_TRACE_LOG -D__TEST_NOT_UPDATE
rel: CFLAGS += 


dev rel: $(OBJS) 
	$(XX) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
	@-rm -rf $(OBJS)

%.o: %.cpp
	$(XX) $(CFLAGS) $(INCLUDE) -c -o $@ $<





.PHONY : all clean
	

clean:
	@-rm -f *.o *.oxx *.po *.so *.d .dep.* $(TARGET) core
	
rebuild: clean all	
	
c: clean

r: rebuild
