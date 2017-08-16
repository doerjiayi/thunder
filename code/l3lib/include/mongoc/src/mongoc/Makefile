CC = gcc
CXX = g++
CXXFLAG = -Wall -g -O0 -fPIC -DNODE_BEAT=10.0  -Wno-deprecated-declarations -Wno-format -Wstrict-overflow 

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

VPATH = . 

SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


LIB_PATH = .

INC := $(INC) \
       -I $(LIB_PATH) \
       -I $(LIB_PATH)/.. \
       -I $(LIB_PATH)/../libbson/src/bson 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ -DMONGOC_COMPILATION \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt -lssl

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

#SO_DEP_PATH = .
#SO_DEP_CPP_SRCS = $(foreach dir, $(SO_DEP_PATH), $(wildcard $(dir)/*.cpp))
#OBJS = $(patsubst %.cpp,%.o,$(SO_DEP_CPP_SRCS))

TARGET = libmonogoc.so

all: $(TARGET)
libmonogoc.so: $(OBJS)
	$(CXX) -fPIE -rdynamic -shared -g -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS) 
	rm -f $(TARGET)
        
        
