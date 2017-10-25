

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
        user_state    status;      //账户状态
        user_type     type;        //账户类型
        userid        uuid;        //账户唯一ID
        int           level;       //账户等级
        int           promoter;    //推广码
        int           score;       //总比分
        int           quality;     //用户品质(新用户, 老用户， 粉丝用户)
        int           health;      //健康值
        int           package;     //最后领取的礼包ID
        int           payment;     //最后领取的充值ID
        int           gold[3];     //金币数量
        int           diamond[3];  //钻石数量
        time_t        createtime;  //账户创建时间
        io::stringc   device;      //设备类型
        io::stringc   idfa;        //广告ID
        io::stringc   pfid;        //渠道ID
        io::stringc   unionid;     //微信联合ID
        io::stringc   nickname;    //昵称
        io::stringc   emoji_name;  //昵称(带emoji符号)
        io::stringc   sex;         //性别
        io::stringc   country;     //国家
        io::stringc   province;    //省
        io::stringc   city;        //所属城市
        io::stringc   head_url;    //头像的URL
        io::stringc   ipaddr;      //用户IP地址
        Json::Value   extended;    //扩展数据
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
    void inc_golds    (int count);  //增加金币
    void dec_golds    (int count);  //减少金币
    void set_golds    (int rest_count, int used_count);
    void set_score    (int score);
    void inc_diamonds (int count);  //增加钻石
    void dec_diamonds (int count);  //减少钻石
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
    bool today_is_shared();   //今日已分享
    bool load_payment_data(); //加载充值数据
    void user_login_reply (int error); //登录应答
    void user_login_reply (const Json::Value &data, int error);
    void send_error_reply (protocol type, int error); //无效请求
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
    virtual bool write_to_cache();    //保存角色数据到缓存
    virtual bool load_from_cache();   //从缓存读取角色数据
    virtual bool load_package_data(); //加载礼包数据
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
    io::stringc  m_token;           //登录令牌
    io::stringc  m_voice_token;     //通知消息令牌
    int          m_index;           //在房间内的索引
    int          m_room_number;     //所属房间编号
    bool         m_run_back;        //是否切换到后台运行了
    bool         m_new_user;        //新用户标记
    bool         m_destroyed;       //已销毁标记
    bool         m_in_the_room;     //是否在房间内
    data         m_data;            //角色数据
    unsigned int m_hash_id;         //帐号的hash值
    unsigned int m_data_hash;       //角色数据hash值
    time_t       m_time_login;      //用户创建时间(上线时间)
    time_t       m_time_online;     //用户在线时间
};
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_PLAYER_H_
