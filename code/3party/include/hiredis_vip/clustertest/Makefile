all: 
	#gcc -L/app/tools/redis-cluster/hiredis-vip-master -lhiredis_vip my_sync.c -o my_sync
	#gcc -L/app/tools/redis-cluster/hiredis-vip-master -lhiredis_vip my_sync2.c -o my_sync2
	#gcc -L/app/tools/redis-cluster/hiredis-vip-master -lhiredis_vip my_pipeline.c -o my_pipeline
	gcc -L/app/tools/redis-cluster/hiredis-vip-master -lhiredis_vip -L/app/analysis/lib/3lib -lev \
	-I/app/analysis/analysisServer/code/l3oss/include/libev my_async.c -o my_async 

	#export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/tools/redis-cluster/hiredis-vip-master:/app/3lib/lib
