#include "zhuanzhuan_player.h"
#include "zhuanzhuan_algo.h"
using namespace mj_base;
namespace zhuanzhuan_mj{
	zhuanzhuan_player::zhuanzhuan_player(){
	}

	void zhuanzhuan_player::reset(){
		__super::reset();

	}

	void zhuanzhuan_player::export_data(Json::Value& value)
	{
		__super::export_data(value);

	}

	void zhuanzhuan_player::analy_data(const Json::Value& value)
	{
		__super::analy_data(value);

	}

	void zhuanzhuan_player::do_peng(int card_id)
	{
		if(hand[card_id] == 3)
			forbidden_gang.push_back(card_id);

		hand[card_id]-=2;

		group_info gf;
		gf.type = group_type::type_ke;
		gf.card_id = card_id;
		gf.list.push_back(card_id);
		group_cards.push_back(gf);
	}
}
