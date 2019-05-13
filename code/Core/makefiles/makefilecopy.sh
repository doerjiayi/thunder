#!/bin/bash
#为了方便逻辑节点，统一使用的makefile文件模板
#拷贝makefile模板到指定节点 ./makefilecopy.sh AccessServer
#make_plugins_list.conf 

makefiles=./Core/makefiles

echo "copying makefile..."
#复制makefile
if [ $# -lt 1 ]; then 
	echo "$#"
    while read nodetype src_path dest_path others
	do 
		if test -d "./${nodetype}/src" 
		then
	        echo "${nodetype}:"
	        echo "cp -f ${makefiles}/makefileserver ./${nodetype}/src/makefile"
			cp -f ${makefiles}/makefileserver ./${nodetype}/src/makefile
			#复制makefile
		 	for file in `ls ./${nodetype}/src`
			do 
				if test -d "./${nodetype}/src/${file}"
				then
					if [ "${file:0:3}" == "Cmd" ] || [ "${file:0:6}" == "Module" ]
					then
						echo "$file ${file:0:3}"
						echo "cp -f ${makefiles}/makefileso ./${nodetype}/src/${file}/Makefile"
						cp -f ${makefiles}/makefileso ./${nodetype}/src/${file}/Makefile
					else
						for subfile in `ls ./${nodetype}/src/${file}`
						do
							if test -d "./${nodetype}/src/${file}/${subfile}"
							then
								if [ "${subfile:0:3}" == "Cmd" ] || [ "${subfile:0:6}" == "Module" ]
								then
									echo "$subfile ${subfile:0:3}"
									echo "cp -f ${makefiles}/makefileso2 ./${nodetype}/src/${file}/${subfile}/Makefile"
									cp -f ${makefiles}/makefileso2 ./${nodetype}/src/${file}/${subfile}/Makefile
								fi
							fi
						done				
					fi
				fi
			done
		else
			echo "${nodetype} not exist"
		fi
	done < plugins_logic.conf 
else
	echo "$# ${1}" 
	nodetype=${1}
	if test -d "./${nodetype}/src" 
	then
        echo "${nodetype}:"
        echo "cp -f ${makefiles}/makefileserver ./${nodetype}/src/makefile"
		cp -f ${makefiles}/makefileserver ./${nodetype}/src/makefile
		#复制makefile
	 	for file in `ls ./${nodetype}/src`
		do 
			if test -d "./${nodetype}/src/${file}"
			then
				if [ "${file:0:3}" == "Cmd" ] || [ ${file:0:6} == "Module" ]
				then
					echo "$file ${file:0:3}"
					echo "cp -f ${makefiles}/makefileso ./${nodetype}/src/${file}/Makefile"
					cp -f ${makefiles}/makefileso ./${nodetype}/src/${file}/Makefile
				fi
			fi
		done
	else
		echo "${nodetype} not exist"
	fi
fi


    
