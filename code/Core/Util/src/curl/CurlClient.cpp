#include "CurlClient.hpp"

namespace util
{
//#define TEST_CUTL

CurlClient::CurlClient() :m_bDebug(false)
{
	m_pHttpCallback = NULL;
	m_updateProgress = NULL;
}
CurlClient::~CurlClient()
{
	if (m_pHttpCallback)
	{
		delete m_pHttpCallback;
		m_pHttpCallback = NULL;
	}
}
int CurlClient::OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *)
{
    if(itype == CURLINFO_TEXT)
    {
        printf("[CURLINFO_TEXT]%s", pData);
    }
    else if(itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN]%s", pData);
    }
    else if(itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT]%s", pData);
    }
    else if(itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN]%s", pData);
    }
    else if(itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT]%s", pData);
    }
    return 0;
}

size_t CurlClient::OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
    if( NULL == str || NULL == buffer )
    {
        return -1;
    }
    str->append((const char*)buffer, size * nmemb);
    return size * nmemb;
}

CURLcode CurlClient::PostHttps(const std::string & strUrl, const std::string & strFields,
		std::string & strResponse,const std::string& strUserpwd,eContentType eType,const std::string& strCaPath,int iPort)
{
    CURL* curl = GetHandle();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
	}
    if (iPort > 0)
	{
		curl_easy_setopt(curl, CURLOPT_PORT, iPort);
	}
    if (strUserpwd.size() > 0)
    {
    	curl_easy_setopt(curl, CURLOPT_USERPWD, strUserpwd.c_str());
    }
    struct curl_slist *headers = NULL;
    if (eContentType_json == eType)
    {
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1);    //设置允许302  跳转
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strFields.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strFields.size());  // POST的数据长度, 可以不要此选项
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if (strCaPath.size() > 0)
    {

        curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE,"PEM");//缺省情况就是PEM，另外支持DER
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);//是否检测服务器的证书是否由正规浏览器认证过的授权CA颁发的
        /* Set if we should verify the Common name from the peer certificate in ssl
           * handshake, set 1 to check existence, 2 to ensure that it matches the
           * provided hostname. */
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);//是否检测服务器的域名与证书上的是否一致
        curl_easy_setopt(curl, CURLOPT_CAINFO, strCaPath.c_str());//CURLOPT_CAINFO 证书存放路径
    }
    else
    {
    	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);//connect second
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);//read second
    CURLcode res = curl_easy_perform(curl);
    if (headers)
    {
    	curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return res;
}

CURLcode CurlClient::GetHttps(const std::string & strUrl, std::string & strResponse,
		const std::string& strUserpwd,eContentType eType, const std::string& strCaPath,int iPort)
{
    CURL* curl = GetHandle();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
        curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    }
    else
    {
    	curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    }

    if (iPort > 0)
    {
    	curl_easy_setopt(curl, CURLOPT_PORT, iPort);
    }
    if (strUserpwd.size() > 0)
	{
		curl_easy_setopt(curl, CURLOPT_USERPWD, strUserpwd.c_str());
	}
	struct curl_slist *headers = NULL;
	if (eContentType_json == eType)
	{
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	}
	else if (eContentType_gzip == eType)
	{
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
	}

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1);    //设置允许302  跳转
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if (strCaPath.size() > 0)
	{
		curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE,"PEM");//缺省情况就是PEM，另外支持DER
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);//是否检测服务器的证书是否由正规浏览器认证过的授权CA颁发的
		/* Set if we should verify the Common name from the peer certificate in ssl
		   * handshake, set 1 to check existence, 2 to ensure that it matches the
		   * provided hostname. */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);//是否检测服务器的域名与证书上的是否一致
		curl_easy_setopt(curl, CURLOPT_CAINFO, strCaPath.c_str());//CURLOPT_SSLCERT CURLOPT_CAINFO 证书存放路径
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
	}
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    CURLcode res = curl_easy_perform(curl);
    if (headers)
	{
		curl_slist_free_all(headers);
	}
    curl_easy_cleanup(curl);
    return res;
}

/*  libcurl write callback function */
size_t CurlClient::OnWriteFile(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);//ab+
    return written;
}

CURLcode CurlClient::DownloadFile(const std::string & strUrl, const std::string & strFile,int iPort)
{
    CURL *curl = GetHandle();
    if(NULL == curl)
	{
		return CURLE_FAILED_INIT;//CURLE_FAILED_INIT
	}
	FILE *fp = fopen(strFile.c_str(), "wb");
	if (NULL == fp)
	{
//		printf("failed to fopen(%s) \n",strFile.c_str());
		return CURLE_FAILED_INIT;
	}
	if (iPort > 0)
	{
		curl_easy_setopt(curl, CURLOPT_PORT, iPort);
	}
	curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteFile);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, OnProgressCallback);//设置进度回调函数
	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
	//开始执行请求
	CURLcode res = curl_easy_perform(curl);
	fclose(fp);
	/* Check for errors */
	curl_easy_cleanup(curl);//调用curl_easy_cleanup()释放内存
    return res;
}

void CurlClient::SetProgressFunction(ProgressFunction func)
{
    m_updateProgress = func;
}
void CurlClient::SetProgressCallback(IProgressCallback *pCallback)
{
	if (m_pHttpCallback)
	{
		delete m_pHttpCallback;
	}
    m_pHttpCallback = pCallback;
}


int CurlClient::OnProgressCallback(void *pParam, double dltotal, double dlnow, double ultotal, double ulnow)
{
	CurlClient* pThis = (CurlClient*)pParam;
	int nPos = (dlnow > 0 ) ? (int)((dlnow / dltotal) * 100):0;
	if (pThis->m_pHttpCallback)
	{
		pThis->m_pHttpCallback->OnProgressCallback(nPos);
	}
	if (pThis->m_updateProgress)
	{
		pThis->m_updateProgress(nPos);
	}
	return 0;
}

}

#ifdef TEST_CUTL

void BasicHttps()
{
	printf("%s() BasicHttps",__FUNCTION__);
	std::string website = "https://www.baidu.com/";//"https://www.alipay.com/";
	std::string cert = "/app/thunder/tools/testcurl/certs/ca-bundle.crt";//curl默认生成证书
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	util::CurlClient curl;
	curl.SetDebug();
	std::string strRes;
	CURLcode res = curl.GetHttps(website.c_str(),strRes);//https://www.111cn.net https://www.baidu.com
	if (res > 0)
	{
		std::cout << "\n CURLcode" << res << "\ncert:"<<cert<<"\n";
	}
	else
	{
		std::cout << "\n Gets ok \n";
	}
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
}
void BasicHttpsPost()
{
	printf("%s() BasicHttpsPost",__FUNCTION__);
	std::string website = "https://www.baidu.com/";//"https://www.alipay.com/"; https://www.baidu.com/
	std::string cert;//"/app/thunder/tools/testcurl/certs/ca-bundle.crt";//curl默认生成证书
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	util::CurlClient curl;
	curl.SetDebug();
	std::string strRes;
	CURLcode res = curl.PostHttps(website.c_str(),"",strRes);//https://www.111cn.net https://www.baidu.com
	if (res > 0)
	{
		std::cout << "\n CURLcode" << res << "\ncert:"<<cert<<"\n";
	}
	else
	{
		std::cout << "\n Gets ok \n";
	}
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
}

void JpushTest()
{
//	curl --insecure -X POST -v https://api.jpush.cn/v3/push -H "Content-Type: application/json" -u "7d431e42dfa6a6d693ac2d04:5e987ac6d2e04d95a9d8f0d1" -d '{"platform":"all","audience":"all","notification":{"alert":"Hi,JPush !","android":{"extras":{"android-key1":"android-value1"}},"ios":{"sound":"sound.caf","badge":"+1","extras":{"ios-key1":"ios-value1"}}}}'
	printf("%s() JpushTest",__FUNCTION__);
	std::string website = "https://api.jpush.cn/v3/push";
	std::string strUserpwd = "7d431e42dfa6a6d693ac2d04:5e987ac6d2e04d95a9d8f0d1";
	std::string strData = "{\"platform\":\"all\",\"audience\":\"all\",\"notification\":{\"alert\":\"Hi,JPush !\",\"android\":{\"extras\":{\"android-key1\":\"android-value1\"}},\"ios\":{\"sound\":\"sound.caf\",\"badge\":\"+1\",\"extras\":{\"ios-key1\":\"ios-value1\"}}}}";
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	util::CurlClient curl;
	curl.SetDebug();
	std::string strRes;
	CURLcode res = curl.PostHttps(website.c_str(),strData,strRes,strUserpwd,util::CurlClient::eContentType_json);
	if (res > 0)
	{
		std::cout << "\n CURLcode" << res << "\n";
	}
	else
	{
		std::cout << "\n Gets ok \n";
	}
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
}

void DownloadsFile()
{
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	util::CurlClient curl;
	util::ProgressCallback* pProgressCallback = new util::ProgressCallback();
	curl.SetProgressCallback(pProgressCallback);
	curl.DownloadFile("https://www.baidu.com","DownloadFile.txt");
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
	//DownloadsFile() Benchmark use time(56.567001) ms
}
/*
Value:0
Value:0
Value:9
Value:9
Value:19
Value:19
Value:29
Value:29
Value:39
Value:39
Value:49
Value:49
Value:59
Value:59
Value:89
Value:89
Value:100
Value:100
Value:100
Value:100
curl_easy_perform
curl_easy_cleanup
 * */

void Benchmark()
{
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	for(int i = 0;i < 100;++i)
	{
		util::CurlClient curl;
		std::string strRes;
		curl.GetHttps("http://www.baidu.com",strRes);
//		std::cout << "\n START BUFFER" << strRes << "\n END BUFFER";
	}
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
	//http www.baidu.com
	//Benchmark() Benchmark use time(51.887001) ms    1
	//Benchmark() Benchmark use time(492.786011) ms    10

	//http   share handle
	//Benchmark() Benchmark use time(253.309006) ms 10
	//Benchmark() Benchmark use time(2455.042969) ms 100

	//https https://www.baidu.com
	//Benchmark use time(92.491997) ms 1
	//Benchmark use time(796.396973) ms 10
}

void BenchmarkHttps()
{
	timeval cBeginClock;
	timeval cEndClock;
	gettimeofday(&cBeginClock,NULL);
	for(int i = 0;i < 1;++i)
	{
		util::CurlClient curl;
		std::string strRes;
		curl.GetHttps("https://www.alipay.com/",strRes);
		std::cout << "\n START BUFFER" << strRes << "\n END BUFFER";
	}
	gettimeofday(&cEndClock,NULL);
	float useTime=1000000*(cEndClock.tv_sec-cBeginClock.tv_sec)+
			cEndClock.tv_usec-cBeginClock.tv_usec;
	useTime/=1000;
	printf("%s() Benchmark use time(%lf) ms",__FUNCTION__,useTime);
	//BenchmarkHttps() Benchmark use time(225.962997) ms 1
	//BenchmarkHttps() Benchmark use time(2398.467041) ms 10
}

void ares()
{
	curl_version_info_data*info=curl_version_info(CURLVERSION_NOW);
	if (info->features&CURL_VERSION_ASYNCHDNS) {
	printf( "ares enabled\n");
	} else {
	printf( "ares NOT enabled\n");
	}
}

int main() {
	ares();
//	Benchmark();
//	BenchmarkHttps();
//	BasicHttps();
	JpushTest();
//	BasicHttpsPost();
//	DownloadsFile();
	return 0;
}

#endif

