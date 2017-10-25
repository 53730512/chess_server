

#include <algorithm>
#include "protocol.h"
#include "../../io_handler.h"

#define ERROR_BREAK(code) {retCode = code; goto __RESULT;}
////////////////////////////////////////////////////////////////////////////////
namespace zhajh{
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
		m_need_payment		= false;
		m_time_limit		= false;
		m_cur_player		= -1;
		m_zhuang			= 0;
		m_cur_zhu			= 0;
		m_bet_round			= 0;
		m_waitTime			= 0;
		m_option.clear();
		memset(m_pokers, 0, sizeof(m_pokers));
		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].reset();
		}

		m_teshu				= 0;
		m_yazhu				= 1;
		m_jiafen			= false;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_rules(const Json::Value &option)
	{
		m_option = option;
		m_diamond_pay = option["fangfei"].asInt() ? 8 : 8;
		m_diamond_pay *= option["jushu"].asInt() ? 2 : 1;
		m_zhifu		  = option["fangfei"].asInt() ? 2 : 1;
		m_round_total = option["jushu"].asInt() ? 16 : 8;
		m_men_round       = option["menpai"].asInt();
		m_teshu		  = option["teshu"].asInt();
		m_jiafen	= option["jiafen"].asBool();
		if(m_men_round < 0 || m_men_round > 3)
			return false;
		switch(option["yazhu"].asInt())
		{
		case 0:
		case 1:
			m_yazhu = option["yazhu"].asInt() + 1;
			break;
		case 2:
			m_yazhu = 5;
			break;
		case 3:
			m_yazhu = 30;
			break;
		}
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
		m_men_round							= context["men_round"].asInt();
		m_teshu							= context["teshu"].asInt();
		m_yazhu							= context["yazhu"].asInt();
		m_time_limit					= context["time_limit"].asBool();
		m_jiafen						= context["jiafen"].asBool();
		m_cur_player					= context["cur_player"].asInt();
		m_cur_zhu						= context["cur_zhu"].asInt();
		m_bet_round						= context["bet_round"].asInt();
		m_zhuang						= context["zhuang"].asInt();
		m_waitTime						= context["waitTime"].asInt();
		m_last_winner					= context["last_winner"].asInt();
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
			member.ko			= members[i]["ko"].asBool();
			member.bet_count	= members[i]["bet_count"].asInt();
			member.kan_pai		= members[i]["kan_pai"].asBool();
			member.qi_pai		= members[i]["qi_pai"].asBool();
			member.win_count	= members[i]["win_count"].asInt();

			member.ko_round		= members[i]["ko_round"].asInt();
			member.qi_round		= members[i]["qi_round"].asInt();
			member.kan_round	= members[i]["kan_round"].asInt();
			if(members[i].isMember("hand"))
			{
				for (Json::ArrayIndex j = 0; j < members[i]["hand"].size(); j++){
					member.hand.push_back(members[i]["hand"][j].asInt());
				}
			}

			if(members[i].isMember("history"))
			{
				for (Json::ArrayIndex j = 0; j < members[i]["history"].size(); j++){
					const Json::Value& item = members[i]["history"][j];

					std::vector<int> vt;
					for(int k=0; k<(int)item.size();k++)
					{
						vt.push_back(item[k].asInt());
					}

					member.history.push_back(vt);
				}
			}

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
		result["option"]						 = m_option;

		result["context"]["is_opening"]			= is_opening();
		result["context"]["status"]				= m_status;
		result["context"]["idle_time"]			= m_idle_time; //?

		result["context"]["round"]				= m_round;
		result["context"]["round_total"]		= m_round_total;
		result["context"]["zhifu"]				= m_zhifu;
		result["context"]["diamond_pay"]		= m_diamond_pay;
		result["context"]["need_payment"]		= m_need_payment;

		result["context"]["next"]				= m_next;
		result["context"]["men_round"]			= m_men_round;
		result["context"]["teshu"]				= m_teshu;
		result["context"]["yazhu"]				= m_yazhu;
		result["context"]["time_limit"]			= m_time_limit;
		result["context"]["jiafen"]				= m_jiafen;
		result["context"]["cur_player"]			= m_cur_player;
		result["context"]["cur_zhu"]			= m_cur_zhu;
		result["context"]["bet_round"]			= m_bet_round;
		result["context"]["zhuang"]				= m_zhuang;
		result["context"]["waitTime"]			= m_waitTime;
		result["context"]["last_winner"]		= m_last_winner;

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
			member["ko"]			= m_members[i].ko;
			member["bet_count"]		= m_members[i].bet_count;
			member["win_count"]		= m_members[i].win_count;
			member["kan_pai"]		= m_members[i].kan_pai;
			member["qi_pai"]		= m_members[i].qi_pai;

			member["ko_round"]		= m_members[i].ko_round;
			member["qi_round"]		= m_members[i].qi_round;
			member["kan_round"]		= m_members[i].kan_round;
			for (size_t j = 0; j < m_members[i].hand.size(); j++){
				//Json::Value card;
				//card["id"] = poker::get_point(m_members[i].hand[j]);
				//card["type"] = poker::get_type(m_members[i].hand[j]);
				member["hand"].append(m_members[i].hand[j]);
			}

			for(size_t j=0; j<m_members[i].history.size(); j++)
			{
				Json::Value hData;
				for(size_t k=0; k<m_members[i].history[j].size(); k++)
				{
					hData.append(m_members[i].history[j][k]);
				}
				member["history"].append(hData);
			}
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
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			Json::Value score;
			score["uuid"] = m_members[i].uuid;
			score["index"] = i;
			score["score"] = m_members[i].score;
			score["max_score"] = m_members[i].max_score;
			score["max_type"] = m_members[i].max_type;
			score["win_count"] = m_members[i].win_count;
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

		if(m_status != room_state::room_playing)
			return;
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_enter(io_player::value_type player)
	{
		int index = -1; //默认用户索引(无效索引)
		time_t time_now = time(0);
		Json::Value notify;
		std::vector<int> empty_index;
		for (int i = 0; i < MAX_MEMBERS; i++){
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
			if (empty_index.size() == MAX_MEMBERS){
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
			for (int i = 0; i < MAX_MEMBERS; i++){
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
			for (int i = 0; i < MAX_MEMBERS; i++){
				if (m_members[i].uuid == 0)
					continue;
				member_count++;
			}
			for (int i = 0; i < MAX_MEMBERS; i++){
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
				info.format("%s 进入房间，人数(%d/%d)", nickname.c_str(), member_count, MAX_MEMBERS);
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
		for (int i = 0; i < MAX_MEMBERS; i++){
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
		case zhajh_ready:
			on_ready(uuid, data);
			break;
		case zhajh_protocol::zhajh_begin:
			on_round_begin(uuid, data);
			break;
		case zhajh_protocol::zhajh_gen_zhu:
			on_gen_zhu(uuid, data);
			break;
		case zhajh_protocol::zhajh_jia_zhu:
			on_jia_zhu(uuid, data);
			break;
		case zhajh_protocol::zhajh_kan_pai:
			on_kan_pai(uuid, data);
			break;
		case zhajh_protocol::zhajh_qi_pai:
			on_qi_pai(uuid, data);
			break;
		case zhajh_protocol::zhajh_bi_pai:
			on_bi_pai(uuid, data);
			break;
		case zhajh_room_broadcast:
			on_room_broadcast(uuid, data);
			break;
		}
		parent::on_request(player, type, data);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_destroy(bool completed)
	{
		//统计用户的总成绩(多个游戏)
		for (int i = 0; i < MAX_MEMBERS; i++){
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
			for (int i = 0; i < MAX_MEMBERS; i++){
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
		for (int i = 0; i < MAX_MEMBERS; i++){
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
			for (int i = 0; i < MAX_MEMBERS; i++){
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
		if (m_status > room_wait_ready){
			send((protocol)zhajh_ready, notify, uuid, 1);
			return;
		}
		time_t time_now   = time(0);
		int  index_ready  = -1;
		int  count_ready  = 0;
		bool is_all_ready = true;
		for (int i = 0; i < MAX_MEMBERS; i++){
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
		send_room((protocol)zhajh_ready, notify, 0);
		//如果所有人都准备且大于1人
		if (is_all_ready && count_ready > 1){
			if (m_round > 0){
				init_next_round(); //开始下一局
			} else {
				int index_first_ready = 0;
				time_t min_time_ready = 0;
				for (int i = 0; i < MAX_MEMBERS; i++){
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
				//	set_room_status(room_wait_begin);
				//userid uuid = m_members[index_first_ready].uuid;
				//send((protocol)paodk_begin, notify, uuid, 0);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_round_begin(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_ready){
			send((protocol)room_wait_ready, notify, uuid, 1);
			return;
		}
		int  count_ready = 0;
		bool is_all_ready = true;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			} else if (m_members[i].time_ready){
				count_ready++;
			} else {
				is_all_ready = false;
			}
		}
		if (!is_all_ready){ //不是所有人都准备好了
			send((protocol)zhajh_begin, notify, uuid, 20);
			return;
		}
		if (count_ready < 2){ //至少要有2个人准备好了才能开局
			send((protocol)zhajh_begin, notify, uuid, 30);
			return;
		}
		init_next_round(); //初始化下一局(在这里是第一局)
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_room_broadcast(userid uuid, const Json::Value &data)
	{
		send_room((protocol)zhajh_room_broadcast, data, 0);
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
		m_cur_zhu = m_yazhu;
		m_bet_round = 0;
		if(m_cur_zhu > 5)
			m_cur_zhu = 1;
		//m_compareList.clear();
		if(m_round > 1)
			m_zhuang = m_last_winner;

		int nPlayerNum = get_player_number();
		//设定第一个出牌的用户
		if(m_round == 1)
		{
			for (int i = 0; i < MAX_MEMBERS; i++){
				if (m_members[i].uuid == 0){
					continue; //没参与牌局
				}

				if(get_creater() == m_members[i].uuid)
				{
					m_cur_player = i;
					m_zhuang = m_cur_player;
					break;
				}
			}
		}
		else
		{
			m_cur_player = m_zhuang;
		}


		//srand(1234567895);

		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].init();
			if (m_members[i].uuid == 0){
				continue; //没参与牌局
			}
			Json::Value notify;
			notify["round"]        = m_round;
			notify["round_total"]  = m_round_total;
			notify["di_zhu"] = m_cur_zhu;
			notify["bet_round"] = 1;
			notify["zhuang"] = m_zhuang;
			userid uuid = m_members[i].uuid;
			m_members[i].bet_count += m_cur_zhu;
			send((protocol)zhajh_begin, notify, uuid, 0);
		}
		init_next_poker();  //初始化牌池
		init_user_poker();  //初始化用户手牌



		//第一个的下手先操作
		turn_to_next_player();


		//发送等待出牌协议
		send_wait_operation();

		//设置状态为比赛中qi
		set_room_status(room_playing);
	}
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_poker()
	{
		//初始化牌局参数
		m_next = 0;
		memset(m_pokers, 0, sizeof(m_pokers));
		//初始化牌池数据
		std::vector<poker::value_t> temp_pokers;
		int max_poker_point = poker::point_J_black;
		for (int j = poker::d_diamond; j < poker::k_joker; j++){
			for (int i = poker::point_A; i < max_poker_point; i++){
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
		for(int j=0; j<3; j++)
		{
			for(int i=0; i<MAX_MEMBERS; i++)
			{
				room_member& member = m_members[i];

				member.hand.push_back(m_pokers[m_next]);
				m_next++;
			}
		}

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
#ifdef _DEBUG
		//m_members[0].hand.clear();
		//m_members[1].hand.clear();

		//m_members[0].hand.push_back(poker::set_value(poker::point_2, poker::h_heart));
		//m_members[0].hand.push_back(poker::set_value(poker::point_3, poker::d_diamond));
		//m_members[0].hand.push_back(poker::set_value(poker::point_4, poker::h_heart));

		//m_members[1].hand.push_back(poker::set_value(poker::point_A, poker::c_club));
		//m_members[1].hand.push_back(poker::set_value(poker::point_2, poker::c_club));
		//m_members[1].hand.push_back(poker::set_value(poker::point_3, poker::d_diamond));


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
	}
	////////////////////////////////////////////////////////////////////////////////


	void game_room::on_gen_zhu(userid uuid, const Json::Value& data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);
		notify["index"] = user_index;
		std::vector<int> vt;

		if(m_status != room_playing)
			ERROR_BREAK(1);

		if(user_index != m_cur_player)
			ERROR_BREAK(2);

		int drop_count = m_members[user_index].kan_pai?m_cur_zhu*2:m_cur_zhu;
		if(m_yazhu < 10 && drop_count  == 8)
		{
			drop_count = 10;
		}
		notify["drop_count"] = drop_count;

		//10轮以后只能比牌
		if(m_bet_round + 1 > 10)
			ERROR_BREAK(3);

		m_members[user_index].bet_count += m_members[user_index].kan_pai?m_cur_zhu*2:m_cur_zhu;
		send_room((protocol)zhajh_gen_zhu, notify, retCode);

		vt.push_back(record_type::gen);
		vt.push_back(drop_count);
		m_members[user_index].history.push_back(vt);
		//切换到下个人
		if(is_last_operation_player(m_cur_player))
			m_bet_round++;

		turn_to_next_player();
		send_wait_operation();
		return;
__RESULT:
		send((protocol)zhajh_gen_zhu, notify, uuid, retCode);
	}

	void game_room::on_jia_zhu(userid uuid, const Json::Value& data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);
		std::vector<int> vt;
		if(m_status != room_playing)
			ERROR_BREAK(1);

		if(user_index != m_cur_player)
			ERROR_BREAK(2);

		//10轮以后只能比牌
		if(m_bet_round + 1 > 10)
			ERROR_BREAK(3);

		notify["index"] = user_index;
		int target_bet = data["zhu"].asInt();

		int drop_count = 0;
		if(m_yazhu < 10) //1,2,5
		{
			if(m_cur_zhu > m_yazhu)
				ERROR_BREAK(31);

			m_cur_zhu *= 2;

			drop_count = m_members[user_index].kan_pai ? m_cur_zhu*2 : m_cur_zhu;
			if(drop_count == 8)
				drop_count = 10;
		}
		else{ //1-30;
			if(m_cur_zhu >= 15)
				ERROR_BREAK(4);

			if(target_bet < m_cur_zhu || target_bet > 15)
				ERROR_BREAK(5);

			m_cur_zhu = target_bet;
			drop_count = m_members[user_index].kan_pai ? m_cur_zhu*2 : m_cur_zhu;
		}

		m_members[user_index].bet_count += drop_count;
		notify["di_zhu"] = m_cur_zhu;
		notify["drop_count"] = drop_count;

		send_room((protocol)zhajh_jia_zhu, notify, retCode);


		vt.push_back(record_type::jia);
		vt.push_back(drop_count);
		m_members[user_index].history.push_back(vt);

		//切换到下个人
		if(is_last_operation_player(m_cur_player))
			m_bet_round++;

		turn_to_next_player();
		send_wait_operation();
		return;

__RESULT:
		send((protocol)zhajh_jia_zhu, notify, uuid, retCode);
	}

	void game_room::on_qi_pai(userid uuid, const Json::Value& data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);
		if(m_status != room_playing)
			ERROR_BREAK(1);

		notify["index"] = user_index;
		if(user_index != m_cur_player)
			ERROR_BREAK(2);

		if(m_members[user_index].qi_pai)
			ERROR_BREAK(3);

		////10轮以后只能比牌
		//if(m_bet_round + 1 > 10)
		//	ERROR_BREAK(4);

		m_members[user_index].qi_pai = true;
		m_members[user_index].qi_round = m_bet_round+1;

		send_room((protocol)zhajh_qi_pai, notify, 0);

		int left_index = 0;
		if(check_game_over(left_index))
		{
			if(m_round < m_round_total)
				publish_result(left_index, false);
			else
				publish_result(left_index, true);
		}
		else{
			//切换到下个人
			if(is_last_operation_player(m_cur_player))
				m_bet_round++;
			turn_to_next_player();
			send_wait_operation();
		}

__RESULT:
		send((protocol)zhajh_qi_pai, notify, uuid, retCode);
	}

	void game_room::on_kan_pai(userid uuid, const Json::Value& data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);

		if(m_status != room_playing)
			ERROR_BREAK(1);

		notify["index"] = user_index;
		if(m_members[user_index].kan_pai)
			ERROR_BREAK(2);

		if(m_bet_round+1 <= m_men_round)
			ERROR_BREAK(3);

		if(m_members[user_index].qi_pai 
			||m_members[user_index].ko)
			ERROR_BREAK(4);

		m_members[user_index].kan_pai = true;

		m_members[user_index].kan_round = m_bet_round+1;
		if(is_player_done_operation(user_index))
		{
			m_members[user_index].kan_round = m_bet_round+2;
		}

		for(int i=0; i<MAX_MEMBERS; i++)
		{
			if(m_members[i].uuid == 0)
				continue;

			notify["hand"].clear();
			notify["type"] = get_group_type(m_members[i].hand).type;
			if(i == user_index){
				for(size_t j=0; j<m_members[i].hand.size();j++)
				{
					notify["hand"].append(m_members[i].hand[j]);
				}
			}

			send((protocol)zhajh_kan_pai, notify, m_members[i].uuid, retCode);
		}

		return;

__RESULT:
		send((protocol)zhajh_kan_pai, notify, uuid, retCode);
	}

	void game_room::on_bi_pai(userid uuid, const Json::Value& data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);
		int target_index= data["index"].asInt();
		std::vector<int> vt;
		if(m_status != room_playing)
			ERROR_BREAK(1);

		if(user_index != m_cur_player)
			ERROR_BREAK(2);

		if(m_bet_round+1 < 3)
			ERROR_BREAK(5);

		if(m_members[user_index].ko
			||m_members[target_index].qi_pai)
			ERROR_BREAK(6);

		if(target_index < 0 
			|| target_index >= MAX_MEMBERS
			|| m_members[target_index].uuid == 0
			|| m_members[target_index].ko
			|| m_members[target_index].qi_pai
			||target_index == user_index)
			ERROR_BREAK(3);
		if(m_status != room_playing)
			ERROR_BREAK(4);

		m_members[user_index].bet_count += m_cur_zhu*(m_members[user_index].kan_pai?2:1);

		notify["src_index"] = user_index;
		notify["dst_index"] = target_index;
		notify["drop_count"] = m_cur_zhu*(m_members[user_index].kan_pai?2:1);
		if(card_compare(m_members[user_index].hand, m_members[target_index].hand, m_teshu))
		{
			notify["winner"] = user_index;
			m_members[target_index].ko = true;
			m_members[target_index].ko_round = m_bet_round+1;
		}
		else
		{
			notify["winner"] = target_index;
			m_members[user_index].ko = true;
			m_members[user_index].ko_round = m_bet_round+1;
		}

		vt.push_back(record_type::bi);
		vt.push_back(m_cur_zhu*(m_members[user_index].kan_pai?2:1));
		vt.push_back(target_index);
		m_members[user_index].history.push_back(vt);

		// 最后两个人比牌,才带过去牌
		int left_index = 0;
		if(check_game_over(left_index))
		{
			for(size_t i=0; i<m_members[user_index].hand.size(); i++)
			{
				notify["src_hand"].append(m_members[user_index].hand[i]);
			}

			for(size_t i=0; i<m_members[target_index].hand.size(); i++)
			{
				notify["dst_hand"].append(m_members[target_index].hand[i]);
			}

			//m_compareList.push_back(user_index);
			//m_compareList.push_back(target_index);
			send_room((protocol)zhajh_bi_pai, notify, 0);
			if(m_round < m_round_total)
				publish_result(left_index, false);
			else
				publish_result(left_index, true);
		}
		else{
			send_room((protocol)zhajh_bi_pai, notify, 0);
			//切换到下个人
			if(is_last_operation_player(m_cur_player))
				m_bet_round++;
			turn_to_next_player();
			send_wait_operation();
		}
		return;

__RESULT:
		send((protocol)zhajh_bi_pai, notify, uuid, retCode);
	}

	void game_room::set_room_status(room_state status)
	{
		set_dirty();
		m_idle_time = 0;
		m_status    = status;
		//广播状态改变消息
		Json::Value notify;
		notify["room_state"] = status;
		send_room((protocol)zhajh_status_change, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::publish_result(int winner, bool completed)
	{
		m_last_winner = winner;
		Json::Value notify;
		notify["winner"] = winner;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["zhuang"] = m_zhuang;

		room_member& winner_data = m_members[winner];
		m_members[winner].win_count++;

		int winner_card_type = get_group_type(m_members[winner].hand).type;

		int  member_count = 0;
		int total_score = 0;
		for(int i=0; i<MAX_MEMBERS; i++)
		{
			room_member& player_data = m_members[i];
			if(player_data.uuid != 0)
			{
				if(m_jiafen)
				{
					if(winner_card_type == CARD_GROUP_TYPE::BAOZI)
						player_data.bet_count += 10;
					else if(winner_card_type == CARD_GROUP_TYPE::SHUNJIN)
						player_data.bet_count += 5;
				}

				total_score += player_data.bet_count;
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
			hMember["score"] = -player_data.bet_count;
			for(size_t j=0;j<player_data.hand.size();j++)
			{
				hMember["hand"].append(player_data.hand[j]);
			}
			int type = get_group_type(player_data.hand).type;
			hMember["type"] = type;

			if(type > player_data.max_type)
				player_data.max_type = type;

			player_data.score -= player_data.bet_count;
			player_data.time_ready = 0;//清空准备时间

			for(size_t j=0; j<m_members[i].history.size(); j++)
			{
				Json::Value hData;
				for(size_t k=0; k<m_members[i].history[j].size(); k++)
				{
					hData.append(m_members[i].history[j][k]);
				}
				hMember["history"].append(hData);
			}

			hMember["total_score"] = player_data.score;
			hMember["win_count"] = player_data.win_count;
			hMember["max_type"] = player_data.max_type;
			hMember["max_score"] = player_data.max_score;
			hMember["ko_round"] = player_data.ko_round;
			hMember["qi_round"] = player_data.qi_round;
			hMember["kan_round"] = player_data.kan_round;
			notify["members"].append(hMember);

		}
		m_members[winner].score += total_score;

		if(total_score - m_members[winner].bet_count > m_members[winner].max_score)
			m_members[winner].max_score = total_score - m_members[winner].bet_count;

		if (!is_free_time()){
			if (m_round == 1 && m_zhifu == 2){ //房主支付
				int payment = m_diamond_pay * member_count;
				deduct_diamond(get_creater(), payment);
			}
		}

		notify["members"][winner]["score"] = total_score - m_members[winner].bet_count;
		notify["members"][winner]["total_score"] = m_members[winner].score;
		notify["members"][winner]["max_score"] = m_members[winner].max_score;
		send_room((protocol)zhajh_publish_result, notify, 0);

		if(completed){
			set_completed(); //设置比赛结束标记
			set_room_status(room_completed);
		}
		else{
			set_room_status(room_state::room_wait_ready);
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
		notify["status"] = m_status;
		notify["round"] = m_round;
		notify["round_total"] = m_round_total;
		notify["cur_player"] = m_cur_player;
		notify["zhuang"] = m_zhuang;
		notify["bet_round"] = m_bet_round+1;
		notify["di_zhu"] = m_cur_zhu;

		//if(!m_compareList.empty())
		//{
		//	for(size_t j =0; j<m_compareList.size(); j++)
		//	{
		//		room_member& member = m_members[m_compareList[j]];
		//		Json::Value hData;
		//		hData["index"] = m_compareList[j];
		//		for(size_t k=0; k<member.hand.size();k++)
		//		{
		//			hData["hand"].append(member.hand[k]);
		//		}
		//		hData["type"] = get_group_type(member.hand).type;

		//		notify["compare"].append(hData);
		//	}
		//}

		for(size_t i=0; i<MAX_MEMBERS; i++)
		{
			room_member& member = m_members[i];
			Json::Value hData;
			Json::Value hCard;

			//开局中,看牌的玩家发送手牌
			if(m_status == room_playing)
			{
				if(userIndex == i && member.kan_pai)
				{
					for(size_t j=0; j<m_members[userIndex].hand.size(); j++)
					{
						hData["hand"].append(m_members[userIndex].hand[j]);
					}
				}
			}
			else
			{
				//局数大于1,同步上局的手牌
				if(m_round >= 1)
				{
					//if(m_members[i].kan_pai)
					//{
					for(size_t j=0; j<m_members[i].hand.size(); j++)
					{
						hData["hand"].append(m_members[i].hand[j]);
					}

					hData["type"] = get_group_type(m_members[i].hand).type;
					//}

				}
			}

			hData["score"] = member.score;
			hData["win_count"] = member.win_count;
			hData["qi_pai"] = member.qi_pai;
			hData["kan_pai"] = member.kan_pai;
			hData["bet_count"] = member.bet_count;
			hData["ko"] = member.ko;
			notify["members"].append(hData);
		}

		if(m_status != room_playing && m_round >= 1)
		{
			notify["winner"] = m_last_winner;
		}

		send((protocol)zhajh_sync_data, notify, player, 0);

		if(m_status == room_playing)
		{
			Json::Value notify;

			notify["index"]	= m_cur_player;
			notify["bet_round"] = m_bet_round+1;
			send((protocol)zhajh_wait_operation, notify, player, 0);

		}
	}

	void game_room::turn_to_next_player()
	{
		while(true)
		{
			m_cur_player++;
			if(m_cur_player >= MAX_MEMBERS)
				m_cur_player = 0;
			if(m_members[m_cur_player].uuid !=0
				&&!m_members[m_cur_player].qi_pai
				&&!m_members[m_cur_player].ko)
				break;
		}
	}

	bool game_room::is_last_operation_player(int index)
	{
		if(index == m_zhuang)
			return true;

		if(!m_members[m_zhuang].qi_pai && !m_members[m_zhuang].ko)
		{
			return index == m_zhuang;
		}
		else
		{
			while (true)
			{
				index++;
				if(index >= MAX_MEMBERS)
					index = 0;
				if(index == m_zhuang)
					return true;

				if(m_members[index].uuid != 0
					&&!m_members[index].qi_pai
					&&!m_members[index].ko)
					return false;
			}
		}
	}

	bool game_room::is_player_done_operation(int index)
	{
		//正在操作的玩家返回false
		if(m_cur_player == index)
			return false;

		//自己是庄,本轮肯定没有操作过
		if(index == m_zhuang)
			return true;

		//在当前操作者和庄之间有当前玩家,肯定没操作过
		int beginIndex = m_cur_player;
		while (true)
		{
			beginIndex++;
			if(beginIndex >= MAX_MEMBERS)
				beginIndex = 0;
			if(beginIndex == m_zhuang)
				return false;

			if(beginIndex == index)
				return false;
		}

		return true;
	}

	//广播该某人出牌了，first_play为true，代表新一轮出牌，客户端清理桌面牌
	void game_room::send_wait_operation()
	{
		room_member& player = m_members[m_cur_player];

		Json::Value notify;
		notify["index"]	= m_cur_player;
		notify["bet_round"] = m_bet_round+1;
		send_room((protocol)zhajh_wait_operation, notify, 0);
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

	bool game_room::check_game_over(int& index )
	{
		int count = 0;
		for(int i=0; i< MAX_MEMBERS; i++)
		{
			if(m_members[i].uuid != 0
				&&!m_members[i].qi_pai
				&&!m_members[i].ko)
			{
				index = i;
				count++;
			}
		}

		return count <= 1;
	}

	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
