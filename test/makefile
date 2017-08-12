CC = gcc
CXX = g++
CXXFLAG = -Wall -Wno-format -g -O2 

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

THUNDER_PATH = ..

VPATH = $(THUNDER_PATH)/test $(THUNDER_PATH)/src/cmd $(THUNDER_PATH)/src/codec $(THUNDER_PATH)/src/labor $(THUNDER_PATH)/src/protocol \
$(THUNDER_PATH)/src/session $(THUNDER_PATH)/src/step $(THUNDER_PATH)/src/storage $(THUNDER_PATH)/src/utility
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

INC := $(INC) \
       -I $(THUNDER_PATH)/src \
       -I $(THUNDER_PATH)/src/utility \
       -I $(THUNDER_PATH)/3lib/include \
       -I $(THUNDER_PATH)/3lib/include/mysql \
       -I $(THUNDER_PATH)/3lib/include/libev \
       -I $(THUNDER_PATH)


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lz -lc -ldl -lrt -lssl \
           -L$(THUNDER_PATH)/3lib/lib -lmysqlclient_r \
           -L$(THUNDER_PATH)/3lib/lib -lhiredis \
           -L$(THUNDER_PATH)/3lib/lib -lcryptopp \
           -L$(THUNDER_PATH)/3lib/lib -lprotobuf \
           -L$(THUNDER_PATH)/3lib/lib -llog4cplus \
           -L$(THUNDER_PATH)/3lib/lib -lev 

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

TARGET: testserver

testserver: $(OBJS)
	$(CXX) $(CXXFLAG) -o $@ $^ $(LDFLAGS)
%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f main.o
	rm -f testserver
        
        
