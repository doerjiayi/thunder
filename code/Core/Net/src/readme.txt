编译说明：
_GNU_SOURCE：
_ISOC99_SOURCE, _POSIX_SOURCE, _XOPEN_SOURCE都是功能测试宏，
用于指示是否包含对应标准的特性，
各种标准化工作（ANSI, ISO, POSIX, FIPS等），
不同的标准支持实现了不同的特性，
如系统时间的获取，stat结构是在ANSI标准中是不支持的，
定义了_GNU_SOURCE相当于开启了对所有特性的支持。

ccache
加速c++编译缓存软件

-g -Og：
开启编译信息，优化编译级别

-ggdb：
产生的debug信息更倾向于给GDB使用的。一般常用gdb

_REENTRANT：
（1）它会对部分函数重新定义它们的可安全重入的版本，这些函数名字一般不会发生改变，只是会在函数名后面添加_r字符串，如函数名gethostbyname变成gethostbyname_r。
（2）stdio.h中原来以宏的形式实现的一些函数将变成可安全重入函数。
（3）在error.h中定义的变量error现在将成为一个函数调用，它能够以一种安全的多线程方式来获取真正的errno的值

NODE_BEAT：
通知中心的心跳时间

-std=c++11：
必须支持c++11，至少是gcc4.8

-Wno-deprecated-declarations：
关闭过期警告，用到某些第三方库

 -Wno-pmf-conversions：
关闭函数转化警告，用到成员函数 与普通函数指针转换

-m64： 
64位重新编译，避免其他系统编译过

-Wl,--export-dynamic：
链接器产生可执行文件时会将所有全局符号导出到动态符号表，为了方便动态库中调用别的动态库

-Wall：
开启警告信息


makefiles文件夹：
makefile文件模板。会被脚本拷贝到各个编译目录

