

#ifndef __WEB_IO_SERVICE_H_
#define __WEB_IO_SERVICE_H_
////////////////////////////////////////////////////////////////////////////////
#include "io_service.hpp"
////////////////////////////////////////////////////////////////////////////////
typedef http::session::value_type http_context;
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
//获取service实例
////////////////////////////////////////////////////////////////////////////////
inline io_service* instance(){
    return io_service::get_instance();
}
////////////////////////////////////////////////////////////////////////////////
//设置转发配置更新地址
////////////////////////////////////////////////////////////////////////////////
inline void set_forward_url(const std::string &url){
    instance()->set_forward_url(url);
}
////////////////////////////////////////////////////////////////////////////////
//绑定本地端口(监听)
////////////////////////////////////////////////////////////////////////////////
inline bool listen(unsigned short port, const char *host = 0, int backlog = 32){
    return instance()->listen(port, host, backlog);
}
////////////////////////////////////////////////////////////////////////////////
//验证connect的url是否正确
////////////////////////////////////////////////////////////////////////////////
inline bool verify_url(http_context session){
    return instance()->verify_url(session);
}
////////////////////////////////////////////////////////////////////////////////
//恢复会话连接(重建会话)
////////////////////////////////////////////////////////////////////////////////
inline void reconnect(http_context session){
    return instance()->reconnect(session);
}
////////////////////////////////////////////////////////////////////////////////
//连接对端端口(连接)
////////////////////////////////////////////////////////////////////////////////
inline void connect(const char *host, unsigned short port, const void *context = 0){
    return instance()->connct(host, port, context);
}
////////////////////////////////////////////////////////////////////////////////
//运行本地服务
////////////////////////////////////////////////////////////////////////////////
inline void run(io_callback *callback = 0, int milliseconds = 0, const void *context = 0){
    instance()->run(callback, milliseconds, context);
}
////////////////////////////////////////////////////////////////////////////////
//判断服务是否正在运行
////////////////////////////////////////////////////////////////////////////////
inline bool is_runing(){
    return instance()->is_runing();
}
////////////////////////////////////////////////////////////////////////////////
//停止运行本地服务
////////////////////////////////////////////////////////////////////////////////
inline void stop(){
    instance()->stop();
}
////////////////////////////////////////////////////////////////////////////////
//断开所有网络连接
////////////////////////////////////////////////////////////////////////////////
inline void clear(){
    instance()->clear();
}
////////////////////////////////////////////////////////////////////////////////
//断开指定的网络连接
////////////////////////////////////////////////////////////////////////////////
inline void close(channal id){
    instance()->close(id);
}
////////////////////////////////////////////////////////////////////////////////
//断开指定的网络连接
////////////////////////////////////////////////////////////////////////////////
inline void close(http_context session){
    instance()->close(session);
}
////////////////////////////////////////////////////////////////////////////////
//获取指定会话实例
////////////////////////////////////////////////////////////////////////////////
inline http_context get_session(channal id){
    return instance()->get_session(id);
}
////////////////////////////////////////////////////////////////////////////////
//获取当前会话实例
////////////////////////////////////////////////////////////////////////////////
inline http_context get_cur_session(){
    return instance()->get_cur_session();
}
////////////////////////////////////////////////////////////////////////////////
//向服务发送一个自定义消息
////////////////////////////////////////////////////////////////////////////////
inline void post(int id, int value, const void *context){
    instance()->post(id, value, context);
}
////////////////////////////////////////////////////////////////////////////////
//给当前的会话发送数据
////////////////////////////////////////////////////////////////////////////////
inline void send(const std::string &data){
    instance()->send(data, get_cur_session());
}
////////////////////////////////////////////////////////////////////////////////
//给指定的会话发送数据
////////////////////////////////////////////////////////////////////////////////
inline void send(const std::string &data, channal id){
    instance()->send(data, get_session(id));
}
////////////////////////////////////////////////////////////////////////////////
//给指定的会话发送数据
////////////////////////////////////////////////////////////////////////////////
inline void send(const std::string &data, http_context session){
    instance()->send(data, session);
}
////////////////////////////////////////////////////////////////////////////////
//设置当前会话超时秒数
////////////////////////////////////////////////////////////////////////////////
inline void set_timeout(unsigned int timeout){
    instance()->set_timeout(timeout);
}
////////////////////////////////////////////////////////////////////////////////
//设置当前会话压缩选项
////////////////////////////////////////////////////////////////////////////////
inline void set_compress(bool compress){
    instance()->set_compress(compress);
}
////////////////////////////////////////////////////////////////////////////////
//获取当前会话类型
////////////////////////////////////////////////////////////////////////////////
inline http::session_type get_session_type(){
    return get_cur_session()->type;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前会话ID
////////////////////////////////////////////////////////////////////////////////
inline channal get_channal_id(){
    return get_cur_session()->id;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前会话上下文
////////////////////////////////////////////////////////////////////////////////
inline const void* get_context(){
    return get_cur_session()->context;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前http请求方法
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_query_type(){
    return get_cur_session()->request.method;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前http请求的url
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_query_url(){
    return get_cur_session()->request.url;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前http请求的路径
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_query_path(){
    return get_cur_session()->request.uri.url;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前http请求的数据(POST或WS)
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_query_data(){
    return get_cur_session()->recved;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前请求者真实ip地址
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_remote_addr(){
    return get_cur_session()->real_ipaddr;
}
////////////////////////////////////////////////////////////////////////////////
//获取当前http请求的GET参数值
////////////////////////////////////////////////////////////////////////////////
inline const std::string get_query_string(const char *name){
    const http::uri &uri = get_cur_session()->request.uri;
    return uri.get_param(name);
}
////////////////////////////////////////////////////////////////////////////////
} //End of namespace http
////////////////////////////////////////////////////////////////////////////////
#endif  //__WEB_IO_SERVICE_H_
