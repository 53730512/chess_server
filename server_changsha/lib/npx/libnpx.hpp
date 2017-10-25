

#ifndef __NPX_CPLUSPLUS_H_
#define __NPX_CPLUSPLUS_H_

////////////////////////////////////////////////////////////////////////////////
#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <string.h>
#include <stdarg.h>
#include <libnpx.h>
////////////////////////////////////////////////////////////////////////////////
#ifndef NPX_WINDOWS
#ifndef __int64
#define __int64 long long
#endif
inline int _vscprintf(const char *format, va_list pargs){
    int retval; 
    va_list argcopy;
    va_copy(argcopy, pargs); 
    retval = vsnprintf(0, 0, format, argcopy); 
    va_end(argcopy); 
    return retval;
}
#endif
////////////////////////////////////////////////////////////////////////////////
namespace io{
////////////////////////////////////////////////////////////////////////////////
//垮平台信号量类, 用于线程间同步
////////////////////////////////////////////////////////////////////////////////
class signal final{
public:
    virtual ~signal(){
        npx_signal_delete(m_signal);
    }
    inline signal()
        : m_signal(npx_signal_new()){
    }
    inline bool wait(unsigned int timeout){
        return npx_signal_wait(m_signal, timeout);
    }
    inline void set(){
        npx_signal_set(m_signal);
    }
    inline void reset(){
        npx_signal_reset(m_signal);
    }
private:
    npx_signal m_signal;
};
////////////////////////////////////////////////////////////////////////////////
//FIFO 队列, 用于线程间数据通讯或队列
////////////////////////////////////////////////////////////////////////////////
class queue final{
public:
    virtual ~queue(){
        npx_buffer_delete(m_buffer);
    }
    inline queue(int init = 1024, int limit = 0)
        : m_buffer(npx_buffer_new(init, limit)){
    }
    inline int used(){
        return npx_buffer_used(m_buffer);
    }
    inline void clear(){
        npx_buffer_clear(m_buffer);
    }
    inline int read(int size){
        return npx_buffer_submit(m_buffer, size);
    }
    inline int read(char *buffer, int size){
        return npx_buffer_read(m_buffer, buffer, size);
    }
    inline int write(const char *data, int size){
        return npx_buffer_write(m_buffer, data, size);
    }
    inline const char* data(){
        return npx_buffer_data(m_buffer);
    }
private:
    npx_buffer m_buffer;
};
////////////////////////////////////////////////////////////////////////////////
//字符串类, 为了便于使用, 扩展了标准库的 string 类
////////////////////////////////////////////////////////////////////////////////
class stringc final : public std::string{
public:
    template<typename _Ty>
    inline void write(const _Ty &v){
        write((const void*)&v, sizeof(v));
    }
    template<typename _Ty>
    inline _Ty read(bool &error){
        _Ty v;
        error = !read((void*)&v, sizeof(v));
        return (!error ? v : (_Ty)0);
    }
    template<typename _Ty>
    inline stringc& operator<<(_Ty v){
        return write<_Ty>(v), *this;
    }
    template<typename _Ty>
    inline stringc& operator>>(_Ty& v){
        return (v = read<_Ty>()), *this;
    }
public:
    inline bool read(void *buffer, size_t bytes){
        if (size() < bytes)
            return false;
        memcpy(buffer, (const void*)c_str(), bytes);
        return erase(0, bytes), true;
    }
    inline void write(const void *data, size_t bytes){
        append((const char*)data, bytes);
    }
public:
    inline void write_char(char v){
        write<char>(v);
    }
    inline void write_unsigned_char(unsigned char v){
        write<unsigned char>(v);
    }
    inline void write_short(int16_t v){
        write<int16_t>(v);
    }
    inline void write_unsigned_short(unsigned short v){
        write<unsigned short>(v);
    }
    inline void write_int(int v){
        write<int>(v);
    }
    inline void write_unsigned_int(unsigned int v){
        write<unsigned int>(v);
    }
    inline void write_float(float v){
        write<float>(v);
    }
    inline void write_double(double v){
        write<double>(v);
    }
    inline void write_int64(__int64 v){
        write<__int64>(v);
    }
    inline void write_unsigned_int64(unsigned __int64 v){
        write<unsigned __int64>(v);
    }
public:
    inline char read_char(bool &error){
        return read<char>(error);
    }
    inline unsigned char read_unsigned_char(bool &error){
        return read<unsigned char>(error);
    }
    inline short read_short(bool &error){
        return read<short>(error);
    }
    inline unsigned short read_unsigned_short(bool &error){
        return read<unsigned short>(error);
    }
    inline int read_int(bool &error){
        return read<int>(error);
    }
    inline unsigned int read_unsigned_int(bool &error){
        return read<unsigned int>(error);
    }
    inline float read_float(bool &error){
        return read<float>(error);
    }
    inline double read_double(bool &error){
        return read<double>(error);
    }
    inline __int64 read_int64(bool &error){
        return read<__int64>(error);
    }
    inline unsigned __int64 read_unsigned_int64(bool &error){
        return read<unsigned __int64>(error);
    }
public:
    inline stringc(/* default */)
        : std::string(){
    }
    inline stringc(const char *str)
        : std::string(str){
    }
    inline stringc(const char *data, size_t size)
        : std::string(data, size){
    }
    inline stringc(const stringc &v)
        : std::string(v){
    }
    inline stringc(size_t count, const char c)
        : std::string(count, c){
    }
    inline stringc(stringc &&right){
        swap(right);
    }
    inline stringc& operator=(const stringc &v){
        return clear(), assign(v), (*this);
    }
    inline stringc get_char(size_t &pos){
        stringc result;
        const char *begin = c_str() + pos;
        const char *next = npx_utf8_next(begin);
        if (next == 0){
            result = begin;
            pos += result.size();
            return std::move(result);
        }
        pos += (next - begin);
        result.append(begin, next - begin);
        return std::move(result);
    }
    inline bool is_utf8(){
        return npx_is_utf8(c_str());
    }
    inline size_t count(){
        return npx_utf8_count(c_str());
    }
    inline char* data(){
        return const_cast<char*>(c_str());
    }
    inline stringc to_utf8(const char *locale = 0){
        char data[8192];
        int bytes = (int)sizeof(data);
        int count = npx_to_utf8(c_str(), data, bytes, locale);
        stringc result;
        if (count <= bytes){
            result.append(data, count);
            return std::move(result);
        }
        char *buffer = new char[count];
        if (!buffer){
            return std::move(result);
        }
        count = npx_to_utf8(c_str(), buffer, count, locale);
        result.append(buffer, count);
        delete[] buffer;
        return std::move(result);
    }
    inline stringc to_ascii(const char *locale = 0){
        char data[8192];
        int bytes = (int)sizeof(data);
        int count = npx_to_ascii(c_str(), data, bytes, locale);

        stringc result;
        if (count <= bytes){
            result.append(data, count);
            return std::move(result);
        }
        char *buffer = new char[count];
        if (!buffer){
            return std::move(result);
        }
        count = npx_to_ascii(c_str(), buffer, count, locale);
        result.append(buffer, count);
        delete[] buffer;
        return std::move(result);
    }
    inline stringc to_hexfmt(){
        char data[8192];
        int len   = (int)size();
        int bytes = (int)sizeof(data);
        int count = npx_hex_format(c_str(), len, data, bytes);

        stringc result;
        if (count <= bytes){
            result.append(data, count - 1);
            return std::move(result);
        }
        char *buffer = new char[count];
        if (!buffer){
            return std::move(result);
        }
        count = npx_hex_format(c_str(), len, buffer, count);
        result.append(buffer, count - 1);
        delete[] buffer;
        return std::move(result);
    }
    inline stringc& toupper(){
        for (size_t i = 0; i < size(); i++)
            (*this)[i] = (char)::toupper((*this)[i]);
        return (*this);
    }
    inline stringc& tolower(){
        for (size_t i = 0; i < size(); i++)
            (*this)[i] = (char)::tolower((*this)[i]);
        return (*this);
    }
    inline stringc& format(const char *fmtstr, ...){
        char data[8192];
        char *buffer = data;
        va_list args;
        va_start(args, fmtstr);

        size_t bytes = _vscprintf(fmtstr, args) + 1;
        if (bytes > sizeof(data)){
            buffer = (char*)malloc(bytes);
            if (!buffer)
                return (*this);
        }
        vsprintf(buffer, fmtstr, args);
        assign(buffer);
        va_end(args);
        if (buffer != data)
            free(buffer);
        return (*this);
    }
    inline stringc& strrstr(const char *oldstr, const char *newstr){
        std::string::size_type pos = find(oldstr);
        while(pos != std::string::npos){
            replace(pos, strlen(oldstr), newstr);
            pos = find(oldstr, pos + strlen(newstr));
        }
        return (*this);
    }
};
////////////////////////////////////////////////////////////////////////////////
//io 事件回调类, 必须要从该类派生并重载需要的函数
////////////////////////////////////////////////////////////////////////////////
class callback{
    friend class service;
protected:
    virtual void on_update (int delta){}
    virtual void on_timer  (int id, int delta){}
    virtual void on_connect(npx_socket id){}
    virtual void on_error  (npx_socket id, int code){}
    virtual void on_accept (npx_socket id, npx_socket acceptor){}
    virtual void on_receive(npx_socket id, const char *data, int bytes){}
    virtual void on_message(npx_socket id, int value, const void *context){}
};
////////////////////////////////////////////////////////////////////////////////
//io 服务类, 处理 io 异步事件并将激活的事件或网络消息投递给 callback 类
////////////////////////////////////////////////////////////////////////////////
class service final{
public:
    typedef std::shared_ptr<service> value_type;
    inline static value_type create(callback &proc){
        return value_type(new service(proc));
    }
    virtual void stop(){
        if (m_stopped.exchange(true))
            return;
        m_thread->join();
        m_thread.reset();
    }
    virtual ~service(){
        stop();
        npx_ios_delete(m_npx);
    }
    inline bool is_stopped() const{
        return m_stopped.load();
    }
    inline int select(unsigned int timeout = 0){
        npx_event ev;
        int result = npx_ios_select(m_npx, &ev, timeout);
        if (result)
            enum_event(&ev);
        return result;
    }
    inline void run(){
        if (!m_stopped.exchange(false))
            return;
        m_thread.reset(new std::thread([this](){main_loop();}));
    }
    inline npx_socket socket(){
        return npx_tcp_socket(m_npx);
    }
    inline void post(int id, int value, const void *context){
        npx_ios_post(m_npx, id, value, context);
    }
    inline void update(int milliseconds){
        npx_set_update(m_npx, milliseconds);
    }
    inline void set_timer(int id, int milliseconds){
        npx_set_timer(m_npx, id, milliseconds);
    }
    inline void kill_timer(int id){
        npx_kill_timer(m_npx, id);
    }
private:
    inline service(callback &iproc)
        : m_npx(npx_ios_new(NPX_VERSION))
        , m_callback(iproc)
        , m_stopped(true){
    }
    inline void socket_error(npx_socket acceptor, int type){
        if (!is_stopped() && type == ev_accept){
            accept_next(acceptor);
        }
    }
    inline bool accept_next(npx_socket acceptor){
        if (NPX_INVALID_ID(acceptor)){
            return false;
        }
        npx_socket accept = socket();
        npx_result n = npx_accept(accept, acceptor);
        return NPX_SUCCESS(n);
    }
    inline void main_loop(){
        while (!m_stopped.load()){
            select(100); //Up to block 100ms
        }
    }
    inline void enum_event(const npx_event *ev){
        switch (ev->type){
        case ev_connect:
            m_callback.on_connect(ev->id);
            break;
        case ev_timer:
            m_callback.on_timer(ev->id, ev->value);
            break;
        case ev_update:
            m_callback.on_update(ev->value);
            break;
        case ev_accept:
            accept_next(ev->acceptor);
            m_callback.on_accept(ev->id, ev->acceptor);
            break;
        case ev_post:
            m_callback.on_message(ev->id, ev->value, ev->context);
            break;
        case ev_error:
            socket_error(ev->acceptor, ev->action);
            m_callback.on_error(ev->id, ev->code);
            npx_close(ev->id);
            break;
        case ev_receive:{
            const char *data = npx_receive(ev->id, ev->value);
            m_callback.on_receive(ev->id, data, ev->value);
            }
            break;
        default: break;
        }
    }
private:
    npx_handle m_npx; //npx handle.
    callback  &m_callback;
    std::atomic<bool> m_stopped;
    std::shared_ptr<std::thread> m_thread;
};
////////////////////////////////////////////////////////////////////////////////
} //end namespace io.
////////////////////////////////////////////////////////////////////////////////
namespace ip{
////////////////////////////////////////////////////////////////////////////////
namespace tcp{
////////////////////////////////////////////////////////////////////////////////
//网络函数集, 用于对 io socket 进行处理
////////////////////////////////////////////////////////////////////////////////
inline int socket(io::service::value_type ios){
    return ios->socket();
}
////////////////////////////////////////////////////////////////////////////////
inline void close(npx_socket id){
    npx_close(id);
}
////////////////////////////////////////////////////////////////////////////////
inline int send(npx_socket id, const char *data, int bytes){
    return npx_send(id, data, bytes);
}
////////////////////////////////////////////////////////////////////////////////
inline int send(npx_socket id, const io::stringc &data){
    return send(id, data.c_str(), (int)data.size());
}
////////////////////////////////////////////////////////////////////////////////
inline const char* get_peer_name(npx_socket id){
    return npx_get_peername(id);
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned short get_peer_port(npx_socket id){
    return npx_get_peerport(id);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_key(npx_socket id, const char *key, int bytes){
    npx_result ret = npx_set_key(id, key, bytes);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_key(npx_socket id, const io::stringc &key){
    return set_key(id, key.c_str(), (int)key.size());
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_suspend(npx_socket id, int limit, int delay){
    npx_result ret = npx_set_suspend(id, limit, delay);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_nagle(npx_socket id, bool enabled){
    npx_result ret = npx_set_nagle(id, enabled);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_compress(npx_socket id, bool enabled){
    npx_result ret = npx_set_compress(id, enabled);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_archive(npx_socket id, int limit, bool enabled){
    npx_result ret = npx_set_archive(id, enabled, limit);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_queue(npx_socket id, int limit){
    npx_result ret = npx_set_queue(id, limit);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool set_buffer(npx_socket id, buf_type type, int size){
    npx_result ret = npx_set_buffer(id, type, size);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
inline bool listen(npx_socket id, unsigned short port, const char *host = 0, int backlog = 5){
    bool result = NPX_SUCCESS(npx_listen(id, port, host, backlog));
    for (int i = 0; result && i < backlog; i++)
        npx_accept(npx_tcp_socket(npx_get_handle(id)), id);
    return result;
}
////////////////////////////////////////////////////////////////////////////////
inline bool connect(npx_socket id, const char *host, unsigned short port){
    npx_result ret = npx_connect(id, host, port);
    return NPX_SUCCESS(ret);
}
////////////////////////////////////////////////////////////////////////////////
} //end namespace tcp.
////////////////////////////////////////////////////////////////////////////////
} //end namespace ip.
////////////////////////////////////////////////////////////////////////////////
namespace hash{
////////////////////////////////////////////////////////////////////////////////
//hash 函数集
////////////////////////////////////////////////////////////////////////////////
inline unsigned int md5(const char *data, int bytes, unsigned char out[16]){
    return npx_hash_md5(data, bytes, out);
}
////////////////////////////////////////////////////////////////////////////////
inline io::stringc md5(const char *data, int bytes){
    unsigned char buffer[16];
    int n = md5(data, bytes, buffer);

    io::stringc result;
    for (int i = 0; i < n; i++){
        char hex[3];
        sprintf(hex, "%02x", buffer[i]);
        result += hex;
    }
    return std::move(result);
}
////////////////////////////////////////////////////////////////////////////////
inline io::stringc md5(const io::stringc &data){
    return md5(data.c_str(), (int)data.size());
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned int hmac_md5(const char *data, int bytes, const char *key, int keylen, unsigned char out[16]){
    return npx_hash_hmac_md5(data, bytes, key, keylen, out);
}
////////////////////////////////////////////////////////////////////////////////
inline io::stringc hmac_md5(const char *data, int bytes, const char *key, int keylen){
    unsigned char buffer[16];
    int n = hmac_md5(data, bytes, key, keylen, buffer);

    io::stringc result;
    for (int i = 0; i < n; i++){
        char hex[3];
        sprintf(hex, "%02x", buffer[i]);
        result += hex;
    }
    return std::move(result);
}
////////////////////////////////////////////////////////////////////////////////
inline io::stringc hmac_md5(const io::stringc &data, const io::stringc &key){
    return hmac_md5(data.c_str(), (int)data.size(), key.c_str(), (int)key.size());
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned short chksum16(const char *data, int bytes){
    return npx_hash_short(data, bytes);
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned short chksum16(const io::stringc &data){
    return chksum16(data.c_str(), (int)data.size());
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned int chksum32(const char *data, int bytes){
    return npx_hash_int(data, bytes);
}
////////////////////////////////////////////////////////////////////////////////
inline unsigned int chksum32(const io::stringc &data){
    return chksum32(data.c_str(), (int)data.size());
}
////////////////////////////////////////////////////////////////////////////////
} //end namespace hash
////////////////////////////////////////////////////////////////////////////////

#endif //__NPX_CPLUSPLUS_H_
