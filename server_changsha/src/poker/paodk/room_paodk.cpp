

#include <algorithm>
#include "protocol.h"
#include "../../io_handler.h"

#define ERROR_BREAK(code) {retCode = code; goto __RESULT;}
////////////////////////////////////////////////////////////////////////////////
namespace paodk{
	////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	const time_t dismiss_time_limit = 0;
#else
	const time_t dismiss_time_limit = 180;
#endif

	const time_t delay_send_disable_op_time = 600;
	////////////////////////////////////////////////////////////////////////////////
	game_room::game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
		: room_basic(creater, type, ruleid, number, time_now, owner_exit)
	{
		m_idle_time			= 0;
		m_round				= 0;
		m_round_total		= 0;
		m_zhifu				= 0;
		m_diamond_pay		= 0;
		m_dismiss_index		= -1;
		m_status			= room_wait_ready;
		m_next				= 0;
		m_max_players		= 0;
		m_need_payment		= false;
		m_time_limit		= false;
		m_cur_player		= -1;
		m_tgLast_card.clear();
		m_waitTime			= 0;
		m_option.clear();
		memset(m_pokers, 0, sizeof(m_pokers));
		for (int i = 0; i < m_max_players; i++){
			m_members[i].reset();
		}

		m_show_left			= false;
		m_hongtao10			= false;
		m_qie_endtime		= 0;
		m_qie_index			= 0;
		m_already_qie		= false;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_rules(const Json::Value &option)
	{
		m_option = option;
		m_diamond_pay = option["fangfei"].asInt() ? 8 : 8;
		m_diamond_pay *= option["jushu"].asInt() ? 2 : 1;

		m_zhifu		  = option["fangfei"].asInt() ? 2 : 1;
		m_round_total = option["jushu"].asInt() ? 20 : 10;
		m_wanfa       = option["wanfa"].asInt() ? 15 : 16;
		m_max_players = option["renshu"].asInt() ? 2 : 3;
		m_bomb_kechai = option["zhadan"].asInt() ? false : true;
		m_four_dai    = option["sidai"].asInt() ? 3 : 0;
		m_hongtao10	  = option["hongtao"].asBool();
		m_show_left	  = option["showLeft"].asBool();
		set_pay_type(m_zhifu);
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_context(const Json::Value &context)
	{
		m_dismiss_index = -1;
		if (context["is_opening"].asBool()){ //已开局
			set_open();
		}

		m_status						= (room_state)context["status"].asInt();
		m_round							= context["round"].asInt();
		m_round_total					= context["round_total"].asInt();
		m_zhifu							= context["zhifu"].asInt();
		m_diamond_pay					= context["diamond_pay"].asInt();
		m_need_payment					= context["need_payment"].asBool();
		m_idle_time						= context["idle_time"].asInt();

		m_next							= context["next"].asInt();
		m_wanfa							= context["wanfa"].asInt();
		m_four_dai						= context["four_dai"].asInt();
		m_bomb_kechai					= context["bomb_kechai"].asBool();
		m_time_limit					= context["time_limit"].asBool();
		m_show_left						= context["show_left"].asBool();
		m_hongtao10						= context["hongtao10"].asBool();
		m_cur_player					= context["cur_player"].asInt();
		m_waitTime						= context["waitTime"].asInt();
		m_tgLast_card.last_player		= context["last_player"].asInt();
		m_qie_endtime					= context["qie_endtime"].asInt();
		m_qie_index						= context["qie_index"].asInt();
		m_already_qie					= context["already_qie"].asBool();
		m_tgLast_card.last_play_type	= (CARD_GROUP_TYPE)context["last_play_type"].asInt();

		if(context.isMember("last_play_cards"))
		{
			for(Json::ArrayIndex i=0; i<context["last_play_cards"].size();i++)
			{
				m_tgLast_card.last_play_cards.push_back(context["last_play_cards"][i].asInt());
			}
		}

		if(context.isMember("history"))
		{
			for(Json::ArrayIndex i=0; i<context["history"].size(); i++)
			{
				const Json::Value& hData = context["history"][i];
				int index = hData["index"].asInt();
				std::vector<poker::value_t> tgList;
				tgList.clear();
				if(hData.isMember("list"))
				{
					for(int j=0; j<(int)hData["list"].size(); j++)
					{
						tgList.push_back(hData["list"][j].asInt());
					}
				}

				m_history.push_back(std::make_pair(index, tgList));
			}
		}

		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_members(const Json::Value &members)
	{
		if (!members.isArray()){
			return false;
		}
		//导入成员数据
		for (Json::ArrayIndex i = 0; i < members.size(); i++){
			int index = members[i]["index"].asInt();
			room_member &member = m_members[index];
			member.uuid         = members[i]["uuid"].asUInt();
			member.nickname     = members[i]["nickname"].asString().c_str();
			member.device       = members[i]["device"].asString().c_str();
			member.voice_token  = members[i]["voice_token"].asString().c_str();
			member.sex          = members[i]["sex"].asString().c_str();
			member.head_url     = members[i]["head_url"].asString().c_str();
			member.ipaddr       = members[i]["ipaddr"].asString().c_str();
			member.score        = members[i]["score"].asInt();
			member.time_ready   = members[i]["time_ready"].asUInt();
			member.bomb			= members[i]["bomb"].asInt();
			member.bomb_score		= members[i]["bomb_score"].asInt();
			member.hongtao		= members[i]["hongtao"].asBool();
			member.total_bomb	= members[i]["total_bomb"].asInt();
			member.max_score		= members[i]["max_score"].asInt();
			member.win_count		= members[i]["win_count"].asInt();
			member.baodan			= members[i]["baodan"].asBool();
			if(members[i].isMember("hand"))
			{
				for (Json::ArrayIndex j = 0; j < members[i]["hand"].size(); j++){
					member.hand.push_back(members[i]["hand"][j].asInt());
				}
			}
			if(members[i].isMember("init_hand"))
			{
				for (Json::ArrayIndex j = 0; j < members[i]["init_hand"].size(); j++){
					member.init_hand.push_back(members[i]["init_hand"][j].asInt());
				}
			}

			//if(members[i].isMember("history"))
			//{
			//	const Json::Value& history = members[i]["history"];
			//	for(Json::ArrayIndex j=0; j<history.size(); j++)
			//	{
			//		std::vector<poker::value_t> tgList;
			//		for(Json::ArrayIndex k=0; k<history[j].size(); k++)
			//		{
			//			tgList.push_back(history[j][k].asInt());
			//		}
			//		member.playCards.push_back(tgList);
			//	}
			//}
			member.time_agree = 0;
			member.time_enter = 0;
			insert_member(member.uuid, index, member.nickname, member.sex, member.head_url);
		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	io::stringc game_room::export_data() const
	{
		Json::Value result;
		result["option"]                     = m_option;

		result["context"]["is_opening"]		= is_opening();
		result["context"]["status"]			= m_status;
		result["context"]["idle_time"]		= m_idle_time; //?

		result["context"]["round"]			= m_round;
		result["context"]["round_total"]		= m_round_total;
		result["context"]["zhifu"]			= m_zhifu;
		result["context"]["diamond_pay"]		= m_diamond_pay;
		result["context"]["need_payment"]		= m_need_payment;

		result["context"]["next"]				= m_next;
		result["context"]["wanfa"]			= m_wanfa;
		result["context"]["four_dai"]			= m_four_dai;
		result["context"]["bomb_kechai"]		= m_bomb_kechai;
		result["context"]["time_limit"]		= m_time_limit;
		result["context"]["show_left"]		= m_show_left;
		result["context"]["hongtao10"]		= m_hongtao10;				
		result["context"]["qie_endtime"]		= m_qie_endtime;
		result["context"]["qie_index"]		= m_qie_index;
		result["context"]["already_qie"]	= m_already_qie;
		result["context"]["cur_player"]		= m_cur_player;
		result["context"]["waitTime"]			= m_waitTime;

		result["context"]["last_player"]			= m_tgLast_card.last_player;
		result["context"]["last_play_type"]			= m_tgLast_card.last_play_type;
		for(size_t i=0; i<m_tgLast_card.last_play_cards.size();i++)
		{
			result["context"]["last_play_cards"].append(m_tgLast_card.last_play_cards[i]);
		}

		for(size_t i=0; i<m_history.size(); i++)
		{
			Json::Value hData;
			hData["index"] = m_history[i].first;
			const std::vector<poker::value_t>& cardList = m_history[i].second;
			for(size_t j=0; j<cardList.size(); j++)
			{
				hData["list"].append(cardList[j]);
			}

			result["context"]["history"].append(hData);
		}

		//导出成员数据
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			Json::Value member;
			member["index"]        = i;
			member["uuid"]         = m_members[i].uuid;
			member["nickname"]     = m_members[i].nickname;
			member["device"]       = m_members[i].device;
			member["voice_token"]  = m_members[i].voice_token;
			member["sex"]          = m_members[i].sex;
			member["head_url"]     = m_members[i].head_url;
			member["ipaddr"]       = m_members[i].ipaddr;
			member["score"]        = m_members[i].score;
			member["time_ready"]   = m_members[i].time_ready;
			member["bomb"]		  = m_members[i].bomb;
			member["bomb_score"]		= m_members[i].bomb_score;
			member["hongtao"]	  = m_members[i].hongtao;
			member["total_bomb"]	  = m_members[i].total_bomb;
			member["max_score"]	  = m_members[i].max_score;
			member["win_count"]	  = m_members[i].win_count;
			member["baodan"]		  = m_members[i].baodan;
			for (size_t j = 0; j < m_members[i].hand.size(); j++){
				//Json::Value card;
				//card["id"] = poker::get_point(m_members[i].hand[j]);
				//card["type"] = poker::get_type(m_members[i].hand[j]);
				member["hand"].append(m_members[i].hand[j]);
			}

			for (size_t j = 0; j < m_members[i].init_hand.size(); j++){
				//Json::Value card;
				//card["id"] = poker::get_point(m_members[i].hand[j]);
				//card["type"] = poker::get_type(m_members[i].hand[j]);
				member["init_hand"].append(m_members[i].init_hand[j]);
			}

			//for(size_t j=0; j<m_members[i].playCards.size(); j++)
			//{
			//	const std::vector<poker::value_t>& tgList = m_members[i].playCards[j];
			//	Json::Value cardList;
			//	for(size_t k=0; k<tgList.size(); k++)
			//	{
			//		cardList.append(tgList[k]);
			//	}

			//	member["history"].append(cardList);

			//}
			result["members"].append(member);
		}
		return result.toFastString().c_str();
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::import_data(const io::stringc &data)
	{
		Json::Value init;
		Json::Reader reader;
		if (!reader.parse(data, init)){
			return false;
		}
		if (init.isMember("option")){ //第一次创建房间
			if (!init_room_rules(init["option"])){
				return false;
			}
		}
		if (init.isMember("context")){
			if (!init_room_context(init["context"])){
				return false;
			}
		}
		if (init.isMember("members")){
			if (!init_room_members(init["members"])){
				return false;
			}
		}
		m_voice_url = config::get("voice_notify_url");
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	io::stringc game_room::get_options() const
	{
		io::stringc option(m_option.toFastString().c_str());

		return std::move(option);
	}
	////////////////////////////////////////////////////////////////////////////////
	io::stringc game_room::export_score() const
	{
		Json::Value result;
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			Json::Value score;
			score["uuid"] = m_members[i].uuid;
			score["index"] = i;
			score["score"] = m_members[i].score;
			score["max_score"] = m_members[i].max_score;
			score["win_count"] = m_members[i].win_count;
			score["total_bomb"] = m_members[i].total_bomb;
			result.append(score);
		}
		io::stringc output(result.toFastString().c_str());
		return std::move(output);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_create()
	{
		parent::on_create();
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_update(int delta)
	{
		parent::on_update(delta);
		//如果房间在解散中
		if (is_dismissing()){
			int index = m_dismiss_index;
			time_t dismiss_time = m_members[index].time_agree;
			time_t time_now = time(0);
			if (time_now - dismiss_time > dismiss_time_limit){
				set_dismissed();
			}
			return;
		}

		if(m_waitTime > 0)
		{
			m_waitTime -= delta;
			if(m_waitTime <= 0)
			{
				m_waitTime = 0;
				if(m_status == room_state::room_qie_pai)
				{
					//状态判断
					init_next_round();
				}
				else{
					if(m_status != room_state::room_playing)
						return;

					send_wait_play_card(false, true);
					turn_to_next_player();
					if(m_tgLast_card.last_player == m_cur_player)
					{
						if(m_tgLast_card.last_play_type == CARD_GROUP_TYPE::BOMB)
							on_play_bomb(m_cur_player);
						m_tgLast_card.clear();
						send_wait_play_card(true);
					}
					else{
						if(check_can_play_card(m_cur_player))
						{
							send_wait_play_card(false);
						}
						else
						{
							m_waitTime = delay_send_disable_op_time;
						}
					}
				}
			}

		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_enter(io_player::value_type player)
	{
		int index = -1; //默认用户索引(无效索引)
		time_t time_now = time(0);
		Json::Value notify;
		std::vector<int> empty_index;
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == player->get_uuid()){
				m_members[i].time_enter  = time_now;
				m_members[i].ipaddr      = player->get_ipaddr();
				m_members[i].device      = player->get_device();
				m_members[i].voice_token = player->get_voice_token();
				index = i;
			} else if (m_members[i].uuid == 0){
				empty_index.push_back(i);
			} else { //有其他成员存在
				Json::Value member;
				member["index"]      = i;
				member["uuid"]       = m_members[i].uuid;
				member["score"]      = m_members[i].score;
				member["nickname"]   = m_members[i].nickname;
				member["sex"]        = m_members[i].sex;
				member["head_url"]   = m_members[i].head_url;
				member["ipaddr"]     = m_members[i].ipaddr;
				member["time_enter"] = m_members[i].time_enter;
				member["time_ready"] = m_members[i].time_ready;
				notify["members"].append(member);
			}
		}
		if (index < 0){ //不是房间成员
			if (is_opening()){
				send(user_enter_room, notify, player, 406);
				return; //已开局
			}
			if (empty_index.empty()){
				send(user_enter_room, notify, player, 407);
				return; //人已满
			}
			index = rand() % (int)empty_index.size();
			//index = empty_index[index];
			index = empty_index[0];
			//第一个加入房间的人默认准备
			if (empty_index.size() == m_max_players){
				m_members[index].time_ready = time_now;
			} else {
				m_members[index].time_ready = 0;
			}

			m_members[index].time_enter  = time_now;
			m_members[index].uuid        = player->get_uuid();
			m_members[index].nickname    = player->get_nickname();
			m_members[index].sex         = player->get_sex();
			m_members[index].head_url    = player->get_head_url();
			m_members[index].ipaddr      = player->get_ipaddr();
			m_members[index].device      = player->get_device();
			m_members[index].voice_token = player->get_voice_token();
		}
		player->set_index(index);
		//把房间内其他成员的信息告诉当前用户
		notify["creater"]     = get_creater();
		notify["index"]       = index;
		notify["option"]      = m_option;
		notify["round"]       = m_round;
		notify["round_total"] = m_round_total;
		notify["room_state"]  = m_status;
		notify["number"]      = get_number();
		notify["uuid"]        = m_members[index].uuid;
		notify["score"]       = m_members[index].score;
		notify["time_enter"]  = m_members[index].time_enter;
		notify["time_ready"]  = m_members[index].time_ready;
		notify["game_type"]   = get_game_type();
		send(user_enter_room, notify, player, 0);
		//获取用户的综合分数
		m_members[index].health = player->get_health();
		//把当前用户的信息告诉房间内其他人
		notify.clear();
		notify["index"]       = index;
		notify["uuid"]        = m_members[index].uuid;
		notify["score"]       = m_members[index].score;
		notify["nickname"]    = m_members[index].nickname;
		notify["sex"]         = m_members[index].sex;
		notify["head_url"]    = m_members[index].head_url;
		notify["ipaddr"]      = m_members[index].ipaddr;
		notify["time_enter"]  = m_members[index].time_enter;
		notify["time_ready"]  = m_members[index].time_ready;
		notify["game_type"]   = get_game_type();
		//如果已开局则同步房间数据
		send_other(user_enter_room, notify, player, 0);
		if (is_opening()){
			sync_room_data(player);
		}
		//如果房间正在解散中
		if (is_dismissing()){
			room_member &member = m_members[m_dismiss_index];
			time_t time_dismiss = member.time_agree;
			time_t dead_line = time_dismiss + dismiss_time_limit;
			notify.clear();
			notify["dead_line"]             = dead_line;
			notify["applicant"]["index"]    = m_dismiss_index;
			notify["applicant"]["time"]     = time_dismiss;
			notify["applicant"]["uuid"]     = player->get_uuid();
			notify["applicant"]["nickname"] = player->get_nickname();
			for (int i = 0; i < m_max_players; i++){
				if (m_members[i].uuid == 0){
					continue;
				} else if (m_members[i].time_agree){
					notify["consenter"].append(i);
				}
			}
			send(room_dismiss_request, notify, player, 0);
		}
		//如果未开局且设置了通知地址
		if (!is_opening() && !m_voice_url.empty()){
			int member_count = 0;
			for (int i = 0; i < m_max_players; i++){
				if (m_members[i].uuid == 0)
					continue;
				member_count++;
			}
			for (int i = 0; i < m_max_players; i++){
				if (m_members[i].uuid == 0){
					continue;
				}
				bool need_notify = false;
				io_player::value_type user = io_player::find(m_members[i].uuid);
				if (user){
					need_notify = user->is_run_back();
				} else {
					need_notify = true;
				}
				if (!need_notify){
					continue;
				}
				if (m_members[i].voice_token.empty()){
					continue;
				}
				//构建通知信息
				io::stringc info;
				io::stringc nickname = player->get_nickname();
				nickname = nickname.to_ascii();
				info.format("%s 进入房间，人数(%d/%d)", nickname.c_str(), member_count, m_max_players);
				info = info.to_utf8();
				info = http::url::encode(info).c_str();
				//构建通知 URL
				io::stringc url;
				url.format(m_voice_url.c_str(), m_members[i].device.c_str(), m_members[i].voice_token.c_str(), info.c_str());
				http::instance()->get_url_async(1, url);
#ifndef POKER_PUBLISH
				PRINT("%s\r\n", url.c_str());
#endif
			}
		}
		parent::on_enter(player);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_leave(io_player::value_type player, bool is_exit)
	{
		Json::Value notify;
		if (is_exit){
			//已开局任何人不能退出房间
			if (is_opening()){
				send(user_leave_room, notify, player, 504);
				return;
			}
			//如果不允许创建者离开房间
			if (!is_owner_exit()){
				if (get_creater() == player->get_uuid()){
					send(user_leave_room, notify, player, 505);
					return;
				}
			}
		}
		int index = -1;
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == player->get_uuid()){
				index = i;
				if (is_exit){
					m_members[i].reset();
				} else {
					m_members[i].time_enter = 0;
				}
				break;
			}
		}
		notify["index"] = index;
		notify["uuid"]  = player->get_uuid();
		notify["exit"]  = is_exit;
		send_room(user_leave_room, notify, 0); //广播给所有人用户离开房间
		parent::on_leave(player, is_exit);
	}
	void game_room::on_request(io_player::value_type player, protocol type, const Json::Value &data)
	{
		userid uuid = player->get_uuid();
		switch (type){
		case room_dismiss_request:
			on_dismiss(uuid, data);
			break;
		case room_dismiss_response:
			on_dismiss_reply(uuid, data);
			break;
		case paodk_ready:
			on_ready(uuid, data);
			break;
		case paodk_round_begin:
			on_round_begin(uuid, data);
			break;
		case paodk_room_broadcast:
			on_room_broadcast(uuid, data);
			break;
		case paodk_play_card:
			on_play_card(uuid, data);
			break;
		case paodk_qie_pai:
			on_qie_pai(uuid,data);
			break;
		}
		parent::on_request(player, type, data);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_destroy(bool completed)
	{
		//统计用户的总成绩(多个游戏)
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			userid uuid = m_members[i].uuid;
			io_player::value_type player = io_player::find(uuid);
			if (player){
				player->set_score(m_members[i].score);
			}
		}
		//如果需要支付且是大赢家支付
		if (m_need_payment && m_zhifu == 3){ //大赢家支付
			std::vector<int> big_winner;
			int max_score = 0;
			for (int i = 0; i < m_max_players; i++){
				if (m_members[i].uuid == 0){
					continue;
				}
				if (m_members[i].score > max_score){
					big_winner.clear();
					big_winner.push_back(i);
					max_score = m_members[i].score;
				} else if (m_members[i].score == max_score){
					big_winner.push_back(i);
				}
			}
			for (size_t i = 0; i < big_winner.size(); i++){
				userid uuid = m_members[big_winner[i]].uuid;
				deduct_diamond(uuid, m_diamond_pay);
			}
		}
		parent::on_destroy(completed);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_dismiss(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (is_dismissing() || is_dismissed()){
			send(room_dismiss_request, notify, uuid, 601);
			return;
		}
		if (!is_opening()){
			if (get_creater() == uuid){
				set_dismissed();
			} else {
				//未开局时非房主不能解散房间
				send(room_dismiss_request, notify, uuid, 602);
			}
			return;
		}
		time_t time_now = time(0);
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == uuid){
				set_dismiss();
				m_dismiss_index = i;
				m_members[i].time_agree = time_now;
				break;
			}
		}
		time_t dead_line = time_now + dismiss_time_limit;
		notify["dead_line"]             = dead_line;
		notify["applicant"]["index"]    = m_dismiss_index;
		notify["applicant"]["time"]     = time_now;
		notify["applicant"]["uuid"]     = m_members[m_dismiss_index].uuid;
		notify["applicant"]["nickname"] = m_members[m_dismiss_index].nickname;
		notify["consenter"].append(m_dismiss_index);
		send_room(room_dismiss_request, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_dismiss_reply(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		bool agree = data["agree"].asBool();
		if (!is_dismissing()){
			send(room_dismiss_request, notify, uuid, 701);
			return;
		}
		if (agree == false){ //拒绝解散
			set_continue();
			if (m_dismiss_index >= 0){
				m_members[m_dismiss_index].time_agree = 0;
				m_dismiss_index = -1;
			}
		} else { //同意解散
			bool is_all_agree = true;
			for (int i = 0; i < m_max_players; i++){
				if (m_members[i].uuid == 0){
					continue;
				} else if (m_members[i].uuid == uuid){
					notify["consenter"].append(i);
					m_members[i].time_agree = time(0);
				} else if (m_members[i].time_agree == 0){
					is_all_agree = false;
				} else {
					notify["consenter"].append(i);
				}
			}
			if (is_all_agree){
				set_dismissed(); //经所有人同意解散房间
			}
		}
		notify["responder"] = uuid;
		notify["agree"]     = agree;
		notify["dismissed"] = is_dismissed();
		send_room(room_dismiss_response, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_ready(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status > room_wait_begin){
			send((protocol)paodk_ready, notify, uuid, 1);
			return;
		}
		time_t time_now   = time(0);
		int  index_ready  = -1;
		int  count_ready  = 0;
		bool is_all_ready = true;
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == 0){
				continue;
			} else if (m_members[i].uuid == uuid){
				index_ready = i;
				if (m_members[i].time_ready == 0)
					count_ready++;
				m_members[i].time_ready = time_now;
			} else if (m_members[i].time_ready){
				count_ready++;
			} else {
				is_all_ready = false;
			}
		}

		notify.clear();
		notify["index"]      = index_ready;
		notify["uuid"]       = uuid;
		notify["time_ready"] = m_members[index_ready].time_ready;
		notify["game_type"]   = get_game_type();
		send_room((protocol)paodk_ready, notify, 0);
		//如果所有人都准备且大于1人
		if (is_all_ready && count_ready > 1){
			if (m_round > 0){
				//init_next_round(); //开始下一局
				wait_qie_pai();
			} else {
				int index_first_ready = 0;
				time_t min_time_ready = 0;
				for (int i = 0; i < m_max_players; i++){
					if (m_members[i].time_ready){
						if (min_time_ready == 0){
							index_first_ready = i;
							min_time_ready = m_members[i].time_ready;
						} else if (m_members[i].time_ready < min_time_ready){
							index_first_ready = i;
							min_time_ready = m_members[i].time_ready;
						}
					}
				}
				notify.clear();
				set_room_status(room_wait_begin);
				//userid uuid = m_members[index_first_ready].uuid;
				//send((protocol)paodk_begin, notify, uuid, 0);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_round_begin(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_begin){
			send((protocol)paodk_round_begin, notify, uuid, 1);
			return;
		}
		int  count_ready = 0;
		bool is_all_ready = true;
		for (int i = 0; i < m_max_players; i++){
			if (m_members[i].uuid == 0){
				continue;
			} else if (m_members[i].time_ready){
				count_ready++;
			} else {
				is_all_ready = false;
			}
		}
		if (!is_all_ready){ //不是所有人都准备好了
			send((protocol)paodk_round_begin, notify, uuid, 20);
			return;
		}
		if (count_ready < 2){ //至少要有2个人准备好了才能开局
			send((protocol)paodk_round_begin, notify, uuid, 30);
			return;
		}
		//init_next_round(); //初始化下一局(在这里是第一局)
		wait_qie_pai();
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_room_broadcast(userid uuid, const Json::Value &data)
	{
		send_room((protocol)paodk_room_broadcast, data, 0);
	}

	void game_room::on_play_card(userid uuid, const Json::Value& data)
	{
		//状态判断
		Json::Value notify;
		int retCode = 0;
		std::vector<poker::value_t> card_list;
		if(m_status != room_playing)
			ERROR_BREAK(1);
		int userIndex = get_index_by_uuid(uuid);
		if(userIndex != m_cur_player)
			ERROR_BREAK(2);

		if(!data.isMember("cards") || data["cards"].size() == 0)
			ERROR_BREAK(3);

		CARD_GROUP_TYPE card_type = (CARD_GROUP_TYPE)data["type"].asInt();
		const Json::Value& cards = data["cards"];


		for(Json::ArrayIndex i=0; i<cards.size();i++)
		{
			poker::type type = (poker::type)cards[i][0].asInt();
			poker::point id = (poker::point)cards[i][1].asInt();
			card_list.push_back(poker::set_value(id, type));
		}

		//从大到小排序
		std::sort(card_list.begin(),card_list.end(),[](poker::value_t& p1, poker::value_t& p2){
			return get_value(p1) > get_value(p2); 
		});
		if(!check_play_card(userIndex, card_type, card_list))
			ERROR_BREAK(4);

		//保存出牌历史 
		//m_members[userIndex].playCards.push_back(card_list);
		m_history.push_back(std::make_pair(userIndex, card_list));
		if(card_type == CARD_GROUP_TYPE::BOMB)
		{
			m_members[userIndex].bomb++;
			m_members[userIndex].total_bomb++;
			//on_play_bomb(userIndex);
		}

		remove_cards(userIndex, card_list);
		bool bfirst = m_tgLast_card.last_play_type == CARD_GROUP_TYPE::NONE;
		//保存上次出牌的人和牌
		m_tgLast_card.last_player = userIndex;
		m_tgLast_card.last_play_type = card_type;
		m_tgLast_card.last_play_cards = card_list;

		m_members[userIndex].baodan = m_members[userIndex].hand.size() == 1;
		notify["type"] = card_type;
		notify["cards"] = data["cards"];
		notify["index"] = userIndex;
		notify["first"] = bfirst;
		notify["baodan"] = m_members[userIndex].baodan;
		send_room((protocol)paodk_protocol::paodk_play_card, notify, 0);
		if(m_members[userIndex].hand.empty())
		{
			if(card_type == CARD_GROUP_TYPE::BOMB)
				on_play_bomb(userIndex);


			if(m_round < m_round_total)
				publish_result(userIndex, false);
			else
				publish_result(userIndex, true);
		}
		else
		{
			turn_to_next_player();

			if(check_can_play_card(m_cur_player))
				send_wait_play_card(false);//发送轮到下个人出牌了
			else{
				m_waitTime = delay_send_disable_op_time;
			}
		}

		return;
__RESULT:
		send((protocol)paodk_play_card, notify, uuid, retCode);
	}

	void game_room::on_qie_pai(userid uuid, const Json::Value& data)
	{
		//状态判断
		Json::Value notify;
		int retCode = 0;
		if(m_status != room_qie_pai)
			ERROR_BREAK(1);
		int userIndex = get_index_by_uuid(uuid);

		if(m_already_qie)
			ERROR_BREAK(4);

		if(get_last_player() != userIndex)
			ERROR_BREAK(2);

		int qie_index = data["index"].asInt();

		if(qie_index < 0
			||qie_index>=m_wanfa*3)
			ERROR_BREAK(3);

		m_already_qie = true;
		notify["index"] = qie_index;
		send_room((protocol)paodk_protocol::paodk_qie_pai, notify, 0);
		m_waitTime = 1700;
		return;

__RESULT:
		send((protocol)paodk_play_card, notify, uuid, retCode);
	}

	void game_room::wait_qie_pai()
	{
		if (!is_opening()){ //如果没有开局
			set_open(); //设置房间已开局标记
			m_cur_player = -1;
		}

		m_history.clear();
		m_already_qie = false;
		int qie_player = get_last_player();
		m_qie_index = m_wanfa*3-1;
		m_qie_endtime = (int)time(0)+6;
		Json::Value notify;
		notify["qie_endtime"] = m_qie_endtime;
		notify["qie_player"] = qie_player;
		notify["qie_index"] = m_qie_index;
		send_room((protocol)paodk_wait_qie_pai, notify, 0);

		m_waitTime = 8600;
		set_room_status(room_state::room_qie_pai);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_round()
	{
		if (!is_opening()){ //如果没有开局
			set_open(); //设置房间已开局标记
			m_cur_player = -1;
		}

		static bool is_set_rand = false;
		if(!is_set_rand)
		{
			srand((unsigned int)time(0));
			is_set_rand = true;
		}

		m_round++;
		m_tgLast_card.clear();
		m_qie_endtime = 0;

		//srand(1234567895);

		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].init();
			if (m_members[i].uuid == 0){
				continue; //没参与牌局
			}
			Json::Value notify;
			notify["round"]        = m_round;
			notify["round_total"]  = m_round_total;
			userid uuid = m_members[i].uuid;
			send((protocol)paodk_round_begin, notify, uuid, 0);
		}
		init_next_poker();  //初始化牌池
		init_user_poker();  //初始化用户手牌

		int nPlayerNum = get_player_number();
		//设定第一个出牌的用户
		if(m_round == 1)
		{
			if(nPlayerNum == 2)//房主先出
			{
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue; //没参与牌局
					}

					if(get_creater() == m_members[i].uuid)
					{
						m_cur_player = i;
						break;
					}
				}
			}
			else//黑桃三先出
			{
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue; //没参与牌局
					}

					std::vector<poker::value_t>& hand = m_members[i].hand;
					for(size_t j=0; j<hand.size(); j++)
					{
						int type = poker::get_type(hand[j]);
						int point = poker::get_point(hand[j]);
						if(type == poker::type::s_spade
							&& point == 3)
						{
							m_cur_player = i;
							break;
						}
					}

					if(m_cur_player >= 0)
						break;
				}
			}
		}

		send_user_poker();  //给用户发送手牌信息

		//发送等待出牌协议
		send_wait_play_card(true);

		//设置状态为比赛中qi
		set_room_status(room_playing);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::send_user_poker()
	{
		int qie_player = get_last_player();
		m_qie_index = m_wanfa*3-1;//rand()%m_wanfa*3;
		for (int i = 0; i < MAX_MEMBERS; i++){

			room_member& member = m_members[i];
			if (member.uuid == 0){
				continue; //没参与牌局
			}

			Json::Value notify;
			for (size_t j = 0; j < member.hand.size(); j++){
				poker::value_t v = member.hand[j];
				Json::Value card;
				card["id"]   = poker::get_point(v);
				card["type"] = poker::get_type(v);
				notify["hand"].append(card);
			}

			userid uuid = m_members[i].uuid;
			notify["cur_player"] = m_cur_player;
			send((protocol)paodk_pokers, notify, uuid, 0);
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_poker()
	{
		//初始化牌局参数
		m_next = 0;
		memset(m_pokers, 0, sizeof(m_pokers));
		int point_A_removed = 0;
		int point_2_removed = 0;
		int point_K_removed = 0;
		//初始化牌池数据
		std::vector<poker::value_t> temp_pokers;
		int max_poker_point = poker::point_J_black;
		for (int j = poker::d_diamond; j < poker::k_joker; j++){
			for (int i = poker::point_A; i < max_poker_point; i++){
				if (i == poker::point_K){
					if (m_wanfa == 15 && point_K_removed < 1){
						point_K_removed++;
						continue;
					}
				}
				if (i == poker::point_A){
					if (m_wanfa == 16){
						if (point_A_removed < 1){
							point_A_removed++;
							continue;
						}
					} else {
						if (point_A_removed < 3){
							point_A_removed++;
							continue;
						}
					}
				}
				if (i == poker::point_2 && point_2_removed < 3){
					point_2_removed++;
					continue;
				}

				poker::value_t v = poker::set_value((poker::point)i, (poker::type)j);
				temp_pokers.push_back(v);
			}
		}
		//打乱扑克顺序(随机)
		std::random_shuffle(temp_pokers.begin(), temp_pokers.end());
		for (size_t i = 0; i < temp_pokers.size(); i++){
			m_pokers[i] = temp_pokers[i];
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_user_poker()
	{
		for(int j=0; j<m_wanfa; j++)
		{
			for(int i=0; i<MAX_MEMBERS; i++)
			{
				room_member& member = m_members[i];

				member.hand.push_back(m_pokers[m_next]);
				m_next++;
			}
		}

		for(int i=0; i<MAX_MEMBERS; i++)
		{
			room_member& member = m_members[i];
			std::sort(member.hand.begin(), member.hand.end(), [](poker::value_t& p1, poker::value_t& p2){
				int value1 = get_value(p1);
				int value2 = get_value(p2);
				if(value1 == value2)
					return poker::get_type(p1) > poker::get_type(p2);
				else
					return value1 > value2;
			});

			member.init_hand = member.hand;
		}
#ifdef _DEBUG
		//m_members[0].hand.clear();
		//m_members[1].hand.clear();

		//m_members[0].hand.push_back(poker::set_value(poker::point_3, poker::h_heart));
		//m_members[0].hand.push_back(poker::set_value(poker::point_3, poker::c_club));
		//m_members[0].hand.push_back(poker::set_value(poker::point_3, poker::d_diamond));
		//m_members[0].hand.push_back(poker::set_value(poker::point_3, poker::s_spade));

		//m_members[0].hand.push_back(poker::set_value(poker::point_Q, poker::h_heart));
		//m_members[0].hand.push_back(poker::set_value(poker::point_Q, poker::c_club));
		//m_members[0].hand.push_back(poker::set_value(poker::point_K, poker::d_diamond));
		//m_members[0].hand.push_back(poker::set_value(poker::point_K, poker::s_spade));

		//m_members[0].hand.push_back(poker::set_value(poker::point_6, poker::h_heart));
		////m_members[0].hand.push_back(poker::set_value(poker::point_6, poker::c_club));
		////m_members[0].hand.push_back(poker::set_value(poker::point_6, poker::d_diamond));
		////m_members[0].hand.push_back(poker::set_value(poker::point_7, poker::d_diamond));
		////m_members[0].hand.push_back(poker::set_value(poker::point_7, poker::c_club));
		////m_members[0].hand.push_back(poker::set_value(poker::point_8, poker::d_diamond));
		////m_members[0].hand.push_back(poker::set_value(poker::point_8, poker::c_club));
		////m_members[0].hand.push_back(poker::set_value(poker::point_9, poker::c_club));
		//m_members[0].hand.push_back(poker::set_value(poker::point_10, poker::c_club));



		//m_members[1].hand.push_back(poker::set_value(poker::point_K, poker::h_heart));
		//m_members[1].hand.push_back(poker::set_value(poker::point_K, poker::c_club));
		//m_members[1].hand.push_back(poker::set_value(poker::point_A, poker::d_diamond));
		//m_members[1].hand.push_back(poker::set_value(poker::point_A, poker::s_spade));

		//m_members[1].hand.push_back(poker::set_value(poker::point_4, poker::h_heart));
		//m_members[1].hand.push_back(poker::set_value(poker::point_4, poker::c_club));
		//m_members[1].hand.push_back(poker::set_value(poker::point_4, poker::d_diamond));
		//m_members[1].hand.push_back(poker::set_value(poker::point_4, poker::s_spade));
		//m_members[1].hand.push_back(poker::set_value(poker::point_7, poker::d_diamond));
		////m_members[1].hand.push_back(poker::set_value(poker::point_7, poker::c_club));
		////m_members[1].hand.push_back(poker::set_value(poker::point_7, poker::s_spade));
		////m_members[1].hand.push_back(poker::set_value(poker::point_8, poker::d_diamond));
		////m_members[1].hand.push_back(poker::set_value(poker::point_8, poker::c_club));
		////m_members[1].hand.push_back(poker::set_value(poker::point_9, poker::c_club));
		////m_members[1].hand.push_back(poker::set_value(poker::point_9, poker::d_diamond));
		////m_members[1].hand.push_back(poker::set_value(poker::point_6, poker::c_club));
		//m_members[1].hand.push_back(poker::set_value(poker::point_10, poker::h_heart));
		////m_members[1].hand.push_back(poker::set_value(poker::point_10, poker::c_club));
		////m_members[1].hand.push_back(poker::set_value(poker::point_7, poker::c_club));

		//for(int i=0; i<MAX_MEMBERS; i++)
		//{
		//	room_member& member = m_members[i];
		//	std::sort(member.hand.begin(), member.hand.end(), [](poker::value_t& p1, poker::value_t& p2){
		//		int value1 = get_value(p1);
		//		int value2 = get_value(p2);
		//		if(value1 == value2)
		//			return poker::get_type(p1) > poker::get_type(p2);
		//		else
		//			return value1 > value2;
		//	});
		//}
#endif // _DEBUG

		if(m_hongtao10)
		{
			for(int i=0; i<MAX_MEMBERS;i++)
			{
				room_member& member = m_members[i];
				for(size_t j=0;j<member.hand.size();j++)
				{
					poker::type type = poker::get_type(member.hand[j]);
					poker::point id = poker::get_point(member.hand[j]);
					if(type == poker::h_heart && id == poker::point_10)
					{
						member.hongtao = true;
						break;
					}
				}
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::set_room_status(room_state status)
	{
		set_dirty();
		m_idle_time = 0;
		m_status    = status;
		//广播状态改变消息
		Json::Value notify;
		notify["room_state"] = status;
		send_room((protocol)paodk_status_change, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::publish_result(int winner, bool completed)
	{
		Json::Value notify;
		notify["winner"] = winner;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;

		room_member& winner_data = m_members[winner];
		m_members[winner].win_count++;

		int  member_count = 0;
		int total_cut_score = 0;
		for(int i=0; i<MAX_MEMBERS; i++)
		{
			room_member& player_data = m_members[i];
			int card_score = 0;
			if(player_data.hand.size()>1)
			{
				card_score = player_data.hand.size()*((player_data.hongtao||winner_data.hongtao)?2:1);
			}

			if(player_data.hand.size() == m_wanfa)
				card_score *= 2;
			if(player_data.uuid != 0)
			{
				total_cut_score += card_score;
				member_count++;
				if (!is_free_time()){
					if (m_round == 1 && m_zhifu == 1){ //AA支付
						deduct_diamond(player_data.uuid, m_diamond_pay);
					}
				}
			}

			Json::Value hMember;
			hMember["index"] = i;
			hMember["uuid"] = player_data.uuid;
			hMember["score"] = 0-card_score + player_data.bomb_score;
			for(size_t j=0;j<player_data.hand.size();j++)
			{
				poker::type type = poker::get_type(player_data.hand[j]);
				poker::point id = poker::get_point(player_data.hand[j]);
				Json::Value hCard;
				hCard["type"] = type;
				hCard["id"] = id;
				hMember["hand"].append(hCard);
			}

			for(size_t j=0;j<player_data.init_hand.size();j++)
			{
				poker::type type = poker::get_type(player_data.init_hand[j]);
				poker::point id = poker::get_point(player_data.init_hand[j]);
				Json::Value hCard;
				hCard["type"] = type;
				hCard["id"] = id;
				hMember["init_hand"].append(hCard);
			}

			player_data.score -= card_score;
			player_data.time_ready = 0;//清空准备时间
			if(0-card_score + player_data.bomb_score > player_data.max_score)
				player_data.max_score = 0-card_score + player_data.bomb_score;


			hMember["total_score"] = player_data.score;
			hMember["bomb"] = player_data.bomb;
			hMember["hongtao"] = player_data.hongtao;
			hMember["total_bomb"] = player_data.total_bomb;
			hMember["max_score"] = player_data.max_score;
			hMember["win_count"] = player_data.win_count;
			hMember["chuntian"] = player_data.hand.size() == m_wanfa;
			notify["members"].append(hMember);
		}
		m_members[winner].score += total_cut_score;

		if (!is_free_time()){
			if (m_round == 1 && m_zhifu == 2){ //房主支付
				int payment = m_diamond_pay * member_count;
				deduct_diamond(get_creater(), payment);
			}
		}

		if(total_cut_score + m_members[winner].bomb_score > m_members[winner].max_score)
			m_members[winner].max_score = total_cut_score + m_members[winner].bomb_score;


		notify["members"][winner]["score"] = total_cut_score + m_members[winner].bomb_score;
		notify["members"][winner]["total_score"] = m_members[winner].score;
		notify["members"][winner]["max_score"] = m_members[winner].max_score;
		send_room((protocol)paodk_protocol::paodk_publish_result, notify, 0);

		if(completed){
			set_completed(); //设置比赛结束标记
			set_room_status(room_completed);
		}
		else{
			set_room_status(room_state::room_wait_ready);
		}

		////准备记录比赛结果
		//for(int i=0; i<MAX_MEMBERS; i++)
		//{
		//	room_member& player_data = m_members[i];
		//	if (player_data.uuid != 0)
		//	{
		//		if(!player_data.playCards.empty())
		//		{
		//			for(size_t j=0; j<player_data.playCards.size();j++)
		//			{
		//				Json::Value list;
		//				for(size_t k=0; k<player_data.playCards[j].size(); k++)
		//				{
		//					list.append(player_data.playCards[j][k]);
		//				}

		//				notify["members"][i]["history"].append(list);
		//			}

		//		}
		//	}
		//}

		for(size_t i=0; i<m_history.size(); i++)
		{
			Json::Value hData;
			hData["index"] = m_history[i].first;
			const std::vector<poker::value_t>& cardList = m_history[i].second;
			for(size_t j=0; j<cardList.size(); j++)
			{
				hData["list"].append(cardList[j]);
			}

			notify["history"].append(hData);
		}

		//记录本轮比赛结果
		io::stringc result(notify.toFastString().c_str());
		unsigned int hash = hash::chksum32(result);
		int visit_code = (hash % 900000) + 100000;
		insert_record(m_round, m_round_total, visit_code, result);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::sync_room_data(io_player::value_type player)
	{
		Json::Value notify;
		int userIndex = get_index_by_uuid(player->get_uuid());
		int qie_player = get_last_player();
		notify["status"] = m_status;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["cur_player"] = m_cur_player;
		notify["last_player"] = m_tgLast_card.last_player;
		notify["last_play_type"] = m_tgLast_card.last_play_type;
		notify["qie_endtime"] = m_qie_endtime;
		notify["qie_index"] = m_qie_index;
		notify["qie_player"] = qie_player;
		notify["already_qie"] = m_already_qie;
		for(size_t i=0;i<m_tgLast_card.last_play_cards.size();i++)
		{
			Json::Value hCard;
			hCard["type"] = poker::get_type(m_tgLast_card.last_play_cards[i]);
			hCard["id"] = poker::get_point(m_tgLast_card.last_play_cards[i]);
			notify["last_cards"].append(hCard);
		}

		for(size_t i=0; i<MAX_MEMBERS; i++)
		{
			room_member& member = m_members[i];
			Json::Value hData;
			Json::Value hCard;

			if(userIndex == i && m_status == room_playing)
			{
				for(size_t j=0; j<m_members[userIndex].hand.size(); j++)
				{
					hCard["type"] = poker::get_type(m_members[userIndex].hand[j]);
					hCard["id"] = poker::get_point(m_members[userIndex].hand[j]);
					hData["hand"].append(hCard);
				}
			}

			hData["score"] = member.score;
			hData["win_count"] = member.win_count;
			hData["max_score"] = member.max_score;
			hData["total_bomb"] = member.total_bomb;
			hData["left_card"] = member.hand.size();
			hData["baodan"] = member.baodan;
			notify["members"].append(hData);
		}

		send((protocol)paodk_protocol::paodk_sync_data, notify, player, 0);

		if(m_waitTime == 0 && m_status == room_playing)
		{
			Json::Value notify;
			notify["first"] = m_tgLast_card.last_player<0;
			notify["index"]	= m_cur_player;
			notify["disable"] = false;
			send((protocol)paodk_wait_play, notify, player, 0);

			//send_wait_play_card(m_tgLast_card.last_player<0, false);
		}
	}

	void game_room::turn_to_next_player()
	{
		while(true)
		{
			m_cur_player++;
			if(m_cur_player >= MAX_MEMBERS)
				m_cur_player = 0;
			if(m_members[m_cur_player].uuid !=0)
				break;
		}
	}

	int game_room::get_next_player()
	{
		int next = m_cur_player;
		while(true)
		{
			next++;
			if(next >= MAX_MEMBERS)
				next = 0;
			if(m_members[next].uuid !=0)
				break;
		}

		return next;
	}

	int game_room::get_last_player(){
		int last = m_cur_player;
		while(true)
		{
			last--;
			if(last < 0)
				last = MAX_MEMBERS-1;
			if(m_members[last].uuid !=0)
				break;
		}

		return last;
	}

	//广播该某人出牌了，first_play为true，代表新一轮出牌，客户端清理桌面牌
	void game_room::send_wait_play_card(bool first_play /* = false */, bool disable_play)
	{
		room_member& player = m_members[m_cur_player];

		Json::Value notify;
		notify["first"] = first_play;
		notify["index"]	= m_cur_player;
		notify["disable"] = disable_play;
		send_room((protocol)paodk_wait_play, notify, 0);

		//记录要不起操作
		if(disable_play)
		{
			std::vector<poker::value_t> tgList;
			tgList.clear();
			m_history.push_back(std::make_pair(m_cur_player,tgList));
		}
	}

	int game_room::get_index_by_uuid(userid uuid)
	{
		for(int i=0;i<MAX_MEMBERS;i++)
		{
			if(m_members[i].uuid == uuid)
				return i;
		}

		return -1;
	}

	//移除手牌
	void game_room::remove_cards(int index, const std::vector<poker::value_t>& list)
	{
		std::vector<poker::value_t>& userHand = m_members[index].hand;
		for(auto it = userHand.begin(); it != userHand.end();)
		{
			bool remove = false;
			for(size_t j=0;j<list.size();j++)
			{
				if(*it == list[j])
				{
					remove = true;
					break;
				}
			}

			if(remove)
				it = userHand.erase(it);
			else
				it++;
		}
	}

	bool game_room::check_play_card(int userIndex, CARD_GROUP_TYPE type, std::vector<poker::value_t>& list)
	{
		if(type <= CARD_GROUP_TYPE::NONE || type > CARD_GROUP_TYPE::BOMB)
			return false;

		auto unique_pos = std::unique(list.begin(), list.end());
		if(unique_pos != list.end()) //有重复的牌
			return false;

		std::vector<poker::value_t>&hand = m_members[userIndex].hand;
		//判断每张牌是否存在
		bool all_card_valid = true;
		for (size_t i=0; i<list.size(); i++)
		{
			bool bfind = false;
			for(size_t j=0; j<hand.size(); j++)
			{
				if(hand[j] == list[i])
				{
					bfind = true;
					break;
				}
			}
			if(!bfind)
			{

				all_card_valid = false;
				break;
			}
		}

		if(!all_card_valid)
			return false;

		if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE)
		{
			if(m_tgLast_card.last_play_type != type && type != CARD_GROUP_TYPE::BOMB)
				return false;
		}

		if(type != CARD_GROUP_TYPE::BOMB && !m_bomb_kechai)
		{
			if(have_card_of_bomb(hand, list))
				return false;
		}

		switch(type)
		{
		case CARD_GROUP_TYPE::SINGLE:
			{
				if(list.size() != 1)
					return false;

				//下家报单,只能出最大的单牌
				int next = get_next_player();
				if(m_members[next].baodan)
				{
					poker::value_t max_single_card = get_max_card(hand);
					if(get_value(list[0]) != get_value(max_single_card))
						return false;
				}

				if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE){
					return get_value(list[0]) > get_value(m_tgLast_card.last_play_cards[0]);
				}

				return true;
			}
			break;
		case CARD_GROUP_TYPE::PAIRS:
			{
				if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE)
				{
					if(list.size() != m_tgLast_card.last_play_cards.size())
						return false;
				}
				else
				{
					if(list.size() < 0 || list.size()%2 !=0)
						return false;
				}


				poker::value_t vt = 0;
				if(list.size() == 2)
					vt = is_double(list);
				else
					vt = is_order_double(list);

				if(vt == 0)
					return false;

				if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE){
					int v1 = get_value(vt);
					int v2 = get_value(m_tgLast_card.last_play_cards[0]);
					return v1 > v2;
					//return get_value(vt) > get_value(m_tgLast_card.last_play_cards[0]);
				}
				else
					return true;
			}
			break;
		case CARD_GROUP_TYPE::PROGRESSION:
			{
				if(list.size() != m_tgLast_card.last_play_cards.size() && m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE)
					return false;
				poker::value_t vt = is_order(list);

				if(vt == 0)
					return false;

				if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::NONE){
					return get_value(vt) > get_value(m_tgLast_card.last_play_cards[0]);
				}
				else
					return true;
			}
			break;
		case CARD_GROUP_TYPE::THREE:
			{
				if(m_tgLast_card.last_play_type == CARD_GROUP_TYPE::NONE)
				{
					poker::value_t vt;
					if(list.size() <= 5){
						vt = is_three_double(list, hand.size() == list.size());
					}
					else{
						vt = is_air_plane(list, hand.size() == list.size());
					}

					return vt != 0;
				}
				else
				{
					if(list.size() > m_tgLast_card.last_play_cards.size())
						return false;
					poker::value_t vt;
					poker::value_t vt_last;
					if(m_tgLast_card.last_play_cards.size() == 5 && list.size() <= 5){
						vt = is_three_double(list, hand.size() == list.size());
						vt_last = is_three_double(m_tgLast_card.last_play_cards, false);
					}
					else{
						vt = is_air_plane(list, hand.size() == list.size());
						vt_last = is_air_plane(m_tgLast_card.last_play_cards, false);
					}

					return get_value(vt) > get_value(vt_last);
				}
			}
			break;
		case CARD_GROUP_TYPE::FOUR:
			{
				return false;
				if(m_tgLast_card.last_play_type == CARD_GROUP_TYPE::NONE)
				{
					poker::value_t vt;
					if(list.size() <= 7){
						vt = is_four_three(list, hand.size() == list.size());
					}
					else{
						vt = is_bomber(list, hand.size() == list.size());
					}

					return vt != 0;
				}
				else
				{
					if(list.size() > m_tgLast_card.last_play_cards.size())
						return false;
					poker::value_t vt;
					poker::value_t vt_last;
					if(m_tgLast_card.last_play_cards.size() == 7 && list.size() <= 7){
						vt = is_four_three(list, hand.size() == list.size());
						vt_last = is_four_three(m_tgLast_card.last_play_cards, false);
					}
					else{
						vt = is_bomber(list, hand.size() == list.size());
						vt_last = is_bomber(m_tgLast_card.last_play_cards, false);
					}

					return get_value(vt) > get_value(vt_last);
				}
			}
			break;
		case CARD_GROUP_TYPE::BOMB:
			{
				if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::BOMB)
					return is_bomb(list) != 0;
				else
				{
					poker::value_t vt = is_bomb(list);
					return get_value(vt) > get_value(m_tgLast_card.last_play_cards[0]);
				}
			}
			break;
		default:
			return false;
			break;
		}

		return false;
	}

	bool game_room::check_can_play_card(int userIndex)
	{
		std::vector<poker::value_t>& hand = m_members[userIndex].hand;

		std::vector<poker::value_t> fourList;
		sort_four(hand, fourList);

		//上家没出炸弹,自己有炸弹,必能出
		if(m_tgLast_card.last_play_type != CARD_GROUP_TYPE::BOMB && !fourList.empty())
			return true;

		switch(m_tgLast_card.last_play_type)
		{
		case CARD_GROUP_TYPE::SINGLE:
			{
				//判断用户首张牌是否足够大
				return get_value(hand[0]) > get_value(m_tgLast_card.last_play_cards[0]);
			}
		case CARD_GROUP_TYPE::PAIRS:
			return have_double(hand, m_tgLast_card.last_play_cards);
		case CARD_GROUP_TYPE::THREE:
			return have_three(hand, m_tgLast_card.last_play_cards);
		case CARD_GROUP_TYPE::FOUR:
			return have_four(hand, m_tgLast_card.last_play_cards);
		case CARD_GROUP_TYPE::BOMB:
			{
				if(fourList.empty())
					return false;

				return get_value(fourList[0]) > get_value(m_tgLast_card.last_play_cards[0]);
			}
			break;
		case CARD_GROUP_TYPE::PROGRESSION:
			return have_order(hand, m_tgLast_card.last_play_cards);
			break;
		default:
			return false;
		}
	}

	int game_room::get_player_number()
	{
		int count = 0;
		for(int i=0; i< MAX_MEMBERS; i++)
		{
			if(m_members[i].uuid != 0)
				count++;
		}

		return count;
	}

	void game_room::on_play_bomb(int index)
	{
		Json::Value notify;
		for(int i=0; i<MAX_MEMBERS;i++)
		{
			room_member& member = m_members[i];
			if(member.uuid == 0)
				continue;
			Json::Value data;
			data["index"] = i;
			if(i == index)
			{
				member.score += BOMB_SCORE*(get_player_number()-1);
				member.bomb_score += BOMB_SCORE*(get_player_number()-1);
				data["bomb_score"] = BOMB_SCORE*(get_player_number()-1);

			}
			else{
				member.score -= BOMB_SCORE;
				member.bomb_score -= BOMB_SCORE;
				data["bomb_score"] = -BOMB_SCORE;
			}

			data["total_score"] = member.score;
			notify["members"].append(data);
		}

		send_room((protocol)paodk_bomb_score, notify, 0);
	}

	bool game_room::have_card_of_bomb(std::vector<poker::value_t>& hand, std::vector<poker::value_t>& play_Cards)
	{
		std::vector<poker::value_t> fourList;
		sort_four(hand, fourList);

		for (size_t i=0; i<play_Cards.size();i++)
		{
			for(size_t j=0; j<fourList.size(); j++)
			{
				if(poker::get_point(play_Cards[i]) == poker::get_point(fourList[j]))
				{
					return true;
				}
			}
		}
		return false;
	}

	poker::value_t game_room::get_max_card(std::vector<poker::value_t>& hand)
	{
		if(m_bomb_kechai)
		{
			return hand[0];
		}
		std::vector<poker::value_t> fourList;
		sort_four(hand, fourList);

		bool bFind = false;
		for(size_t j=0; j<fourList.size();j++)
		{
			if(get_value(hand[0]) == get_value(fourList[j]))
			{
				bFind = true;
				break;
			}
		}
		if(bFind)
			return 0;

		return hand[0];
	}
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
