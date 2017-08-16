#ifndef __FILE_UTIL_H__
#define __FILE_UTIL_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
// 常用文件与目录操作函数

namespace llib
{
    enum FileResult
    {
        eFileResult_system_error = -1,
        eFileResult_param_error = -2,
        eFileResult_file_already_Exist = -3,
        eFileResult_dir_already_Exist = -4,
        eFileResult_archive_already_Exist = -5,
        eFileResult_create_file_error = -6,
        eFileResult_create_dir_error = -7,
        eFileResult_archive_not_Exist = -8,
        eFileResult_dir_not_Exist = -9,
    };
    const char* GetFilesErrorStr(int nErrno);
	/* 判断文件或目录是否存在
	* 在操作系统中，目录也是一个文件，如果要判断一个目录是否存在则应当使用DirectoryExists，
	* 要判断一个文件是否存在且是一个归档文件则应当使用IsArchive。
	* @如果文件或目录存在则返回true否则返回false
	*/
	bool FileExists(const char* sFilePath);
	
	/* 判断文件是否存在且是一个可直接读取的常规文件
	 */
	bool IsArchive(const char* sFilePath);
	
	/* 判断目录是否存在，文件存在且文件是一个目录则返回true否则返回false
	 */
	bool IsDirectory(const char* sDirPath);

	/* 获取文件或目录名称
	 * /abc/123.txt --> 123.txt
	 * /abc/efg/ --> efg
	 * 参数sDirBuf用于存储文件名称字符串
	 * 参数dwBufLen为sNameBuf参数的缓冲区字符（非字节）长度，其中含需要保留的终止字符，
		    如果dwBufLen值为0则函数不会将文件名拷贝到sNameBuf中；
		    如果dwBufLen值非0则函数会将文件名拷贝到sNameBuf中并会在sNameBuf中写入终止符;
		    如果缓冲区不够则只拷贝dwBufLen-1个字符并会在sNameBuf中写入终止符。
		@函数返回拷贝文件名所需的字符长度（含终止符）
	*/
	unsigned int ExtractFileName(const char* sFilePath, char* sNameBuf, unsigned int dwBufLen);
	
	/* 获取文件路径中的文件名部分，不包含文件后缀部分
	 * /abc.txt --> abc
	 * /abc --> abc
	 * 参数sNameBuf用于存储文件名称字符串
	 * 参数dwBufLen为sNameBuf参数的缓冲区字符（非字节）长度，其中含需要保留的终止字符，
		    如果dwBufLen值为0则函数不会将文件名拷贝到sNameBuf中；
		    如果dwBufLen值非0则函数会将文件名拷贝到sNameBuf中并会在sNameBuf中写入终止符;
		    如果缓冲区不够则只拷贝dwBufLen-1个字符并会在sNameBuf中写入终止符。
		@函数返回拷贝文件名所需的字符长度（含终止符）
	*/
	unsigned int ExtractFileNameOnly(const char* sFileName, char* sNameBuf, unsigned int dwBufLen);
	
	/* 获取文件名或文件路径中的文件后缀部分
	 * abc.txt --> .txt
	 * 返回值包含后缀符号'.'
	 */
	const char* ExtractFileExt(const char* sFileName);
	/* 去掉文件名或文件路径中的文件后缀部分，保留路径
     * /abc.txt --> /abc
     * /abc/efg/123.txt -/abc/efg/123
     */
	void DropFileExt(const char* sFileName,char* sFileNameNoExt,int sFileNameNoExtLen);
	/* 获取文件所在目录路径
     * /abc/efg/123.txt --> /abc/efg/
	 * /abc/efg/ --> /abc/
	 * 参数sDirBuf用于存储目录字符串
	 * 参数dwBufLen为sDirName参数的缓冲区字符（非字节）长度，其中含需要保留的终止字符，
	       如果dwBufLen值为0则函数不会将目录路径拷贝到sDirBuf中；
		   如果dwBufLen值非0则函数会将目录路径拷贝到sDirBuf中并会在sDirBuf中写入终止符;
		   如果缓冲区不够则只拷贝dwBufLen-1个字符并会在sDirBuf中写入终止符。
		@函数返回拷贝目录路径所需的字符长度（含终止符）
	 */
	unsigned int ExtractFileDirectory(const char* sFilePath, char* sDirBuf, unsigned int dwBufLen);

	/* 获取文件所在目录路径最近目录的名称
     * /abc/efg/123.txt --> efg
     * /abc/efg/ --> abc
     * 参数sDirBuf用于存储目录字符串
     * 参数dwBufLen为sDirName参数的缓冲区字符（非字节）长度，其中含需要保留的终止字符，
           如果dwBufLen值为0则函数不会将目录路径拷贝到sDirBuf中；
           如果dwBufLen值非0则函数会将目录路径拷贝到sDirBuf中并会在sDirBuf中写入终止符;
           如果缓冲区不够则只拷贝dwBufLen-1个字符并会在sDirBuf中写入终止符。
        @函数返回拷贝目录路径所需的字符长度（含终止符）
     */
	unsigned int ExtractFileNearDirectoryName(const char* sFilePath, char* sDirBuf, unsigned int dwBufLen);
	/* 获取顶层目录名称
	 * (abc/efg/ --> abc)
	 * 参数ppChildDirPath用于存顶层目录之后的目录路径，参数可以为空
	 * 参数sDirName用于存储目录字符串
	 * 参数dwBufLen为sDirName参数的缓冲区字符（非字节）长度，其中含需要保留的终止字符，
	       如果dwBufLen值为0则函数不会将目录名拷贝到sDirName中；
		   如果dwBufLen值非0则函数会将目录名拷贝到sDirName中并会在sDirName中写入终止符;
		   如果缓冲区不够则只拷贝dwBufLen-1个字符并会在sDirName中写入终止符。
		@函数返回拷贝目录名所需的字符长度（含终止符）
	 */
	unsigned int ExtractTopDirectoryName(const char* sDirPath,const char* *ppChildDirPath, char* sDirName, unsigned int dwBufLen);
	/* 逐层创建目录
	 * 如果创建目录/a/b/c/d，最终目录的父目录不存在则逐级创建父目录并创建最终目录
	 * @如果目录完全创建成功则函数返回true，否则返回false。
	 * %如果在创建某个父目录成功后并创建子目录失败，则函数返回false且已经创建的父目录不会被删除。
	 */
	bool DeepCreateDirectory(const char* sDirPath);
	/*
	 * 获取目录里的所有的文件（不包含隐藏文件）
	 * /abc/efg/.  /abc/efg/..  /abc/efg/1.txt /abc/efg/2.txt /abc/efg/3.txt -> 返回 /abc/efg/1.txt /abc/efg/2.txt /abc/efg/3.txt
	 * 参数sDirname为目录
	 * 参数sFileExt为需要的文件拓展名，包含前缀 .
	 * 参数nFileExtLen为需要的文件拓展名的长度，包含前缀 .
	 * 参数filesVec为返回的所有的文件名，含路径
	 * 函数返回-1则失败，返回0则成功
	 * 忽略隐藏文件
	 */
	int GetDirCommonFilesByExt(const char* sDirname,const char* sFileExt,int nFileExtLen,std::vector<std::string> &filesVec,bool boRecursion = false);

	/*
     * 获取目录里的所有的子目录（不包含隐藏文件）
     * /abc/efg/.  /abc/efg/..  /abc/efg/1.txt /abc/efg/2 /abc/efg/3 -> 返回 /abc/efg/2 /abc/efg/3
     * 参数sDirname为目录
     * 参数filesVec为返回的所有的目录名，含路径
     * 函数返回-1则失败，返回0则成功
     * 忽略隐藏文件
     */
	int GetSubDirFromDir(const char* sDirname,std::vector<std::string> &dirsVec);
	/*
     * 移动文件
	 * 参数oldpath旧路径文件
	 * 参数newpath新路径文件
	 * 错误返回值小于0，成功返回0;返回-1时错误原因存于errno
	 */
	int MoveFile(const char *oldpath, const char *newpath,const char*newdir);
	/*
	 * 删除文件
	 * */
	int RemoveFile(const char *filepath);
}

#endif
