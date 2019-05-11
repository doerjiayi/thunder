//使用libcurl开发Http客户端。主要包括同步的HTTP GET、HTTP POST、HTTPS GET、HTTPS POST。
//其官方网站的地址是http://curl.haxx.se/，该网站主要提供了Curl和libcurl。
//Curl用于完成FTP, FTPS, HTTP, HTTPS, GOPHER, TELNET, DICT, FILE 以及 LDAP的命令的请求及接收回馈。
//提供给开发者，用于使用C++跨平台的开发各种网络协议的请求及响应。
//下载libcurl包，如果使用Linux平台，建议下载源文件编译；如果使用Windows平台，建议下载Win32 - MSVC，下载地址是：http://curl.haxx.se/download.html
#ifndef __CUSTOM_CURL_CLIENT_H__
#define __CUSTOM_CURL_CLIENT_H__
#include <string.h>
#include "curl/curl.h"

#ifdef __cplusplus
extern "C++" {
#endif

#include <string>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <functional>

namespace util
{

class IProgressCallback
{
public:
	virtual ~IProgressCallback(){}
    virtual bool OnProgressCallback(int nValue) = 0;
};

//可自定义回调
class ProgressCallback:public IProgressCallback
{
public:
	virtual ~ProgressCallback(){}
    virtual bool OnProgressCallback(int nValue){return true;}
};

class CurlClient  
{  
public:  
	enum eContentType
	{
		eContentType_none = 0,
		eContentType_json = 1,
		eContentType_gzip = 2,
	};
	typedef std::function<void(int)> ProgressFunction;//std::tr1::function std::function<void(int)>
	CurlClient();
    ~CurlClient();
    /** 
    * @brief HTTP  HTTPS POST请求,无证书版本
    * @param strUrl 输入参数,请求的Url地址,如:https://www.alipay.com
    * @param strFields 输入参数,使用如下格式para1=val1?2=val2&…
    * @param strResponse 输出参数,返回的内容
    * @param strCaPath 输入参数,为CA证书的路径.如果输入为空,则不验证服务器端证书的有效性.
    * @param iPort 输入参数,端口为0 则使用默认端口（https为443）
    * @return 返回是否Post成功
    */  
    CURLcode PostHttps(const std::string & strUrl, const std::string & strFields,std::string & strResponse,
    		const std::string& strUserpwd = "",eContentType eType = eContentType_none,
    		const std::string& strCaPath= "",int iPort = 0);
    /** 
    * @brief HTTP HTTPS GET请求,无证书版本
    * @param strUrl 输入参数,请求的Url地址,如:https://www.alipay.com
    * @param strResponse 输出参数,返回的内容
    * @param strCaPath 输入参数,为CA证书的路径.如果输入为空,则不验证服务器端证书的有效性.
    * @param iPort 输入参数,端口为0 则使用默认端口（https为443）
    * @return 返回是否Post成功
    */  
    CURLcode GetHttps(const std::string & strUrl, std::string & strResponse,
    		const std::string& strUserpwd = "",eContentType eType = eContentType_none,
			const std::string& strCaPath= "",int iPort = 0);
    ///@brief      文件下载
    /// @param[in]  url : 要下载文件的url地址
    /// @param[in]  outfilename : 下载文件指定的文件名
    /// @remark
    /// @return     返回0代表成功
    CURLcode DownloadFile(const std::string & strUrl, const std::string & strFile,int iPort = 0);
    /// @brief      进度报告处理
    /// @param[in]  func : 函数地址
    /// @remark
    /// @return     void
    void SetProgressFunction(ProgressFunction func);
    /// @brief      进度报告处理
    /// @param[in]  pCallback : 传入的对象
    /// @remark     使用的类继承于IProgressCallback
    /// @return     void
    void SetProgressCallback(IProgressCallback *pCallback);
    //设置调试模式
	void SetDebug(bool bDebug=true){m_bDebug = bDebug;}
protected:
    //回调处理
    static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
    static size_t OnWriteFile(void *ptr, size_t size, size_t nmemb, FILE *stream);
    static int OnProgressCallback(void *pParam, double dltotal, double dlnow, double ultotal, double ulnow);
    static int OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *);

    CURL* GetHandle()
    {
    	CURL* curl = curl_easy_init();
    	if (curl)
    	{
    		SetShareHandle(curl);
    	}
    	return curl;
    }
    void SetShareHandle(CURL* curl_handle)
    {
        static CURLSH* share_handle = NULL;
        if (!share_handle)
        {
            share_handle = curl_share_init();
            curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        }
        curl_easy_setopt(curl_handle, CURLOPT_SHARE, share_handle);
        curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
		// 在使用share interface的curl_share_init初始化share handle以后，使用curl_share_setopt设置共享对象，目前可以支持cookie和dns，
		//在每一个curl handle执行前，使用CURLOPT_SHARE选项把这个share handle设置给curl handle，这样多个curl handle就可以共用同一个DNS cache了
    }
    //Set the referrer page (needed by some CGIs)
    bool SetReferer(CURL* curl,const std::string& referer) {return curl_easy_setopt(curl,CURLOPT_REFERER,referer.c_str()) == CURLE_OK;}
    //Set the User-Agent string (examined by some CGIs)
	bool SetUserAgent(CURL* curl,const std::string& agent) {return curl_easy_setopt(curl,CURLOPT_USERAGENT,agent.c_str()) == CURLE_OK;}
	//POST static input fields.
	bool SetParms(CURL* curl,const std::string& strParam)//异步才使用本接口
	{
		//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "&logintype=uid&u=xieyan&psw=xxx86");    // 指定post内容
		return (curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strParam.data()) == CURLE_OK) &&
				(curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strParam.size()) == CURLE_OK);
	}
	//设置重定位URL
	bool SetRedirect(CURL* curl,bool re) {return curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,re) == CURLE_OK;}
	bool SetMultipartForm(CURL* curl,struct HttpPost* post) {return curl_easy_setopt(curl, CURLOPT_HTTPPOST, post) == CURLE_OK;}
	bool SSLVerifyPeer(CURL* curl,bool b) {return curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,b) == CURLE_OK;}
	bool SSLVerifyHost(CURL* curl,bool b) {return curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,b) == CURLE_OK;}
	///* Set the interface string to use as outgoing network interface */
	bool SetInterface(CURL* curl,const std::string &inter) {return curl_easy_setopt(curl,CURLOPT_INTERFACE,inter.c_str()) == CURLE_OK;}
	bool SetUserPassword(CURL* curl,const std::string &strPass) {return curl_easy_setopt(curl,CURLOPT_USERPWD,strPass.c_str()) == CURLE_OK;}
	//Set this to force the HTTP request to get back to GET
	bool ForceGet(CURL* curl) {return curl_easy_setopt(curl,CURLOPT_HTTPGET,1)  == CURLE_OK;}
	//HTTP POST method
	bool ForcePost(CURL* curl) {return curl_easy_setopt(curl,CURLOPT_POST,1)  == CURLE_OK;}
	//enables automatic decompression of HTTP downloads,https://curl.haxx.se/libcurl/c/CURLOPT_SSH_COMPRESSION.html
	bool SetSSHCompression(CURL* curl) {return curl_easy_setopt(curl,CURLOPT_SSH_COMPRESSION,1)  == CURLE_OK;}
private:
    bool m_bDebug;  
    ProgressFunction    m_updateProgress;
    IProgressCallback   *m_pHttpCallback;
};  

}

#ifdef  __cplusplus
}
#endif

#endif  


