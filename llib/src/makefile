CXX = g++
CXXFLAG = -Wall -Wno-format -g -O2 -fPIC

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

L3LIB_PATH = ../../l3lib
LLIB_PATH = ..

VPATH = $(LLIB_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

INC := $(INC) \
       -I /usr/include \
       -I $(L3LIB_PATH)/include -I $(L3LIB_PATH)/include/mysql \
       -I $(LLIB_PATH)/src

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(SYSTEM_LIB_PATH) -lpthread -lc -ldl -lrt \
           -L$(L3LIB_PATH)/lib -lprotobuf -lev -llog4cplus

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

LIBRARYS: $(OBJS)
	ar -crv libllib.a $(OBJS)
	ranlib libllib.a
	$(CXX) -shared -Wl,-soname,libllib.so -o libllib.so $(OBJS) -lc -lrt -ldl $(LDFLAGS)
	mkdir -p $(LLIB_PATH)/lib/
	cp -f libllib.a $(LLIB_PATH)/lib/
	cp -f libllib.so $(LLIB_PATH)/lib/
	
install:
	test -d ../../deploy/lib/ && install libllib.so ../../deploy/lib/

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
%.o:%.c
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
clean:
	rm -f $(OBJS)
	rm -f $(LIBRARYS) libllib.a libllib.so
	rm -f libllib.a libllib.so
        
