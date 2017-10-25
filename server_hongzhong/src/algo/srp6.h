

////////////////////////////////////////////////////////////////////////////////
#ifndef __SRP6_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <memory.h>
////////////////////////////////////////////////////////////////////////////////
#ifndef SRP6_LEVEL
#define SRP6_LEVEL    2    /*默认等级*/
#endif
////////////////////////////////////////////////////////////////////////////////
#if     SRP6_LEVEL == 1
#define SRP6_KEY_BITS 256  /*强度最低*/
#elif   SRP6_LEVEL == 2
#define SRP6_KEY_BITS 512  /*强度适中*/
#elif   SRP6_LEVEL == 3
#define SRP6_KEY_BITS 1024 /*强度最高*/
#else
#error  SRP6_LEVEL macro undefined (must be 1, 2 or 3).
#endif
////////////////////////////////////////////////////////////////////////////////
class srp6{
    std::string m_data;
public:
    inline srp6(){g_init();}
    inline const std::string& get_key(){return m_data;}
    const std::string& token(const std::string& pwd);
    const std::string& request(const std::string& token);
    const std::string& response(const std::string& request, const std::string& pwd);
    bool verify(const std::string& response, std::string& verify_ok);
    bool verify(const std::string& verify_ok);
private:
    void g_init();
    const static int hash = 32;
    const static int size = SRP6_KEY_BITS / 32;
    unsigned int  salt[size];
    unsigned char K1[hash], K2[hash], M1[hash], M2[hash];
    unsigned int  g[size], v[size], a[size], b[size], A[size], B[size];
};
////////////////////////////////////////////////////////////////////////////////
#endif //__SRP6_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
