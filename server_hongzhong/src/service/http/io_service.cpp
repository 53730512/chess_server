

#include <thread>
#include <zlib.h>
#include "io_perform.h"
#include "io_service.hpp"
////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
#pragma comment(lib, "libnpx.lib")
#ifndef _DEBUG
#pragma comment(lib, "zlibstat.lib")
#else
#pragma comment(lib, "zlibstatd.lib")
#endif
#endif
////////////////////////////////////////////////////////////////////////////////
typedef struct __post_argv{
    std::string url;
    std::string data;
    inline struct __post_argv(){}
    inline struct __post_argv(const std::string &a, const std::string &b){
        url  = a;
        data = b;
    }
} post_argv;
////////////////////////////////////////////////////////////////////////////////
const static int ZIP_BUFFER_SIZE    = (2 * 1024 * 1024);
const static int BASE64_BUFFER_SIZE = (4 * 1024 * 1024);
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
io_service::io_service()
    : m_zip_buffer(0)
    , m_base64_data(0)
    , m_local_port(80)
    , m_callback(0)
    , m_backlog(32)
    , m_time_now(time(0))
    , m_local_listen(0)
    , m_cur_channal_id(0)
{
    m_forward_cnt = 0;
    m_zip_buffer  = new char[ZIP_BUFFER_SIZE];
    m_base64_data = new char[BASE64_BUFFER_SIZE];
    m_async       = io::service::create(*this);
    m_io_service  = io::service::create(*this);
}
////////////////////////////////////////////////////////////////////////////////
io_service::~io_service()
{
    stop();
    delete[] m_zip_buffer;
    delete[] m_base64_data;
}
////////////////////////////////////////////////////////////////////////////////
io_service* io_service::get_instance()
{
    static std::auto_ptr<io_service> __instance;
    if (!__instance.get())
        __instance.reset(new io_service());
    return __instance.get();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::close(channal id)
{
    if (id <= 0)
        id = m_cur_channal_id;
    ip::tcp::close(id);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::close(session::value_type session)
{
    if (session){
        close(session->id);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::clear()
{
    auto iter = m_sessions.begin();
    for (; iter != m_sessions.end(); iter++){
        close(iter->second->id);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::stop()
{
    if (!m_io_service || m_io_service->is_stopped()){
        return;
    }
    if (m_local_listen){
        close(m_local_listen);
        m_local_listen = 0;
    }
    //转发模式不回调
    if (m_forward_url.empty()){
        if (m_callback){
            m_callback->on_destroy();
            m_callback = 0;
        }
    }
    clear();  //断开所有连接
    m_io_service->kill_timer(1010101);
    m_io_service->kill_timer(1010102);
    m_async->stop();
    m_io_service->stop();
    m_sessions.clear();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::run(io_callback *callback, int milliseconds, const void *context)
{
    if (m_io_service){
        get_forword();
        m_async->run();
        m_io_service->run();
        if (milliseconds > 0){
            m_io_service->update(milliseconds);
        }
        m_io_service->set_timer(1010101, 1000);
        m_io_service->set_timer(1010102, 60000);
        //转发模式不回调
        if (m_forward_url.empty()){
            if (callback){
                m_callback = callback;
                m_callback->on_create(context);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
bool io_service::is_runing() const
{
    return !m_io_service->is_stopped();
}
////////////////////////////////////////////////////////////////////////////////
bool io_service::listen(unsigned short port, const char *host, int backlog)
{
    if (!NPX_INVALID_ID(m_local_listen)){
        return false;
    }
    channal socket = ip::tcp::socket(m_io_service);
    if (!ip::tcp::listen(socket, port, host, backlog)){
        close(socket);
        return false;
    }
    m_local_port   = port;
    m_backlog      = backlog;
    m_local_listen = socket;
    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool io_service::verify_url(session::value_type session)
{
    http::uri &uri = session->request.uri;
    io::stringc argv1 = uri.get_param("context").c_str();
    io::stringc argv2 = uri.get_param("time").c_str();
    io::stringc argv3 = uri.get_param("hash").c_str();
    if (!argv1.empty()){
        io::stringc data;
        data.format("%s%s", argv1.c_str(), argv2.c_str());
        io::stringc hash = hash::hmac_md5(data, HMAC_MD5_KEY);
        if (argv3 != hash){
            return false;
        }
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::connct(const char *host, unsigned short port, const void *context)
{
    session::value_type session = session::create();
    if (!session){
        return;
    }
    channal socket = ip::tcp::socket(m_io_service);
    reset_session(session);
    session->notify       = true;
    session->id           = socket;
    session->context      = context;
    session->status       = conn_connecting;
    session->ipaddr       = host;
    session->port         = port;
    session->real_ipaddr  = session->ipaddr;
    m_sessions[socket]    = session;
    set_timeout(session, SESSION_BUSY_TIMEOUT);
    ip::tcp::connect(socket, host, port);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::reconnect(session::value_type session)
{
    if (session->status == conn_accepted){
        return;
    }
    if (m_local_listen){ //不是因为退出断开则重连
        const char *host    = session->ipaddr.c_str();
        unsigned short port = session->port;
        connct(host, port, session->context);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::post(int id, int value, const void *context)
{
    if (m_io_service){
        m_io_service->post(id, value, context);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_compress(bool compress)
{
    set_compress(get_cur_session(), compress);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_timeout(unsigned int timeout)
{
    set_timeout(get_cur_session(), timeout);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::add_forward(unsigned int port, const std::string &domain, const std::string &addr)
{
    m_forward[port][domain] = addr;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::del_forward(unsigned int port, const std::string &domain)
{
    if (m_forward.find(port) != m_forward.end()){
        if (domain == "*"){
            m_forward.erase(port);
        } else {
            m_forward[port].erase(domain);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::clear_forward()
{
    m_forward.clear();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_forword()
{
    if (!m_perform_data.isObject()){
        return;
    }
    clear_forward();
    Json::Value &data = m_perform_data;
    Json::Value::Members m1 = data.getMemberNames();
    for (auto i1 = m1.begin(); i1 != m1.end(); i1++){
        std::string str_port = *i1;
        unsigned short port = (unsigned short)atoi(str_port.c_str());
        if (port <= 0 || port > 65535){
            continue;
        }
        Json::Value::Members m2 = data[str_port].getMemberNames();
        for (auto i2 = m2.begin(); i2 != m2.end(); i2++){
            std::string domain = *i2;
            if (domain.empty()){
                continue;
            }
            std::string ipaddr = data[str_port][domain].asString();
            if (ipaddr.empty()){
                continue;
            }
            add_forward(port, domain, ipaddr);
        }
    }
    TRACE("* The forward data is updated from %s.\r\n"
        , m_forward_url.c_str()
        );
    m_perform_data.clear();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::get_forword()
{
    if (!m_forward_url.empty()){
        std::string content;
        bool result = get(m_forward_url, content);
        if (result && !content.empty()){
            Json::Value data;
            Json::Reader reader;
            if (reader.parse(content, data)){
                m_perform_data = data;
                on_message(1, 0, 0);
            } else {
                PRINT("* Forward data parse failed: %s\r\n"
                    , m_forward_url.c_str()
                    );
            }
        } else {
            PRINT("* Forward data update failed: %s\r\n"
                , m_forward_url.c_str()
                );
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::get_url_async(int type, const io::stringc &url)
{
    io::stringc *argv = new io::stringc(url);
    if (argv){
        m_async->post(2, type, argv);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_forward_url(const std::string &url)
{
    m_forward_url = url;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send(const std::string &data, channal id, int type)
{
    send(data, get_session(id), type);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send(const std::string &data, int type)
{
    send(data, get_cur_session(), type);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send(const std::string &data, session::value_type session, int type)
{
    if (!session){
        return;
    }
    switch (session->type){
    case type_http:
        send_response(session, data);
        break;
    case type_web_socket:
        send_ws_packet(session, data, type);
        break;
    }
    session->last_sent_time = m_time_now;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_update(int delta)
{
    m_time_now = time(0);
    if (m_callback){
        m_callback->on_update(delta);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_timer(int id, int delta)
{
    time_t time_now = time(0);
    if (id == 1010101){
        auto iter = m_sessions.begin();
        for (; iter != m_sessions.end(); iter++){
            //回调timer事件
            session::value_type session = iter->second;
            m_cur_channal_id = session->id;
            on_timer(session);
            //判断是否发送/接收超时
            if (time_now - session->last_recv_time > session->timeout){
                on_timeout(session);
                close(session->id);
            } else if (time_now - session->last_sent_time > 10) {
                send_ws_ping(session);
            }
        }
        if (time_now % 10 == 0 && m_forward.size()){
            struct tm *ptm = localtime(&time_now);
            PRINT("* [%02d:%02d:%02d] Count of forwarded: %u\r\n"
                , ptm->tm_hour
                , ptm->tm_min
                , ptm->tm_sec
                , m_forward_cnt
                );
        }
    } else if (id == 1010102){
        if (!m_forward_url.empty()){
            std::thread perform([this](){get_forword();});
            perform.detach();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_connect(channal id)
{
    session::value_type session = get_session(id);
    if (!session){
        return;
    }
    session->ipaddr      = npx_get_peername(id);
    session->port        = npx_get_peerport(id);
    session->real_ipaddr = session->ipaddr;
    session->status      = conn_connected;
    if (session->forward){
        session::value_type forward = get_session(session->forward);
        if (forward){
            http::request &request = forward->request;
            const std::string key("X-Forwarded-For");
            std::string x_forwarded_for = request.get_header(key);
            if (x_forwarded_for.empty()){
                request.add_header(key, forward->real_ipaddr);
            }
            on_forward(session, forward);
        }
    } else {
        io::stringc port;
        port.format("%d", npx_get_peerport(id));
        io::stringc host;
        host.format("Host: %s", session->ipaddr.c_str());
        if (port != "80"){
            host += (":" + port);
        }
        io::stringc data;
        unsigned int tm_now = (unsigned int)time(0);
        data.format("%u%u", (unsigned int)session->context, tm_now);
        io::stringc hash = hash::hmac_md5(data, HMAC_MD5_KEY);
        io::stringc argv;
        argv.format("context=%u&time=%u&hash=%s"
            , (unsigned int)session->context
            , tm_now
            , hash.c_str()
            );
        io::stringc request;
        request.format("GET /?%s HTTP/1.1\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n"
            , argv.c_str()
            , host.c_str()
            , "Connection: Upgrade"
            , "Pragma: no-cache"
            , "Cache-Control: no-cache"
            , "Upgrade: websocket"
            , "Sec-WebSocket-Version: 13"
            , "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.98 Safari/537.36"
            , "Accept-Language: zh-CN,zh;q=0.8"
            , "Sec-WebSocket-Key: OruLvnozOokho9iuESHGwg=="
            , "Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits"
            );
        ip::tcp::send(id, request);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_error(channal id, int code)
{
    session::value_type session = get_session(id);
    if (!session){
        return;
    }
    m_cur_channal_id = id;
    on_error(session, code);
    if (session->forward){ //转发连接断开
        if (m_forward_cnt > 0){
            m_forward_cnt--;
        }
        close(session->forward);
        session::value_type forward = get_session(session->forward);
        if (forward){
            forward->forward = 0;
        }
        session->forward = 0;
    }
    close(session->id);
    session->status = conn_unknown;
    m_sessions.erase(session->id);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_accept(channal id, channal acceptor)
{
    session::value_type conn = session::create();
    if (!conn){
        return;
    }
    m_cur_channal_id = id;
    reset_session(conn);
    conn->id             = id;
    conn->status         = conn_accepted;
    conn->ipaddr         = npx_get_peername(id);
    conn->port           = npx_get_peerport(id);
    conn->real_ipaddr    = conn->ipaddr;
    m_sessions[id]       = conn;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_receive(channal id, const char *data, int bytes)
{
    session::value_type session = get_session(id);
    if (!session){
        return;
    }
    m_cur_channal_id = id;
    switch (session->type){
    case type_unknown:
        parse_received(session, data, bytes);
        break;
    case type_http:
        recv_http_data(session, std::string(data, bytes));
        break;
    case type_web_socket:
        recv_ws_data(session, std::string(data, bytes));
        break;
    }
    session->last_recv_time = m_time_now;
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_message(channal id, int value, const void *context)
{
    if (id == 1){
        set_forword();
    } else {
        if (id == 2){
            std::string *argv = (std::string*)context;
            if (value == 1){ //发送通知消息
                post_argv data;
                data.url = *argv;
                bool result = http::get(data.url, data.data);
                post(3, result ? 1 : 0, new post_argv(data));
            }
            delete argv;
        } else if (id == 3){ //处理应答数据
            post_argv *argv = (post_argv*)context;
            on_response(value ? true : false, argv->url, argv->data);
            delete argv;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_response(bool result, const std::string &url, const std::string &data)
{
    console::log("* Notify URL(%s): %s", result ? "Successed" : "Failed", url.c_str());
}
////////////////////////////////////////////////////////////////////////////////
session::value_type io_service::get_cur_session() const
{
    return get_session(m_cur_channal_id);
}
////////////////////////////////////////////////////////////////////////////////
const void* io_service::get_context() const
{
    return get_cur_session()->context;
}
////////////////////////////////////////////////////////////////////////////////
session::value_type io_service::get_session(channal id) const
{
    auto finded = m_sessions.find(id);
    return (finded == m_sessions.end() ? session::value_type() : finded->second);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_compress(session::value_type session, bool compress)
{
    if (session){
        session->compress = compress;
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::set_timeout(session::value_type session, unsigned int timeout)
{
    if (session){
        session->timeout = timeout;
    }
}
////////////////////////////////////////////////////////////////////////////////
std::string io_service::get_ip_address(const std::string &data)
{
    std::string addr;
    std::vector<std::string> addrlist;
    for (size_t i = 0; i < data.size() + 1; i++){
        char c = data.c_str()[i];
        if (!http::is_char(c)){
            break;
        }
        if (c == '\0'){
            if (!addr.empty())
                addrlist.push_back(addr);
            break;
        }
        if (c != ',' && c != ' '){
            addr += c;
        } else if (c == ',') {
            addrlist.push_back(addr);
            addr.clear();
        }
    }
    for (size_t i = 0; i < addrlist.size(); i++){
        addr = addrlist[i];
        if (addr.find("10.") == 0){
            continue;
        }
        bool finded = false;
        for (int j = 16; j < 32; j++){
            io::stringc begin;
            begin.format("172.%d.", j);
            if (addr.find(begin) == 0){
                finded = true;
                break;
            }
        }
        if (finded || addr.find("192.168.") == 0){
            continue;
        }
        break;
    }
    return std::move(addr);
}
////////////////////////////////////////////////////////////////////////////////
bool io_service::is_from_agent(session::value_type session)
{
    const char *ip    = session->ipaddr.c_str();
    int size          = (int)session->ipaddr.size();
    unsigned int hash = hash::chksum32(ip, size);

    http::request &request = session->request;
    std::string forwarded  = request.get_header("X-Forwarded-For");
    if (!forwarded.empty()){
        PRINT("CDN X-Forwarded-For: %s\r\n", forwarded.c_str());
        if (m_proxy.find(hash) == m_proxy.end()){
            m_proxy[hash] = ip;
            PRINT("The new CDN node is added(%s).\r\n", ip);
        }
        session->real_ipaddr = get_ip_address(forwarded).c_str();
    }
    return !forwarded.empty();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::reset_session(session::value_type session)
{
    session->recved.clear();
    session->id             = 0;
    session->forward        = 0;
    session->length         = 0;
    session->status         = conn_unknown;
    session->type           = type_unknown;
    session->from_agent     = false;
    session->notify         = false;
    session->compress       = false;
    session->context        = 0;
    session->port           = 0;
    session->online_time    = m_time_now;
    session->last_recv_time = m_time_now;
    session->last_sent_time = m_time_now;
    session->timeout        = SESSION_IDLE_TIMEOUT;
    session->ipaddr.clear();
    session->real_ipaddr.clear();
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_ws_pong(session::value_type session)
{
    if (session->type != type_web_socket){
        return;
    }
    io::stringc packet;
    return send(packet, session, WSP_TYPE_PONG);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_ws_ping(session::value_type session)
{
    if (session->type != type_web_socket){
        return;
    }
    io::stringc packet;
    return send(packet, session, WSP_TYPE_PING);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_ws_reply(session::value_type session)
{
    http::request &request = session->request;
    std::string protocol;
    protocol = request.get_header("Sec-WebSocket-Protocol");
    std::string selected;
    if (!protocol.empty()){
        size_t pos = protocol.find(' ');
        if (pos != std::string::npos){
            selected = protocol.substr(pos, 0);
        }
    }
    io::stringc response;
    response += "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Pragma: no-cache\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "Upgrade: Websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: ";
    response += request.get_wsakey();
    response += "\r\n";
    if (!selected.empty()){
        response += "Sec-WebSocket-Protocol: ";
        response += selected;
        response += "\r\n";
    }
    response += "Server: websocket io service\r\n";
    response += "Server-Build: ";
    response += __TIMESTAMP__;
    response += "\r\n";
    response += "Copyright: stanzhao, All right reserved\r\n";
    response += "Client-Address: ";
    response += ip::tcp::get_peer_name(session->id);
    response += "\r\n\r\n";
    ip::tcp::send(session->id, response);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_response(session::value_type session, const std::string &data)
{
    if (session->type != type_http){
        return;
    }
    io::stringc body(data.c_str());
    body = body.to_utf8();
    io::stringc size;
    size.format("Content-Length: %d\r\n", (int)body.size());
    io::stringc host;
    http::request &request = session->request;
    host.format("Server: %s\r\n", request.get_header("Host").c_str());

    io::stringc head("HTTP/1.1 200 OK\r\n");
    head += "Content-Type: text/html;charset=utf-8\r\n";
    head += "Pragma: no-cache\r\n";
    head += "Cache-Control: no-cache\r\n";
    head += host;
    head += size;
    head += "Connection: Close\r\n";
    head += "\r\n";

    io::stringc response;
    response += head;
    response += body;
    ip::tcp::send(session->id, response);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_error(session::value_type session, const std::string &data)
{
    io::stringc body(data.c_str());
    body = body.to_utf8();
    io::stringc size;
    size.format("Content-Length: %d\r\n", (int)body.size());
    io::stringc host;
    http::request &request = session->request;
    host.format("Server: %s\r\n", request.get_header("Host").c_str());

    io::stringc head("HTTP/1.1 501 Invalid HTTP Request\r\n");
    head += "Content-Type: text/html;charset=utf-8\r\n";
    head += "Pragma: no-cache\r\n";
    head += "Cache-Control: no-cache\r\n";
    head += host;
    head += size;
    head += "Connection: Close\r\n";
    head += "\r\n";

    io::stringc response;
    response += head;
    response += body;
    ip::tcp::send(session->id, response);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::send_ws_packet(session::value_type session, const std::string &data, int type)
{
    if (session->type != type_web_socket){
        return;
    }
    io::stringc content(data.c_str(), (int)data.size());
    if (type == 1 && !content.is_utf8()){
        content = content.to_utf8();
    }
    //压缩数据包
    if (session->compress){
        uLongf zip_size = ZIP_BUFFER_SIZE;
        if (compress((Bytef*)m_zip_buffer, &zip_size, (Bytef*)content.c_str(), content.size()) == Z_OK){
            content = base64::encode((Bytef*)m_zip_buffer, m_base64_data, zip_size);
        }
    }
    const char *begin = content.c_str();
    const char *end = begin + content.size();
    do{
        io::stringc packet;
        begin = http::wsp_parser::pack(packet, begin, end, type, 0);
        ip::tcp::send(session->id, packet); //发送数据帧
    } while (begin < end);
}
////////////////////////////////////////////////////////////////////////////////
void io_service::parse_request(session::value_type session, const char *data, int bytes)
{
    //解析http请求头
    const char *begin = data;
    const char *end   = begin + bytes;
    http::request_parser::result_type result;
    http::request_parser &parser = session->request_parser;
    //解析请求数据
    std::tie(result, begin) = parser.parse(
        session->request, begin, end, 2048
        );
    if (result == http::wsp_parser::indeterminate){
        return;
    }
    if (result == http::wsp_parser::bad){
        save_context(session);
        send_error(session, "<b>Invalid HTTP Request</b>");
        close(session->id);
        return;
    }
    //过滤请求方法
    const http::request &request = session->request;
    std::string method = request.method;
    if (!http::is_equal(method.c_str(), "GET") && 
        !http::is_equal(method.c_str(), "POST"))
    {
        save_context(session);
        send_error(session, "<b>Invalid HTTP Request</b>");
        close(session->id);
        return;
    }
    const std::string key("Content-Length");
    std::string length = request.uri.get_param(key);
    session->length = atoi(length.c_str());
    if (session->length > HTTP_MAX_POST_LENGTH){
        save_context(session);
        send_error(session, "<b>Invalid HTTP Request</b>");
        close(session->id);
        return;
    }
    //判断是否为转发模式
    unsigned short port = 80;
    std::string host = request.get_header("Host");
    size_t pos = host.find(':');
    if (pos != std::string::npos){
        std::string str_port = host.substr(pos + 1);
        port = (unsigned short)atoi(str_port.c_str());
        host = host.substr(0, pos);
    }
    bool is_forward = false;
    if (m_forward.find(port) != m_forward.end()){
        auto iter = m_forward[port].find(host);
        if (iter != m_forward[port].end()){
            is_forward = true;
            host = iter->second;
            pos = host.find(':');
            if (pos != std::string::npos){
                std::string str_port = host.substr(pos + 1);
                port = (unsigned short)atoi(str_port.c_str());
                host = host.substr(0, pos);
            }
        }
    }
    //判断是否为存活检测
    if (is_forward){
        std::string path = request.uri.url;
        if (path == "/797919b5a9c95dfae9b06cf3086e40c1/"){
            is_forward = false;
        }
    }
    session->from_agent = is_from_agent(session);
    session->type = request.is_wsp() ? type_web_socket : type_http;
    if (!is_forward){ //服务模式
        if (session->type == type_web_socket){  //Websocket请求
            send_ws_reply(session);
            on_accept(session);
        } else if (session->type == type_http){ //http请求
            session->recved.clear();
            const std::string data(begin, end - begin);
            recv_http_data(session, data);
        }
    } else { //转发模式
        session::value_type forward = session::create();
        if (!forward){
            save_context(session);
            send_error(session, "<b>Memory Allocation Failed</b>");
            close(session->id);
            return;
        }
        channal socket = ip::tcp::socket(m_io_service);
        reset_session(forward);
        forward->id           = socket;
        forward->status       = conn_connecting;
        forward->forward      = session->id;
        forward->ipaddr       = host.c_str();
        forward->port         = port;
        session->forward      = socket;
        m_sessions[socket]    = forward;
        set_timeout(session, SESSION_BUSY_TIMEOUT);
        set_timeout(forward, SESSION_BUSY_TIMEOUT);
        //保存已接收的数据内容
        if (session->type == type_http){
            if (http::is_equal(method.c_str(), "POST")){
                const std::string data(begin, end - begin);
                session->recved += data;
            }
        }
        ip::tcp::connect(socket, host.c_str(), port);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::parse_response(session::value_type session, const char *data, int bytes)
{
    //解析http请求头
    const char *begin = data;
    const char *end   = begin + bytes;
    http::response_parser::result_type result;
    http::response_parser &parser = session->response_parser;
    //解析请求数据
    session->type = type_web_socket;
    std::tie(result, begin) = parser.parse(
        session->response, begin, end
        );
    if (result == http::wsp_parser::indeterminate){
        return;
    }
    if (result == http::wsp_parser::bad){
        save_context(session);
        close(session->id);
        return;
    }
    const http::response &response = session->response;
    if (response.status != "101"){
        save_context(session);
        close(session->id);
        return;
    }
    std::string seckey = response.get_header("Sec-WebSocket-Accept");
    if (seckey != "zeoRopjyhYzHzVS+WAYphaPeKBg="){
        save_context(session);
        close(session->id);
        return;
    }
    on_connect(session); //回调
}
////////////////////////////////////////////////////////////////////////////////
void io_service::parse_received(session::value_type session, const char *data, int bytes)
{
    switch (session->status){
    case conn_accepted:  parse_request(session, data, bytes);
        break;
    case conn_connected: parse_response(session, data, bytes);
        break;
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::recv_http_data(session::value_type session, const std::string &data)
{
    if (session->forward){
        channal forward = session->forward;
        ip::tcp::send(forward, data.c_str(), (int)data.size());
        return;
    }
    if (!data.empty()){
        session->recved += data;
    }
    if (session->recved.size() >= session->length){
        on_request(session);
        session->request.reset();
        session->type = type_unknown;
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::recv_ws_data(session::value_type session, const std::string &data)
{
    if (session->forward){
        channal forward = session->forward;
        ip::tcp::send(forward, data.c_str(), (int)data.size());
        return;
    }
    const char *begin    = data.c_str();
    const char *end      = begin + data.size();
    http::packet &packet = session->request.wsp;
    http::wsp_parser &parser = session->parser;

    while (begin < end){
        http::wsp_parser::result_type result;
        std::tie(result, begin) = parser.parse(
            session->request, begin, end, HTTP_MAX_POST_LENGTH
            );
        if (result == http::wsp_parser::indeterminate){
            return;
        }
        if (result == http::wsp_parser::bad){
            close(session->id);
            return;
        }
        int type = packet.type();
        if (WSP_TYPE_RESERVED(type)){
            return;
        }
        if (type == WSP_TYPE_CLOSE){
            close(session->id);
            return;
        }
        if (type == WSP_TYPE_PING){
            send_ws_pong(session);
        } else if (type == WSP_TYPE_TEXT){
            session->recved.clear();
            session->recved.append(packet.data(), packet.size());
            if (!session->recved.is_utf8())
                session->recved = session->recved.to_utf8();
            on_request(session);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::save_context(session::value_type session)
{
    npx_make_dir("./~error");
    std::string logstr;
    if (session->status == conn_accepted){
        logstr = session->request.to_string();
        if (!session->recved.empty()){
            logstr += session->recved;
            logstr += "\r\n\r\n";
        }
    } else if (session->status == conn_connected){
        logstr = session->response.to_string();
    }
    io::stringc filename("./~error/dump.txt");
    FILE *fpw = fopen(filename.c_str(), "a");
    if (fpw){
        fwrite(logstr.c_str(), 1, logstr.size(), fpw);
        fclose(fpw);
    }
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_request(session::value_type session)
{
    if (session->type == type_unknown){
        return;
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        if (session->type == type_http)
            send("<b>The proxy server is running properly</b>", session);
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback){
        m_callback->on_request(session);
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_timer(session::value_type session)
{
    if (session->type == type_unknown){
        return;
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback && session->notify){
        m_callback->on_timer(session);
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_error(session::value_type session, int code)
{
    if (session->type == type_unknown){
        if (session->status != conn_connecting){
            return;
        }
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback && session->notify){
        m_callback->on_error(session, code);
        session->notify = false;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_timeout(session::value_type session)
{
    if (session->type == type_unknown){
        return;
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback && session->notify){
        m_callback->on_timeout(session);
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_connect(session::value_type session)
{
    if (session->type == type_unknown){
        return;
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback){
        session->notify = true;
        m_callback->on_connect(session);
        if (session->timeout == SESSION_IDLE_TIMEOUT){
            set_timeout(session, SESSION_BUSY_TIMEOUT);
        }
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_accept(session::value_type session)
{
    if (session->type == type_unknown){
        return;
    }
    if (session->forward){
        return;
    }
    //转发模式不回调
    if (!m_forward.empty()){
        return;
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    __try{
#endif
    if (m_callback){
        session->notify = true;
        m_callback->on_accept(session);
        if (session->timeout == SESSION_IDLE_TIMEOUT){
            set_timeout(session, SESSION_BUSY_TIMEOUT);
        }
    }
#if defined(NPX_WINDOWS) && defined(PUBLISH_SERVER)
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        save_context(session);
    }
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_service::on_forward(session::value_type session, session::value_type forward)
{
    m_forward_cnt++;
    PRINT("* The connection is created, %s <-> %s.\r\n"
        , forward->real_ipaddr.c_str()
        , session->real_ipaddr.c_str()
        );
    session->type = forward->type;
    std::string head = forward->request.to_string();
    ip::tcp::send(session->id, head.c_str(), (int)head.size());
    if (!session->recved.empty()){
        ip::tcp::send(session->id, session->recved);
    }
}
////////////////////////////////////////////////////////////////////////////////
} //End of namespace ws
////////////////////////////////////////////////////////////////////////////////
