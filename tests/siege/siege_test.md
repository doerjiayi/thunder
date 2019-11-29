安装yum install siege -y
修改配置vim ~/.siege/siege.conf

siege "http://192.168.3.6:27008/Interface/gentoken" -r 100 -c 100