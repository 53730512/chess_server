

#ifndef __ROOM_DOU_NIU_H_
#define __ROOM_DOU_NIU_H_
////////////////////////////////////////////////////////////////////////////////
#include "game_rules.h"
#include "../../room_basic.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
enum room_state{
    room_unknown   = 0,       //未知状态
    room_wait_ready,          //等待用户准备
    room_wait_begin,          //等待用户开局
    room_wait_qiang_zhuang,   //等待用户抢庄
    room_wait_xia_zhu,        //等待闲家下注
    room_wait_show_poker,     //等待用户亮牌
    room_completed            //比赛结束
};
enum huan_zhuang{
    zhuang_unknow  = 0,       //通比牛牛
    zhuang_qiang1  = 1,       //明牌抢庄
    zhuang_qiang2  = 2,       //自由抢庄
    zhuang_niuniu  = 3,       //牛牛上庄
    zhuang_order   = 4,       //轮流坐庄
    zhuang_random  = 5        //随机当庄
};
////////////////////////////////////////////////////////////////////////////////
typedef struct __room_member{
    userid      uuid;         //唯一号
    io::stringc nickname;     //昵称
    io::stringc sex;          //性别
    io::stringc head_url;     //头像URL
    io::stringc ipaddr;       //IP 地址
    io::stringc device;       //设备类型
    io::stringc voice_token;  //通知token
    time_t      time_enter;   //进入时间
    time_t      time_ready;   //准备时间
    time_t      time_agree;   //同意解散
    int         score;        //比赛成绩
    int         health;       //用户健康值
    poker_hand  hand;         //手牌数据
    bool        is_show_hand; //是否亮牌了
    int         show_count;   //显示几张牌(明牌数量)
    int         switch_poker; //换出的牌
    int         switch_count; //换牌次数
    bull_type   switch_style; //换牌后的牌型
    int         xia_zhu;      //下注分数
    int         qiang_zhuang; //抢庄倍数
    bull_type   bull_type;    //牌型类型
public:
    inline void reset(){
        uuid       = 0;
        score      = 0;
        health     = 0;
        time_enter = time_ready = time_agree = 0;
        init();
        nickname.clear();
        sex.clear();
        head_url.clear();
        ipaddr.clear();
    }
    inline void init(){
        is_show_hand = false;
        show_count   = 0;
        switch_poker = 0;
        switch_count = 0;
        xia_zhu      = 0;
        qiang_zhuang = 0;
        bull_type    = bull_null;
        switch_style = bull_null;
        hand.clear();
    }
    inline __room_member(){reset();}
} room_member;
////////////////////////////////////////////////////////////////////////////////
const static int MAX_MEMBERS = 6;
////////////////////////////////////////////////////////////////////////////////
class game_room : public room_basic{
    typedef room_basic parent;
    io::stringc export_data() const;
    io::stringc get_options() const;
    io::stringc export_score() const;
    bool import_data(const io::stringc &data);
    bool init_room_rules(const Json::Value &option);
    bool init_room_context(const Json::Value &context);
    bool init_room_members(const Json::Value &members);
public:
    virtual ~game_room(){}
    game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit);
protected:
    virtual void on_create ();
    virtual void on_update (int delta);
    virtual void on_enter  (io_player::value_type player);
    virtual void on_leave  (io_player::value_type player, bool is_exit);
    virtual void on_request(io_player::value_type player, protocol type, const Json::Value &data);
    virtual void on_destroy(bool completed);
protected:
    virtual void on_dismiss(userid uuid, const Json::Value &data);
    virtual void on_dismiss_reply(userid uuid, const Json::Value &data);
    virtual void on_ready(userid uuid, const Json::Value &data);
    virtual void on_round_begin(userid uuid, const Json::Value &data);
    virtual void on_qiang_zhuang(userid uuid, const Json::Value &data);
    virtual void on_xia_zhu(userid uuid, const Json::Value &data);
    virtual void on_huan_pai(userid uuid, const Json::Value &data);
    virtual void on_publish_hand(userid uuid, const Json::Value &data);
    virtual void on_room_broadcast(userid uuid, const Json::Value &data);
    virtual void on_status_timeout(userid uuid, const Json::Value &data);
private:
    void init_next_round();   //初始化下一轮
    void init_next_poker();   //初始化牌池
    void init_user_poker();   //初始化用户手牌
    void send_user_poker();   //发送用户手牌数据
    void auto_send_ready();   //自动发送准备请求
    void auto_send_qiang();   //自动发送抢庄请求
    void auto_send_xiazhu();  //自动发送下注请求
    void auto_send_publish(); //自动发送亮牌请求
private:
    void publish_result (bool completed); //公布比赛结果
    void sync_room_data (io_player::value_type player); //同步房间数据
    int  get_bull_value (bull_type type); //获取牛牌分值
    int  get_user_score (int index, int other); //获取用户成绩
    int  get_winner_index(); //获取最大牌用户索引
    void set_room_status(room_state status); //设置房间状态
    bull_type get_bull_style(int index); //计算用户牌型
    bull_type get_bull_style(const poker_hand &hand);
private:
    Json::Value m_option;
    int  m_idle_time;          //空闲时间
    int  m_round;              //当前局
    int  m_round_total;        //总局数
    int  m_zhifu;              //支付方式(1: AA 2: 房主 3: 大赢家)
    int  m_fanbei;             //翻倍数
    int  m_diamond_pay;        //建房扣钻数
    int  m_diamond_huanpai;    //换牌扣钻数
    int  m_huan_zhuang;        //换庄类型
    int  m_dismiss_index;      //申请解散的成员索引
    int  m_index_zhuang;       //庄家索引
    bool m_no_flower;          //无花牌
    bool m_need_payment;       //需要支付钻石
    bool m_time_limit;         //是否限时操作
    bool m_qiang_zhuang;       //需要抢庄决定庄
    bool m_intervene;          //是否干预输赢比率
    io::stringc m_voice_url;   //消息推送地址
    std::vector<int> m_xiazhu; //下注分数
    std::vector<int> m_qiang;  //抢庄倍率
    room_state       m_status; //房间状态
    int              m_next;   //下一张牌索引
    poker::value_t   m_pokers[54];
    room_member m_members[MAX_MEMBERS];
};
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_DOU_NIU_H_
