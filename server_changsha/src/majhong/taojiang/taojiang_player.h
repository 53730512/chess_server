#ifndef __TAOJIANG_PLAYER_MJ_H_
#define __TAOJIANG_PLAYER_MJ_H_
#include <vector>
#include "../base/player_base.h"
namespace taojiang_mj{
	class taojiang_player : public mj_base::player_base{
	public:
		//taojiang_player(room_base* room);
		virtual void reset() override;//每轮重置数据函数

		virtual void export_data(Json::Value& value) override;
		virtual void analy_data(const Json::Value& value)override;

		virtual std::vector<int>& do_chi(int begin, int out_card) override;
	public:


	private:

	};
}
#endif
