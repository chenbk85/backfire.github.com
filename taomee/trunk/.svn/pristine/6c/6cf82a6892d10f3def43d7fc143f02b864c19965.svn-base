CC = g++
FLAGS = -g -Wall -fPIC `mysql_config --cflags`


TARGET = libitl_common.a

SRCS	= $(wildcard *.cpp)
OBJS	= $(patsubst %.cpp, %.o, $(SRCS))

INCLUDE = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/ -I/usr/include/libxml2/
LIBS = -ltaomee++ -ltaomee `mysql_config --libs`

all: $(TARGET)

$(TARGET): $(OBJS)
	ar -rvs $(TARGET) $(OBJS)
	@-rm -rf $(OBJS)

%.o: %.cpp
	$(CC) $(FLAGS) $(INCLUDE) -c $< -o $@


.PHONY : all clean

clean:
	@-rm -f $(TARGET) $(OBJS) 

rebuild: clean all

c: clean

r: rebuild


a: gen all
