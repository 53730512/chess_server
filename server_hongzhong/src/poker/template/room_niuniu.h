

#ifndef __ROOM_DOU_NIU_H_
#define __ROOM_DOU_NIU_H_
////////////////////////////////////////////////////////////////////////////////
#include "game_rules.h"
#include "../../room_basic.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
enum room_state{
    room_wait_ready,
    room_wait_begin,
    room_wait_qiang_zhuang,
    room_wait_xia_zhu,
    room_wait_show_poker,
    room_completed
};
////////////////////////////////////////////////////////////////////////////////
typedef struct __room_member{
    userid      uuid;
    io::stringc nickname;
    io::stringc sex;
    io::stringc head_url;
    io::stringc ipaddr;
    time_t      time_enter;
    time_t      time_ready;
    time_t      time_agree;
    poker_hand  hand;
    int         score;
    int         switch_count;
public:
    inline void reset(){
        uuid  = 0;
        score = 0;
        switch_count = 0;
        time_enter = time_ready = time_agree = 0;
        hand.clear();
        nickname.clear();
        sex.clear();
        head_url.clear();
        ipaddr.clear();
    }
    inline __room_member(){reset();}
} room_member;
////////////////////////////////////////////////////////////////////////////////
const static int MAX_MEMBERS = 6;
////////////////////////////////////////////////////////////////////////////////
class game_room : public room_basic{
    typedef room_basic parent;
    io::stringc export_data() const;
    bool import_data(const io::stringc &data);
    bool init_room_rules(const Json::Value &option);
    bool init_room_context(const Json::Value &context);
    bool init_room_members(const Json::Value &members);
public:
    game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit);
protected:
    virtual void on_create ();
    virtual void on_update (int delta);
    virtual void on_enter  (io_player::value_type player);
    virtual void on_leave  (io_player::value_type player, bool is_exit);
    virtual void on_request(io_player::value_type player, protocol type, const Json::Value &data);
    virtual void on_destroy(bool completed);
protected:
    virtual void on_dismiss(io_player::value_type player, const Json::Value &data);
    virtual void on_dismiss_reply(io_player::value_type player, const Json::Value &data);
    virtual void on_ready(io_player::value_type player, const Json::Value &data);
    virtual void on_round_begin(io_player::value_type player, const Json::Value &data);
private:
    void init_next_round();
private:
    Json::Value m_option;
    int m_round;                 //当前局
    int m_round_total;           //总局数
    int m_zhifu;                 //支付方式(1: AA 2: 房主 3: 大赢家)
    int m_fanbei;                //翻倍数
    int m_diamond_pay;           //建房扣钻数
    int m_diamond_huanpai;       //换牌扣钻数
    int m_dismiss_index;         //申请解散的成员索引
    std::vector<int> m_xiazhu;   //下注倍率
    room_state       m_status;   //房间状态
    room_member m_members[MAX_MEMBERS];
};
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_DOU_NIU_H_
