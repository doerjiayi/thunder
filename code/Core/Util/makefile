CXX = $(shell command -v ccache >/dev/null 2>&1 && echo "ccache g++" || echo "g++")
CXXFLAG = -Wall -Wno-format -std=c++11  -g -O2 -fPIC -fno-strict-aliasing -Wno-sign-compare

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

3PARTY_PATH = ../../3party
UTIL_PATH = .
PROTO_PATH = ../Proto
DEPLOY_LIB = ../../../deploy/lib/

#暂时把框架节点协议放入util
VPATH = $(UTIL_PATH)/src $(PROTO_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS) 

INC := $(INC) \
       -I /usr/include \
       -I $(3PARTY_PATH)/include \
       -I $(3PARTY_PATH)/include/mariadb \
       -I $(UTIL_PATH)/src

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(3PARTY_PATH)/lib -lev \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lc -ldl -lrt

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

LIBRARYS: $(OBJS)
	#ar -crv libUtil.a $(OBJS)
	#ranlib libUtil.a
	$(CXX) -shared -Wl,-soname,libUtil.so -o libUtil.so $(OBJS) -lc -lrt -ldl $(LDFLAGS)
	test -d ${DEPLOY_LIB} && install libUtil.so ${DEPLOY_LIB}
	#install -v libUtil.a $(UTIL_PATH)/lib/
	install -v libUtil.so $(UTIL_PATH)/lib/

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
%.o:%.c
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< 
clean:
	rm -f $(OBJS)
	rm -f $(LIBRARYS) 

cleanall:
	rm -f $(OBJS)
	rm -f $(LIBRARYS) 
	rm libUtil.a libUtil.so ./lib/libUtil.a ./lib/libUtil.so
        
        
