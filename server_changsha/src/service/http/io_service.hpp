

#ifndef __WEB_IO_SERVICE_HPP_
#define __WEB_IO_SERVICE_HPP_
////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG  
#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)  
#else  
#define DEBUG_CLIENTBLOCK  
#endif  
#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>
#include <crtdbg.h>
#ifdef _DEBUG  
#define new DEBUG_CLIENTBLOCK  
#endif
////////////////////////////////////////////////////////////////////////////////
//forward config
////////////////////////////////////////////////////////////////////////////////
//{
//    "80" : {
//        "127.0.0.1" : "103.235.232.14",
//        "s.xy.zhanggu88.com" : "27.155.120.90"
//    }
//}
////////////////////////////////////////////////////////////////////////////////
#include "io_parser.h"
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <map>
#include <mutex>
////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
#include <windows.h>
#include <conio.h>
#endif
////////////////////////////////////////////////////////////////////////////////
#define HMAC_MD5_KEY "HQAmZV7eeCrStxEu"
#define SESSION_IDLE_TIMEOUT 10
#define SESSION_BUSY_TIMEOUT 300
#define HTTP_MAX_POST_LENGTH (1024 * 1024)
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
enum session_type{
    type_unknown = 0,    //δ֪�Ự����
    type_http,           //http�Ự
    type_web_socket      //websocket�Ự
};
enum conn_status{
    conn_unknown = 0,    //δ֪����״̬
    conn_accepted,       //�ѽ�������(Accept)
    conn_connecting,     //������(Connecting)
    conn_connected       //������(Connected)
};
////////////////////////////////////////////////////////////////////////////////
typedef npx_socket channal;
////////////////////////////////////////////////////////////////////////////////
typedef struct __session{
    //�ỰID
    channal               id;
    //ת��(�Զ�)�ỰID
    channal               forward;
    //�Ự����
    session_type          type;
    //�Ự״̬
    conn_status           status;
    //�Ƿ����Դ��������
    bool                  from_agent;
    //�Ƿ���Ϣ֪ͨ�ص��ӿ�
    bool                  notify;
    //�Ƿ�ѹ������
    bool                  compress;
    //�Ự����ʱ��(UTC)
    time_t                online_time;
    //������ʱ��(UTC)
    time_t                last_recv_time;
    //�����ʱ��(UTC)
    time_t                last_sent_time;
    //��ʱʱ��(��)
    unsigned int          timeout;
    //POST��������ݳ���
    unsigned int          length;
    //�Զ˶˿�
    unsigned short        port;
    //connect����������Ϣ
    const void*           context;
    //����Զ�IP
    io::stringc           ipaddr;
    //�û���ʵ��IP
    io::stringc           real_ipaddr;
    //���ղ����������
    io::stringc           recved;
    //websocket������
    http::wsp_parser      parser;
    //http������Ϣ
    http::request         request;
    //http���������
    http::request_parser  request_parser;
    //httpӦ����Ϣ
    http::response        response;
    //httpӦ�������
    http::response_parser response_parser;
    //session����ָ������
    typedef std::shared_ptr<__session> value_type;
    //ʵ����һ��sessionʵ��
    inline static value_type create(){
        return value_type(new __session());
    }
} session;
////////////////////////////////////////////////////////////////////////////////
typedef class __io_callback{
public:
    //��ʵ�������������
    virtual void on_create (const void *context) = 0;
    //��ʱ������(��ѭ��)
    virtual void on_update (int delta) = 0;
    //�������󵽴�ʱ������
    virtual void on_request(session::value_type session) = 0;
    //ÿ�뱻����1��
    virtual void on_timer  (session::value_type session) = 0;
    //�������󵽴�ʱ������
    virtual void on_connect(session::value_type session) = 0;
    //�������󵽴�ʱ������
    virtual void on_accept (session::value_type session) = 0;
    //��IO���ִ���ʱ������
    virtual void on_error  (session::value_type session, int code) = 0;
    //�����ӳ�ʱʱ������
    virtual void on_timeout(session::value_type session) = 0;
    //��ʵ��������ǰ����
    virtual void on_destroy() = 0;
} io_callback;
////////////////////////////////////////////////////////////////////////////////
class io_service : public io::callback{
    typedef std::map<std::string, std::string> fwpair_t;
    io::service::value_type m_async;
    io::service::value_type m_io_service;
    std::map<unsigned int, std::string>    m_proxy;
    std::map<unsigned short, fwpair_t>     m_forward;
    std::map<channal, session::value_type> m_sessions;
public:
    static io_service* get_instance();
    virtual ~io_service();
    virtual void stop();
    virtual void clear();
    virtual void close(channal id);
    virtual void close(session::value_type session);
public:
    bool is_runing() const;
    bool listen(unsigned short port, const char *host = 0, int backlog = 32);
    void send(const std::string &data, int type = WSP_TYPE_TEXT);
    void send(const std::string &data, channal id, int type = WSP_TYPE_TEXT);
    void send(const std::string &data, session::value_type session, int type = WSP_TYPE_TEXT);
    void connct(const char *host, unsigned short port, const void *context = 0);
    void run(io_callback *callback = 0, int milliseconds = 0, const void *context = 0);
    void post(int id, int value, const void *context);
    void set_compress(bool compress);
    void set_timeout(unsigned int timeout);
    void set_forward_url(const std::string &url);
    void reconnect(session::value_type session);
    void get_url_async(int type, const io::stringc &url); //�첽����URL
    bool verify_url(session::value_type session);
    const void* get_context() const;
    session::value_type get_cur_session() const;
    session::value_type get_session(channal id) const;
private:
    io_service(); //Ĭ�Ϲ��캯��
    io_service(const io_service &);
    std::string get_ip_address(const std::string &data);
    void clear_forward(); //���forward����
    void set_forword();   //��������forward����
    void get_forword();   //��ȡforward����
    void add_forward(unsigned int port, const std::string &domain, const std::string &addr);
    void del_forward(unsigned int port, const std::string &domain);
private:
    bool is_from_agent (session::value_type session);
    void reset_session (session::value_type session);
    void save_context  (session::value_type session);
    void send_ws_ping  (session::value_type session);
    void send_ws_pong  (session::value_type session);
    void send_ws_reply (session::value_type session);
    void set_timeout   (session::value_type session, unsigned int timeout);
    void set_compress  (session::value_type session, bool compress);
    void send_ws_packet(session::value_type session, const std::string &data, int type);
    void send_error    (session::value_type session, const std::string &data);
    void send_response (session::value_type session, const std::string &data);
    void parse_received(session::value_type session, const char *data, int bytes);
    void parse_request (session::value_type session, const char *data, int bytes);
    void parse_response(session::value_type session, const char *data, int bytes);
    void recv_ws_data  (session::value_type session, const std::string &data);
    void recv_http_data(session::value_type session, const std::string &data);
private:
    void on_connect    (session::value_type session);
    void on_accept     (session::value_type session);
    void on_request    (session::value_type session);
    void on_error      (session::value_type session, int code);
    void on_timer      (session::value_type session);
    void on_timeout    (session::value_type session);
    void on_forward    (session::value_type session, session::value_type forward);
private:
    void on_update     (int delta);
    void on_timer      (int id, int delta);
    void on_connect    (channal id);
    void on_error      (channal id, int code);
    void on_accept     (channal id, channal acceptor);
    void on_receive    (channal id, const char *data, int bytes);
    void on_message    (channal id, int value, const void *context);
    void on_response   (bool result, const std::string &url, const std::string &data);
private:
    char*               m_zip_buffer;
    char*               m_base64_data;
    unsigned short      m_local_port;
    io_callback        *m_callback;
    int                 m_backlog;
    time_t              m_time_now;
    channal             m_local_listen;
    channal             m_cur_channal_id;
    std::string         m_forward_url;
    unsigned int        m_forward_cnt;
    Json::Value         m_perform_data;
};
////////////////////////////////////////////////////////////////////////////////
} //End of namespace http
////////////////////////////////////////////////////////////////////////////////
#endif  //__WEB_IO_SERVICE_HPP_
