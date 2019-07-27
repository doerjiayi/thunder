#!/bin/bash
#编译代码
code_path=`pwd`
cd ${code_path}
run_path=${code_path}/../deploy

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[core,proto,plugin,pre,compile,install]"
	exit 1; 
fi

if [ "$1"x == "compile"x ];then  
	#编译
	cd ./core && ./make_libs.sh all
	cd ${code_path} && ./proto_logic.sh all && ./plugins_logic.sh all
elif [ "$1"x == "install"x ];then 
	#清除
	cd ${run_path}
	./clear.sh all
	#安装
	cd ${run_path}
	./install all
elif [ "$1"x == "core"x ];then 
	cd ./Core &&  ./core.sh all && cd ..
elif [ "$1"x == "all"x ];then 
	#准备工作
	find ./ -maxdepth 5 -type f -name "*.sh"  |xargs -i chmod +x {}
	#拷贝makefile模板到逻辑节点，使用core服务器库需要使用指定统一makefile模板
	./Core/makefiles/makefilecopy.sh
	#编译
	cd ./Core &&  ./core.sh all && cd ..
	cd ${code_path} && ./proto_logic.sh all && ./plugins_logic.sh all
elif [ "$1"x == "proto"x ];then 
	cd ${code_path} && ./proto_logic.sh all
elif [ "$1"x == "plugin"x ];then 
	./plugins_logic.sh all
elif [ "$1"x == "pre"x ];then
	#准备工作
	find ./ -maxdepth 5 -type f -name "*.sh"  |xargs -i chmod +x {}
else
	echo "do nothings"
fi
