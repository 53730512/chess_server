#ifndef __ZHUANZHUAN_PLAYER_MJ_H_
#define __ZHUANZHUAN_PLAYER_MJ_H_
#include <vector>
#include "../base/player_base.h"
namespace zhuanzhuan_mj{
	class zhuanzhuan_player : public mj_base::player_base{
	public:
		//zhuanzhuan_player(room_base* room);
		zhuanzhuan_player();
		virtual void reset() override;//每轮重置数据函数

		virtual void export_data(Json::Value& value) override;
		virtual void analy_data(const Json::Value& value)override;
		virtual void do_peng(int card_id) override;
	public:
	private:

	};
}
#endif
