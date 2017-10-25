

////////////////////////////////////////////////////////////////////////////////
#include "srp6.h"
#include <time.h>
#include <random>
////////////////////////////////////////////////////////////////////////////////
//以下为大数运算函数
////////////////////////////////////////////////////////////////////////////////
namespace bigint{
////////////////////////////////////////////////////////////////////////////////
const static unsigned int hmod[] = {
#if SRP6_KEY_BITS == 1024
    0xEEAF0AB9, 0xADB38DD6, 0x9C33F80A, 0xFA8FC5E8,
    0x60726187, 0x75FF3C0B, 0x9EA2314C, 0x9C256576,
    0xD674DF74, 0x96EA81D3, 0x383B4813, 0xD692C6E0,
    0xE0D5D8E2, 0x50B98BE4, 0x8E495C1D, 0x6089DAD1,
    0x5DC7D7B4, 0x6154D6B6, 0xCE8EF4AD, 0x69B15D49,
    0x82559B29, 0x7BCF1885, 0xC529F566, 0x660E57EC,
    0x68EDBC3C, 0x05726CC0, 0x2FD4CBF4, 0x976EAA9A,
    0xFD5138FE, 0x8376435B, 0x9FC61D2F, 0xC0EB06E3
#elif SRP6_KEY_BITS == 512
    0x8980BE27, 0x5F198BE6, 0x0BCB2616, 0xD18257F8,
    0x392A1863, 0x5BBE332E, 0x316A220C, 0xA2B56F5D,
    0x56801310, 0xECF89997, 0x3E3BF721, 0x05E3EC04,
    0xDA4DFA34, 0x98CF0838, 0x3E640F94, 0xC675B5C9
#elif SRP6_KEY_BITS == 256
    0x41B23FF9, 0x428C04CA, 0xD23BFD6B, 0xD3E7291B,
    0x968ECFB9, 0xC720D9D8, 0x411A3966, 0xA65F56BA
#endif
};
////////////////////////////////////////////////////////////////////////////////
//取模运算
static void mp_mod(unsigned int *u)
{
    __int64 cry;
    const int w = SRP6_KEY_BITS / 32;
    int i, j, n = w, m = w * 2;
    unsigned __int64 quot, rem, nxt, prod;

    while (m){
        if (!u[0])
            m--, u++;
        else
            break;
    }
    for (nxt = 0, i = 0; i <= m - n; i++ ){
        nxt <<= 32;
        nxt |= u[i];
        rem  = nxt % hmod[0];
        quot = nxt / hmod[0];
        while (quot == 0x100000000 || (rem < 0x100000000 && (quot * hmod[1]) > (rem << 32 | u[i + 1]))){
            quot -= 1, rem += hmod[0];
        }
        prod = cry = 0;
        if (quot){
            for (j = n; j--;){
                prod += quot * hmod[j];
                cry  += (unsigned __int64)u[i + j] - (unsigned int)prod;
                u[i + j] = (unsigned int)cry;
                prod >>= 32;
                cry  >>= 32;
            }
        }
        nxt = u[i];
        if (!i){
            if(cry - prod )
                quot++;
            else
                continue;
        }
        else if (u[i - 1] += (unsigned int)(cry - prod))
            quot++;
        else
            continue;
        cry = 0;
        for (j = n; j--;){
            cry += (unsigned __int64)u[i + j] + hmod[j];
            u[i + j] = (unsigned int)cry;
            cry >>= 32;
        }
        if( i )
            u[i - 1] += (unsigned int)cry;
        nxt = u[i];
    }
}
////////////////////////////////////////////////////////////////////////////////
//乘运算
static void mp_mult(unsigned int *dest, unsigned int *what, unsigned int *by)
{
    const int w = SRP6_KEY_BITS / 32;
    int m, n = w;
    unsigned __int64 fact, cry;
    memset (dest, 0, w * 2 * sizeof(int));

    while (n--){
        cry = 0;
        if ((fact = what[n])){
            for (m = w; m--;){
                cry += fact * by[m] + dest[n + m + 1];
                dest[n + m + 1] = (unsigned int)cry;
                cry >>= 32;
            }
        }
        dest[n] = (unsigned int)cry;
    }
}
////////////////////////////////////////////////////////////////////////////////
//幂运算
static void mp_exp(unsigned int *result, unsigned int *base, unsigned int *exponent)
{
    const int w = SRP6_KEY_BITS / 32;
    unsigned int prod[w * 2], term[w];
    int idx = w * 32;

    memset (result, 0, w * sizeof(int));
    memcpy (term, base, w * sizeof(int));
    result[w - 1] = 1;

    while (idx){
        if (*exponent)
            break;
        else
            exponent++, idx -= 32;
    }
    while (idx--){
        if (exponent[idx / 32] & (1 << (31 - (idx & 0x1f)))){
            mp_mult(prod, result, term);
            mp_mod(prod);
            memcpy(result, prod + w, w * sizeof(int));
        }
        mp_mult(prod, term, term);
        mp_mod(prod);
        memcpy(term, prod + w, w * sizeof(int));
    }
}
////////////////////////////////////////////////////////////////////////////////
//乘加运算
static void mp_mpyadd(unsigned int *dest, unsigned int *a, unsigned int b)
{
    const int w = SRP6_KEY_BITS / 32;
    unsigned int result[w * 2];
    int idx = w;
    __int64 cry = 0;

    while (idx--){
        cry += dest[idx];
        cry += a[idx] * (unsigned __int64)b;
        result[idx + w] = (unsigned int)cry;
        result[idx] = 0;
        cry >>= 32;
    }
    //  normalize result
    result[w - 1] = (unsigned int)cry;
    mp_mod(result);
    memset(dest, 0, w * sizeof(int));
    memcpy(dest, result + w, w * sizeof(int));
}
////////////////////////////////////////////////////////////////////////////////
//乘减运算
static void mp_mpysub(unsigned int *dest, unsigned int *a, unsigned int b)
{
    const int w = SRP6_KEY_BITS / 32;
    unsigned int result[w * 2];
    int idx = w;
    __int64 cry = 0;

    // multiply a by b
    while (idx--){
        cry += a[idx] * (unsigned __int64)b;
        result[idx + w] = (unsigned int)cry;
        result[idx] = 0;
        cry >>= 32;
    }
    //  normalize result
    result[w - 1] = (unsigned int)cry;
    mp_mod(result);
    //  subtract result from dest
    for (cry = 0, idx = w; idx--;){
        cry += dest[idx];
        cry -= result[idx + w];
        dest[idx] = (unsigned int)cry;
        cry >>= 32;
    }
    //  normalize result
    if( cry < 0 ){
        for (cry = 0, idx = w; idx--;){
            cry += dest[idx];
            cry += hmod[idx];
            dest[idx] = (unsigned int)cry;
            cry >>= 32;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
} //End namespace bigint
////////////////////////////////////////////////////////////////////////////////
//以下为 sha256 函数
////////////////////////////////////////////////////////////////////////////////
namespace sha{
////////////////////////////////////////////////////////////////////////////////
//private structure for SHA
typedef struct {
    unsigned char    buff[64];  // buffer, digest when full
    unsigned int     h[8];      // state variable of digest
    unsigned __int64 length;    // number of bytes in digest
    int              next;      // next buffer available
} table;
//  SHA 256 routines
//2^32 times the cube root of the first 64 primes 2..311
const static unsigned int k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
////////////////////////////////////////////////////////////////////////////////
//store 64 bit integer
static void putlonglong(unsigned __int64 what, unsigned char *where)
{
    *where++ = (unsigned char)(what >> 56);
    *where++ = (unsigned char)(what >> 48);
    *where++ = (unsigned char)(what >> 40);
    *where++ = (unsigned char)(what >> 32);
    *where++ = (unsigned char)(what >> 24);
    *where++ = (unsigned char)(what >> 16);
    *where++ = (unsigned char)(what >> 8);
    *where++ = (unsigned char)(what);
}
////////////////////////////////////////////////////////////////////////////////
//store 32 bit integer
static void putlong(unsigned int what, unsigned char *where)
{
    *where++ = (unsigned char)(what >> 24);
    *where++ = (unsigned char)(what >> 16);
    *where++ = (unsigned char)(what >> 8);
    *where++ = (unsigned char)(what);
}
////////////////////////////////////////////////////////////////////////////////
//retrieve 32 bit integer
static unsigned int getlong(unsigned char *where)
{
    unsigned int result;
    result = *where++ << 24;
    result |= *where++ << 16;
    result |= *where++ << 8;
    result |= *where++;
    return result;
}
////////////////////////////////////////////////////////////////////////////////
//right rotate bits
static unsigned int rotate(unsigned int what, int bits)
{
    return (what >> bits) | (what << (32 - bits));
}
////////////////////////////////////////////////////////////////////////////////
//right shift bits
static unsigned int shift(unsigned int what, int bits)
{
    return what >> bits;
}
////////////////////////////////////////////////////////////////////////////////
//digest SHA buffer contents
//to state variable
static void digest(table *sha)
{
    unsigned int s0, s1, maj, t0, t1, ch;
    unsigned int a,b,c,d,e,f,g,h;
    unsigned int w[64];
    int i;

    sha->next = 0;
    for (i = 0; i < 16; i++){
        w[i] = getlong(sha->buff + i * sizeof(unsigned int));
    }
    for (i = 16; i < 64; i++){
        s0 = rotate(w[i-15], 7) ^ rotate(w[i-15], 18) ^ shift(w[i-15], 3);
        s1 = rotate(w[i-2], 17) ^ rotate(w[i-2], 19) ^ shift (w[i-2], 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a = sha->h[0];
    b = sha->h[1];
    c = sha->h[2];
    d = sha->h[3];
    e = sha->h[4];
    f = sha->h[5];
    g = sha->h[6];
    h = sha->h[7];

    for( i = 0; i < 64; i++ ){
        s0 = rotate (a, 2) ^ rotate (a, 13) ^ rotate (a, 22);
        maj = (a & b) ^ (b & c) ^ (c & a);
        t0 = s0 + maj;
        s1 = rotate (e, 6) ^ rotate (e, 11) ^ rotate (e, 25);
        ch = (e & f) ^ (~e & g);
        t1 = h + s1 + ch + k[i] + w[i];

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t0 + t1;
    }
    sha->h[0] += a;
    sha->h[1] += b;
    sha->h[2] += c;
    sha->h[3] += d;
    sha->h[4] += e;
    sha->h[5] += f;
    sha->h[6] += g;
    sha->h[7] += h;
}
////////////////////////////////////////////////////////////////////////////////
//start new SHA run
static void begin(table *sha)
{
    sha->length = 0;
    sha->next = 0;
    // 2^32 times the square root of the first 8 primes 2..19
    sha->h[0] = 0x6a09e667;
    sha->h[1] = 0xbb67ae85;
    sha->h[2] = 0x3c6ef372;
    sha->h[3] = 0xa54ff53a;
    sha->h[4] = 0x510e527f;
    sha->h[5] = 0x9b05688c;
    sha->h[6] = 0x1f83d9ab;
    sha->h[7] = 0x5be0cd19;
}
////////////////////////////////////////////////////////////////////////////////
//add to current SHA buffer
//digest when full
static void next(table *sha, unsigned char *what, int len)
{
    while (len--){
        sha->length++;
        sha->buff[sha->next] = *what++;
        if (++sha->next == 512/8)
            digest(sha);
    }
}
////////////////////////////////////////////////////////////////////////////////
//finish SHA run, output 256 bit result
static void finish(table *sha, unsigned char *out)
{
    int idx;
    sha->buff[sha->next] = 0x80;
    if (++sha->next == 512 / 8)
        digest(sha);
    // pad with zeroes until almost full
    // leaving room for length, below
    while (sha->next != 448 / 8){
        sha->buff[sha->next] = 0;
        if (++sha->next == 512 / 8)
            digest(sha);
    }
    // n.b. length doesn't include padding from above
    putlonglong(sha->length * 8, sha->buff + 448 / 8);
    sha->next += sizeof(unsigned __int64);    // must be full now
    digest(sha);
    // output the result, big endian
    for (idx = 0; idx < 256 / 32; idx++){
        putlong(sha->h[idx], out + idx * sizeof(unsigned int));
    }
}
////////////////////////////////////////////////////////////////////////////////
} //End namespace bigint
////////////////////////////////////////////////////////////////////////////////
//以下为 srp6 算法实现
////////////////////////////////////////////////////////////////////////////////
inline unsigned int lrand()
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    return mt();
}
////////////////////////////////////////////////////////////////////////////////
void srp6::g_init()
{
    const unsigned int gv[] = {
        2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,
        61,67,71,73,79,83,89,97,101,103,107,109,113
    };
    memset(g, 0, sizeof(g));
    g[size - 1] = 113;
}
////////////////////////////////////////////////////////////////////////////////
const std::string& srp6::token(const std::string& pwd)
{
    sha::table sha[1];
    for (int i = 0; i < size; i++){
        salt[i] = lrand();
    }
    unsigned int x[size];
    memset(x, 0, sizeof(x));

    sha::begin(sha);
    sha::next(sha, (unsigned char*)salt, sizeof(salt));
    sha::next(sha, (unsigned char*)pwd.c_str(), (int)pwd.size());
    sha::finish(sha, (unsigned char*)(x + size) - hash);
    //x = H(s, P)

    bigint::mp_exp(v, g, x);
    //v = g^x

    m_data.clear();
    m_data.append((char*)salt, sizeof(salt));
    m_data.append((char*)v, sizeof(v));
    return m_data;
}
////////////////////////////////////////////////////////////////////////////////
const std::string& srp6::request(const std::string& token)
{
    if (token.size() != sizeof(salt) + sizeof(v)){
        m_data.clear();
        return m_data;
    }
    for (int i = 0; i < size; i++){
        b[i] = lrand();
    }
    const char *p = token.c_str();
    memcpy((char*)salt, p, sizeof(salt));
    memcpy(v, p + sizeof(salt), sizeof(v));

    bigint::mp_exp(B, g, b);
    bigint::mp_mpyadd(B, v, 3);
    //B = 3v + g^b

    m_data.clear();
    m_data.append((char*)salt, sizeof(salt));
    m_data.append((char*)B, sizeof(B));
    return m_data;
}
////////////////////////////////////////////////////////////////////////////////
const std::string& srp6::response(const std::string& request, const std::string& pwd)
{
    if (request.size() != sizeof(salt) + sizeof(B)){
        m_data.clear();
        return m_data;
    }
    for (int i = 0; i < size; i++){
        a[i] = lrand();
    }
    const char *p = request.c_str();
    memcpy((char*)salt, p, sizeof(salt));
    memcpy(B, p + sizeof(salt), sizeof(B));

    sha::table sha[1];
    unsigned int prod[size * 2];
    unsigned int x[size], u[size];
    unsigned int s1[size], s2[size], s3[size];

    memset(&x, 0, sizeof(x));
    sha::begin (sha);
    sha::next  (sha, (unsigned char*)salt, sizeof(salt));
    sha::next  (sha, (unsigned char*)pwd.c_str(), (int)pwd.size());
    sha::finish(sha, (unsigned char*)(x + size) - hash);
    //x = H(s, P)

    bigint::mp_exp(A, g, a);
    //A = g ^ a

    memset (u, 0, sizeof(u));
    sha::begin (sha);
    sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)B, size * sizeof(unsigned int));
    sha::finish( sha, (unsigned char *)(u + size) - hash);
    //u = H(A, B)

    bigint::mp_exp(v, g, x);
    bigint::mp_mpysub(B, v, 3); // re-use v as g^x
    bigint::mp_exp (s1, B, a);
    bigint::mp_exp (s2, B, u);
    bigint::mp_exp (s3, s2, x);
    bigint::mp_mult(prod, s1, s3);
    bigint::mp_mod(prod);       // normalize S version one
    //S1 = (B - 3g^x)^(a + ux)

    memcpy(B, p + sizeof(salt), sizeof(B));

    sha::begin (sha);
    sha::next  (sha, (unsigned char*)(prod + size), size * sizeof(unsigned int));
    sha::finish(sha, (unsigned char*)K1);

    sha::begin (sha);
    sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)B, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)K1, hash);
    sha::finish(sha, M1);

    m_data.clear();
    m_data.append((char*)A, sizeof(A));
    m_data.append((char*)M1, sizeof(M1));
    return m_data;
}
////////////////////////////////////////////////////////////////////////////////
bool srp6::verify(const std::string& response, std::string& verify_ok)
{
    if (response.size() != sizeof(A) + sizeof(M1)){
        return false;
    }
    sha::table sha[1];
    const char *p = response.c_str();
    memcpy(A, p, sizeof(A));
    memcpy(M1, p + sizeof(A), sizeof(M1));

    unsigned char M3[hash];
    unsigned int prod[size * 2];
    unsigned int t[size], u[size], S2[size];

    memset(u, 0, sizeof(u));
    sha::begin (sha);
    sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)B, size * sizeof(unsigned int));
    sha::finish(sha, (unsigned char*)(u + size) - hash);
    //u = H(A, B)

    bigint::mp_exp (t, v, u);
    bigint::mp_mult(prod, A, t);
    bigint::mp_mod (prod);
    bigint::mp_exp (S2, prod + size, b);    // compute S version two
    //S2 = (A * v^u) ^ b

    sha::begin (sha);
    sha::next  (sha, (unsigned char*)S2, size * sizeof(unsigned int));
    sha::finish(sha, (unsigned char*)K2);

    sha::begin (sha);
    sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)B, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)K2, hash);
    sha::finish(sha, M2);
    //计算M2 = (A, B, S2)

    int result = memcmp(M1, M2, sizeof(M1));
    if (result == 0){ //ok
        sha::begin (sha);
        sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
        sha::next  (sha, (unsigned char*)M2, hash);
        sha::next  (sha, (unsigned char*)K2, hash);
        sha::finish(sha, M3);

        verify_ok.clear();
        verify_ok.append((char*)M3, sizeof(M3));

        m_data.clear();
        m_data.append((char*)K2, sizeof(K2));
        return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////
bool srp6::verify(const std::string& verify_ok)
{
    if (verify_ok.size() != sizeof(K2)){
        return false;
    }
    sha::table sha[1];
    unsigned char M4[hash];
    memset(&M4, 0, sizeof(M4));

    sha::begin (sha);
    sha::next  (sha, (unsigned char*)A, size * sizeof(unsigned int));
    sha::next  (sha, (unsigned char*)M1, hash);
    sha::next  (sha, (unsigned char*)K1, hash);
    sha::finish(sha, M4);

    std::string data((char*)M4, sizeof(M4));
    if (data == verify_ok){
        m_data.clear();
        m_data.append((char*)K1, sizeof(K1));
        return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////
