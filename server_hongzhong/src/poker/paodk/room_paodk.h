

#ifndef __ROOM_PAO_DK_H_
#define __ROOM_PAO_DK_H_
////////////////////////////////////////////////////////////////////////////////
#include "game_rules.h"
#include "../../room_basic.h"
#include <vector>
#include <queue> 
////////////////////////////////////////////////////////////////////////////////
namespace paodk{
	inline int get_value(poker::value_t v){return poker::get_point(v) + (poker::get_point(v) < 3 ? 13 : 0);}
	inline int get_value(poker::point p){return p + (p < 3 ? 13 : 0);}
	////////////////////////////////////////////////////////////////////////////////
	enum room_state{
		room_unknown   = 0,       //未知状态
		room_wait_ready,          //等待用户准备
		room_qie_pai,			  //切牌
		room_wait_begin,          //等待用户开局
		room_playing,			  //比赛中
		room_completed            //比赛结束
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
		std::vector<poker::value_t>  hand;//手牌数据
		std::vector<poker::value_t>  init_hand;//手牌数据
		//std::vector<std::vector<poker::value_t>> playCards;//出牌历史
		int			bomb;		  //炸弹数	
		bool			hongtao;		  //红桃加倍
		int			bomb_score;	  //本轮炸弹得分
		int			total_bomb;	  //炸弹总数
		int			max_score;	  //最大得分
		int			win_count;	  //胜利局数
		bool			baodan;		  //是否报单
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
			//playCards.clear();
			bomb			= 0;
			bomb_score	= 0;
			hongtao		= false;
			total_bomb	= 0;
			max_score	= 0;
			win_count	= 0;
			baodan		= false;
		}
		inline void init(){
			hand.clear();
			init_hand.clear();
			time_ready = 0;
			//playCards.clear();
			bomb = 0;
			bomb_score = 0;
			hongtao = false;
			baodan = false;
		}
		inline __room_member(){reset();}
	} room_member;

	struct last_card_info
	{
		int							last_player;			//上次出牌的玩家
		std::vector<poker::value_t>	last_play_cards;//上一把出的牌
		CARD_GROUP_TYPE				last_play_type; //
		void clear()
		{
			last_player = -1;
			last_play_cards.clear();
			last_play_type = CARD_GROUP_TYPE::NONE;
		}
	};
	////////////////////////////////////////////////////////////////////////////////
	const static int MAX_MEMBERS = 3;
	const static int BOMB_SCORE = 10;
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

		void on_play_card(userid uuid, const Json::Value& data);
		void on_qie_pai(userid uuid, const Json::Value& data);
	private:
		void wait_qie_pai();	  //等待切牌
		void init_next_round();   //初始化下一轮
		void init_next_poker();   //初始化牌池
		void init_user_poker();   //初始化用户手牌
		void send_user_poker();   //发送用户手牌数据
		void turn_to_next_player();
		void send_wait_play_card(bool first_play = false, bool disable_play = false);
		int get_next_player();
		int get_last_player();
	private:
		void publish_result (int winner, bool completed); //公布比赛结果
		void sync_room_data (io_player::value_type player); //同步房间数据
		void set_room_status(room_state status); //设置房间状态
	private:
		int get_index_by_uuid(userid uuid);
		void remove_cards(int index, const std::vector<poker::value_t>& list);
		bool check_play_card(int userIndex, CARD_GROUP_TYPE type, std::vector<poker::value_t>& list);//检测出牌
		bool check_can_play_card(int userIndex);
		int get_player_number();
		void on_play_bomb(int index);

		bool have_card_of_bomb(std::vector<poker::value_t>& hand, std::vector<poker::value_t>& outCard);
		poker::value_t get_max_card(std::vector<poker::value_t>& hand);
	private:
		Json::Value m_option;
		int  m_idle_time;				//空闲时间
		int  m_round;				//当前局
		int  m_round_total;			//总局数
		int  m_zhifu;				//支付方式(1: AA 2: 房主 3: 大赢家)
		int  m_diamond_pay;			//建房扣钻数
		int  m_wanfa;				//15张还是16张
		int  m_max_players;			//最大人数
		int  m_four_dai;				//4带3
		int  m_dismiss_index;			//申请解散的成员索引
		bool m_bomb_kechai;			//炸弹可拆
		bool m_need_payment;			//需要支付钻石
		bool m_time_limit;			//是否限时操作
		bool m_show_left;				//显示剩余牌数
		bool m_hongtao10;				//红桃10翻倍
		int m_qie_endtime;			// 切牌结束时间
		int m_qie_index;			//切牌的下标
		bool m_already_qie;			//是否已经切过牌了
		io::stringc m_voice_url;   //消息推送地址
		room_state       m_status; //房间状态
		int              m_next;   //下一张牌索引
		poker::value_t   m_pokers[54];
		room_member m_members[MAX_MEMBERS];
		int m_cur_player;			//当前操作的玩家

		std::vector<std::pair<int, std::vector<poker::value_t>>> m_history;
		last_card_info	m_tgLast_card;
		int m_waitTime;

	};
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为单牌
	inline int is_single(std::vector<poker::value_t>& data){
		return data.size() == 1;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为对牌
	inline poker::value_t is_double(std::vector<poker::value_t>& data){
		poker::value_t vt = 0;
		if (data.size() != 2){
			return vt;
		}

		if(get_value(data[0]) == get_value(data[1]))
			vt = data[0];

		return vt;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为三带二
	inline poker::value_t is_three_double(std::vector<poker::value_t>& data,  bool is_end){
		if (data.size() < 3 || data.size() > 5){
			return poker::point_null;
		}
		//如果牌局没结束且出牌数不为5
		if (!is_end && data.size() != 5){
			return 0;
		}

		//判断连续三张牌是否相同
		for (size_t i = 0; i < data.size() - 2; i++){
			if (get_value(data[i]) == get_value(data[i + 1]) && get_value(data[i + 1]) == get_value(data[i + 2])){
				return data[i];
			}
		}
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为四带三
	inline poker::value_t is_four_three(std::vector<poker::value_t>& data, bool is_end){
		if (data.size() < 4 || data.size() > 7){
			return 0;
		}
		//如果牌局没结束且出牌数不为7
		if (!is_end && data.size() != 7){
			return 0;
		}

		for (size_t i = 0; i < data.size() - 3; i++){
			if (get_value(data[i]) == get_value(data[i + 1]) 
				&& get_value(data[i + 1]) == get_value(data[i + 2])
				&& get_value(data[i + 2]) == get_value(data[i + 3])){
					return data[i];
			}
		}
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为飞机
	inline poker::value_t is_air_plane(std::vector<poker::value_t>& data, bool is_end){
		//判断牌数是否符合要求
		if (data.size() < 6 || (!is_end && data.size() % 5)){
			return 0;
		}

		std::vector<poker::value_t> threeList;
		//分离出三张一样的牌数
		for (size_t i = 0; i < data.size() - 2; i++){
			if (get_value(data[i]) == get_value(data[i + 1]) && get_value(data[i + 1]) == get_value(data[i + 2])){
				threeList.push_back(data[i]);	
				i += 2;
			}
		}

		if (threeList.size() < 2){ //至少要有2个3带牌
			return 0;
		}

		//计算最大的连续三张牌的数量
		int index = 0;
		int new_index = 0;
		int max_order_three = 1, cur_order_three = 1;
		for (size_t i = 0; i < threeList.size() - 1; i++){
			if(poker::get_point(threeList[i]) == 2)
				continue;

			if (get_value(threeList[i])-1 == get_value(threeList[i+1])){
				cur_order_three++;
			} else {
				if (cur_order_three > max_order_three){
					max_order_three = cur_order_three;
					index = new_index;
				}
				if(new_index != index)
					new_index = i+1;
				cur_order_three = 1;
			}
		}
		if (cur_order_three > max_order_three){
			max_order_three = cur_order_three;
		}

		if (max_order_three < 2){ //至少要有2个连续的3带牌
			return 0;
		}
		if (!is_end){
			while(max_order_three > 2)
			{
				if(max_order_three * 5 == data.size()){
					return threeList[index];
				}
				max_order_three--;
			}
		}
		size_t need_count = max_order_three * 5;
		return (need_count >= threeList.size()) ? threeList[index] : 0;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为飞机(四飞)
	inline poker::value_t is_bomber(std::vector<poker::value_t>& data, bool is_end){
		//判断牌数是否符合要求
		if (data.size() < 8 || (!is_end && data.size() % 7)){
			return 0;
		}

		std::vector<poker::value_t> fourList;
		//分离出四张一样的牌数
		for (size_t i = 0; i < data.size() - 3; i++){
			if (get_value(data[i]) == get_value(data[i + 1]) 
				&& get_value(data[i + 1]) == get_value(data[i + 2])
				&& get_value(data[i + 2]) == get_value(data[i + 3])){
					fourList.push_back(data[i]);	
					i += 3;
			}
		}
		if (fourList.size() < 2){ //至少要有2个4带牌
			return 0;
		}
		int index = 0;
		int newIndex = 0;
		int max_order_four = 1, cur_order_four = 1;
		for (size_t i = 0; i < fourList.size() - 1; i++){
			if (get_value(fourList[i])-1 == get_value(fourList[i+1])){
				cur_order_four++;
			} else {
				if (cur_order_four > max_order_four){
					max_order_four = cur_order_four;
					index = newIndex;
				}
				if(newIndex == index)
					newIndex = i+1;
				cur_order_four = 1;
			}
		}
		if (cur_order_four > max_order_four){
			max_order_four = cur_order_four;
		}
		if (max_order_four < 2){ //至少要有2个连续的3带牌
			return 0;
		}
		if (!is_end){
			for (int i = max_order_four; i > 0; i--, index++){
				if (i * 7 == data.size()){
					return fourList[index];
				}
			}
			return 0;
		}

		if (!is_end){

			while(max_order_four > 2)
			{
				if(max_order_four * 7 == data.size()){
					return fourList[index];
				}
				max_order_four--;
			}
		}
		size_t need_count = max_order_four * 7;
		return (need_count >= data.size()) ? fourList[index] : 0;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为炸弹
	inline poker::value_t is_bomb(std::vector<poker::value_t>& data){
		if (data.size() != 4){
			return 0;
		}

		if (poker::get_point(data[0]) == poker::get_point(data[1]) && poker::get_point(data[1]) == poker::get_point(data[2])&&poker::get_point(data[2])==poker::get_point(data[3])){
			return data[0];
		}
		return 0;
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为顺子
	inline poker::value_t is_order(std::vector<poker::value_t>& data){
		if (data.size() < 5 || data.size() > 12){
			return 0;
		}

		for(size_t i=0; i<data.size()-1;i++){
			if(poker::get_point(data[i]) == 2 //有2跳过
				||poker::get_point(data[i+1]) == 2){
					return 0;
			}

			//判断是否连续
			if(get_value(data[i])-1 != get_value(data[i+1]))
				return 0;
		}

		return data[0];
	}
	////////////////////////////////////////////////////////////////////////////////
	//判断是否为连对
	inline poker::value_t is_order_double(std::vector<poker::value_t>& data){
		if (data.size() < 4 || data.size() > 16 || data.size() % 2){
			return 0;
		}

		std::vector<poker::point> point_data;
		for(size_t i=0; i<data.size(); i++)
		{
			point_data.push_back(poker::get_point(data[i]));
		}

		//去重
		point_data.erase(std::unique(point_data.begin(), point_data.end()), point_data.end());
		if(point_data.size()*2 != data.size())
			return 0;

		for(size_t i=0; i<point_data.size()-1;i++){
			if(point_data[i] == 2 //有2跳过
				||point_data[i+1] == 2){
					return 0;
			}

			//判断是否连续
			if(get_value(point_data[i])-1 != get_value(point_data[i+1]))
				return 0;
		}

		return data[0];
	}
	////////////////////////////////////////////////////////////////////////////////
	inline bool have_double(std::vector<poker::value_t>& data, std::vector<poker::value_t>& last){

		std::vector<poker::point> point_list;
		for(size_t i=0; i<data.size(); i++)
		{
			point_list.push_back(poker::get_point(data[i]));
		}

		std::vector<poker::point> double_list;
		for(size_t i=0; i<point_list.size()-1; i++)
		{
			if(point_list[i] == 2)
				continue;

			if(point_list[i] == point_list[i+1])
				double_list.push_back(point_list[i]);
		}

		double_list.erase(std::unique(double_list.begin(), double_list.end()), double_list.end());

		if(double_list.empty())
			return false;

		int beginValue = get_value(last[0]);//起始
		size_t num = last.size()/2;//个数

		if(double_list.size() < num
			|| get_value(double_list[0]) <= beginValue)
			return false;

		size_t curNum = 1;
		if(curNum >= num)
			return true;

		if(double_list.size() < 2)
			return false;
		for(size_t i=0; i<double_list.size()-1; i++)
		{
			if(get_value(double_list[i]) <= beginValue && curNum == 1)
				break;

			if(get_value(double_list[i])-1 == get_value(double_list[i+1]))
			{
				curNum++;
				if(curNum >= num)
					return true;
			}
			else
				curNum = 1;
		}

		return curNum > num;
	}
	////////////////////////////////////////////////////////////////////////////////
	inline bool have_three(std::vector<poker::value_t>& data, std::vector<poker::value_t>& last){
		std::vector<poker::point> point_list_init;
		for(size_t i=0; i<last.size(); i++)
		{
			point_list_init.push_back(poker::get_point(last[i]));
		}

		if(point_list_init.size()<3)
			return false;

		std::vector<poker::point> three_list_init;
		for(size_t i=0; i<point_list_init.size()-2; i++)
		{
			if(point_list_init[i] == 2)
				continue;

			if(point_list_init[i] == point_list_init[i+1]
			&&point_list_init[i] == point_list_init[i+2])
				three_list_init.push_back(point_list_init[i]);
		}

		three_list_init.erase(std::unique(three_list_init.begin(), three_list_init.end()), three_list_init.end());

		std::vector<poker::point> point_list;
		for(size_t i=0; i<data.size(); i++)
		{
			point_list.push_back(poker::get_point(data[i]));
		}
		if(point_list.size() < 3)
			return false;

		std::vector<poker::point> three_list;
		for(size_t i=0; i<point_list.size()-2; i++)
		{
			if(point_list[i] == 2)
				continue;

			if(point_list[i] == point_list[i+1]
			&&point_list[i] == point_list[i+2])
				three_list.push_back(point_list[i]);
		}

		three_list.erase(std::unique(three_list.begin(), three_list.end()), three_list.end());

		if(three_list.empty())
			return false;

		int beginValue = get_value(three_list_init[0]);//起始
		size_t num = last.size()/5;//个数

		if(three_list.size() < num
			|| get_value(three_list[0]) <= beginValue)
			return false;

		size_t curNum = 1;
		if(curNum >= num)
			return true;


		for(size_t i=0; i<three_list.size()-1; i++)
		{
			if(get_value(three_list[i]) <= beginValue && curNum == 1)
				break;

			if(get_value(three_list[i])-1 == get_value(three_list[i+1]))
			{
				curNum++;

				if(curNum >= num)
					return true;
			}
			else
				curNum = 1;
		}

		return curNum > num;
	}

	inline bool have_four(std::vector<poker::value_t>& data, std::vector<poker::value_t>& last)
	{
		std::vector<poker::point> point_list_init;
		for(size_t i=0; i<last.size(); i++)
		{
			point_list_init.push_back(poker::get_point(last[i]));
		}

		if(point_list_init.size()<4)
			return false;

		std::vector<poker::point> four_list_init;
		for(size_t i=0; i<point_list_init.size()-3; i++)
		{
			if(point_list_init[i] == 2)
				continue;

			if(point_list_init[i] == point_list_init[i+1]
			&&point_list_init[i] == point_list_init[i+2]
			&&point_list_init[i] == point_list_init[i+3])
				four_list_init.push_back(point_list_init[i]);
		}

		four_list_init.erase(std::unique(four_list_init.begin(), four_list_init.end()), four_list_init.end());

		std::vector<poker::point> point_list;
		for(size_t i=0; i<data.size(); i++)
		{
			point_list.push_back(poker::get_point(data[i]));
		}

		if(point_list.size()<4)
			return false;

		std::vector<poker::point> four_list;
		for(size_t i=0; i<point_list.size()-3; i++)
		{
			if(point_list[i] == 2)
				continue;

			if(point_list[i] == point_list[i+1]
			&&point_list[i] == point_list[i+2]
			&&point_list[i] == point_list[i+3])
				four_list.push_back(point_list[i]);
		}

		four_list.erase(std::unique(four_list.begin(), four_list.end()), four_list.end());

		if(four_list.empty())
			return false;

		int beginValue = get_value(four_list_init[0]);//起始
		size_t num = last.size()/7;//个数

		if(four_list.size() < num
			|| get_value(four_list[0]) <= beginValue)
			return false;

		size_t curNum = 1;
		if(curNum >= num)
			return true;


		for(size_t i=0; i<four_list.size()-1; i++)
		{
			if(get_value(four_list[i]) <= beginValue && curNum == 1)
				break;

			if(get_value(four_list[i])-1 == get_value(four_list[i-1]))
			{
				curNum++;
				if(curNum >= num)
					return true;
			}
			else
				curNum = 1;
		}

		return curNum > num;
	}
	////////////////////////////////////////////////////////////////////////////////
	inline void sort_four(std::vector<poker::value_t>& data, std::vector<poker::value_t>& out_data){
		if(data.size() < 4)
			return;

		for(size_t i=0; i<data.size()-3; i++)
		{
			if(get_value(data[i]) == get_value(data[i+1])
				&&get_value(data[i]) == get_value(data[i+2])
				&&get_value(data[i]) == get_value(data[i+3]))
			{
				out_data.push_back(data[i]);
				i+=3;
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	inline bool have_order(std::vector<poker::value_t>& data, std::vector<poker::value_t>& last){
		std::vector<poker::point> point_list;

		for(size_t i=0; i<data.size(); i++)
		{
			poker::point pt = poker::get_point(data[i]);
			if(pt != 2)
				point_list.push_back(pt);
		}

		point_list.erase(std::unique(point_list.begin(), point_list.end()), point_list.end());

		int beginValue = get_value(last[0]);
		size_t num = last.size();

		if(point_list.size() < num
			|| get_value(point_list[0]) < beginValue)
			return false;

		if(point_list.size() < 2)
			return false;

		size_t curNum = 1;
		for(size_t i=0; i<point_list.size()-1; i++)
		{
			if(get_value(point_list[i]) <= beginValue && curNum == 1)
				break;

			if(get_value(point_list[i]) - 1 == get_value(point_list[i+1]))
			{
				curNum++;
				if(curNum >= num)
					return true;
			}
			else
				curNum = 1;
		}

		return false;
	}
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_PAO_DK_H_
