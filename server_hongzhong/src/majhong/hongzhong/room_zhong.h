

#ifndef __ROOM_ZHONG_MJ_H
#define __ROOM_ZHONG_MJ_H
////////////////////////////////////////////////////////////////////////////////
#include "../mj.h"
#include "../base/room_base.h"
#include "zhong_algo.h"
#include <vector>
#include <queue> 
////////////////////////////////////////////////////////////////////////////////
namespace zhong_mj{
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
		virtual void on_gang_pai(userid uuid, const Json::Value &data);
	private:
		virtual void init_next_mahjong() override;
		virtual void init_next_round() override;   //初始化下一轮
		virtual void publish_result (int winner, bool completed) override; //公布比赛结果
		virtual void sync_room_data (io_player::value_type player) override; //同步房间数据
		virtual void round_clear() override;
		virtual void do_hu(int user_index, bool direct=true) override;
		virtual void analy_hu_style(int style, std::vector<int>& vt) override;
		virtual bool check_right_priority(int  user_index, mj_base::right_type type, int card_id);
		virtual bool check_qishouhu(mj_base::player_base* player) override;
		virtual void check_guo(mj_base::player_base* player) override;

	private:
		virtual int get_left_card_number()	override;
	private:
		bool m_enable_7dui;

	};
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_PAO_DK_H_
