#include "taojiang_player.h"
#include "taoj_algo.h"
using namespace mj_base;
namespace taojiang_mj{

	//taojiang_player::taojiang_player(room_base* room){
	//}

	void taojiang_player::reset(){
		__super::reset();
	}

	void taojiang_player::export_data(Json::Value& value)
	{
		__super::export_data(value);

	}

	void taojiang_player::analy_data(const Json::Value& value)
	{
		__super::analy_data(value);

	}


	std::vector<int>& taojiang_player::do_chi(int begin, int out_card)
	{
		group_info gf;
		gf.type = group_type::type_shun;
		gf.card_id = begin;
		for(int i=begin; i<begin+3;i++)
		{
			gf.list.push_back(i);
			if(i == out_card)
				continue;

			hand[i]--;
		}

		//把吃的牌放在中间
		if(out_card == gf.list[0])
		{
			std::swap(gf.list[0], gf.list[1]);
		}
		else if(out_card == gf.list[2])
		{
			std::swap(gf.list[1], gf.list[2]);
		}

		group_cards.push_back(gf);

		return group_cards[group_cards.size()-1].list;
	}

}
