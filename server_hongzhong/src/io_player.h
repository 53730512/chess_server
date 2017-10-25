

#ifndef __IO_PLAYER_H_
#define __IO_PLAYER_H_
////////////////////////////////////////////////////////////////////////////////
#include "io_protocol.h"
#include "service/json/json.h"
#include "service/http/io_service.h"
#include "service/http/io_perform.h"
////////////////////////////////////////////////////////////////////////////////
#define WECHAT_PFID  "wechat"
////////////////////////////////////////////////////////////////////////////////
inline __int64 _atoll(const char *p)
{
    int minus = 0;
    __int64 value = 0;
    if (*p == '-'){
        minus++;
        p++;
    }
    while (*p >= '0' && *p <= '9'){
        value *= 10;
        value += *p - '0';
        p++;
    }
    return minus ? 0 - value : value;
}
////////////////////////////////////////////////////////////////////////////////
enum user_type{
    user_type_user  = 0,
    user_type_admin
};
enum user_state{
    user_state_ok   = 0,
    user_state_lock
};
enum user_golds{
    currency_rest   = 0,
    currency_used,
    currency_total
};
typedef unsigned int userid;
////////////////////////////////////////////////////////////////////////////////
class io_player
    : public std::enable_shared_from_this<io_player>{
    const http_context m_context;
    io_player(http_context context);
public:
    typedef struct __data{
        user_state    status;      //�˻�״̬
        user_type     type;        //�˻�����
        userid        uuid;        //�˻�ΨһID
        int           level;       //�˻��ȼ�
        int           promoter;    //�ƹ���
        int           score;       //�ܱȷ�
        int           quality;     //�û�Ʒ��(���û�, ���û��� ��˿�û�)
        int           health;      //����ֵ
        int           package;     //�����ȡ�����ID
        int           payment;     //�����ȡ�ĳ�ֵID
        int           gold[3];     //�������
        int           diamond[3];  //��ʯ����
        time_t        createtime;  //�˻�����ʱ��
        io::stringc   device;      //�豸����
        io::stringc   idfa;        //���ID
        io::stringc   pfid;        //����ID
        io::stringc   unionid;     //΢������ID
        io::stringc   nickname;    //�ǳ�
        io::stringc   emoji_name;  //�ǳ�(��emoji����)
        io::stringc   sex;         //�Ա�
        io::stringc   country;     //����
        io::stringc   province;    //ʡ
        io::stringc   city;        //��������
        io::stringc   head_url;    //ͷ���URL
        io::stringc   ipaddr;      //�û�IP��ַ
        Json::Value   extended;    //��չ����
        unsigned int  get_hash() const;
        Json::Value   to_json_object() const;
        void from_json_object(const Json::Value &data);
    } data;
    typedef std::shared_ptr<io_player> value_type;
public:
    static void       update(int delta);
    static void       clear();
    static void       destroy(value_type player);
    static void       broadcast(protocol type, const Json::Value &data);
    static value_type create(http_context context);
    static value_type find(userid uuid);
    static value_type find(http::channal id);
    static value_type find(http_context context);
    static size_t     get_player_count();
public:
    void close();
    void set_uuid     (userid uuid);
    void send         (protocol type, const Json::Value &data, int errcode = 0);
    void inc_golds    (int count);  //���ӽ��
    void dec_golds    (int count);  //���ٽ��
    void set_golds    (int rest_count, int used_count);
    void set_score    (int score);
    void inc_diamonds (int count);  //������ʯ
    void dec_diamonds (int count);  //������ʯ
    void set_diamonds (int rest_count, int used_count);
public:
    inline bool        is_run_back()        const{return m_run_back;}
    inline bool        is_in_the_room()     const{return m_in_the_room;}
    inline int         get_index()          const{return m_index;}
    inline int         get_room_number()    const{return m_room_number;}
    inline io::stringc get_token()          const{return m_token;}
    inline io::stringc get_voice_token()    const{return m_voice_token;}
    inline time_t      get_login_time()     const{return m_time_login;}
    inline time_t      get_create_time()    const{return m_data.createtime;}
    inline user_state  get_state()          const{return m_data.status;}
    inline user_type   get_type()           const{return m_data.type;}
    inline userid      get_uuid()           const{return m_data.uuid;}
    inline int         get_level()          const{return m_data.level;};
    inline int         get_promoter()       const{return m_data.promoter;}
    inline int         get_score()          const{return m_data.score;}
    inline int         get_health()         const{return m_data.health;}
    inline int         get_quality()        const{return m_data.quality;}
    inline int         get_package_id()     const{return m_data.package;}
    inline int         get_payment()        const{return m_data.payment;}
    inline int         get_golds_rest()     const{return m_data.gold[currency_rest];};
    inline int         get_golds_used()     const{return m_data.gold[currency_used];};
    inline int         get_diamonds_rest()  const{return m_data.diamond[currency_rest];};
    inline int         get_diamonds_used()  const{return m_data.diamond[currency_used];};
    inline io::stringc get_device()         const{return m_data.device;}
    inline io::stringc get_idfa()           const{return m_data.idfa;}
    inline io::stringc get_pfid()           const{return m_data.pfid;}
    inline io::stringc get_unionid()        const{return m_data.unionid;}
    inline io::stringc get_nickname()       const{return m_data.nickname;}
    inline io::stringc get_emoji_name()     const{return m_data.emoji_name;}
    inline io::stringc get_sex()            const{return m_data.sex;}
    inline io::stringc get_country()        const{return m_data.country;}
    inline io::stringc get_province()       const{return m_data.province;}
    inline io::stringc get_city()           const{return m_data.city;}
    inline io::stringc get_head_url()       const{return m_data.head_url;}
    inline io::stringc get_ipaddr()         const{return m_data.ipaddr;}
    inline Json::Value get_extended()       const{return m_data.extended;}
    inline const http_context get_context() const{return m_context;}
    inline unsigned int get_hash_id()       const{return m_hash_id;}
    std::string get_promoter_url() const;
public:
    virtual ~io_player(){}
    virtual void on_update(int delta);
    virtual void on_create();
    virtual void on_timeout();
    virtual void on_error(int error);
    virtual void on_timer();
    virtual void on_destroy();
    virtual void on_login(const io::stringc &data);
    virtual void on_request(const io::stringc &data);
public:
    bool today_is_shared();   //�����ѷ���
    bool load_payment_data(); //���س�ֵ����
    void user_login_reply (int error); //��¼Ӧ��
    void user_login_reply (const Json::Value &data, int error);
    void send_error_reply (protocol type, int error); //��Ч����
    void user_package_data(const Json::Value &data, int error);
    void user_payment_data(const Json::Value &data, int error);
public:
    inline void  set_index      (int index)   {m_index         = index;}
    inline void  set_level      (int level)   {m_data.level    = level;}
    inline void  set_promoter   (int promoter){m_data.promoter = promoter;}
    inline void  set_quality    (int quality) {m_data.quality  = quality;}
    inline void  set_room_number(int number)  {m_room_number   = number;}
    inline void  set_in_the_room(bool inside) {m_in_the_room   = inside;}
    inline void  set_package    (int package) {m_data.package  = package;}
    inline void  set_payment    (int payment) {m_data.payment  = payment;}
    inline void  set_extended(const Json::Value &value){m_data.extended = value;}
private:
    virtual bool write_to_cache();    //�����ɫ���ݵ�����
    virtual bool load_from_cache();   //�ӻ����ȡ��ɫ����
    virtual bool load_package_data(); //�����������
    virtual bool player_login (unsigned int utc, bool rlogin);
    virtual bool wechat_login (unsigned int utc, bool rlogin);
    virtual bool xrgame_login (unsigned int utc, bool rlogin);
    virtual void parse_request(protocol type, const Json::Value &data);
    virtual void parse_unknown(protocol type, const Json::Value &data);
private:
    virtual void on_enter_room (const Json::Value &data);
    virtual void on_leave_room (const Json::Value &data);
    virtual void on_keep_alive (const Json::Value &data);
    virtual void on_create_room(const Json::Value &data);
    virtual void on_payment_sign(const Json::Value &data);
    virtual void on_bind_promoter(const Json::Value &data);
    virtual void on_wechat_shared(const Json::Value &data);
    virtual void on_load_zhanji(const Json::Value &data);
    virtual void on_load_zhanji_round(const Json::Value &data);
    virtual void on_run_back_notify(const Json::Value &data);
    virtual void on_set_voice_token(const Json::Value &data);
private:
    io::stringc  m_token;           //��¼����
    io::stringc  m_voice_token;     //֪ͨ��Ϣ����
    int          m_index;           //�ڷ����ڵ�����
    int          m_room_number;     //����������
    bool         m_run_back;        //�Ƿ��л�����̨������
    bool         m_new_user;        //���û����
    bool         m_destroyed;       //�����ٱ��
    bool         m_in_the_room;     //�Ƿ��ڷ�����
    data         m_data;            //��ɫ����
    unsigned int m_hash_id;         //�ʺŵ�hashֵ
    unsigned int m_data_hash;       //��ɫ����hashֵ
    time_t       m_time_login;      //�û�����ʱ��(����ʱ��)
    time_t       m_time_online;     //�û�����ʱ��
};
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_PLAYER_H_
