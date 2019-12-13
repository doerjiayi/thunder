安装yum install siege -y
修改配置vim ~/.siege/siege.conf
siege "http://192.168.11.66:27008/gentoken" -r 100 -c 100
siege "http://192.168.11.66:27008/Echo" -r 100 -c 100
siege "http://192.168.11.66:27008/gentoken?token=6718307704189747201&key=6718307704189747202" -r 100 -c 100



yum -y install httpd-tools
ab -c 500 -n 100000 http://192.168.11.66:27008/Echo
ab -c 500 -n 100000 http://192.168.11.66:27008/gentoken