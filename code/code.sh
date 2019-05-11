#!/bin/bash

if [ $# -ne 1 ]
then
    echo "usage: $0 all/Net/Core/Logic or  \$worker_path."
    exit 0
fi

CODE_LINES_ALL=0
CODE_LINES=0

function compute_line()
{
    local file_name=$1
    local protofile=`echo $file_name |grep .pb.`
    if [ -z $protofile ]; then
    	local lines=`wc -l $file_name | awk '{print $1}'`
    	CODE_LINES=`expr $CODE_LINES + $lines`
    fi
}


function rotate_compute_line()
{
    #echo `pwd`
    local found_files=`ls -l | grep "^-" | awk '{if (match($NF, ".cpp$") || match($NF, ".h$") || match($NF, ".cc$") || match($NF, ".c$") || match($NF, ".cxx$") || match($NF, ".hpp$") || match($NF, ".hh$") || match($NF, ".hxx$") ) {print $NF}}'`
    target_files=${found_files}
    #echo "target_files = $target_files"
    for f in $target_files
    do
        compute_line $f
    done
}

function rotate_dir()
{
    local current_dir=`pwd`
    local target_dir=$1
    cd "$current_dir/$target_dir"
    local found_dirs=`ls -l | grep "^d" | awk '{print $NF}'`
    local sub_dirs=${found_dirs}
    #echo "sub_dirs = {${sub_dirs}}"
    if [ -n "${sub_dirs}" ]
    then
        for d in $sub_dirs
        do
            #echo "rotate_dir $d"
            rotate_dir $d
        done
    fi
    rotate_compute_line
    cd $current_dir
}

echo "code line:"
if [ "$1"x == "all"x ]; then  
    filelist=`ls -t .`
    #echo "filelist $filelist"
    for filename in $filelist ; do
        CODE_LINES=0
        test -d ${filename} && rotate_dir ${filename} && echo "${filename} : $CODE_LINES" && let CODE_LINES_ALL+=$CODE_LINES
    done
    echo "all line: $CODE_LINES_ALL"
elif [ "$1"x == "Net"x ]; then 
    arr=('Core/Util','Core/Net') 
    filelist=`ls -t .`
    #echo "filelist $filelist"
    for filename in $filelist ; do
        if echo "${arr[@]}" | grep -w "$filename" &>/dev/null; then
            CODE_LINES=0
            test -d ${filename} && rotate_dir ${filename} && echo "${filename} : $CODE_LINES" && let CODE_LINES_ALL+=$CODE_LINES
        fi 
    done
    echo "all line: $CODE_LINES_ALL"
elif [ "$1"x == "Core"x ]; then 
    arr=('Core') 
    filelist=`ls -t .`
    #echo "filelist $filelist"
    for filename in $filelist ; do
        if echo "${arr[@]}" | grep -w "$filename" &>/dev/null; then
            CODE_LINES=0
            test -d ${filename} && rotate_dir ${filename} && echo "${filename} : $CODE_LINES" && let CODE_LINES_ALL+=$CODE_LINES
        fi 
    done
    echo "all line: $CODE_LINES_ALL"
elif [ "$1"x == "Logic"x ]; then 
	arr=('Collect','Session','User') 
    filelist=`ls -t .`
    #echo "filelist $filelist"
    for filename in $filelist ; do
        if echo "${arr[@]}" | grep -w "$filename" &>/dev/null; then
            CODE_LINES=0
            test -d ${filename} && rotate_dir ${filename} && echo "${filename} : $CODE_LINES" && let CODE_LINES_ALL+=$CODE_LINES
        fi 
    done
    echo "all line: $CODE_LINES_ALL"
else
    WORK_PATH=$1
    test ! -d $WORK_PATH && echo "${WORK_PATH} is not dir,please input all,frame,or dir name" && exit 1
    rotate_dir $WORK_PATH
    echo "code line: $WORK_PATH : $CODE_LINES"
fi
    





