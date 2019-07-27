 基于规则的分词方法
分词算法--单词索引树
基于TRIE索引树（又称单词索引树 http://baike.baidu.com/view/1436495.htm）的逐字匹配算法,是建立在树型词典机制上，
匹配的过程是从索引树的根结点依次同步匹配待查词中的每个字，可以看成是对树 某一分枝的遍历。
每个问题搜索出所有的分词，把所有的分词进行哈希并保存到map表，map表的键为分词的哈希值，哈希表的值为文档的内容，每个分词对应多个文档。
在程序启动是加载所有的引擎问题的文档内容（包括问题和答案）（问题和其内容保存在redis和mysql）。
修改知识库问题时，会增量发送到搜索引擎，并在下一个定时器触发时重建索引。每隔较长一段时间也会重新全量获取引擎问题的文档内容，并重建索引。
定期检查文档是否需要重建，记录最近重建分词map表时间。

词库来源于搜狗词库
在程序启动时，加载所有的词库到单词索引树，会消耗一定的内存建立常驻内存的单词索引树。

词库停用词
属于停用词的单词，不会被加载到单词索引树。


问题检索 最大匹配方案
每个请求的问题搜索出所有的分词，每个分词获取其对应的一个或者多个问题的文档，并把所有的结果进行合并，被匹配文档id最多的则选择为相关度最大的文档，并返回其对应的文档内容。

问题检索 编辑距离方案
对于被匹配分词个数相同的文档，比较其引擎问题与请求问题的编辑距离，距离较近的优先级越高。

词频（需要优化）
需要根据TFIDF权值计算方式，增加词频优先级处理。

测试：
搜索引擎在词库 354507个， ai问题 43个 时，占用实际内存为3.6g
ps 指令 RSZ 真实内存使用（KB）  VSZ 虚拟内存量（KB）
[imdev@node3 RobotServer]$ ps -e -o "pid,comm,args,pcpu,rsz,vsz"|grep robot | sort -nr --key=5
23039 Robot_robot_W0  Robot_robot_W0               0.4 3725884 3902228
12972 fdfs_storaged   /app/robot/fastdfs/FastDFS/  0.0 2541212 2985408
26508 fdfs_trackerd   /app/robot/fastdfs/FastDFS/  0.0 83552 430596

top指令
[imdev@node3 RobotServer]$ top -p 23039
top - 14:01:58 up 208 days,  4:29,  4 users,  load average: 0.00, 0.02, 0.00
Tasks:   1 total,   0 running,   1 sleeping,   0 stopped,   0 zombie
Cpu(s):  0.1%us,  0.3%sy,  0.0%ni, 99.6%id,  0.0%wa,  0.0%hi,  0.0%si,  0.0%st
Mem:  16207408k total, 15891692k used,   315716k free,   270420k buffers
Swap:  4194296k total,   119168k used,  4075128k free,  4679012k cached

  PID USER      PR  NI  VIRT  RES  SHR S %CPU %MEM    TIME+  COMMAND
23039 imdev     20   0 3810m 3.6g 6104 S  0.0 23.0   0:07.02 Robot_robot_W0


搜索引擎在词库 620579个， ai问题 43个 时，占用实际内存为6.7g
ps -e -o "pid,comm,args,pcpu,rsz,vsz"|grep robot | sort -nr --key=5
8346 Robot_robot_W0  Robot_robot_W0               3.3 7020252 7200716

top -p 8346
top - 15:22:39 up 215 days,  5:49,  6 users,  load average: 0.08, 0.72, 0.79
Tasks:   1 total,   0 running,   1 sleeping,   0 stopped,   0 zombie
Cpu(s):  2.4%us,  0.4%sy,  0.0%ni, 97.2%id,  0.0%wa,  0.0%hi,  0.0%si,  0.0%st
Mem:  16207408k total, 14375812k used,  1831596k free,    35612k buffers
Swap:  4194296k total,  3158660k used,  1035636k free,   288376k cached

  PID USER      PR  NI  VIRT  RES  SHR S %CPU %MEM    TIME+  COMMAND
 8346 imdev     20   0 7031m 6.7g 6272 S  0.0 43.3   0:16.17 Robot_robot_W0

压测：
SearchKeys(toSearchLine,words):toSearchLine(你我金融是什么) use time(60.000000)ms,try num(10000) words size(4)
 toSearchLine(你我金融是什么) SearchWords word(你我) wordNo(1)
 toSearchLine(你我金融是什么) SearchWords word(金) wordNo(2)
 toSearchLine(你我金融是什么) SearchWords word(融) wordNo(3)
 toSearchLine(你我金融是什么) SearchWords word(是什) wordNo(4)
约160000qps

SearchKeys(toSearchLine,words):toSearchLine(你我贷，国内领先的在线P2P信用投融资平台（是互联网金融 ITFIN 产品的一种），金融改革践行者，专注中小微企业、个体户及农户的资金发展需求，长期致力于全民信用体系的构建与完善。)
use time(70.000000)ms,try num(1000) words size(40)
约14000qps

结巴
GetBestAiQuestionByQuestion(toSearchLine,question):toSearchLine(最远到期月是多少) use time(20.000000)ms,try num(1000)
ai question(你好，最远到期月是多少) uiMatchWordsCounter(5) nLevenshtein(9)
use time(150.000000)ms,try num(1000) ai question(你好，你是什么性别呀) uiMatchWordsCounter(6) nLevenshtein(230)

结巴 关键词
 GetBestAiQuestionByQuestion(toSearchLine,question):toSearchLine(最远到期月是多少) use time(30.000000)ms,try num(1000)
ai question(你好，最远到期月是多少) uiMatchWordsCounter(3) nLevenshtein(9) dWweight(21.234035)

结巴 关键词(未使用Levenshtein) qps 50 *　1000 = 5w qps
GetBestAiQuestionByQuestion(toSearchLine,question):toSearchLine(最远到期月是多少) use time(20.000000)ms,try num(1000)
ai question(你好，最远到期月是多少) uiMatchWordsCounter(3) nLevenshtein(0) dWweight(21.234035)

结巴 关键词(使用了lcs) qps 50 *　1000 = 5w qps
AiQuestionListByReqQuestion toSearchLine(最远到期月是多少) use time(20.000000)ms,try num(1000) aiQuestionVec size(1)
Test() toSearchLine(最远到期月是多少),best match ai question(你好，最远到期月是多少) questionid(4)
Test() toSearchLine(最远到期月是多少) match ai question(你好，最远到期月是多少) questionid(4) questionNo(1) uiMatchWordsCounter(3) dWweight(21.234035),nLevenshtein(0) nLCS(24)
GetBestAiQuestionByQuestion(toSearchLine,question):toSearchLine(最远到期月是多少) use time(10.000000)ms,try num(1000) ai question(你好，最远到期月是多少) uiMatchWordsCounter(3) d
 
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:80] Test() 保兑银行:no
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:86] Test() SearchWords word(银行,4.670437) wordNo(1)
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:86] Test() SearchWords word(保兑,11.739204) wordNo(2)
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:90] Test() 韩玉鉴赏:no
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:96] Test() SearchWords word(鉴赏,9.543969) wordNo(1)
[2019-04-21 00:25:09,952][INFO] [../../src/SearchEngine/SearchEngine.cpp:96] Test() SearchWords word(韩玉,11.739204) wordNo(2)
 
分类：
 toSearchLine(你我金融是什么) use time(180.000000)ms,try num(10000) oClassifyQuestionVec.size(3)
    uiMatchCounter(2) strReqQuestionType(生活百科) no(1)
    strReqQuestionType(生活百科) match word(你我)
    strReqQuestionType(生活百科) match word(金)
    uiMatchCounter(1) strReqQuestionType(人文科学) no(2)
    strReqQuestionType(人文科学) match word(融)
    uiMatchCounter(1) strReqQuestionType(社会科学) no(3)
    strReqQuestionType(社会科学) match word(是什)
 toSearchLine(你我贷，国内领先的在线P2P信用投融资平台（是互联网金融 ITFIN 产品的一种），金融改革践行者，专注中小微企业、个体户及农户的资金发展需求，长期致力于全民信用体系的构建与完善。)
 use time(140.000000)ms,try num(1000) oClassifyQuestionVec.size(5)
oClassifyQuestionVec uiMatchCounter(18) strReqQuestionType(生活百科) no(1)
strReqQuestionType(生活百科) match word(你我)
strReqQuestionType(生活百科) match word(国)
strReqQuestionType(生活百科) match word(信)
strReqQuestionType(生活百科) match word(投)
strReqQuestionType(生活百科) match word(资)
strReqQuestionType(生活百科) match word(金)
strReqQuestionType(生活百科) match word(的一)
strReqQuestionType(生活百科) match word(金)
strReqQuestionType(生活百科) match word(行者)
strReqQuestionType(生活百科) match word(微)
strReqQuestionType(生活百科) match word(资)
strReqQuestionType(生活百科) match word(金)
strReqQuestionType(生活百科) match word(发)
strReqQuestionType(生活百科) match word(期)
strReqQuestionType(生活百科) match word(力)
strReqQuestionType(生活百科) match word(信)
strReqQuestionType(生活百科) match word(系)
strReqQuestionType(生活百科) match word(建)
oClassifyQuestionVec uiMatchCounter(10) strReqQuestionType(社会科学) no(2)
strReqQuestionType(社会科学) match word(内领)
strReqQuestionType(社会科学) match word(平台)
strReqQuestionType(社会科学) match word(改)
strReqQuestionType(社会科学) match word(革)
strReqQuestionType(社会科学) match word(企业)
strReqQuestionType(社会科学) match word(个体)
strReqQuestionType(社会科学) match word(展)
strReqQuestionType(社会科学) match word(需求)
strReqQuestionType(社会科学) match word(民)
strReqQuestionType(社会科学) match word(体)
oClassifyQuestionVec uiMatchCounter(4) strReqQuestionType(人文科学) no(3)
strReqQuestionType(人文科学) match word(融)
strReqQuestionType(人文科学) match word(融)
strReqQuestionType(人文科学) match word(融)
strReqQuestionType(人文科学) match word(善)
oClassifyQuestionVec uiMatchCounter(2) strReqQuestionType(工程应用) no(4)
test() strReqQuestionType(工程应用) match word(在线)
strReqQuestionType(工程应用) match word(互联网)
oClassifyQuestionVec uiMatchCounter(1) strReqQuestionType(农林鱼畜) no(5)
strReqQuestionType(农林鱼畜) match word(产品)
