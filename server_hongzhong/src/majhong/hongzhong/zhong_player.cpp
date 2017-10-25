#include "zhong_player.h"
#include "zhong_algo.h"
using namespace mj_base;
namespace zhong_mj{
	zhong_player::zhong_player(){
	}

	void zhong_player::reset(){
		__super::reset();

	}

	void zhong_player::export_data(Json::Value& value)
	{
		__super::export_data(value);

	}

	void zhong_player::analy_data(const Json::Value& value)
	{
		__super::analy_data(value);

	}

	void zhong_player::do_peng(int card_id)
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
