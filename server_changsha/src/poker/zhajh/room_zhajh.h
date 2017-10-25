

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
		room_unknown   = 0,       //δ֪״̬
		room_wait_ready,          //�ȴ��û�׼��
		room_playing,			  //������
		room_completed            //��������
	};

	enum record_type{
		gen	= 1,
		jia,
		bi,
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
		int			max_score;	  //���÷�
		int			max_type;	  //�������
		int         health;       //�û�����ֵ	
		std::vector<poker::value_t>  hand;//��������
		bool		ko;			  //�Ƿ���̭
		int			bet_count;	  //��ע����
		int			win_count;	  //ʤ������
		bool		kan_pai;	  //�Ƿ���
		bool		qi_pai;		  //�Ƿ�����

		std::vector<std::vector<int>> history;//������ʷ
		int			ko_round;		//��KO����
		int			qi_round;		//��������
		int			kan_round;		//��������
		
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
		void init_next_round();   //��ʼ����һ��
		void init_next_poker();   //��ʼ���Ƴ�
		void init_user_poker();   //��ʼ���û�����
		void turn_to_next_player();
		bool is_last_operation_player(int index);//�ж��û��ǲ������һ�����������
		bool is_player_done_operation(int index);//�ж��û���ǰ�ֲ�������û��
		void send_wait_operation();
	private:
		void publish_result (int winner, bool completed); //�����������
		void sync_room_data (io_player::value_type player); //ͬ����������
		void set_room_status(room_state status); //���÷���״̬
		bool check_game_over(int& index);
	private:
		int get_index_by_uuid(userid uuid);
		int get_player_number();
	private:
		Json::Value m_option;
		int  m_idle_time;				//����ʱ��
		int  m_round;					//��ǰ��
		int  m_round_total;				//�ܾ���
		int  m_zhifu;					//֧����ʽ(1: AA 2: ���� 3: ��Ӯ��)
		int  m_diamond_pay;				//����������
		int  m_men_round;				//����Ȧ��
		int	 m_teshu;					//235 0=����aaa 1=���ڱ���
		int m_yazhu;					//1,3,5,30;30�ĳ�ʼѺעΪ1;
		bool m_jiafen;					//����10��,��˳5��

		int  m_dismiss_index;			//�����ɢ�ĳ�Ա����
		bool m_need_payment;			//��Ҫ֧����ʯ
		bool m_time_limit;				//�Ƿ���ʱ����
		io::stringc m_voice_url;		//��Ϣ���͵�ַ
		room_state       m_status;		//����״̬
		int              m_next;		//��һ��������
		poker::value_t   m_pokers[54];
		room_member m_members[MAX_MEMBERS];
		int m_cur_player;				//��ǰ���������
		int m_cur_zhu;					//��ǰ��ע(�����µ�)
		int m_bet_round;				//��ע����
		int m_zhuang;					//ׯ��λ��-���һ�����������
		int m_last_winner;				//�ϰ�Ӯ�����

		int m_waitTime;
		//std::vector<int> m_compareList;

	};


	//�Ƿ�Ϊ����
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

	//�Ƿ�Ϊͬ��
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

	//�Ƿ�Ϊ˳��
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

	//�Ƿ�Ϊ����
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

		//�Ӵ�С����
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
