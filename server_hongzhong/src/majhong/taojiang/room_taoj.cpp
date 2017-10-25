

#include <algorithm>
#include "../protocol.h"
#include "../../io_handler.h"
#include "taojiang_player.h"

using namespace mj_base;
#define ERROR_BREAK(code) {retCode = code; goto __RESULT;}
////////////////////////////////////////////////////////////////////////////////
namespace taojiang_mj{
	///////////////////////////////////////////////////////////////////////////////

	const time_t delay_send_disable_op_time = 600;
	////////////////////////////////////////////////////////////////////////////////
	game_room::game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
		: room_base(creater, type, ruleid, number, time_now, owner_exit)
	{
		m_algo = new taoj_algo();
	}

	//基类会自动调用
	void game_room::round_clear()
	{
		__super::round_clear();
		m_ding_to_end		= 0; 
		m_dingwang			= 0;
	}

	bool game_room::init_room_rules(const Json::Value &option)
	{

		m_round_total = option["jushu"].asInt() ? 16 : 8;

		switch (option["renshu"].asInt())
		{
		case 0:
			m_renshu = 4;
			m_diamond_pay = 6;
			break;
		case 1:
			m_renshu = 3;
			m_diamond_pay = 8;
			break;
		case 2:
			m_renshu = 2;
			m_diamond_pay = 9;
			break;
		default:
			m_renshu = 4;
			m_diamond_pay = 6;
			break;
		}

		m_diamond_pay *= option["jushu"].asInt() ? 2 : 1;
		switch (option["fangfei"].asInt())
		{
		case 0:
			m_zhifu = 2;
			break;
		case 1:
			m_zhifu = 1;
			break;
		case 2:
			m_zhifu = 3;
			break;
		default:
			m_zhifu = 2;
			break;
		}

		parent::init_room_rules(option);
		//初始化角色数据
		for(int i=0; i<m_renshu; i++)
		{
			taojiang_player* player = new taojiang_player();
			m_members.push_back(player);
		}

		//抓鸟信息
		m_zhuaniao = option["zhuaniao"].asInt();
		if(m_zhuaniao > 2)
			m_zhuaniao = 2;
		if(m_zhuaniao < 0)
			m_zhuaniao = 0;

		m_beilv = option["beilv"].asInt();
		if(m_beilv < 1)
			m_beilv = 1;
		if(m_beilv > 100)
			m_beilv = 100;

		return true;
	}

	bool game_room::init_room_context(const Json::Value &context)
	{
		parent::init_room_context(context);
		m_ding_to_end					= context["ding_to_end"].asInt();
		m_dingwang						= context["dingwang"].asInt();

		return true;
	}

	bool game_room::init_room_members(const Json::Value &members)
	{
		parent::init_room_members(members);
		if (!members.isArray()){
			return false;
		}
		//导入成员数据
		//for (Json::ArrayIndex i = 0; i < members.size(); i++){
		//	int index = members[i]["index"].asInt();
		//}
		return true;
	}

	io::stringc game_room::export_data() const
	{
		Json::Value result;
		parent::export_data(result);

		result["context"]["ding_to_end"]		= m_ding_to_end;
		result["context"]["dingwang"]			= m_dingwang;

		return result.toFastString().c_str();
	}

	io::stringc game_room::export_score() const
	{
		Json::Value result;
		for (size_t i = 0; i < m_members.size(); i++){
			player_base* player = m_members[i];
			if (player->uuid == 0){
				continue;
			}
			Json::Value score;
			score["uuid"] = player->uuid;
			score["index"] = i;
			score["score"] = player->score;
			score["max_score"] = player->max_score;
			score["zimo_count"] = player->zimo_count;
			score["win_count"] = player->win_count;
			result.append(score);
		}
		io::stringc output(result.toFastString().c_str());
		return std::move(output);
	}

	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_update(int delta)
	{
		parent::on_update(delta);
	}

	void game_room::on_request(io_player::value_type player, protocol type, const Json::Value &data)
	{
		//userid uuid = player->get_uuid();
		parent::on_request(player, type, data);
	}

	void game_room::init_next_mahjong()
	{
		//初始化牌局参数
		m_next = 0;
		m_mahjong.clear();

		for(int i=0; i < 4; i++)
		{
			for(int j=0; j<30; j++)
			{
				if(j%10 ==0)
					continue;
				m_mahjong.push_back(j);
			}
		}

		//打乱扑克顺序(随机)
		std::random_shuffle(m_mahjong.begin(), m_mahjong.end());
	}


	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_round()
	{
		if (!is_opening()){ //如果没有开局
			set_open(); //设置房间已开局标记
			m_cur_player = -1;
		}

		//设定随机种子
		static bool is_set_rand = false;
		if(!is_set_rand)
		{
			srand((unsigned int)time(0));
			is_set_rand = true;
		}

		//清理数据
		round_clear();
		m_round++;

		m_block_json.clear();
		m_block_time = 0;

		//设定庄(第一轮设置房主为庄)
		if(m_round == 1)
		{
			for (size_t i = 0; i < m_members.size(); i++){
				if(get_creater() == m_members[i]->uuid)
				{
					m_zhuang = i;
					break;
				}
			}
		}
		else
		{
			if (m_last_winner >= 0)
				m_zhuang = m_last_winner;
		}

		//当前出牌用户设置为庄
		m_cur_player = m_zhuang;
		//初始化牌池
		init_next_mahjong();  


		//设置定王牌
		int shazi1 = 0, shazi2 = 0;
		char small_shaizi = 0;
		char total = get_rand_dice(small_shaizi);
		char big_shaizi   = total - small_shaizi;
		m_ding_to_end = (total + small_shaizi) * 2;
		if (rand() % 2){
			shazi1 = small_shaizi;
			shazi2 = big_shaizi;
		} else {
			shazi1 = big_shaizi;
			shazi2 = small_shaizi;
		}

		//发送骰子
		Json::Value hPack;
		hPack["shaizi"].append(shazi1);
		hPack["shaizi"].append(shazi2);
		send_room((protocol)taojiang_shaizi, hPack, 0);

		//设置状态为比赛中qi
		set_room_status(room_playing);

		init_user_mahjong(); //初始化用户手牌

		//设置定王牌
		int total_count = get_left_card_number() + 8; 
		m_dingwang			 = m_mahjong[m_mahjong.size() - m_ding_to_end];
		m_wang				 = m_dingwang + 1;
		if (m_wang % 10 == 0){
			m_wang -= 9;
		}

		//发送游戏开始
		for (size_t i = 0; i < m_members.size(); i++){
			m_members[i]->reset();
			if (m_members[i]->uuid == 0){
				continue; //没参与牌局
			}
			Json::Value notify;
			notify["round"]        = m_round;
			notify["round_total"]  = m_round_total;
			notify["zhuang"]	   = m_zhuang;
			notify["shaizi"].append(shazi1);
			notify["shaizi"].append(shazi2);
			notify["ding"]			= m_dingwang;
			notify["wang"]			= m_wang;
			notify["left_card"]		= total_count;
			notify["count_to_di"]   = total_count - m_ding_to_end;
			userid uuid = m_members[i]->uuid;
			send((protocol)room_begin, notify, uuid, 0);
		}

		send_user_mahjong(); //发送手牌信息

		fa_pai();
	}
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	void game_room::publish_result(int winner, bool completed)
	{
		bool save_record = m_status == room_state::room_playing;

		clear_all_rights();
		m_gangzi.clear();

		m_last_winner = winner;
		Json::Value notify;
		notify["winner"].append(winner);
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["zhuang"] = m_zhuang;
		notify["creater"] = get_creater();
		notify["room_id"] = get_number();
		notify["ding"]			= m_dingwang;
		notify["wang"]			= m_wang;
		notify["left_card"]		= 108-(m_members.size() * 13);
		notify["count_to_di"]   = 108-(m_members.size() * 13) - m_ding_to_end;

		int win_score = 0;
		for(size_t i=0; i<m_members.size();i++)
		{
			Json::Value member;
			player_base* player = m_members[i];

			if (!is_free_time()&& winner > -200){
				if (m_round == 1 && m_zhifu == 1){ //AA支付
					deduct_diamond(player->uuid, m_diamond_pay);
				}
			}

			player->time_ready = 0;
			member["uuid"] = player->uuid;
			member["zimo_count"] = player->zimo_count;
			member["win_count"] = player->win_count;
			for (size_t j = 0; j < 49; j++){
				for(int k=0; k<player->hand[j]; k++)
				{
					member["hand"].append(j);
				}


				//初始手牌,用于战绩
				for(int k=0; k<player->init_hand[j]; k++)
				{
					member["init_hand"].append(j);
				}
			}

			for (size_t j=0;j<player->group_cards.size(); j++)
			{
				Json::Value group;
				group["type"] = player->group_cards[j].type;
				for(size_t k=0; k<player->group_cards[j].list.size();k++)
				{
					group["list"].append(player->group_cards[j].list[k]);
				}

				member["groups"].append(group);
			}

			member["score"] = 0;
			member["max_score"] = player->max_score;
			member["total_score"] = player->score;

			member["score"] = 0;
			if(winner >= 0 && winner != i){
				//不是自摸胡,跳过非放炮和被抢杠的人
				if(!m_zimo_hu && m_cur_player != i){

					member["total_score"] = player->score;
					member["zhongniao"] = player->zhongniao;
					notify["members"].append(member);
					continue;
				}

				int score = m_members[winner]->hu_fan;
				if(m_members[winner]->zhongniao > 0)
					score = int(score * pow(2, m_members[winner]->zhongniao));

				if(player->zhongniao > 0)
					score = int(score * pow(2, player->zhongniao));

				std::vector<int>& hu_list = m_members[winner]->hu_type_list;

				if(!m_zimo_hu)
				{
					if(m_members.size() == 4
						&&std::find(hu_list.begin(), hu_list.end(), mj_style::hu_gangpao) == hu_list.end()
						&&std::find(hu_list.begin(), hu_list.end(), mj_style::hu_qiang) == hu_list.end())
					{
						score = (int)((double)score * 1.5);
					}
				}

				//乘以倍率
				score *= m_beilv;

				player->score -= score;
				m_members[winner]->score += score;
				win_score += score;
				member["score"] = -score;

			}

			member["total_score"] = player->score;
			member["zhongniao"] = player->zhongniao;
			notify["members"].append(member);
		}

		if (!is_free_time() && winner > -200){
			if (m_round == 1 && m_zhifu == 2){ //房主支付
				int payment = m_diamond_pay * m_members.size();
				deduct_diamond(get_creater(), payment);
			}
		}

		//修正胜利者的分数信息
		if(winner >= 0 && winner < (int)notify["members"].size())
		{
			notify["members"][winner]["score"] = win_score;
			notify["members"][winner]["total_score"] = m_members[winner]->score;
			if(m_members[winner]->max_score < win_score)
				m_members[winner]->max_score = win_score;
			notify["members"][winner]["max_score"] = m_members[winner]->max_score;

			notify["hu_card"] = m_members[winner]->hu_card;
		}

		if(completed){
			set_completed(); //设置比赛结束标记
			set_room_status(room_completed);
		}
		else{
			set_room_status(room_state::room_wait_ready);
		}

		////记录本轮比赛结果
		notify["history"] = m_history;

		io::stringc result(notify.toFastString().c_str());
		unsigned int hash = hash::chksum32(result);
		int visit_code = (hash % 900000) + 100000;

		notify["visit_code"] = visit_code;
		send_room((protocol)room_publish_result, notify, 0);

		if(save_record)
			insert_record(m_round, m_round_total, visit_code, result);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::sync_room_data(io_player::value_type player)
	{
		Json::Value notify;
		int userIndex = player->get_index();
		notify["status"] = m_status;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["cur_player"] = m_cur_player;
		notify["zhuang"] = m_zhuang;
		if(m_status == room_state::room_playing)
		{
			notify["ding"]			= m_dingwang;
			notify["wang"]			= m_wang;

			notify["left_card"]		= get_left_card_number() + 8;
			notify["count_to_di"]   = get_left_card_number() +8 - m_ding_to_end;
		}

		player_base* sync_player = NULL;
		for (size_t i = 0; i < m_members.size(); i++){
			player_base* player_1 = m_members[i];
			if (player_1->uuid == 0){
				continue; //没参与牌局
			}

			Json::Value data;
			data["index"] = i;
			data["score"] = player_1->score;
			data["ting"] = player_1->is_ting;
			if(player_1->uuid == player->get_uuid())
				sync_player = player_1;

			if(m_status == room_state::room_playing)
			{
				for (size_t j = 0; j < 49; j++){
					for(int k=0; k<player_1->hand[j]; k++)
					{
						if(i == player->get_index())
							data["hand"].append(j);
						else
							data["hand"].append(0);
					}
				}

				for(size_t j=0;j<player_1->group_cards.size(); j++)
				{
					Json::Value group;
					group["type"] = player_1->group_cards[j].type;
					std::vector<int>& list = player_1->group_cards[j].list;
					for(size_t k=0; k<list.size(); k++)
					{
						group["list"].append(list[k]);
					}
					data["groups"].append(group);
				}

				for(size_t j=0; j<player_1->out_card_list.size();j++)
				{
					data["out_card"].append(player_1->out_card_list[j]);
				}
			}

			notify["members"].append(data);
		}

		notify["last_get_card"] = sync_player->last_get_card;


		send((protocol)room_sync_data, notify, player, 0);

		//推送个人权限
		send_operation(player->get_index());
	}

	bool game_room::deal_chupai(int deal_pos, int card_id)
	{
		int cur_pos = deal_pos;
		bool someone_have_right = false;
		while(true)
		{
			cur_pos++;
			if(cur_pos >= (int)m_members.size())
				cur_pos=0;
			if(cur_pos == deal_pos)
				break;

			int next_player_index = find_next_player();

			player_base* player = m_members[cur_pos];
			prepare_aglo(player);

			//判胡
			if(!check_guo_hu(cur_pos) && m_enable_dian_pao)
			{
				if(m_algo->finish(card_id, false, player->first_operation))
					player->add_right(right_type::hu);
			}

			if(!player->is_ting)
			{
				if(next_player_index == cur_pos)
				{
					//判吃
					if(m_algo->enable(card_id, taoj_algo::type_shun))
						player->add_right(right_type::chi);
				}

				//判碰
				if(m_algo->enable(card_id, taoj_algo::type_ke))
					player->add_right(right_type::peng);
			}

			player->hu_card = card_id;
			//听牌状态才能杠
			if(player->is_ting || m_algo->is_ting())
			{
				//判杠
				if(m_algo->enable(card_id, false))
				{
					player->add_right(right_type::gang);
					player->add_can_gang(card_id);
				}
			}

			if(player->get_rights_count() > 0)
			{
				someone_have_right = true;
				send_operation(cur_pos);
			}

		}
		return someone_have_right;
	}

	void game_room::do_hu(int user_index, bool direct)
	{
		player_base* player = m_members[user_index];
		player->hu_type_list.clear();
		//胜利总数增加
		player->win_count++;
		//自摸
		if(m_cur_player == user_index)
		{
			m_zimo_hu = true;
			prepare_aglo(player);	
			if(m_gangzi.empty())
			{
				player->hand[player->last_get_card]--;
				m_algo->finish(player->hu_card, true, player->first_operation, mj_gang::gang_not);
				player->hand[player->last_get_card]++;
			}
			else{

				//杠上开花
				m_algo->finish(player->hu_card, true, player->first_operation, mj_gang::gang_hua);
				player->hand[player->hu_card]++;
			}



			player->hu_fan = m_algo->value() ;
			int style = m_algo->style();
			if(m_enable_tian_hu)
			{
				style |= mj_style::hu_daodi;
				if(style & hu_ying)
					player->hu_fan += 4;
				else
					player->hu_fan += 2;
			}
			analy_hu_style(style, player->hu_type_list);

			//自摸次数增加
			player->zimo_count++;
		}
		else{
			if(m_gangzi.empty() && m_right_wait.gang_index == -1) //没有可胡的杠子并且不是抢杠胡
			{

				prepare_aglo(player);
				m_algo->finish(player->hu_card, true, player->first_operation, mj_gang::gang_not);

				player->hu_fan = m_algo->value();
				player->hand[player->hu_card]++;
				int style = m_algo->style();

				analy_hu_style(style, player->hu_type_list);
			}
			else{ //杠上炮或抢杠胡,已被抢人为基准,来找最大番型
				int style = 0;
				//摸杠被抢,去掉杠牌,进行番型判断
				if(m_members[m_cur_player]->get_hand_card_num() % 3 == 2)
				{
					if(m_right_wait.card_id > 0 && m_right_wait.card_id < 49)
						m_members[m_cur_player]->hand[m_right_wait.card_id]--;
				}
				for(int i=0; i<39; i++)
				{
					if(i%10 ==0)
						continue;

					prepare_aglo(m_members[m_cur_player]);
					if(m_algo->finish(i, false, false, m_gangzi.empty()?mj_gang::gang_qiang:mj_gang::gang_pao))
					{
						int fan = m_algo->value();
						int style1 = m_algo->style();
						if(fan > player->hu_fan)
						{
							player->hu_fan = fan;
							style = style1;
						}
					}
				}

				analy_hu_style(style, player->hu_type_list);
				player->hand[player->hu_card]++;
				player->hu_fan *= (m_members.size()-1);
			}
		}

		m_gangzi.clear();
		Json::Value notify;

		m_niao.clear();
		//抓鸟
		if(m_zhuaniao > 0){
			for(int i=0; i<m_zhuaniao; i++){
				m_niao.push_back(m_mahjong[m_next++]);
			}
		}

		for(size_t i=0; i<m_niao.size(); i++){
			notify["niao"].append(m_niao[i]);
			int zhong_index = (user_index + m_niao[i]%10 - 1) % m_members.size();
			notify["zhong_niao"].append(zhong_index);
			m_members[zhong_index]->zhongniao++;
		}

		notify["index"].append(user_index);
		notify["fan"].append(player->hu_fan);
		notify["niao_index"] = m_cur_player;

		Json::Value hStyles;
		for(size_t i=0; i<player->hu_type_list.size(); i++)
		{
			hStyles.append(player->hu_type_list[i]);
		}
		notify["styles"].append(hStyles);
		send_room((protocol)room_hu, notify, 0);

		notify["type"] = history_type::hu_pai;
		notify["show"] = direct;
		notify["op_index"] = user_index;
		player->set_rights(notify);
		m_history.append(notify);
	}

	void game_room::do_gang(int user_index, int card_id, bool zimo, bool direct)
	{
		//杠牌后不可能在倒地胡
		m_enable_tian_hu = false;
		group_type gang_type = m_members[user_index]->do_gang(card_id, zimo);

		Json::Value notify;
		notify["index"] = user_index;
		notify["card"] = card_id;
		notify["type"] = gang_type;
		if(zimo)
			notify["from"] = m_cur_player;
		else
			notify["from"] = m_last_out_player;

		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			notify["card"] = card_id;

			send((protocol)room_gang_pai, notify, player->uuid, 0);
		}

		if(!zimo)
		{
			if(m_last_out_player > 0)
				m_members[m_last_out_player]->remove_last_out_card();
		}

		if(get_left_card_number() < 2)
		{
			publish_result(-1, m_round >= m_round_total);
			return;
		}

		m_gangzi.clear();
		m_gangzi.push_back(m_mahjong[m_next++]);
		m_gangzi.push_back(m_mahjong[m_next++]);
#ifdef _DEBUG
		//m_gangzi.clear();
		m_gangzi[0] = 2;
		//	m_gangzi.push_back(2);
#endif

		Json::Value notify1;
		notify1["index"] = user_index;
		for(size_t i=0; i<m_gangzi.size(); i++)
		{
			notify1["card"].append(m_gangzi[i]);

			m_members[user_index]->add_out_card(m_gangzi[i]);
		}

		send_room((protocol)taojiang_gang_zi, notify1, 0);

		for(size_t i=0; i<m_gangzi.size(); i++)
		{
			notify["gangzi"].append(m_gangzi[i]);
		}
		notify["card"] = card_id;
		notify["gtype"] = gang_type;
		notify["type"] = history_type::gang_pai;
		notify["show"] = direct;
		m_members[user_index]->set_rights(notify);
		m_history.append(notify);

		bool have_right = false;
		for(size_t i=0; i<m_members.size(); i++)
		{
			if(i== user_index)
				continue;

			player_base* player = m_members[i];
			//清理权限,判断能不能胡杠子
			player->clear_rights();
			prepare_aglo(player);

			int fan = 0;

			//判胡
			if(m_algo->finish(m_gangzi[0], false, false, mj_gang::gang_hua))
			{
				player->add_right(right_type::hu);
				player->hu_card = m_gangzi[0];
				fan = m_algo->value();
			}

			//判胡
			if(m_algo->finish(m_gangzi[1], false, false, mj_gang::gang_hua))
			{
				player->add_right(right_type::hu);
				if(m_algo->value() > fan)
					player->hu_card = m_gangzi[1];
			}

			if(player->get_rights_count() > 0)
			{
				send_operation(i);
				have_right = true;
			}
		}
		if(!have_right)
			turn_to_next_player();
	}

	int game_room::get_left_card_number()
	{
		return m_mahjong.size() - m_next - 8;
	}

	void game_room::check_my_turn_gang(player_base* player){

		//已经有杠权限了,取消自检
		if(player->check_right(right_type::gang))
			return;

		//检测手牌有没有4张一样的
		for(int i=0; i<49; i++)
		{
			if(player->hand[i] == 4)
			{
				player->do_gang(i, true);
				prepare_aglo(player);

				//判听
				if(m_algo->is_ting())
				{
					player->add_right(right_type::gang);
					player->add_can_gang(i);
				}

				player->remove_gang(i);
				player->hand[i]+=4;
			}
		}


		//判断有没有能成杠的刻字
		for (size_t i=0; i<player->group_cards.size(); i++)
		{
			if(player->group_cards[i].type == group_type::type_ke)
			{
				if(player->hand[player->group_cards[i].card_id] !=1)
					continue;

				player->hand[player->group_cards[i].card_id] --;

				prepare_aglo(player);

				//判听
				if(player->is_ting || m_algo->is_ting())
				{
					player->add_right(right_type::gang);
					player->add_can_gang(player->group_cards[i].card_id);
				}
				player->hand[player->group_cards[i].card_id]++;
			}
		}
	}

	void game_room::on_guo(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);

		player_base* player = m_members[user_index];
		if(player->get_rights_count() <= 0)
			ERROR_BREAK(1);

		if(player->get_right_count() == 1
			&&player->check_right(right_type::chu))
			ERROR_BREAK(2);

		send((protocol)room_guo, notify, uuid, retCode);

		notify["index"] = user_index;
		notify["type"] = history_type::guo;
		player->set_rights(notify);
		m_history.append(notify);

		//轮到出牌时点过
		if(m_cur_player == user_index && m_gangzi.empty())
		{
			player->clear_rights();

			send((protocol)room_guo, notify, uuid, retCode);

			//听牌后,不胡自动出牌
			if(player->is_ting)
			{
				player->add_right(right_type::chu);
				Json::Value hPack;
				hPack["type"] = (protocol)room_chu_pai;
				hPack["id"] = player->last_get_card;
				hPack["force"] = true;
				hPack["uuid"] = player->uuid;
				m_block_json.push_back(hPack);
			}
			else{
				player->add_right(right_type::chu);
				//重新通知出牌权限
				send_operation(user_index);
			}
		}
		else{

			if(player->check_right(right_type::hu))
				player->guo_hu = true;

			player->clear_rights();
			if(!check_right_priority(user_index, right_type::none, 0))
			{
			}
		}

		return;
__RESULT:
		send((protocol)room_guo, notify, uuid, retCode);
	}

	void game_room::on_gang_pai(userid uuid, const Json::Value &data){
		Json::Value notify;
		int retCode = 0;
		int card_id = data["card"].asInt();
		int user_index = get_index_by_uuid(uuid);

		if(!generic::valid(card_id))
			ERROR_BREAK(1);

		player_base* player = m_members[user_index];
		//没有杠牌权限
		if(!player->check_right(right_type::gang))
			ERROR_BREAK(2);

		//不在可杠列表
		if(std::find(player->gang_list.begin(), player->gang_list.end(), card_id) == player->gang_list.end())
			ERROR_BREAK(3);

		m_cur_player = user_index;
		m_members[m_cur_player]->guo_hu = false;

		//如果有出牌权限,则判断自成杠
		if(player->check_right(right_type::chu))
		{
			player->clear_rights();
			if(m_members[user_index]->hand[card_id] == 1 //手里一张才可能是摸杠
				&& check_other_hu(user_index, card_id, true))
			{
				m_right_wait.card_id = card_id;
				m_right_wait.gang_index = user_index;
				notify["drop"] = true;
				notify["index"] = user_index;
				send((protocol)room_gang_pai, notify, uuid, retCode);
			}
			else
			{
				player->is_ting = true;
				do_gang(user_index, card_id, true);
			}

		}
		else
		{
			player->clear_rights();
			check_player_hu(m_last_out_player, card_id, true);
			//检测杠牌请求
			if(!check_right_priority(user_index, right_type::gang, card_id)) //杠牌失败(有人胡)
			{
				notify["drop"] = true;
				send((protocol)room_gang_pai, notify, uuid, retCode);
				notify["show"] = true;
				notify["index"] = user_index;
				notify["type"] = history_type::gang_pai;
				player->set_rights(notify);
				m_history.append(notify);
			}

		}


		return;

__RESULT:
		send((protocol)room_gang_pai, notify, uuid, retCode);
	}

	bool game_room::check_other_hu(int except_index, int card_id, bool gang)
	{
		bool someone_have_right = false;
		for(size_t i=0; i<m_members.size(); i++)
		{
			if(i == except_index)
				continue;

			//检测上个出牌人能不能胡
			player_base* player = m_members[i];
			prepare_aglo(player);

			bool send = true;
			if(player->check_right(right_type::hu))
				send = false;
			//判胡,没有过胡,或抢杠胡
			if((!check_guo_hu(i)
				||gang)
				&& m_algo->finish(card_id, false, player->first_operation, gang?mj_gang::gang_qiang:mj_gang::gang_not))
			{
				player->add_right(right_type::hu);
				player->hu_card = card_id;
			}

			if(player->get_rights_count() > 0 && send)
			{
				someone_have_right = true;
				send_operation(i);
			}
		}

		return someone_have_right;
	}

	void game_room::analy_hu_style(int style, std::vector<int>& vt)
	{
		//enum mj_style{
		//	hu_ping     = 0x0001, //平胡*
		//	hu_peng     = 0x0002, //碰碰胡*
		//	hu_jiang    = 0x0004, //将将胡*
		//	hu_qing     = 0x0008, //清一色*
		//	hu_tian     = 0x0010, //天胡
		//	hu_tiantian = 0x0020, //天天胡
		//	hu_di       = 0x0040, //地胡
		//	hu_didi     = 0x0080, //地地胡
		//	hu_daodi    = 0x0100, //倒地胡**
		//	hu_heitian  = 0x0200, //黑天胡
		//	hu_duidui   = 0x0400, //七小对*
		//	hu_ganghua  = 0x0800, //杠上花
		//	hu_gangpao  = 0x1000, //杠上炮
		//	hu_qiang    = 0x2000, //抢杠胡
		//	hu_ying     = 0x4000  //硬庄
		//};

		if(style & hu_ping)
			vt.push_back(hu_ping);

		if(style & hu_peng)
			vt.push_back(hu_peng);

		if(style & hu_jiang)
			vt.push_back(hu_jiang);

		if(style & hu_qing)
			vt.push_back(hu_qing);

		if(style & hu_tian)
			vt.push_back(hu_tian);

		if(style & hu_tiantian)
			vt.push_back(hu_tiantian);

		if(style & hu_di)
			vt.push_back(hu_di);

		if(style & hu_didi)
			vt.push_back(hu_didi);

		if(style & hu_daodi)
			vt.push_back(hu_daodi);

		if(style & hu_heitian)
			vt.push_back(hu_heitian);

		if(style & hu_duidui)
			vt.push_back(hu_duidui);

		if(style & hu_ganghua)
			vt.push_back(hu_ganghua);

		if(style & hu_gangpao)
			vt.push_back(hu_gangpao);

		if(style & hu_qiang)
			vt.push_back(hu_qiang);

		if(style & hu_ying)
			vt.push_back(hu_ying);
	}



	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
