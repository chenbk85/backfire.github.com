XX       = g++
CFLAGS   = -Wall -O -ggdb -Wno-non-virtual-dtor -D_GNU_SOURCE 
LIBS     = -L ./commonlib/ -lxml2 -lpthread -lcairo -lrrd -lcurl  `mysql_config --libs`
TARGET   = ../bin/oa_head
INCLUDES = -I ./commonlib/ -I /usr/include/libxml2/
SOURCES  = $(wildcard ./*.cpp)
OBJS     = $(patsubst %.cpp,%.o,$(SOURCES)) 

#SUBSRCDIR = `find . ! -path "." -type d | grep -v svn`
#SUBSOURCES  = $(foreach dir, $(SUBSRCDIR), $(wildcard $(dir)/*.cpp))
#SUBOBJS  = $(patsubst %.cpp,%.o,$(SUBSOURCES))

SUBSOURCES  = $(wildcard ./*/*.cpp) $(wildcard ./data-process/*/*.cpp)
SUBOBJS  = $(patsubst %.cpp,%.o,$(SUBSOURCES) $(SUBSOURCES2))


all: dev

dev: CFLAGS += -DENABLE_TRACE_LOG
rel: CFLAGS += 

dev rel : $(OBJS) $(SUBOBJS)
	$(XX) $(OBJS) $(SUBOBJS) $(SUBOBJS2) -o $(TARGET) $(LIBS)
	rm -rf *.o
	
%.o: %.cpp
	$(XX) $(CFLAGS) $(INCLUDES) -c $< -o $@	
	
$(SUBOBJS):
	@make_all_sub_dir()\
	{\
		for dir in `ls`; do \
			if [ -d $$dir ]; then \
				cd $$dir;\
				make_all_sub_dir;\
				if [ -f makefile -o -f Makefile ]; then\
					make;\
				fi;\
				cd ..;\
			fi;\
		done;\
	};\
	make_all_sub_dir		

clean:
	rm -rf *.o $(TARGET)
	@make_clean_sub_dir()\
	{\
		for dir in `ls`; do \
			if [ -d $$dir ]; then \
				cd $$dir;\
				make_clean_sub_dir;\
				if [ -f makefile -o -f Makefile ]; then\
					make clean;\
				fi;\
				cd ..;\
			fi;\
		done;\
	};\
	make_clean_sub_dir
	
rebuild: clean all	
	
c: clean

r: rebuild
