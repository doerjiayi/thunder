CC = gcc
CXX = g++
CFLAGS = -g -O0 #-Wall
CPPFLAGS =  -D_GNU_SOURCE=1 -D_REENTRANT -fPIC -DNODE_BEAT=10.0 -Wno-deprecated-declarations
#CXXFLAG = -g -O2 -Wall -fdiagnostics-show-caret -Wextra -pedantic -Wstrict-aliasing -Wstrict-overflow -Woverloaded-virtual -Wold-style-cast -Wc++0x-compat -Wc++11-compat -Wundef -Wshadow -Wformat -Wsuggest-attribute=format -Wsuggest-attribute=noreturn -Wno-variadic-macros -fvisibility=hidden
CXXFLAG = -O0 -Wall -ggdb -m64 -Wl,--export-dynamic # -lrt -std=c++11 -pthread  #-static-libgcc -static-libstdc++

DEFINE_SERVER = -DNODE_TYPE_NORMAL
DEFINE_CENTER_SERVER = -DNODE_TYPE_CENTER
DEFINE_GATE_SERVER = -DNODE_TYPE_GATE

YELLOW = "\e[33;1m"
WHITE = "\e[37;1m"

ifeq ($(unit_test),y)
CPPFLAGS += -DUNIT_TEST
endif

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
SYSTEM_LIB_PATH:=/usr/lib64
else
SYSTEM_LIB_PATH:=/usr/lib
endif

THUNDER_3LIB_PATH = ../../l3lib
THUNDER_LIB_PATH = ../../llib/src
THUNDER_PATH = ..

INC := $(INC) \
       -I $(THUNDER_3LIB_PATH)/include \
       -I $(THUNDER_3LIB_PATH)/include/mysql \
       -I $(THUNDER_3LIB_PATH)/include/libev \
       -I $(THUNDER_LIB_PATH)/ \
       -I $(THUNDER_PATH)/src \
       -I $(THUNDER_PATH)/

LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(SYSTEM_LIB_PATH) -lpthread \
           -L$(SYSTEM_LIB_PATH) -lz -lc -ldl -lrt -lssl \
           -L$(THUNDER_3LIB_PATH)/lib -lmysqlclient_r \
           -L$(THUNDER_3LIB_PATH)/lib -lhiredis \
           -L$(THUNDER_3LIB_PATH)/lib -lcryptopp \
           -L$(THUNDER_3LIB_PATH)/lib -lprotobuf \
           -L$(THUNDER_3LIB_PATH)/lib -llog4cplus \
           -L$(THUNDER_3LIB_PATH)/lib -lev 

SUB_INCLUDE = cmd step session labor codec protocol storage 
COMM_PATH = $(THUNDER_PATH)/src/cmd \
            $(THUNDER_PATH)/src/step \
            $(THUNDER_PATH)/src/session \
            $(THUNDER_PATH)/src/labor \
            $(THUNDER_PATH)/src/protocol \
            $(THUNDER_PATH)/src/storage 

#执行文件
VPATH = $(THUNDER_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)

CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

#公共库
COMM_CPP_SRCS = $(foreach dir, $(COMM_PATH), $(wildcard $(dir)/*.cpp))
COMM_CC_SRCS = $(foreach dir, $(COMM_PATH), $(wildcard $(dir)/*.cc))
COMM_C_SRCS = $(foreach dir, $(COMM_PATH), $(wildcard $(dir)/*.c))
COMM_OBJS = $(patsubst %.cpp,%.o,$(COMM_CPP_SRCS)) $(patsubst %.c,%.o,$(COMM_C_SRCS)) $(patsubst %.cc,%.o,$(COMM_CC_SRCS))

COMPILE_TARGETS = ThunderServer ThunderCenterServer ThunderGateServer libthunder.so
SERVER_TARGETS = LogicServer DataProxyServer DbAgentWrite DbAgentRead LogServer RobotServer CrmServer
GATE_TARGETS = WebServer LoginServer GateServer FileServer ESAgentR ESAgentW ESAgentS
CENTER_TARGETS = CenterServer


all: $(COMPILE_TARGETS)
	@for target in $(SERVER_TARGETS);do cp -vf ThunderServer $$target; done
	@for target in $(CENTER_TARGETS);do cp -vf ThunderCenterServer $$target; done
	@for target in $(GATE_TARGETS);do cp -vf ThunderGateServer $$target; done

ThunderServer:$(OBJS)
	echo -e $(YELLOW) "$(CXX) $(CXXFLAG) $(DEFINE_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)"
	echo -e $(WHITE)
	$(CXX) $(CXXFLAG) $(DEFINE_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)
	
ThunderCenterServer:$(OBJS)
	echo -e $(YELLOW) "$(CXX) $(CXXFLAG) $(DEFINE_CENTER_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)" $(WHITE)
	$(CXX) $(CXXFLAG) $(DEFINE_CENTER_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)
	
ThunderGateServer:$(OBJS)
	echo -e $(YELLOW) "$(CXX) $(CXXFLAG) $(DEFINE_GATE_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)" $(WHITE)
	$(CXX) $(CXXFLAG) $(DEFINE_GATE_SERVER) $(UNITTEST) -o $@ $(OBJS) $(LDFLAGS) $(INC)

libthunder.so: $(COMM_OBJS)
	$(CXX) $(UNITTEST) -fPIE -rdynamic -shared -g -o $@ $^ $(LDFLAGS)
	mkdir -p $(THUNDER_PATH)/lib/
	cp -f libthunder.so $(THUNDER_PATH)/lib/

#安装头文件
#mkdir -p $(THUNDER_PATH)/include/
#@for dir in $(SUB_INCLUDE); do mkdir -p $(THUNDER_PATH)/include/$$dir; 	cp $(THUNDER_PATH)/src/$$dir/*.hpp $(THUNDER_PATH)/include/$$dir/ >> /dev/null 2>&1; cp $(THUNDER_PATH)/src/$$dir/*.h $(THUNDER_PATH)/include/$$dir/ >> /dev/null 2>&1; done
#cp -f $(THUNDER_PATH)/src/*.hpp $(THUNDER_PATH)/include/
install: 
	#安装执行文件
	@for bin in $(TARGETS); do install -v $$bin ../../../deploy/bin/ ;done
	#安装库文件
	install -v libthunder.so ../../../deploy/lib/

test: 
	cd ../test && make && ./test
	
%.o:%.cpp
	$(CXX) $(INC) $(CPPFLAGS) $(CFLAGS) $(DEFINE_SERVER) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CPPFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CFLAGS) $(UNITTEST) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -f ./*Server ./DbA* ./*.so ./ESA*


