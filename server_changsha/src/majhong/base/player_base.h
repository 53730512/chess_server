#ifndef __PLAYER_BASE_MJ_H_
#define __PLAYER_BASE_MJ_H_
#include <vector>
#include "../../io_player.h"
#include "game_rules.h"
namespace mj_base{
	struct group_info 
	{
		group_type type;
		std::vector<int> list;
		int card_id;
		group_info(){
			type = group_type::type_none;
			list.clear();
			card_id = 0;
		}
	};

	class player_base{
	public:
		player_base();
		virtual void reset();//每轮重置数据函数
		virtual void export_data(Json::Value& value);
		virtual void analy_data(const Json::Value& value);
		void set_index(int index){this->index = index;};
		int get_index(){return index;};

		bool is_have_card(int card_id);

		//牌的操作
		void remove_card(int card_id);
		void add_card(int card_id);

		void add_out_card(int card_id);
		void remove_last_out_card();

		//权限操作
		void add_right(right_type rt);
		void clear_rights();
		int get_rights_count(){return this->right_list.size();}
		mj_base::right_type get_biggest_right();
		bool check_right(right_type rt);
		int get_right_count();

		//是否能吃
		bool check_chi_enable(int begin, int out_card);
		bool check_peng_enable(int out_card);
		//吃碰杠牌操作处理
		virtual std::vector<int>& do_chi(int begin, int out_card);
		virtual void do_peng(int card_id);
		group_type do_gang(int card_id, bool zimo = false);
		void remove_gang(int card_id);
		void bugang_to_peng(int card_id);
		void add_can_gang(int card_id);//可杠牌处理
		void clear_hand();

		void set_rights(Json::Value& hValue);
		bool is_gang_forbidden(int card_id);

		int get_hand_card_num();//获得手牌数量,不包括组牌
	public:
		int			index;		  //在房间的序号
		userid      uuid;         //唯一号
		io::stringc nickname;     //昵称
		io::stringc sex;          //性别
		io::stringc head_url;     //头像URL
		io::stringc ipaddr;       //IP 地址
		io::stringc device;       //设备类型
		io::stringc voice_token;  //通知token
		time_t      time_enter;   //进入时间
		time_t      time_agree;   //同意解散
		int         score;        //比赛成绩
		int			round_score;  //当前轮的分数
		int			max_score;	  //最大得分
		int         health;       //用户健康值	
		int			zimo_count;	//自摸次数
		int			win_count;		//胜利次数
		std::vector<int>		out_card_list;//出牌历史

		//每轮需重置的数据
		int  hand[49];//手牌数据
		int  init_hand[49];//初始手牌数据
		std::vector<group_info> group_cards;//组牌数据(吃碰杠)
		std::vector<right_type> right_list;
		std::vector<right_type> right_list_backup;
		std::vector<int> gang_list;//可以杠的牌
		int			hu_card;	  //胡牌的牌值
		std::vector<int> hu_type_list;//胡的番型列表
		int			hu_fan;		   //胡的番值;
		time_t      time_ready;   //准备时间
		bool		first_operation;//是不是第一次操作,吃碰杠出牌后设为false;
		int			last_get_card;
		int			last_out_card;
		int			zhongniao;   //是否中鸟
		bool		is_ting;	//是否听牌
		bool		guo_hu;		//是否过胡

		//杠相关
		int			minggang_num;
		int			bugang_num;
		int			angang_num;
		int			diangang_num;

		int			minggang_total_num;
		int			bugang_total_num;
		int			angang_total_num;
		int			diangang_total_num;

		std::vector<int> forbidden_gang;//手里3张 碰后禁止再杠列表
	};
}
#endif
