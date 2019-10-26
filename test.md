
测试接入层和逻辑层简单交互：token服务，生成token和访问token

查看服务
lsof -Pni4 | grep LISTEN 
请求生成token 和 key
curl 'http://192.168.3.6:27008/Interface/gentoken'
验证token 和 组合key
curl 'http://192.168.3.6:27008/Interface/gentoken?token=6718307704189747201&key=6718307704189747202'

