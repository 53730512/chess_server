

#include <algorithm>
#include "../protocol.h"
#include "../../io_handler.h"
#include "zhong_player.h"

using namespace mj_base;
#define ERROR_BREAK(code) {retCode = code; goto __RESULT;}
////////////////////////////////////////////////////////////////////////////////
namespace zhong_mj{

	const time_t delay_send_disable_op_time = 600;
	////////////////////////////////////////////////////////////////////////////////
	game_room::game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
		: room_base(creater, type, ruleid, number, time_now, owner_exit)
	{
		m_algo = new zhong_algo();
		m_enable_7dui = false;
	}

	//基类会自动调用
	void game_room::round_clear()
	{
		__super::round_clear();
	}

	bool game_room::init_room_rules(const Json::Value &option)
	{

		m_round_total = option["jushu"].asInt() ? 16 : 8;

		//switch (option["renshu"].asInt())
		//{
		//case 0:
		m_renshu = 4;
		m_diamond_pay = 6;
		//	break;
		//case 1:
		//	m_renshu = 3;
		//	m_diamond_pay = 8;
		//	break;
		//case 2:
		//	m_renshu = 2;
		//	m_diamond_pay = 9;
		//	break;
		//default:
		//	m_renshu = 4;
		//	m_diamond_pay = 6;
		//	break;
		//}

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
		m_enable_dian_pao = false;

		//初始化角色数据
		for(int i=0; i<m_renshu; i++)
		{
			m_members.push_back(new zhong_player());
		}

		//抓鸟信息
		switch(option["zhuaniao"].asInt())
		{
		case 0:
			m_zhuaniao = 0;
			break;
		case 1:
			m_zhuaniao = 2;
			break;
		case 2:
			m_zhuaniao = 3;
			break;
		case 3:
			m_zhuaniao = 4;
			break;
		default:
			m_zhuaniao = 2;
			break;
		}

		//设定癞子
		//if(option["laizi"].asBool())
		m_wang = mj_card_value::zhong;

		m_enable_7dui = option["qidui"].asBool();
		m_zhuangxian = false;//option["zhuangxian"].asBool();
		m_enable_chi = false;
		return true;
	}

	bool game_room::init_room_context(const Json::Value &context)
	{
		parent::init_room_context(context);

		m_enable_7dui = context["7dui"].asBool();
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

		result["context"]["7dui"]				= m_enable_7dui;

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
			//万字牌
			for(int j=0; j<10; j++)
			{
				if(j%10 ==0)
					continue;
				m_mahjong.push_back(j);
			}
			//条和筒
			if(m_renshu > 2)
			{
				for(int j=10; j<30; j++)
				{
					if(j%10 ==0)
						continue;
					m_mahjong.push_back(j);
				}
			}

			//二人有东南西北中发白
			if(m_renshu == 2)
			{
				for(int j=30; j<38; j++)
				{
					if(j%10 ==0)
						continue;
					m_mahjong.push_back(j);
				}
			}
			else
			{
				//3,4人 如果红中为癞子,则加进牌组
				if(m_wang > 0)
					m_mahjong.push_back(35);
			}
		}

		//打乱扑克顺序(随机)
		std::random_shuffle(m_mahjong.begin(), m_mahjong.end());

		m_cards_end_id = m_mahjong[m_mahjong.size() -1];
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
			if (m_next_zhuang >= 0)
				m_zhuang = m_next_zhuang;
		}

		//当前出牌用户设置为庄
		m_cur_player = m_zhuang;

		//初始化牌池
		init_next_mahjong(); 

		m_init_card_num = m_mahjong.size();

		//设置状态为比赛中qi
		set_room_status(room_playing);
		init_user_mahjong(); //初始化用户手牌

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
			notify["wang"]			= m_wang;
			notify["left_card"]		= get_left_card_number();
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
		m_next_zhuang = m_zhuang;
		clear_all_rights();

		Json::Value notify;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["zhuang"] = m_zhuang;
		notify["creater"] = get_creater();
		notify["room_id"] = get_number();
		notify["wang"]			= m_wang;
		notify["left_card"]		= m_init_card_num-(m_members.size() * 13);

		int  hu_card = 0;
		std::vector<int> winner_list;
		if(winner >= 0)
		{
			winner_list.push_back(winner);
			m_next_zhuang = winner;
		}
		else if(winner == -5){
			winner_list = m_right_wait.hu_list;
			if(winner_list.size() == 1)
				m_next_zhuang = winner_list[0];
			else
				m_next_zhuang = m_cur_player;
		}
		else if(winner == -1)//臭庄,则最后摸牌人坐庄
			m_next_zhuang = m_cur_player;

		if(winner == -200)
			notify["winner"].append(winner);
		else if(winner == -1)
			notify["winner"].append(-1);
		else{
			for(size_t i=0; i<winner_list.size(); i++)
			{
				notify["winner"].append(winner_list[i]);
			}
		}

		//杠分计算
		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			if(player->minggang_num > 0)
				player->round_score += player->minggang_num*(m_members.size() - 1);

			if(player->diangang_num > 0)
				player->round_score -= player->diangang_num*(m_members.size() - 1);

			if(player->bugang_num > 0 || player->angang_num > 0)
			{
				player->round_score += (m_members.size()-1)*player->bugang_num;
				player->round_score += (m_members.size()-1)*player->angang_num * 2;
				for(size_t j=0; j<m_members.size(); j++)
				{
					if(i == j)
						continue;

					m_members[j]->round_score -= player->bugang_num;
					m_members[j]->round_score -= player->angang_num*2;
				}
			}
		}

		//胡牌分数计算
		if(!winner_list.empty())
		{
			hu_card = m_members[winner_list[0]]->hu_card;
			//自摸胡,胡者加分,
			if(m_zimo_hu)
			{

				m_members[winner_list[0]]->round_score += 2*(m_members.size() -1);

				//鸟
				m_members[winner_list[0]]->round_score += m_members[winner_list[0]]->zhongniao*(m_members.size() -1) * 2;

				for(size_t i=0; i<m_members.size(); i++)
				{
					if(i== winner_list[0])
						continue;

					m_members[i]->round_score -= 2;
					m_members[i]->round_score -= m_members[winner_list[0]]->zhongniao*2;
				}

			}
			else{//抢杠胡
				for(size_t i=0; i<winner_list.size(); i++)
				{
					player_base* player = m_members[winner_list[i]];
					player->round_score += (2 + player->zhongniao * 2)*3;
					m_members[m_cur_player]->round_score -= (2 + player->zhongniao * 2)*3;
				}
			}
		}


		int win_score = 0;
		for(size_t i=0; i<m_members.size();i++)
		{
			Json::Value member;
			player_base* player = m_members[i];

			if (!is_free_time()&& winner > -2){
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

			//最高分更能
			if(player->round_score > player->max_score)
				player->max_score = player->round_score;

			player->score += player->round_score;
			member["score"] = player->round_score;
			member["max_score"] = player->max_score;
			member["total_score"] = player->score;
			member["zhongniao"] = player->zhongniao;

			member["angang"] = player->angang_num;
			member["minggang"] = player->minggang_num;
			member["bugang"] = player->bugang_num;
			member["diangang"] = player->diangang_num;

			if(!winner_list.empty() && !m_zimo_hu)
			{
				if(i == m_cur_player)
					member["fangpao"] = 1;

			}
			else
				member["fangbao"] = 0;

			notify["members"].append(member);
		}

		if (!is_free_time() && winner > -200){
			if (m_round == 1 && m_zhifu == 2){ //房主支付
				int payment = m_diamond_pay * m_members.size();
				deduct_diamond(get_creater(), payment);
			}
		}

		notify["hu_card"] = hu_card;

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
			notify["wang"]			= m_wang;

			notify["left_card"]		= get_left_card_number();
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


	void game_room::do_hu(int user_index, bool direct)
	{
		Json::Value notify;
		m_niao.clear();

		int zhong_niao = 0;
		//抓鸟
		if(m_zhuaniao > 0){
			for(int i=0; i<m_zhuaniao; i++){
				m_niao.push_back(m_mahjong[m_next++]);
				notify["niao"].append(m_niao[i]);

				int index = m_zhuang;
				if(m_niao[i]%10 == 1
					||m_niao[i]%10 == 5
					||m_niao[i]%10 == 9
					||m_niao[i] == 35)
				{

					m_members[index]->zhongniao++;
				}
				else if (m_niao[i]%10 == 2
					||m_niao[i]%10 == 6)
				{
					index += 1;
					if(index > 3)
						index -= 4;
					m_members[index]->zhongniao++;
				}
				else if (m_niao[i]%10 == 3
					||m_niao[i]%10 == 7 )
				{
					index += 2;
					if(index > 3)
						index -= 4;
					m_members[index]->zhongniao++;
				}
				else if (m_niao[i]%10 == 4
					||m_niao[i]%10 == 8 )
				{
					index += 3;
					if(index > 3)
						index -= 4;
					m_members[index]->zhongniao++;
				}

				//中鸟下标
				if(m_cur_player == user_index){
					if(index == user_index)
						notify["zhong_niao"].append(i);
					else
					{
						m_members[index]->zhongniao--;
					}
				}
				else{
					bool bhave = false;
					for(size_t j=0; j<m_right_wait.hu_list.size(); j++)
					{
						if(m_right_wait.hu_list[j] == index)
						{
							bhave = true;
							notify["zhong_niao"].append(i);
						}
					}

					if(!bhave)
						m_members[index]->zhongniao--;
				}
			}
		}

		notify["niao_index"] = m_cur_player;

		//自摸
		if(m_cur_player == user_index)
		{
			m_zimo_hu = true;
			player_base* player = m_members[user_index];
			player->hu_type_list.clear();
			player->hu_fan = 2;
			player->hu_type_list.push_back(hu_zimo);

			notify["index"].append(user_index);
			notify["fan"].append(player->hu_fan);
			Json::Value hStyle;
			hStyle.append(hu_zimo);
			notify["styles"].append(hStyle);
			notify["niao_index"] = user_index;

			//自摸次数增加
			player->zimo_count++;
			player->win_count++;
		}
		else{

			int temp_index = 0;
			for(size_t i=0; i<m_right_wait.hu_list.size(); i++)
			{
				player_base* player = m_members[m_right_wait.hu_list[i]];
				player->hu_type_list.clear();
				player->hu_fan = 1;
				player->hu_type_list.push_back(hu_ping);
				player->hand[player->hu_card]++; //把胡的牌 放入手牌

				notify["index"].append(m_right_wait.hu_list[i]);
				notify["fan"].append(player->hu_fan);

				Json::Value hStyle;
				if(m_right_wait.gang_index >= 0)
					hStyle.append(hu_qiang);
				else
					hStyle.append(hu_ping);
				notify["styles"].append(hStyle);
				player->win_count++;

				temp_index++;
			}

			if(m_right_wait.hu_list.size() == 1)
				notify["niao_index"] = m_right_wait.hu_list[0];

			//抢杠胡,把杠牌人的补杠变为碰
			if(m_right_wait.gang_index >= 0)
			{
				m_members[m_right_wait.gang_index]->bugang_to_peng(m_members[m_right_wait.hu_list[0]]->hu_card);
				m_members[m_right_wait.gang_index]->bugang_num--;
			}
		}

		send_room((protocol)room_hu, notify, 0);

		if(user_index < 0 && !m_right_wait.hu_list.empty())
			user_index = m_right_wait.hu_list[m_right_wait.hu_list.size() - 1];

		notify["type"] = history_type::hu_pai;
		notify["show"] = direct;
		notify["op_index"] = user_index;

		if(user_index >= 0)
			m_members[user_index]->set_rights(notify);
		m_history.append(notify);
	}

	int game_room::get_left_card_number()
	{
		return m_mahjong.size() - m_next - m_zhuaniao;
	}

	void game_room::analy_hu_style(int style, std::vector<int>& vt)
	{

		if(style & hu_ping)
			vt.push_back(hu_ping);

		if(style & hu_zimo)
			vt.push_back(hu_zimo);

	}

	void game_room::check_guo(player_base* player)
	{
		if(m_cur_player == player->get_index())
		{
			if(player->check_right(right_type::gang))
			{
				for(size_t i=0; i<player->group_cards.size();i++)
				{
					if(player->group_cards[i].type == group_type::type_ke && player->group_cards[i].list[0] == player->last_get_card)
					{
						player->forbidden_gang.push_back(player->last_get_card);
						break;
					}
				}
			}
		}
	}

	bool game_room::check_qishouhu(player_base* player)
	{
		return player->hand[m_wang] >= 4;
	}


	bool game_room::check_right_priority(int user_index, right_type type, int card_id)
	{
		bool direct = m_right_wait.index != user_index;
		if(type > right_type::none)
		{
			if(type > m_right_wait.right) //权限高,则直接替换
			{
				m_right_wait.index = user_index;
				m_right_wait.right = type;
				m_right_wait.card_id = card_id;

				if(type == right_type::hu)
					m_right_wait.hu_list.push_back(user_index);
			}
			else if(type == m_right_wait.right)//相同权限,比较玩家座次优先级
			{
				if(type == right_type::hu)
				{
					m_right_wait.hu_list.push_back(user_index);
					m_right_wait.index = user_index;
					m_right_wait.right = type;
					m_right_wait.card_id = card_id;
				}
				else
				{
					if(check_index_priority(user_index, m_right_wait.index))
					{
						m_right_wait.index = user_index;
						m_right_wait.right = type;
						m_right_wait.card_id = card_id;
					}	
				}
			}
		}

		//检测有没有人比当前保存的操作权限高
		for(size_t i=0; i<m_members.size(); i++)
		{
			if(i == m_right_wait.index)
				continue;

			player_base* player = m_members[i];
			for(size_t j=0; j<player->right_list.size(); j++)
			{
				if(player->right_list[j] > m_right_wait.right)
					return false;
				else if(player->right_list[j] == m_right_wait.right 
					&& m_right_wait.right != right_type::none)
				{
					//还有其他人能胡,等待
					if(m_right_wait.right == right_type::hu)
						return false;

					if(check_index_priority(i, m_right_wait.index))
						return false;
				}

			}
		}

		//清理所有人权限
		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			player->clear_rights();
		}

		switch(m_right_wait.right)
		{
		case right_type::chi:
			do_chi(m_right_wait.index, m_right_wait.card_id, m_members[m_last_out_player]->last_out_card, direct);
			break;
		case right_type::peng:
			do_peng(m_right_wait.index, m_right_wait.card_id,direct);
			break;
		case right_type::gang:
			do_gang(m_right_wait.index, m_right_wait.card_id, false, direct);
			break;
		case right_type::none:
			{
				//放弃抢杠胡
				if(m_right_wait.gang_index > 0){
					fa_pai();
				}
				else
				{
					//切换到下个玩家
					turn_to_next_player();
				}
			}
			break;
		case right_type::hu:
			{
				do_hu(-1, direct);
				publish_result(-5, m_round >= m_round_total);
			}
			break;
		}

		if(m_status != room_state::room_playing)
			return true;

		bool no_response = m_right_wait.index == user_index;
		m_right_wait.clear();
		//如果本次处理的行为,与发起者的操作一样,如发起吃,自己吃或别人吃都会返回吃的协议,从而解决客户端协议返回需求卡死问题, 
		//否则需要给发起行为返回一个空的对应协议

		return no_response;
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
		m_members[user_index]->guo_hu = false;

		//如果有出牌权限,则判断自成杠
		if(player->check_right(right_type::chu))
		{
			player->clear_rights();
			do_gang(user_index, card_id, true);
		}
		else
		{
			player->clear_rights();
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

	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
