#include "FileUtil.h"

namespace llib
{

const char* GetFilesErrorStr(int nErrno)
{
    if(eFileResult_system_error == nErrno)
    {
        return ::strerror(errno);
    }
    else if(eFileResult_param_error == nErrno)
    {
        return "param error";
    }
    else if (eFileResult_file_already_Exist == nErrno)
    {
        return "archive or dir already Exist";
    }
    else if(eFileResult_dir_already_Exist == nErrno)
    {
        return "dir already Exist";
    }
    else if(eFileResult_archive_already_Exist == nErrno)
    {
        return "archive already Exist";
    }
    else if(eFileResult_create_file_error == nErrno)
    {
        return "create file error";
    }
    else if(eFileResult_create_dir_error == nErrno)
    {
        return "create dir error";
    }
    else if (eFileResult_archive_not_Exist == nErrno)
    {
        return "archive not exist";
    }
    else if (eFileResult_dir_not_Exist == nErrno)
    {
        return "dir not exist";
    }
    return "unknown error";
}

bool FileExists(const char* sFilePath)
{
    if(IsArchive(sFilePath))
    {
        return true;
    }
    else if(IsDirectory(sFilePath))
    {
        return true;
    }
    return false;
}

bool IsArchive(const char* sFilePath)
{
    //简单判断文件是否存在
    struct stat file;
    int res = stat(sFilePath, &file);
    if(-1 != res && S_ISREG(file.st_mode))
    {
        return true;
    }
    else return false;
}

bool IsDirectory(const char* sDirPath)
{
	struct stat file;
	int res = stat(sDirPath, &file);
	if(-1 != res && S_ISDIR(file.st_mode))
	{
		return true;
	}
	return false;
}

unsigned int ExtractFileName(const char* sFilePath, char* sNameBuf, unsigned int dwBufLen)
{
	const char* sNameStart,* sNameEnd = sFilePath + strlen(sFilePath) - 1;
	//跳过目录名称后连续的'/'
	while ( sNameEnd >= sFilePath && (*sNameEnd == '/') )
	{
		sNameEnd--;
	}
	sNameStart = sNameEnd;
	//定位目录名称起始的位置
	while ( sNameStart >= sFilePath )
	{
		if ( *sNameStart == '/'  )
			break;
		sNameStart--;
	}
	sNameStart++;
	//拷贝目录名称
	if ( sNameStart <= sNameEnd )
	{
		unsigned int dwNameLen = sNameEnd - sNameStart+1;
		if ( dwBufLen > 0 )
		{
			if ( dwBufLen > dwNameLen )
				dwBufLen = dwNameLen;
			else dwBufLen--;
			memcpy(sNameBuf, sNameStart, sizeof(*sNameStart) * dwBufLen);
			sNameBuf[dwBufLen] = 0;
		}
		return dwNameLen;
	}
	return 0;
}

unsigned int ExtractFileNameOnly(const char* sFileName, char* sNameBuf, unsigned int dwBufLen)
{
	//文件名为空则直接返回0
	if ( !*sFileName )
	{
		if ( dwBufLen > 0 )
			sNameBuf[0] = 0;
		return 0;
	}
	const char* sNameStart, *sNameEnd = sFileName + strlen(sFileName) - 1;
	//如果文件是目录
	if ( *sNameEnd == '/')
	{
		//跳过目录名称后连续的'/'
		while ( sNameEnd >= sFileName && (*sNameEnd == '/') )
		{
			sNameEnd--;
		}
	}
	else
	{
		const char* sPtr = sNameEnd;
		//找到文件后缀部分的起始位置
		while ( sPtr >= sFileName )
		{
			if (*sPtr == '.')
			{
				sNameEnd = sPtr - 1;
				break;
			}
			if (*sPtr == '/')//如果是目录则
				break;
			sPtr--;
		}
	}

	sNameStart = sNameEnd - 1;//sNameEnd为文件名末尾位置（不算后缀名和点）
	//定位目录名称后的起始的位置
	while ( sNameStart >= sFileName )
	{
		if ( *sNameStart == '/')
			break;
		sNameStart--;
	}
	sNameStart++;
	//拷贝目录名称
	if ( sNameStart <= sNameEnd )
	{
		unsigned int dwNameLen = sNameEnd - sNameStart + 1;
		if ( dwBufLen > 0 )
		{
			if ( dwBufLen > dwNameLen )
				dwBufLen = dwNameLen;
			else dwBufLen--;
			memcpy(sNameBuf, sNameStart, sizeof(*sNameStart) * dwBufLen);
			sNameBuf[dwBufLen] = 0;
		}
		return dwNameLen;
	}
	return 0;
}

const char* ExtractFileExt(const char* sFileName)
{
	const char* sResult = NULL;
	while (*sFileName)
	{
		if (*sFileName == '.')//返回.后的数据（包括.）
			sResult = sFileName;
		sFileName++;
	}
	return sResult;
}

void DropFileExt(const char* sFileName,char* sFileNameNoExt,int sFileNameNoExtLen)
{
    int n = snprintf(sFileNameNoExt,sFileNameNoExtLen-1,sFileName);
    if(n >= sFileNameNoExtLen)
    {
        n = sFileNameNoExtLen-1;
    }
    sFileNameNoExt[n] = 0;
    char* pFileNameNoExt = sFileNameNoExt + n -1;
    while (pFileNameNoExt >= sFileNameNoExt)
    {
        if (*pFileNameNoExt == '.')//截断.之后的数据
        {
            *pFileNameNoExt = 0;
            break;
        }
        pFileNameNoExt--;
    }
}

unsigned int ExtractFileDirectory(const char* sFilePath, char* sDirBuf, unsigned int dwBufLen)
{
	const char* sDirEnd = sFilePath + strlen(sFilePath) - 1;
	while (sDirEnd >= sFilePath && *sDirEnd != '/')//跳过末尾的'/'之后的字符
	{
		sDirEnd--;
	}
	if ( sDirEnd > sFilePath )
	{
	    unsigned int dwNameLen = sDirEnd - sFilePath;
		if ( dwBufLen > 0 )
		{
			if ( dwBufLen > dwNameLen )
				dwBufLen = dwNameLen;
			else dwBufLen--;

			memcpy(sDirBuf, sFilePath, sizeof(*sDirBuf) * dwBufLen);
			sDirBuf[dwBufLen] = 0;
		}
		return dwNameLen;
	}
	return 0;
}

unsigned int ExtractFileNearDirectoryName(const char* sFilePath, char* sDirBuf, unsigned int dwBufLen)
{
    const char* sDirEnd = sFilePath + strlen(sFilePath) - 1;
    while (sDirEnd >= sFilePath && *sDirEnd != '/')//跳过末尾的'/'之后的字符
    {
        sDirEnd--;
    }
    const char* sDirFirst = sDirEnd - 1;
    while(sDirFirst >= sFilePath && *sDirFirst != '/')
    {
        sDirFirst--;
    }
    ++sDirFirst;
    if ( sDirEnd > sFilePath && sDirFirst > sFilePath)
    {
        unsigned int dwNameLen = sDirEnd - sDirFirst;
        if ( dwBufLen > 0 )
        {
            if ( dwBufLen > dwNameLen )
                dwBufLen = dwNameLen;
            else dwBufLen--;

            memcpy(sDirBuf, sDirFirst, sizeof(*sDirBuf) * dwBufLen);
            sDirBuf[dwBufLen] = 0;
        }
        return dwNameLen;
    }
    return 0;
}

unsigned int ExtractTopDirectoryName(const char* sDirPath, const char* *ppChildDirPath,   char* sDirName, unsigned int dwBufLen)
{
	const char* sNameEnd;
	//跳过目录名称前连续的'/'
	while ( *sDirPath && (*sDirPath == '/') )
	{
		sDirPath++;
	}
	sNameEnd = sDirPath;

	//定位目录名称起始的位置
	while ( *sNameEnd )
	{
		if ( *sNameEnd == '/')
			break;
		sNameEnd++;
	}
	//拷贝目录名称
	if ( sNameEnd > sDirPath )
	{
		unsigned int dwNameLen = sNameEnd - sDirPath;
		if ( dwBufLen > 0 )
		{
			if ( dwBufLen > dwNameLen )
				dwBufLen = dwNameLen;
			else dwBufLen--;

			memcpy(sDirName, sDirPath, sizeof(*sDirPath) * dwBufLen);
			sDirName[dwBufLen] = 0;

			if (ppChildDirPath)
				*ppChildDirPath = sNameEnd;
		}
		return dwNameLen;
	}
	return 0;
}

bool DeepCreateDirectory(const char* sDirPath)
{
	char sPath[4096];
	char* sPathPtr = sPath;
	unsigned int dwNameLen, dwBufLen = sizeof(sPath) - 1;

	while (true)
	{
		dwNameLen = ExtractTopDirectoryName(sDirPath, &sDirPath, sPathPtr, dwBufLen);
		//如果目录名称长度超过目录缓冲区长度则放弃
		if ( dwNameLen >= dwBufLen )
			return false;
		//如果目录名称长度为0则表示所有目录均已创建完成
		if (dwNameLen == 0)
			return true;
		sPathPtr += dwNameLen;
		struct stat fileStat;
		if(-1 == stat(sPath, &fileStat))//检查不存在才创建目录
		{
			if(ENOENT == errno)//The directory does not exist
			{
				if(-1 == mkdir(sPath, S_IRWXG|S_IRWXU))
				{
					return false;
				}
			}
		}
		sPathPtr[0] = '/';
		sPathPtr++;
		if ( dwBufLen > dwNameLen )
			dwBufLen -= dwNameLen + 1;
		else dwBufLen = 0;
	}
	return false;
}


int GetDirCommonFilesByExt(const char* sDirname,const char* sFileExt,int nFileExtLen,std::vector<std::string> &filesVec,bool boRecursion)
{
    DIR* dp;
    struct dirent* dirp;
    /* open dirent directory */
    if((dp = ::opendir(sDirname)) == NULL)
    {
        return -1;
    }
    char fullname[255];
    //read all files in this dir
    while((dirp = ::readdir(dp)) != NULL)
    {
        if(dirp->d_name)
        {
            ::memset(fullname, 0, sizeof(fullname));
            /* ignore hidden files */
            if(dirp->d_name[0] == '.')
                continue;
            ::strncpy(fullname, sDirname, sizeof(fullname));//目录名
            ::strncat(fullname, "/", sizeof(fullname));
            strncat(fullname, dirp->d_name, sizeof(fullname));//文件名
//            printf("\t sDirname(%s) d_name(%s) file(%s)\n",sDirname,dirp->d_name,fullname);
            if(IsArchive(fullname))
            {
//                printf("\t Archive: %s\n",fullname);
                const char* fileExt = ExtractFileExt(dirp->d_name);//获取文件拓展名
                if (fileExt && 0 == ::strncmp(fileExt,sFileExt,nFileExtLen))//比较需要的拓展名
                {
                    filesVec.push_back(fullname);
                }
            }
            else if (IsDirectory(fullname))
            {
                if (boRecursion)
                {
//                    printf("\t Directory: %s\n",fullname);
                   if (-1 == GetDirCommonFilesByExt(fullname,sFileExt,nFileExtLen,filesVec))
                   {
                       ::closedir(dp);
                       return -1;
                   }
                }
            }
        }
    }
    ::closedir(dp);
    return 0;
}

int GetSubDirFromDir(const char* sDirname,std::vector<std::string> &dirsVec)
{
    DIR* dp;
    struct dirent* dirp;
    /* open dirent directory */
    if((dp = ::opendir(sDirname)) == NULL)
    {
        return -1;
    }
    char fullname[255];
    //read all files in this dir
    while((dirp = ::readdir(dp)) != NULL)
    {
        if(dirp->d_name)
        {
            ::memset(fullname, 0, sizeof(fullname));
            /* ignore hidden files */
            if(dirp->d_name[0] == '.')
                continue;
            ::strncpy(fullname, sDirname, sizeof(fullname));//目录名
            ::strncat(fullname, "/", sizeof(fullname));
            strncat(fullname, dirp->d_name, sizeof(fullname));//文件名
//            printf("\t sDirname(%s) d_name(%s) file(%s)\n",sDirname,dirp->d_name,fullname);
            if(IsArchive(fullname))
            {
                continue;
            }
            else if (IsDirectory(fullname))
            {
                dirsVec.push_back(fullname);
            }
        }
    }
    ::closedir(dp);
    return 0;
}

int MoveFile(const char *oldpath, const char *newpath,const char*newdir)
{
    if(!oldpath||!newpath)
    {
        return eFileResult_param_error;
    }
    if(FileExists(newpath))//判断新文件是否存在
    {
        return eFileResult_file_already_Exist;
    }
    if(!IsArchive(oldpath))//判断旧文件是否存在
    {
        return eFileResult_archive_already_Exist;
    }
    if(newdir)//需要新的目录
    {
        if(!IsDirectory(newdir))//判断该目录是否已存在
        {
            if(!DeepCreateDirectory(newdir))//没有该目录则创建(会判断所有的父目录是否存在,不存在则创建)
            {
                return eFileResult_create_dir_error;
            }
        }
    }
    return ::rename(oldpath,newpath);
}

int RemoveFile(const char *filepath)
{
    if(!filepath)
    {
        return eFileResult_param_error;
    }
    if(!IsArchive(filepath))//判断文件是否存在
    {
        return eFileResult_archive_not_Exist;
    }
    return ::unlink(filepath);
}


};
