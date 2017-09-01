#!/bin/bash
#编译代码
code_path=`pwd`
cd ${code_path}
run_path=${code_path}/../deploy
lib3_path=/app/analysis/analysisServer/deploy/3lib
code_lib3_path=${code_path}/l3lib/lib

if [ $# -lt 1 ]; then 
    echo "USAGE: $0 param1" 
    echo "please input option[install,all,pre]"
	exit 1; 
fi

if [ $1 == "compile" ];then  
	#清除
	cd ${run_path}
	./clear.sh all
	#编译
	cd ${code_path}
	./make_libs.sh all && ./make_plugins.sh all
elif [ $1 == "install" ];then 
	#清除
	cd ${run_path}
	./clear.sh all
	#安装
	cd ${run_path}
	./install all
elif [ $1 == "all" ];then 
	#清除
	cd ${run_path}
	./clear.sh all
	#编译
	cd ${code_path}
	./make_libs.sh all && ./make_plugins.sh all
	#安装
	cd ${run_path}
	./install.sh all
elif [ $1 == "pre" ];then
	#准备工作
	find ./ -maxdepth 3 -type f -name "*.sh"  |xargs -i chmod +x {}
	test ! -d ${code_lib3_path} && test -d ${lib3_path} && ln -s ${lib3_path} ${code_lib3_path} && echo "ln 3lib for code"
	test ! -d ${run_path}/bin && mkdir ${run_path}/bin
	test ! -d ${run_path}/lib && mkdir ${run_path}/lib
	test ! -d ${run_path}/plugins && mkdir ${run_path}/plugins
	cd ${run_path} 
	chmod +x ./install.sh && ./install.sh pre && echo "install.sh pre ok"
else
	echo "do nothings"
fi
