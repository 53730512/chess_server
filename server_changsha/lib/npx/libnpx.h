

#ifndef __NPX_LIBRARY_H_
#define __NPX_LIBRARY_H_

////////////////////////////////////////////////////////////////////////////////
#include <libnpxos.h>
////////////////////////////////////////////////////////////////////////////////
#ifdef  NPX_VERSION
#undef  NPX_VERSION
#endif
#define NPX_VERSION        1001
#define NPX_API
////////////////////////////////////////////////////////////////////////////////
#define NPX_INFINITE       0xffffffff
#define NPX_OK             0x00000000
#define NPX_SUCCESS(v)     ((v) == NPX_OK)
#define NPX_INVALID_ID(id) (id <= 0 || id > 0xffff)
////////////////////////////////////////////////////////////////////////////////
#pragma pack(push)
#pragma pack(1)
////////////////////////////////////////////////////////////////////////////////
typedef void* npx_handle;
typedef void* npx_buffer;
typedef void* npx_signal;
typedef int   npx_socket;
typedef int   npx_result;
typedef int   npx_version;
////////////////////////////////////////////////////////////////////////////////
typedef enum __npx_type{
    ev_unknow = 0,
    ev_timer,
    ev_update,
    ev_connect,
    ev_accept,
    ev_post,
    ev_error,
    ev_receive
} npx_type;
////////////////////////////////////////////////////////////////////////////////
typedef enum __buf_type{
    buffer_send    = 1,
    buffer_receive = 2
} buf_type;
////////////////////////////////////////////////////////////////////////////////
typedef struct __npx_event{
    npx_type    type;      //事件类型
    npx_socket  id;        //socket 唯一号
    npx_socket  acceptor;  //监听 socket 唯一号
    int         value;     //事件值
    int         code;      //错误代码
    int         action;    //发生错误的行为
    const void* context;   //事件上下文
} npx_event;
////////////////////////////////////////////////////////////////////////////////
#pragma pack(pop)
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif
////////////////////////////////////////////////////////////////////////////////
// 以下函数为 socket 函数，用于 TCP 异步网络通讯及消息传输
////////////////////////////////////////////////////////////////////////////////
NPX_API npx_handle     npx_ios_new      (npx_version version = NPX_VERSION);
NPX_API npx_result     npx_set_compress (npx_socket id, bool enabled);
NPX_API npx_result     npx_set_nagle    (npx_socket id, bool enabled);
NPX_API npx_result     npx_set_archive  (npx_socket id, bool enabled, int limit);
NPX_API npx_result     npx_set_queue    (npx_socket id, int limit);
NPX_API npx_result     npx_set_suspend  (npx_socket id, int limit, int delay);
NPX_API npx_result     npx_set_key      (npx_socket id, const char *key, int bytes);
NPX_API npx_result     npx_set_buffer   (npx_socket id, buf_type type, int size);
NPX_API const char*    npx_get_peername (npx_socket id);
NPX_API unsigned short npx_get_peerport (npx_socket id);
NPX_API npx_handle     npx_get_handle   (npx_socket id);
NPX_API npx_result     npx_listen       (npx_socket id, unsigned short port, const char *host = 0, int backlog = 5);
NPX_API npx_result     npx_accept       (npx_socket id, npx_socket listen);
NPX_API npx_result     npx_connect      (npx_socket id, const char *host, unsigned short port);
NPX_API int            npx_send         (npx_socket id, const char *data, int bytes);
NPX_API const char*    npx_receive      (npx_socket id, int bytes);
NPX_API void           npx_close        (npx_socket id);
NPX_API npx_socket     npx_tcp_socket   (npx_handle handle);
NPX_API int            npx_ios_select   (npx_handle handle, npx_event *pev, unsigned int timeout = 0);
NPX_API void           npx_ios_post     (npx_handle handle, int id, int value, const void *context);
NPX_API void           npx_set_update   (npx_handle handle, int milliseconds);
NPX_API void           npx_set_timer    (npx_handle handle, int id, int milliseconds);
NPX_API void           npx_kill_timer   (npx_handle handle, int id);
NPX_API void           npx_ios_delete   (npx_handle handle);
NPX_API npx_version    npx_get_version  ();
////////////////////////////////////////////////////////////////////////////////
// 以下函数为 circular_buffer 函数, 用于线程通讯，传递消息
////////////////////////////////////////////////////////////////////////////////
NPX_API npx_buffer     npx_buffer_new   (int init = 1024, int limit = 0);
NPX_API int            npx_buffer_submit(npx_buffer buffer, int size);
NPX_API void           npx_buffer_clear (npx_buffer buffer);
NPX_API int            npx_buffer_read  (npx_buffer buffer, char *out, int size);
NPX_API int            npx_buffer_write (npx_buffer buffer, const char *data, int size);
NPX_API const char*    npx_buffer_data  (npx_buffer buffer);
NPX_API int            npx_buffer_used  (npx_buffer buffer);
NPX_API void           npx_buffer_delete(npx_buffer buffer);
////////////////////////////////////////////////////////////////////////////////
// 以下为信号量处理函数，用于线程同步
////////////////////////////////////////////////////////////////////////////////
NPX_API npx_signal     npx_signal_new   ();
NPX_API void           npx_signal_set   (npx_signal signal);
NPX_API bool           npx_signal_wait  (npx_signal signal, unsigned int timeout);
NPX_API void           npx_signal_reset (npx_signal signal);
NPX_API void           npx_signal_delete(npx_signal signal);
////////////////////////////////////////////////////////////////////////////////
// 以下为一些常用的函数封装，用于对字符串的格式化，转换及数据的 hash 运算
////////////////////////////////////////////////////////////////////////////////
NPX_API void           npx_wait         ();
NPX_API void           npx_wait_cancel  ();
NPX_API unsigned int   npx_tick_count   ();
NPX_API const char*    npx_get_path     ();
NPX_API npx_result     npx_set_path     (const char *path);
NPX_API npx_result     npx_make_dir     (const char *path);
NPX_API int            npx_to_utf8      (const char *data, char *buffer, int size, const char *locale = 0);
NPX_API int            npx_to_ascii     (const char *data, char *buffer, int size, const char *locale = 0);
NPX_API int            npx_utf8_count   (const char *data);
NPX_API bool           npx_is_utf8      (const char *data);
NPX_API const char*    npx_utf8_next    (const char *data);
NPX_API int            npx_hex_format   (const char *data, int bytes, char *buffer, int size);
NPX_API unsigned short npx_hash_short   (const char *data, int bytes);
NPX_API unsigned int   npx_hash_int     (const char *data, int bytes);
NPX_API unsigned int   npx_hash_md5     (const char *data, int bytes, unsigned char out[16]);
NPX_API unsigned int   npx_hash_hmac_md5(const char *data, int bytes, const char *key, int keylen, unsigned char out[16]);
////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
////////////////////////////////////////////////////////////////////////////////
#endif //__NPX_LIBRARY_H_
