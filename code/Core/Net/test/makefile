CC = gcc
CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXXFLAG = -Wall -Wno-format -std=c++11  -g -O2 

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif
3PARTY_PATH = ../../../3party
UTIL_PATH = ../../Util

NET_PATH = ..

VPATH = $(NET_PATH)/test $(NET_PATH)/src/codec $(NET_PATH)/src/protocol
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(3PARTY_PATH)/include \
       -I $(UTIL_PATH)/src -I $(UTIL_PATH)/src/Util \
       -I $(NET_PATH)/src 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(UTIL_PATH)/lib -lUtil \
           -L$(3PARTY_PATH)/lib -lprotobuf \
           -L$(3PARTY_PATH)/lib -lev \
           -L$(3PARTY_PATH)/lib -lmariadb \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -ldl

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

TARGET: StarLoad

StarLoad: $(OBJS)
	$(CXX) $(CXXFLAG) -o $@ $^ $(LDFLAGS)
%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f main.o
	rm -f StarLoad
        
        
