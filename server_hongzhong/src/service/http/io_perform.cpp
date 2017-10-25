

#include <curl/curl.h>
#include "io_header.h"
////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
#ifdef _DEBUG
#pragma comment(lib, "libcurl_d.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif
#endif
////////////////////////////////////////////////////////////////////////////////
static bool http_init()
{
    CURLcode result;
    result = curl_global_init(CURL_GLOBAL_DEFAULT);
    return (result == CURLE_OK);
}
////////////////////////////////////////////////////////////////////////////////
static void* http_open(const char *url, int timeout)
{
    if (!url){
        return 0;
    }
    CURL *handle = curl_easy_init();
    if (!handle){
        return 0;
    }
    CURLcode result;
    result = curl_easy_setopt(handle, CURLOPT_URL, url);
    result = curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    result = curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);
    result = curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 2);
    result = curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, false); // 跳过证书检查  
    result = curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, true);  // 从证书中检查SSL加密算法是否存在
#ifdef _DEBUG
    result = curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
#endif
    return handle;
}
////////////////////////////////////////////////////////////////////////////////
static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    std::string *content = (std::string*)userp;
    content->append((const char*)buffer, size * nmemb);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
static bool http_perform(void *handle, std::string &content)
{
    struct curl_slist *headers = 0;
    headers = curl_slist_append(headers, "Accept-Language: zh-cn,zh;q=0.5");
    headers = curl_slist_append(headers, "Accept-Charset: GB2312,utf-8;q=0.7,*;q=0.7");
    headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
    headers = curl_slist_append(headers, "Connection: Keep-Alive");
    headers = curl_slist_append(headers, "Pragma: no-cache");

    CURLcode result;
    result = curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1);
    result = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
    result = curl_easy_setopt(handle, CURLOPT_WRITEDATA, &content);
    result = curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
    result = curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.98 Safari/537.36");
    result = curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    result = curl_easy_perform(handle);
    curl_slist_free_all(headers); /* free the header list */

    int http_code = 0;
    result = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code == 0)
        http_code = 200;
    return (result == CURLE_OK && http_code == 200);
}
////////////////////////////////////////////////////////////////////////////////
static void http_close(void *handle)
{
    if (!handle)
        return;
    curl_easy_cleanup(handle);
}
////////////////////////////////////////////////////////////////////////////////
static void http_free()
{
    curl_global_cleanup();
}
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
bool init()
{
    return http_init();
}
////////////////////////////////////////////////////////////////////////////////
bool get(const std::string url, std::string &output)
{
    bool result = false;
    void *handle = http_open(url.c_str(), 2);
    if (handle){
        result = http_perform(handle, output);
        http_close(handle);
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////
void free()
{
    http_free();
}
////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////
