echo 服务测试
/usr/bin/siege -c 300 -r 300 'http://192.168.18.68:21000/hello POST ${FILE_DATA}'

length:
38
  done.

Transactions:                  90000 hits
Availability:                 100.00 %
Elapsed time:                  12.50 secs
Data transferred:              32.27 MB
Response time:                  0.04 secs
Transaction rate:            7200.00 trans/sec
Throughput:                     2.58 MB/sec
Concurrency:                  298.03
Successful transactions:       90000
Failed transactions:               0
Longest transaction:            0.05
Shortest transaction:           0.00