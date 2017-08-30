压测指令
echo 服务测试（消息体长度38字节）
/usr/bin/siege -c 300 -r 300 'http://192.168.18.68:21000/hello POST ${FILE_DATA}'

压测结果
Transactions:                  90000 hits
Availability:                 100.00 %
Elapsed time:                  12.30 secs
Data transferred:              32.27 MB
Response time:                  0.04 secs
Transaction rate:            7317.07 trans/sec
Throughput:                     2.62 MB/sec
Concurrency:                  298.41
Successful transactions:       90000
Failed transactions:               0
Longest transaction:            0.07
Shortest transaction:           0.00

资源消耗
ps -ef |grep HelloThunder
imdev     4725     1  0 15:22 ?        00:00:03 HelloThunder                                                                                                   
imdev     4727  4725  0 15:22 ?        00:00:25 HelloThunder_W0                                                                                                
imdev    22550 14836  0 16:18 pts/10   00:00:00 grep HelloThunder

top -p  4727
top - 16:16:12 up 302 days,  6:43,  8 users,  load average: 0.08, 0.03, 0.01
Tasks:   1 total,   1 running,   0 sleeping,   0 stopped,   0 zombie
Cpu(s): 19.2%us, 14.6%sy,  0.0%ni, 61.6%id,  0.0%wa,  0.0%hi,  4.6%si,  0.0%st
Mem:  16207408k total, 12515028k used,  3692380k free,   263868k buffers
Swap:  4194296k total,  4194292k used,        4k free,  3006920k cached

PID USER      PR  NI  VIRT  RES  SHR S %CPU %MEM    TIME+  COMMAND       
4727 imdev     20   0 92464  15m 4380 R 100.0  0.1   0:18.35 HelloThunder_W0        

top -p  4725
top - 16:19:11 up 302 days,  6:46,  8 users,  load average: 0.09, 0.05, 0.01
Tasks:   1 total,   0 running,   1 sleeping,   0 stopped,   0 zombie
Cpu(s): 19.0%us, 13.7%sy,  0.0%ni, 62.6%id,  0.0%wa,  0.0%hi,  4.6%si,  0.0%st
Mem:  16207408k total, 12508020k used,  3699388k free,   263876k buffers
Swap:  4194296k total,  4194292k used,        4k free,  3008752k cached

PID USER      PR  NI  VIRT  RES  SHR S %CPU %MEM    TIME+  COMMAND                                                                                                                                  
4725 imdev     20   0 73228 3368 2096 S 13.7  0.0   0:04.41 HelloThunder 
