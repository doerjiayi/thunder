CC = gcc
CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXXFLAG = -g -Og -fPIC -DNODE_BEAT=10.0 -Wall -Wno-deprecated-declarations -Wno-format -std=c++11  -Wstrict-overflow

ifeq ($(unit_test),y)
CXXFLAG += -DUNIT_TEST
endif

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
3PARTY_PATH = ../../../../3party
UTIL_PATH = ../../../Util
PROTO_PATH = ../../../Proto

NET_PATH = ../../../Net
PLUGIN_PATH = ../..

VPATH = . \
        .. \
        $(PLUGIN_PATH)/protocol \
        $(NET_PATH)/src/cmd \
        $(NET_PATH)/src/step \
        $(NET_PATH)/src/session \
        $(NET_PATH)/src/labor \
        $(NET_PATH)/src/protocol 
SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I .. \
       -I $(PROTO_PATH) -I $(PROTO_PATH)/src -I $(PROTO_PATH)/include \
       -I $(3PARTY_PATH)/include \
       -I $(3PARTY_PATH)/include/mariadb \
       -I $(3PARTY_PATH)/include/redlock \
       -I $(UTIL_PATH)/src -I $(UTIL_PATH)/src/util \
       -I $(NET_PATH)/src 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(UTIL_PATH)/lib -lUtil \
           -L$(3PARTY_PATH)/lib -lhiredis_vip \
           -L$(3PARTY_PATH)/lib -lhiredis -lredlock \
           -L$(3PARTY_PATH)/lib -lprotobuf \
           -L$(3PARTY_PATH)/lib -llog4cplus \
           -L$(3PARTY_PATH)/lib -ljemalloc -ltcmalloc \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -ldl -lrt  

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

SO_PATH = . #$(NET_PATH)/plugins
STEP_PATH = .
COMM_PATH = .. \
            $(NET_PATH)/plugins/protocol \
            $(PROTO_PATH) \
            $(NET_PATH)/src/cmd \
            $(NET_PATH)/src/step \
            $(NET_PATH)/src/session \
            $(NET_PATH)/src/labor \
            $(NET_PATH)/src/protocol 
            #$(NET_PATH)/src

SO_PATH = . 
SO_DEP_PATH = $(SO_PATH) $(PLUGIN_PATH)/src
STEP_PATH = .
CMD_SO_CPP_SRCS = $(foreach dir, $(SO_PATH), $(wildcard $(dir)/Cmd*.cpp))
MODULE_SO_CPP_SRCS = $(foreach dir, $(SO_PATH), $(wildcard $(dir)/Module*.cpp))
SO_CPP_SRCS = $(CMD_SO_CPP_SRCS) $(MODULE_SO_CPP_SRCS)
STEP_CPP_SRCS = $(foreach dir, $(STEP_PATH), $(wildcard $(dir)/*.cpp))
CMD_DEP_CPP_SRCS = $(foreach dir, $(SO_DEP_PATH), $(wildcard $(dir)/*.cpp))
CMD_DEP_C_SRCS = $(foreach dir, $(SO_DEP_PATH), $(wildcard $(dir)/*.c))
STEP_OBJS = $(patsubst %.cpp,%.o,$(STEP_CPP_SRCS))
SO_OBJS = $(patsubst %.cpp,%.o,$(SO_CPP_SRCS))
CMD_DEP_CPP_OBJS = $(patsubst %.cpp,%.o,$(CMD_DEP_CPP_SRCS)) 
CMD_DEP_C_OBJS = $(patsubst %.c,%.o,$(CMD_DEP_C_SRCS))
PB_OBJS = $(patsubst %.pb.cc,%.pb.o,$(CC_SRCS))
SOS = $(patsubst %.o,%.so,$(SO_OBJS))

TARGET = $(SOS)

all: $(TARGET)

.SECONDARY: $(PB_OBJS) $(CMD_DEP_CPP_OBJS) $(CMD_DEP_C_OBJS)  

Cmd%.so:Cmd%.o $(PB_OBJS) $(CMD_DEP_CPP_OBJS) $(CMD_DEP_C_OBJS)  
	$(CXX) -fPIC -rdynamic -shared -g -o $@ $^ $(LDFLAGS)
Module%.so:Module%.o $(PB_OBJS) $(CMD_DEP_CPP_OBJS) $(CMD_DEP_C_OBJS)  
	$(CXX) -fPIC -rdynamic -shared -g -o $@ $^ $(LDFLAGS)

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(SO_OBJS) $(STEP_OBJS)
	rm -f $(TARGET)
        
        
