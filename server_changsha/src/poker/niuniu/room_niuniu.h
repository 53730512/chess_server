

#ifndef __ROOM_DOU_NIU_H_
#define __ROOM_DOU_NIU_H_
////////////////////////////////////////////////////////////////////////////////
#include "game_rules.h"
#include "../../room_basic.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
enum room_state{
    room_unknown   = 0,       //δ֪״̬
    room_wait_ready,          //�ȴ��û�׼��
    room_wait_begin,          //�ȴ��û�����
    room_wait_qiang_zhuang,   //�ȴ��û���ׯ
    room_wait_xia_zhu,        //�ȴ��м���ע
    room_wait_show_poker,     //�ȴ��û�����
    room_completed            //��������
};
enum huan_zhuang{
    zhuang_unknow  = 0,       //ͨ��ţţ
    zhuang_qiang1  = 1,       //������ׯ
    zhuang_qiang2  = 2,       //������ׯ
    zhuang_niuniu  = 3,       //ţţ��ׯ
    zhuang_order   = 4,       //������ׯ
    zhuang_random  = 5        //�����ׯ
};
////////////////////////////////////////////////////////////////////////////////
typedef struct __room_member{
    userid      uuid;         //Ψһ��
    io::stringc nickname;     //�ǳ�
    io::stringc sex;          //�Ա�
    io::stringc head_url;     //ͷ��URL
    io::stringc ipaddr;       //IP ��ַ
    io::stringc device;       //�豸����
    io::stringc voice_token;  //֪ͨtoken
    time_t      time_enter;   //����ʱ��
    time_t      time_ready;   //׼��ʱ��
    time_t      time_agree;   //ͬ���ɢ
    int         score;        //�����ɼ�
    int         health;       //�û�����ֵ
    poker_hand  hand;         //��������
    bool        is_show_hand; //�Ƿ�������
    int         show_count;   //��ʾ������(��������)
    int         switch_poker; //��������
    int         switch_count; //���ƴ���
    bull_type   switch_style; //���ƺ������
    int         xia_zhu;      //��ע����
    int         qiang_zhuang; //��ׯ����
    bull_type   bull_type;    //��������
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
    void init_next_round();   //��ʼ����һ��
    void init_next_poker();   //��ʼ���Ƴ�
    void init_user_poker();   //��ʼ���û�����
    void send_user_poker();   //�����û���������
    void auto_send_ready();   //�Զ�����׼������
    void auto_send_qiang();   //�Զ�������ׯ����
    void auto_send_xiazhu();  //�Զ�������ע����
    void auto_send_publish(); //�Զ�������������
private:
    void publish_result (bool completed); //�����������
    void sync_room_data (io_player::value_type player); //ͬ����������
    int  get_bull_value (bull_type type); //��ȡţ�Ʒ�ֵ
    int  get_user_score (int index, int other); //��ȡ�û��ɼ�
    int  get_winner_index(); //��ȡ������û�����
    void set_room_status(room_state status); //���÷���״̬
    bull_type get_bull_style(int index); //�����û�����
    bull_type get_bull_style(const poker_hand &hand);
private:
    Json::Value m_option;
    int  m_idle_time;          //����ʱ��
    int  m_round;              //��ǰ��
    int  m_round_total;        //�ܾ���
    int  m_zhifu;              //֧����ʽ(1: AA 2: ���� 3: ��Ӯ��)
    int  m_fanbei;             //������
    int  m_diamond_pay;        //����������
    int  m_diamond_huanpai;    //���ƿ�����
    int  m_huan_zhuang;        //��ׯ����
    int  m_dismiss_index;      //�����ɢ�ĳ�Ա����
    int  m_index_zhuang;       //ׯ������
    bool m_no_flower;          //�޻���
    bool m_need_payment;       //��Ҫ֧����ʯ
    bool m_time_limit;         //�Ƿ���ʱ����
    bool m_qiang_zhuang;       //��Ҫ��ׯ����ׯ
    bool m_intervene;          //�Ƿ��Ԥ��Ӯ����
    io::stringc m_voice_url;   //��Ϣ���͵�ַ
    std::vector<int> m_xiazhu; //��ע����
    std::vector<int> m_qiang;  //��ׯ����
    room_state       m_status; //����״̬
    int              m_next;   //��һ��������
    poker::value_t   m_pokers[54];
    room_member m_members[MAX_MEMBERS];
};
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_DOU_NIU_H_
