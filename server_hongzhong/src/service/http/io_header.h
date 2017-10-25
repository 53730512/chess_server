

#ifndef __WS_HEADER_H_
#define __WS_HEADER_H_
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <tuple>
#include <string>
#include <memory>
////////////////////////////////////////////////////////////////////////////////
#include "../debug.h"
#include "../algo/base64.h"
#include "../algo/sha1.h"
////////////////////////////////////////////////////////////////////////////////
#define WSP_TYPE_MIX     0x00
#define WSP_TYPE_TEXT    0x01
#define WSP_TYPE_BINARY  0x02
#define WSP_TYPE_CLOSE   0x08
#define WSP_TYPE_PING    0x09
#define WSP_TYPE_PONG    0x0a
#define WSP_TYPE_RESERVED(n) (n > 0x0a || (n > 0x02 && n < 0x08))
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
inline std::string& value_null(){
    static std::string value;
    return value.clear(), value;
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_equal(const char *p1, const char *p2){
    while (*p1 && *p2){
        if (toupper(*p1) != toupper(*p2))
            return false;
        p1++, p2++;
    }
    return (*p1 == 0 && *p2 == 0);
}
////////////////////////////////////////////////////////////////////////////////
namespace url{
////////////////////////////////////////////////////////////////////////////////
inline unsigned char to_hex(const unsigned char &x){
    return x > 9 ? x - 10 + 'A' : x + '0';
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned char from_hex(const unsigned char &x){
    return isdigit(x) ? x - '0' : toupper(x) - 'A' + 10;
}
////////////////////////////////////////////////////////////////////////////////
inline std::string encode(const char *url, size_t size){
    std::string ret;
    for (register size_t ix = 0; ix < size; ix++ ){
        unsigned char buf[4];
        memset(buf, 0, 4);
        if(url[ix] == '.' || isalnum((unsigned char)url[ix])){
            buf[0] = url[ix];
        } else {
            buf[0] = '%';
            buf[1] = to_hex((unsigned char)url[ix] >> 4);
            buf[2] = to_hex((unsigned char)url[ix] % 16);
        }
        ret += (char *)buf;
    }
    return ret;
};
////////////////////////////////////////////////////////////////////////////////
inline std::string encode(const std::string& url){
    return encode(url.c_str(), url.size());
}
////////////////////////////////////////////////////////////////////////////////
inline std::string decode(const char *url, size_t size){
    std::string ret;
    for (register size_t ix = 0; ix < size; ix++ ){
        unsigned char ch = 0;
        if(url[ix]=='%'){
            ch = (from_hex(url[ix+1])<<4);
            ch |= from_hex(url[ix+2]);
            ix += 2;
        } else if (url[ix] == '+') {
            ch = ' ';
        } else {
            ch = url[ix];
        }
        ret += (char)ch;
    }
    return ret;
}
////////////////////////////////////////////////////////////////////////////////
inline std::string decode(const std::string& url){
    return decode(url.c_str(), url.size());
}
////////////////////////////////////////////////////////////////////////////////
} // The end of namespace url.
////////////////////////////////////////////////////////////////////////////////
typedef struct __header{
    std::string name;
    std::string value;
} header;
////////////////////////////////////////////////////////////////////////////////
typedef struct __uri{
    std::string url;
    std::vector<header> params;
    inline const std::string& get_param(const std::string &name) const{
        for (int i = 0; i < (int)params.size(); i++)
            if (is_equal(name.c_str(), params[i].name.c_str()))
                return params[i].value;
        return value_null();
    }
    inline void clear(){reset();}
    inline void reset(){params.clear();}
} uri;
////////////////////////////////////////////////////////////////////////////////
typedef struct __packet{
    inline void reset(){
        if (!queue){
            queue = std::make_shared<io::queue>();
        }
        h1 = h2 = 0;
        queue->clear();
        memset(mask, 0, sizeof(mask));
        memset(length, 0, sizeof(length));
    }
    inline int size() const{
        int bytes = (h2 & 0x7f);
        if (bytes < 126){
            return bytes;
        }
        if (bytes == 126)
            return (int)(*(unsigned short*)length);
        return (int)(*(unsigned __int64*)length);
    }
    inline void clear(){reset();}
    inline const char* data(){return queue->data();}
    inline bool is_eof()  const{return (h1 & 0x80) > 0;}
    inline int  rsv()     const{return (h1 & 0x70);}
    inline int  type()    const{return (h1 & 0x0f);}
    inline bool is_mask() const{return (h2 & 0x80) > 0;}
public:
    unsigned char h1;
    unsigned char h2;
    unsigned char mask[4];
    unsigned char length[8];
    std::shared_ptr<io::queue> queue;
} packet;
////////////////////////////////////////////////////////////////////////////////
typedef struct __request{
    inline void reset(){
        method.clear();
        url.clear();
        uri.clear();
        http_version_major = http_version_minor = 0;
        headers.clear();
    }
    inline void clear(){
        reset();
    }
    inline bool is_wsp() const{
         std::string data = get_header("Connection");
         if (data.empty()){
             return false;
         }
         data = get_header(data);
         if (data.empty()){
             return false;
         }
         return (is_equal(data.c_str(), "websocket"));
    }
    inline std::string to_string() const{
        io::stringc head;
        head.format("%s %s HTTP/%d.%d\r\n"
            , method.c_str()
            , url.c_str()
            , http_version_major
            , http_version_minor
            );
        for (register int i = 0; i < (int)headers.size(); i++){
            head += headers[i].name;
            head += ": ";
            head += headers[i].value;
            head += "\r\n";
        }
        return std::move(head + "\r\n");
    }
    inline std::string get_wsakey(){
        std::string key = get_header("Sec-WebSocket-Key");
        if (key.empty()){
            return "";
        }
        key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        SHA1Context sha;
        if (SHA1Reset(&sha) != shaSuccess){
            return "";
        }
        if (SHA1Input(&sha, (unsigned char*)key.c_str(), (int)key.size()) != shaSuccess){
            return "";
        }
        unsigned char hash[SHA1HashSize];
        if (SHA1Result(&sha, hash) != shaSuccess){
            return "";
        }
        char base64_v[64];
        base64::encode(hash, base64_v, (int)sizeof(hash));
        return std::string(base64_v);
    }
    inline const std::string& get_header(const std::string &name) const{
        for (int i = 0; i < (int)headers.size(); i++)
            if (is_equal(name.c_str(), headers[i].name.c_str()))
                return headers[i].value;
        return value_null();
    }
    inline void add_header(const std::string &name, const std::string &value){
        header new_header;
        new_header.name  = name;
        new_header.value = value;
        headers.push_back(new_header);
    }
    std::string method;
    std::string url;
    uri         uri;
    packet      wsp;
    int         http_version_major;
    int         http_version_minor;
    std::vector<header> headers;
} request;
////////////////////////////////////////////////////////////////////////////////
typedef struct __response{
    std::string status;
    std::string result;
    int http_version_major;
    int http_version_minor;
    std::vector<header> headers;
    inline void reset(){
        status.clear();
        result.clear();
        http_version_major = http_version_minor = 0;
        headers.clear();
    }
    inline void clear(){
        reset();
    }
    inline std::string to_string(){
        io::stringc head;
        head.format("HTTP/%d.%d %s %s\r\n"
            , http_version_major
            , http_version_minor
            , status.c_str()
            , result.c_str()
            );
        for (int i = 0; i < (int)headers.size(); i++){
            head += "\"";
            head += headers[i].name;
            head += ": ";
            head += headers[i].value;
            head += "\r\n";
        }
        return std::move(head + "\r\n");
    }
    inline std::string get_header(const std::string &name) const{
        for (register int i = 0; i < (int)headers.size(); i++)
            if (_stricmp(name.c_str(), headers[i].name.c_str()) == 0)
                return headers[i].value;
        return value_null();
    }
} response;
////////////////////////////////////////////////////////////////////////////////
} // namespace http
////////////////////////////////////////////////////////////////////////////////
#endif //__WS_HEADER_H_
