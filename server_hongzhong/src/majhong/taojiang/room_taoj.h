

#ifndef __ROOM_TAOJIANG_MJ_H_y
#define __ROOM_TAOJIANG_MJ_H_
////////////////////////////////////////////////////////////////////////////////
#include "../mj.h"
#include "../base/room_base.h"
#include "taoj_algo.h"
#include <vector>
#include <queue> 
////////////////////////////////////////////////////////////////////////////////
namespace taojiang_mj{
	////////////////////////////////////////////////////////////////////////////////
	class game_room : public mj_base::room_base{
		typedef room_base parent;
		virtual io::stringc export_score() const override;
		virtual bool init_room_rules(const Json::Value &option) override;
		virtual bool init_room_context(const Json::Value &context) override;
		virtual bool init_room_members(const Json::Value &members) override;
	public:
		game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit);
		io::stringc export_data() const;
	protected:
		virtual void on_update (int delta);
		virtual void on_request(io_player::value_type player, protocol type, const Json::Value &data);
		virtual void on_gang_pai(userid uuid, const Json::Value &data)override;
		virtual void on_guo(userid uuid, const Json::Value &data) override;
	private:
		virtual void init_next_mahjong() override;
		virtual void init_next_round() override;   //初始化下一轮
		virtual void publish_result (int winner, bool completed) override; //公布比赛结果
		virtual void sync_room_data (io_player::value_type player) override; //同步房间数据
		virtual void round_clear() override;
		virtual bool deal_chupai(int deal_pos, int card_id) override;//出牌检测吃碰杠胡,有则返回true,
		virtual void do_gang(int user_index, int card_id, bool zimo/* =false */, bool direct=true) override;
		virtual void do_hu(int user_index, bool direct=true) override;
		virtual void analy_hu_style(int style, std::vector<int>& vt) override;
		virtual void check_my_turn_gang(mj_base::player_base* player);//自己摸牌后杠检测
		virtual bool check_other_hu(int except_index, int card_id, bool gang = false);
	private:
		virtual int get_left_card_number()	override;
	private:
		int             m_ding_to_end;				//定王牌到牌墙最后一张牌的距离
		int				m_dingwang;					//定王牌


	};
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_PAO_DK_H_
