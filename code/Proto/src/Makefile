CC = gcc
CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXXFLAG = -g -Og -fPIC -DNODE_BEAT=10.0 -Wall -Wno-deprecated-declarations -Wno-format -std=c++11  -Wstrict-overflow

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
3PARTY_PATH = ../../Core/../3party
PROTO_PATH = .
NET_PATH = ../../Core/Net

VPATH = .

SUB_DIRS := $(VPATH)
#SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(PROTO_PATH) \
       -I $(PROTO_PATH)/src   \
       -I $(NET_PATH)/src \
       -I $(3PARTY_PATH)/include 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(3PARTY_PATH)/lib -lprotobuf \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

SO_PATH = . 
PB_OBJS = $(patsubst %.pb.cc,%.pb.o,$(CC_SRCS))

TARGET = libProto.so

all: $(TARGET)

$(TARGET): $(OBJS) 
	$(CXX) -fPIC -rdynamic -shared -g -o $@ $^ $(LDFLAGS)
	cp $(TARGET) ../../../deploy/lib

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS) 
	rm -f $(TARGET)
        
        
