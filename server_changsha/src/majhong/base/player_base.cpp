#include "player_base.h"
#include "../mj.h"
namespace mj_base{
	player_base::player_base(){
		index		= 0;
		uuid		= 0;
		score		= 0;
		max_score	= 0;
		health		= 0;
		time_enter	= time_ready = time_agree = 0;
		reset();
		nickname.clear();
		sex.clear();
		head_url.clear();
		ipaddr.clear();
		zimo_count = 0;
		win_count = 0;
		clear_hand();

		minggang_total_num = 0;
		bugang_total_num = 0;
		angang_total_num = 0;
		diangang_total_num = 0;
	}

	void player_base::reset(){

		time_ready = 0;
		group_cards.clear();
		right_list.clear();
		right_list_backup.clear();
		first_operation = true;
		last_get_card = 0;
		last_out_card = 0;
		hu_card = 0;
		out_card_list.clear();
		gang_list.clear();
		zhongniao = 0;
		is_ting = false;
		guo_hu = false;
		hu_fan = 0;
		hu_type_list.clear();

		minggang_num = 0;
		bugang_num = 0;
		angang_num = 0;
		diangang_num = 0;
		forbidden_gang.clear();
		round_score = 0;
	}

	void player_base::export_data(Json::Value& value)
	{
		value["index"]        = index;
		value["uuid"]         = uuid;
		value["nickname"]     = nickname;
		value["device"]       = device;
		value["voice_token"]  = voice_token;
		value["sex"]          = sex;
		value["head_url"]     = head_url;
		value["ipaddr"]       = ipaddr;
		value["score"]        = score;
		value["zimo_count"]        = zimo_count;
		value["win_count"]        = win_count;
		value["time_ready"]   = time_ready;
		value["first_op"]	  = first_operation;
		value["last_get_card"]	= last_get_card;
		value["last_out_card"] = last_out_card;
		value["hu_card"] = hu_card;
		value["is_ting"] = is_ting;
		value["guo_hu"] = guo_hu;

		value["minggang_num"] = minggang_num;
		value["bugang_num"] = bugang_num;
		value["angang_num"] = angang_num;
		value["diangang_num"] = diangang_num;
		value["minggang_total_num"] = minggang_total_num;
		value["bugang_total_num"] = bugang_total_num;
		value["angang_total_num"] = angang_total_num;
		value["diangang_total_num"] = diangang_total_num;
		for (size_t j = 0; j < 49; j++){
			value["hand"].append(hand[j]);
		}
		for (size_t j = 0; j < 49; j++){
			value["init_hand"].append(init_hand[j]);
		}

		for(size_t j=0; j<out_card_list.size(); j++)
		{
			value["out_card_list"].append(out_card_list[j]);
		}

		for(size_t i=0; i<group_cards.size(); i++){
			group_info& gf = group_cards[i];
			Json::Value hItem;
			hItem["type"] = gf.type;
			hItem["card_id"] = gf.card_id;
			for(size_t j=0; j<gf.list.size();j++)
			{
				hItem["list"].append(gf.list[j]);
			}
			value["group_cards"].append(hItem);
		}

		for(size_t i=0; i<gang_list.size(); i++)
		{
			value["gang_list"].append(gang_list[i]);
		}

		for(size_t i=0;i<right_list.size();i++)
		{
			value["right_list"].append(right_list[i]);
		}

		for(size_t i=0;i<right_list_backup.size();i++)
		{
			value["right_list_backup"].append(right_list_backup[i]);
		}

		for(size_t i=0;i<forbidden_gang.size();i++)
		{
			value["forbidden_gang"].append(forbidden_gang[i]);
		}
	}

	void player_base::analy_data(const Json::Value& value)
	{
		uuid         = value["uuid"].asUInt();
		nickname     = value["nickname"].asString().c_str();
		device       = value["device"].asString().c_str();
		voice_token  = value["voice_token"].asString().c_str();
		sex          = value["sex"].asString().c_str();
		head_url     = value["head_url"].asString().c_str();
		ipaddr       = value["ipaddr"].asString().c_str();
		score        = value["score"].asInt();
		zimo_count   = value["zimo_count"].asInt();
		win_count   = value["win_count"].asUInt();
		time_ready   = value["time_ready"].asUInt();
		first_operation = value["first_op"].asBool();
		last_get_card = value["last_get_card"].asUInt();
		last_out_card = value["last_out_card"].asUInt();
		hu_card = value["hu_card"].asUInt();
		is_ting = value["is_ting"].asBool();
		guo_hu = value["guo_hu"].asBool();

		minggang_num = value["minggang_num"].asInt();
		bugang_num = value["bugang_num"].asInt();
		angang_num = value["angang_num"].asInt();
		diangang_num = value["diangang_num"].asInt();
		minggang_total_num = value["minggang_total_num"].asInt();
		bugang_total_num = value["bugang_total_num"].asInt();
		angang_total_num = value["angang_total_num"].asInt();
		diangang_total_num = value["diangang_total_num"].asInt();

		if(value.isMember("hand"))
		{
			for (Json::ArrayIndex j = 0; j < value["hand"].size(); j++){
				hand[j] = value["hand"][j].asInt();
			}
		}

		if(value.isMember("init_hand"))
		{
			for (Json::ArrayIndex j = 0; j < value["init_hand"].size(); j++){
				init_hand[j] = value["init_hand"][j].asInt();
			}
		}

		if(value.isMember("out_card_list"))
		{
			for (Json::ArrayIndex j = 0; j < value["out_card_list"].size(); j++){


				out_card_list.push_back(value["out_card_list"][j].asInt());
			}
		}

		if(value.isMember("group_cards")){
			for(Json::ArrayIndex i=0; i<value["group_cards"].size(); i++){
				const Json::Value& hValue = value["group_cards"][i];

				group_info gf;
				gf.type = (group_type)hValue["type"].asInt();
				gf.card_id = hValue["card_id"].asInt();
				for(Json::ArrayIndex j =0; j<hValue["list"].size(); j++)
				{
					gf.list.push_back(hValue["list"][j].asInt());
				}

				group_cards.push_back(gf);
				//group_cards.push_back(std::make_pair((group_type)value["group_cards"][i]["type"].asInt(), value["group_cards"][i]["point"].asInt()));
			}
		}

		if(value.isMember("gang_list")){
			for(Json::ArrayIndex i=0; i<value["gang_list"].size(); i++)
			{
				gang_list.push_back(value["gang_list"][i].asInt());
			}
		}

		if(value.isMember("right_list"))
		{
			for(Json::ArrayIndex i=0; i<value["right_list"].size(); i++)
			{
				right_list.push_back((right_type)value["right_list"][i].asInt());
			}
		}

		if(value.isMember("right_list_backup"))
		{
			for(Json::ArrayIndex i=0; i<value["right_list_backup"].size(); i++)
			{
				right_list_backup.push_back((right_type)value["right_list_backup"][i].asInt());
			}
		}

		if(value.isMember("forbidden_gang"))
		{
			for(Json::ArrayIndex i=0; i<value["forbidden_gang"].size(); i++)
			{
				forbidden_gang.push_back((right_type)value["forbidden_gang"][i].asInt());
			}
		}
	}

	bool player_base::is_have_card(int card_id)
	{
		if(generic::valid(card_id))
			return hand[card_id] > 0;

		return false;
	}

	void player_base::remove_card(int card_id)
	{
		//有移除牌的行为,说明操作过了
		first_operation = false;
#ifdef _DEBUG
		assert(hand[card_id] > 0);
#endif // DEBUG
		hand[card_id]--;
	}

	void player_base::add_card(int card_id)
	{
		hand[card_id]++;
		last_get_card = card_id;

	}

	void player_base::add_out_card(int card_id){
		out_card_list.push_back(card_id);
	}

	void player_base::remove_last_out_card(){
		out_card_list.pop_back();
	}

	void player_base::add_right(right_type rt){
		if(std::find(right_list.begin(), right_list.end(), rt) != right_list.end())
			return;

		right_list.push_back(rt);
	}

	void player_base::clear_rights(){
		if(!right_list.empty())
			right_list_backup = right_list;

		right_list.clear();

		gang_list.clear();
		//	hu_card = 0;
	}

	right_type player_base::get_biggest_right()
	{
		return right_list[0];
	}

	bool player_base::check_right(right_type rt)
	{
		return std::find(right_list.begin(), right_list.end(), rt) != right_list.end();
	}

	int player_base::get_right_count()
	{
		return (int)right_list.size();
	}

	bool player_base::check_chi_enable(int begin, int out_card){
		if(begin > 29)
			return false;

		bool include_out_card = false;
		for(int i=begin; i<begin+3;i++)
		{
			if(!generic::valid(i))
				return false;

			if(hand[i] <=0 && i != out_card)
				return false;

			if(i == out_card)
				include_out_card = true;
		}

		return include_out_card;
	}

	bool player_base::check_peng_enable(int out_card)
	{
		if(!generic::valid(out_card))
			return false;
		return hand[out_card] >=2;
	}

	std::vector<int>& player_base::do_chi(int begin, int out_card)
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

		group_cards.push_back(gf);

		return group_cards[group_cards.size()-1].list;
	}

	void player_base::do_peng(int card_id)
	{
		hand[card_id]-=2;

		group_info gf;
		gf.type = group_type::type_ke;
		gf.card_id = card_id;
		gf.list.push_back(card_id);
		group_cards.push_back(gf);
	}

	void player_base::add_can_gang(int card_id)
	{
		if(std::find(gang_list.begin(), gang_list.end(), card_id) != gang_list.end())
			return;

		gang_list.push_back(card_id);
	}

	void player_base::bugang_to_peng(int card_id)
	{
		for(size_t i=0; i<group_cards.size();i++)
		{
			if(group_cards[i].type == group_type::type_bu_gang && group_cards[i].list[0] == card_id)
			{
				//将刻改成明杠并且把手牌中的单牌 删除
				group_cards[i].type = group_type::type_ke;
				break;
			}
		}
	}

	void player_base::remove_gang(int card_id)
	{
		for(size_t i=0; i<group_cards.size(); i++)
		{
			if(group_cards[i].card_id == card_id
				&&group_cards[i].type == type_an_gang)
			{
				group_cards.erase(group_cards.begin() + i);
				break;
			}
		}
	}

	group_type player_base::do_gang(int card_id, bool zimo /* = false */)
	{
		group_type result = group_type::type_ming_gang;
		group_info gf;
		gf.list.push_back(card_id);
		gf.card_id = card_id;
		if(zimo)
		{
			if(hand[card_id] == 4)
			{
				hand[card_id] -= 4;

				gf.type = group_type::type_an_gang;
				group_cards.push_back(gf);//暗杠

				result = group_type::type_an_gang;
			}
			else{
				for(size_t i=0; i<group_cards.size();i++)
				{
					if(group_cards[i].type == group_type::type_ke && group_cards[i].list[0] == card_id)
					{
						//将刻改成明杠并且把手牌中的单牌 删除
						group_cards[i].type = group_type::type_bu_gang;
						hand[card_id]--;
						result = group_type::type_bu_gang;
						break;
					}
				}
			}
		}
		else{
			hand[card_id] -= 3;
			gf.type = group_type::type_ming_gang;
			group_cards.push_back(gf);
		}


		return result;
	}

	void player_base::clear_hand()
	{
		memset(hand, 0, sizeof(int)*49);
		memset(init_hand, 0, sizeof(int)*49);
	}

	void player_base::set_rights(Json::Value& hValue)
	{
		if(!right_list.empty())
		{
			for(size_t j=0; j<right_list.size(); j++)
			{
				hValue["rights"].append(right_list[j]);
			}
		}
		else
		{
			for(size_t j=0; j<right_list_backup.size(); j++)
			{
				hValue["rights"].append(right_list_backup[j]);
			}
		}
	}

	bool player_base::is_gang_forbidden(int card_id){
		return std::find(forbidden_gang.begin(), forbidden_gang.end(), card_id) != forbidden_gang.end();
	}

	int player_base::get_hand_card_num()
	{
		int num = 0;
		for(int i=0; i<49; i++)
		{
			num += hand[i];
		}

		return num;
	}
}
