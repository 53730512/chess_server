

#ifndef __ROOM_BASE_MJ_H_
#define __ROOM_BASE_MJ_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../room_basic.h"
#include "player_base.h"
#include <vector>
#include <queue> 
#include "algo_base.h"
#include "game_rules.h"

////////////////////////////////////////////////////////////////////////////////
namespace mj_base{
	enum room_state{
		room_unknown   = 0,       //未知状态
		room_wait_ready,          //等待用户准备
		room_playing,			  //比赛中
		room_completed            //比赛结束
	};

	struct right_wait
	{
		right_wait()
		{
			index = -1;
			right = 0;
			card_id = 0;
			gang_index = -1;
			hu_list.clear();
		}

		void  clear(){
			index = -1;
			right = 0;
			card_id = 0;
			gang_index = -1;
			hu_list.clear();
		}

		int index;
		int right;
		int card_id;//吃的话保存 三张牌的第一张
		int gang_index;
		std::vector<int> hu_list;
	};

	class room_base : public room_basic{
		typedef room_basic parent;

		io::stringc get_options() const;
		virtual io::stringc export_score() const = 0 ;//const;
		virtual bool import_data(const io::stringc &data);
	public:
		room_base(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit);
		~room_base(){
			delete m_algo;
			for(size_t i=0; i<m_members.size();i++){
				delete m_members[i];
			}
			m_members.clear();
		}
		void export_data(Json::Value& result) const;
		virtual bool init_room_rules(const Json::Value &option);
		virtual bool init_room_context(const Json::Value &context);
		virtual bool init_room_members(const Json::Value &members);
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
		virtual void on_room_broadcast(userid uuid, const Json::Value &data);

		void on_chu_pai(userid uuid, const Json::Value &data);
		void on_chi_pai(userid uuid, const Json::Value &data);
		void on_peng_pai(userid uuid, const Json::Value &data);
		virtual void on_gang_pai(userid uuid, const Json::Value &data);
		virtual void on_guo(userid uuid, const Json::Value &data);
		void on_hu(userid uuid, const Json::Value &data);
	protected:
		virtual void init_next_round() = 0; 
		virtual void turn_to_next_player();
		virtual void round_clear();
		virtual bool check_guo_hu(int index);
		virtual bool check_other_hu(int except_index, int card_id, bool gang = false);
		bool check_player_hu(int index, int card_id, bool gang);
		virtual void check_mo_pai_right(mj_base::player_base* player);
		virtual void check_my_turn_gang(mj_base::player_base* player);//自己摸牌后杠检测//转转函数
		virtual bool deal_chupai(int deal_pos, int card_id); //转转出牌
		virtual void sync_room_data (io_player::value_type player)=0; //同步房间数据

		virtual void publish_result (int winner, bool completed) = 0;


		virtual void prepare_aglo(mj_base::player_base* player);
		virtual void init_next_mahjong();   //初始化牌池
		void init_user_mahjong();   //初始化用户手牌
		void send_user_mahjong();   //发送用户手牌数据
		void set_room_status(room_state status); //设置房间状态
		void fa_pai();
		void send_wait_operation(bool all = false);
		void send_operation(int index);

	private:
		//void init_next_round();   //初始化下一轮
		bool is_last_operation_player(int index);//判断用户是不是最后一个操作的玩家
		bool is_player_done_operation(int index);//判断用户当前轮操作过了没有
	protected:

		char get_rand_dice(char& small_shaizi);         //获取随机骰子
		int get_player_number();
		virtual int get_left_card_number();
		int get_index_by_uuid(userid uuid);

		int find_next_player();
		//多人操作有效性判断,临时存储,及是否最搞优先级处理
		virtual bool check_right_priority(int  user_index, right_type type, int card_id);
		//清理每个人的权限
		void clear_all_rights();
		//检测下标操作优先级
		bool check_index_priority(int src_index, int dst_index);
		//进行吃牌行为
		void do_chi(int user_index, int begin, int out_card, bool direct);
		void do_peng(int user_index, int out_card, bool direct);

		virtual void do_gang(int user_index, int card_id, bool zimo=false, bool direct=true);
		virtual void do_hu(int user_index, bool direct = true) = 0;
		virtual bool check_qishouhu(player_base* player){return false;};
		virtual void check_guo(player_base* player){}

		//解析int番值到数组
		virtual void analy_hu_style(int style, std::vector<int>& vt){};
	public:
		std::vector<player_base*> m_members;
		Json::Value		m_option;
		int				m_idle_time;				//空闲时间
		int				m_round;					//当前局
		int				m_round_total;				//总局数
		int				m_zhifu;					//支付方式(1: AA 2: 房主 3: 大赢家)
		int				m_diamond_pay;				//建房扣钻数
		int				m_wang;						//王牌
		int				m_renshu;					//人数
		int				m_enable_chi;				//是否允许吃
		int				m_zhuaniao;					//抓鸟
		int				m_beilv;					//倍率
		bool			m_enable_dian_pao;			//是否允许点炮
		bool			m_zhuangxian;				//是否分庄闲

		int				m_init_card_num;			//初始牌池数量
		int				m_zhuang;					//庄
		int				m_dismiss_index;			//申请解散的成员索引
		bool			m_time_limit;				//是否限时操作
		io::stringc		m_voice_url;				//消息推送地址
		room_state      m_status;					//房间状态
		int             m_next;						//下一张牌索引	

		std::vector<int>m_mahjong;		
		int				m_cur_player;				//当前操作的玩家
		int				m_last_out_player;		    //最后出牌的玩家
		int				m_last_winner;				//上把赢的玩家
		int				m_next_zhuang;				//下一把的庄
		int				m_cards_end_id;				//最后一张牌的牌值
		right_wait		m_right_wait;				//操作列表
		std::vector<int> m_gangzi;					//杠子
		std::vector<int> m_niao;					//抓鸟
		Json::Value		m_history;					//战绩历史
		bool			m_enable_tian_hu;			//是否满足天胡
		bool			m_zimo_hu;					//是否是自摸胡

		algo_base*		m_algo;

		int			m_block_time;					//听牌出牌阻塞时间
		std::vector<Json::Value>		m_block_json;				//听牌出牌阻塞协议;

	};
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_PAO_DK_H_
