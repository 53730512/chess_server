

#ifndef __ROOM_ZHA_JH_H_
#define __ROOM_ZHA_JH_H_
////////////////////////////////////////////////////////////////////////////////
#include "game_rules.h"
#include "../../room_basic.h"
#include <vector>
#include <queue> 
////////////////////////////////////////////////////////////////////////////////
namespace zhajh{
	inline int get_value(poker::value_t v){return poker::get_point(v) + (poker::get_point(v) == 1 ? 13 : 0);}
	inline int get_value(poker::point p){return p + (p == 1 ? 13 : 0);}
	////////////////////////////////////////////////////////////////////////////////

	struct GROUP_INFO
	{
		CARD_GROUP_TYPE type;
		std::vector<poker::value_t> valueList;

		GROUP_INFO(){
			type = CARD_GROUP_TYPE::NONE;
			valueList.clear();
		}
	};

	enum room_state{
		room_unknown   = 0,       //未知状态
		room_wait_ready,          //等待用户准备
		room_playing,			  //比赛中
		room_completed            //比赛结束
	};

	enum record_type{
		gen	= 1,
		jia,
		bi,
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
		int			max_score;	  //最大得分
		int			max_type;	  //最大牌型
		int         health;       //用户健康值	
		std::vector<poker::value_t>  hand;//手牌数据
		bool		ko;			  //是否被淘汰
		int			bet_count;	  //下注总数
		int			win_count;	  //胜利次数
		bool		kan_pai;	  //是否看牌
		bool		qi_pai;		  //是否弃牌

		std::vector<std::vector<int>> history;//出牌历史
		int			ko_round;		//被KO轮数
		int			qi_round;		//棋牌轮数
		int			kan_round;		//看牌轮数
		
	public:
		inline void reset(){
			uuid       = 0;
			score      = 0;
			max_score  = 0;
			max_type   = 0;
			health     = 0;
			time_enter = time_ready = time_agree = 0;
			init();
			nickname.clear();
			sex.clear();
			head_url.clear();
			ipaddr.clear();

			ko = false;
			bet_count = 0;
			win_count = 0;
			kan_pai = false;
			qi_pai = false;
			hand.clear();
			history.clear();

			ko_round = -1;
			qi_round = -1;
			kan_round = -1;
		}
		inline void init(){
			hand.clear();
			history.clear();

			time_ready = 0;
			ko = false;
			bet_count = 0;
			kan_pai = false;
			qi_pai = false;

			ko_round = -1;
			qi_round = -1;
			kan_round = -1;
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
		virtual void on_room_broadcast(userid uuid, const Json::Value &data);

		void on_gen_zhu(userid uuid, const Json::Value& data);
		void on_jia_zhu(userid uuid, const Json::Value& data);
		void on_qi_pai(userid uuid, const Json::Value& data);
		void on_kan_pai(userid uuid, const Json::Value& data);
		void on_bi_pai(userid uuid, const Json::Value& data);
	private:
		void init_next_round();   //初始化下一轮
		void init_next_poker();   //初始化牌池
		void init_user_poker();   //初始化用户手牌
		void turn_to_next_player();
		bool is_last_operation_player(int index);//判断用户是不是最后一个操作的玩家
		bool is_player_done_operation(int index);//判断用户当前轮操作过了没有
		void send_wait_operation();
	private:
		void publish_result (int winner, bool completed); //公布比赛结果
		void sync_room_data (io_player::value_type player); //同步房间数据
		void set_room_status(room_state status); //设置房间状态
		bool check_game_over(int& index);
	private:
		int get_index_by_uuid(userid uuid);
		int get_player_number();
	private:
		Json::Value m_option;
		int  m_idle_time;				//空闲时间
		int  m_round;					//当前局
		int  m_round_total;				//总局数
		int  m_zhifu;					//支付方式(1: AA 2: 房主 3: 大赢家)
		int  m_diamond_pay;				//建房扣钻数
		int  m_men_round;				//闷牌圈数
		int	 m_teshu;					//235 0=大于aaa 1=大于豹子
		int m_yazhu;					//1,3,5,30;30的初始押注为1;
		bool m_jiafen;					//豹子10分,金顺5分

		int  m_dismiss_index;			//申请解散的成员索引
		bool m_need_payment;			//需要支付钻石
		bool m_time_limit;				//是否限时操作
		io::stringc m_voice_url;		//消息推送地址
		room_state       m_status;		//房间状态
		int              m_next;		//下一张牌索引
		poker::value_t   m_pokers[54];
		room_member m_members[MAX_MEMBERS];
		int m_cur_player;				//当前操作的玩家
		int m_cur_zhu;					//当前底注(暗牌下的)
		int m_bet_round;				//下注轮数
		int m_zhuang;					//庄家位置-最后一个操作的玩家
		int m_last_winner;				//上把赢的玩家

		int m_waitTime;
		//std::vector<int> m_compareList;

	};


	//是否为豹子
	inline bool is_bao_zi(std::vector<poker::value_t>& card_list, GROUP_INFO& gf)
	{
		if(poker::get_point(card_list[0]) == poker::get_point(card_list[1])
			&&poker::get_point(card_list[0]) == poker::get_point(card_list[2]))
		{
			gf.type = CARD_GROUP_TYPE::BAOZI;
			gf.valueList.push_back(card_list[0]);
			return true;
		}

		return false;
	}

	//是否为同花
	inline bool is_jin_hua(std::vector<poker::value_t>& card_list, GROUP_INFO& gf)
	{
		if(poker::get_type(card_list[0]) == poker::get_type(card_list[1])
			&&poker::get_type(card_list[0]) == poker::get_type(card_list[2]))
		{
			gf.type = CARD_GROUP_TYPE::JINHUA;
			gf.valueList.push_back(card_list[0]);
			gf.valueList.push_back(card_list[1]);
			gf.valueList.push_back(card_list[2]);
			return true;
		}

		return false;
	}

	//是否为顺子
	inline bool is_shunzi(std::vector<poker::value_t>& card_list, GROUP_INFO& gf)
	{
		
		if(get_value(card_list[0]) - 1 == get_value(card_list[1])
			&&get_value(card_list[0]) - 2 == get_value(card_list[2]))
		{
			gf.valueList.clear();
			gf.type = CARD_GROUP_TYPE::SHUNZI;
			gf.valueList.push_back(card_list[0]);
			return true;
		}
		else
		{
			if(poker::get_point(card_list[0]) == 1
				&&poker::get_point(card_list[1]) == 3
				&&poker::get_point(card_list[2]) == 2)
			{
				gf.valueList.clear();
				gf.type = CARD_GROUP_TYPE::SHUNZI;
				gf.valueList.push_back(card_list[1]);
				return true;
			}
		}

		return false;
	}

	//是否为对子
	inline bool is_duizi(std::vector<poker::value_t>& card_list, GROUP_INFO& gf)
	{
		if(poker::get_point(card_list[0]) == poker::get_point(card_list[1]))
		{
			gf.type = CARD_GROUP_TYPE::PAIRS;
			gf.valueList.push_back(card_list[0]);
			gf.valueList.push_back(card_list[2]);
			return true;
		}
		else if(poker::get_point(card_list[1]) == poker::get_point(card_list[2]))
		{
			gf.type = CARD_GROUP_TYPE::PAIRS;
			gf.valueList.push_back(card_list[1]);
			gf.valueList.push_back(card_list[0]);
			return true;
		}

		return false;
	}

	inline bool is_235(std::vector<poker::value_t>& card_list, GROUP_INFO& gf)
	{
		if(poker::get_point(card_list[0]) == 5
			&&poker::get_point(card_list[1]) == 3
			&&poker::get_point(card_list[2]) == 2)
		{
			gf.type = CARD_GROUP_TYPE::SPECIAL;
			gf.valueList.push_back(card_list[0]);
			return true;
		}
		return false;
	}

	inline GROUP_INFO get_group_type(std::vector<poker::value_t> card_list)
	{
		GROUP_INFO gf;
		if(card_list.empty())
			return std::move(gf);

		//从大到小排序
		std::sort(card_list.begin(),card_list.end(),[](poker::value_t& p1, poker::value_t& p2){
			return get_value(p1) > get_value(p2); 
		});

		gf.type = CARD_GROUP_TYPE::COMMON;
		if(!is_bao_zi(card_list, gf))
		{
			if(is_jin_hua(card_list, gf))
			{
				if(is_shunzi(card_list, gf))
				{
					gf.type = CARD_GROUP_TYPE::SHUNJIN;
				}
			}
			else{
				if(!is_shunzi(card_list, gf))
				{
					if(!is_duizi(card_list, gf))
					{
						if(!is_235(card_list, gf))
						{
							gf.valueList.push_back(card_list[0]);
							gf.valueList.push_back(card_list[1]);
							gf.valueList.push_back(card_list[2]);
						}
					}
				}
			}
		}


		return std::move(gf);
	}

	inline bool card_compare(std::vector<poker::value_t>& card_list1, std::vector<poker::value_t>& card_list2, int teshu=0)
	{
		GROUP_INFO gf1 = get_group_type(card_list1);
		GROUP_INFO gf2 = get_group_type(card_list2);

		if(gf1.type != gf2.type)
		{
			if(teshu == 0){
				if(gf1.type == CARD_GROUP_TYPE::SPECIAL && gf2.type == CARD_GROUP_TYPE::BAOZI)
				{	
					if(poker::get_point(card_list2[0]) == poker::point_A
						&&poker::get_point(card_list2[1]) == poker::point_A
						&&poker::get_point(card_list2[2]) == poker::point_A)
						return true;
					else
						return false;
				}
				else if(gf1.type == CARD_GROUP_TYPE::BAOZI && gf2.type == CARD_GROUP_TYPE::SPECIAL)
				{
					if(poker::get_point(card_list1[0]) == poker::point_A
						&&poker::get_point(card_list1[1]) == poker::point_A
						&&poker::get_point(card_list1[2]) == poker::point_A)
						return false;
					else
						return true;
				}
			}
			else{
				if(gf1.type == CARD_GROUP_TYPE::SPECIAL && gf2.type == CARD_GROUP_TYPE::BAOZI)
					return true;
				else if(gf1.type == CARD_GROUP_TYPE::BAOZI && gf2.type == CARD_GROUP_TYPE::SPECIAL)
					return false;
			}

			if(gf1.type == CARD_GROUP_TYPE::SPECIAL)
				return false;

			if(gf2.type == CARD_GROUP_TYPE::SPECIAL)
				return true;

			return gf1.type > gf2.type;
		}
		else{
			if(gf1.type == CARD_GROUP_TYPE::PAIRS)
			{
				if(get_value(gf1.valueList[0]) != get_value(gf2.valueList[0]))
				{
					return get_value(gf1.valueList[0]) > get_value(gf2.valueList[0]);
				}
				else{
					return get_value(gf1.valueList[1]) > get_value(gf2.valueList[1]);
				}
			}
			else if(gf1.type == CARD_GROUP_TYPE::COMMON
				||gf1.type == CARD_GROUP_TYPE::JINHUA)
			{
				if(get_value(gf1.valueList[0]) != get_value(gf2.valueList[0]))
				{
					return get_value(gf1.valueList[0]) > get_value(gf2.valueList[0]);
				}
				else{
					if(get_value(gf1.valueList[1]) != get_value(gf2.valueList[1]))
						return get_value(gf1.valueList[1]) > get_value(gf2.valueList[1]);
					else
						return get_value(gf1.valueList[2]) > get_value(gf2.valueList[2]);
				}
			}
			else
			{
				return get_value(gf1.valueList[0]) > get_value(gf2.valueList[0]);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_PAO_DK_H_
