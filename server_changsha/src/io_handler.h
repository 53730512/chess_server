

#ifndef __IO_HANDLER_H_
#define __IO_HANDLER_H_
////////////////////////////////////////////////////////////////////////////////
#include "room_any.h"
#include "io_config.h"
////////////////////////////////////////////////////////////////////////////////
enum io_type{
    type_player   = 1,  //用户请求
    type_internal = 2   //内部请求
};
////////////////////////////////////////////////////////////////////////////////
typedef struct __online_t{
    http::channal   id; //服务器网络ID
    time_t          last_active;
} online_t;
////////////////////////////////////////////////////////////////////////////////
class io_handler : public http::io_callback{
    friend class io_player;
    friend class room_basic;
    friend class http::io_service;
    io_type get_io_type(http_context context);
public:
    static io_handler* get_instance();
    void close(http::channal id);
    void close(http_context context);
    void send(http_context context, const std::string &data);
    void send(http::channal id, int protocol, const Json::Value &data, int errcode = 0);
    void send(http_context context, int protocol, const Json::Value &data, int errcode = 0);
    bool send_to_cache(int protocol, const Json::Value &data, int errcode = 0);
protected:
    void on_ws_connect(http_context context);
    void on_ws_accept (http_context context);
    void on_ws_request(http_context context);
    void on_http_get  (http_context context);
    void on_http_post (http_context context);
    void on_parse_data(http_context context);
private:
    void on_save_player(const Json::Value &data);
    void on_load_player(const Json::Value &data);
    void on_reset_cache(const Json::Value &data);
    void on_reset_cache_ret(const Json::Value &data, int error);
    void on_offline_user(const Json::Value &data);
    void on_load_player_ret(const Json::Value &data, int error);
    void on_load_package(const Json::Value &data);
    void on_load_package_ret(const Json::Value &data, int error);
    void on_load_payment(const Json::Value &data);
    void on_load_payment_ret(const Json::Value &data, int error);
    void on_active_user(const Json::Value &data);
    void on_active_user_ret(const Json::Value &data, int error);
    void on_diamond_change(const Json::Value &data);
    void on_offline_diamond(const Json::Value &data);
private:
    void on_diamond_give(const http::uri &uri);
    void on_callback_payment(const http::uri &uri);
    void on_uuid_by_hashid(const http::uri &uri);
    void on_hashid_by_uuid(const http::uri &uri);
    void on_callback_payment(const Json::Value data, int error);
private:
    void on_create    (const void *context);
    void on_update    (int delta);
    void on_connect   (http_context context);
    void on_accept    (http_context context);
    void on_request   (http_context context);
    void on_timer     (http_context context);
    void on_error     (http_context context, int code);
    void on_timeout   (http_context context);
    void on_destroy   ();
private:
    int  get_server_id();
    int  register_user(userid uuid, const io::stringc &unionid, const io::stringc &nickname, const io::stringc &ip);
    void reset_user_cache();
private:
    io_type m_run_type;
    bool    m_mem_inited;
    int     m_title_idle;
    io::signal m_exit_signal;
    http_context m_session_cache;
    std::map<userid, online_t> m_online_users;
};
////////////////////////////////////////////////////////////////////////////////
inline io_handler* handler(){return io_handler::get_instance();}
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_HANDLER_H_
