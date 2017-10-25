

#include <algorithm>
#include "room_base.h"
#include "../../io_handler.h"
#include "../protocol.h"
#define ERROR_BREAK(code) {retCode = code; goto __RESULT;}
////////////////////////////////////////////////////////////////////////////////
namespace mj_base{
	////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	const time_t dismiss_time_limit = 0;
#else
	const time_t dismiss_time_limit = 180;
#endif

	const time_t delay_send_disable_op_time = 600;
	////////////////////////////////////////////////////////////////////////////////
	room_base::room_base(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
		: room_basic(creater, type, ruleid, number, time_now, owner_exit)
	{
		m_idle_time			= 0;
		m_round				= 0;
		m_round_total		= 0;
		m_zhifu				= 0;
		m_renshu			= 0;
		m_diamond_pay		= 0;
		m_zhuaniao			= 0;

		m_dismiss_index		= -1;
		m_status			= room_wait_ready;
		m_next				= 0;
		m_time_limit		= false;
		m_cur_player		= -1;
		m_last_out_player	= -1;
		m_zhuang			= 0;
		m_enable_chi		= true;
		m_option.clear();
		m_last_winner = 0;
		round_clear();
		m_beilv = 1;
		m_enable_dian_pao = true;
		m_zhuangxian	= true;
		m_wang				= 0;
		m_block_json.clear();
		m_block_time = 0;
		m_init_card_num = 0;
		m_next_zhuang = -1;
		m_cards_end_id = 0;
	}
	void room_base::round_clear()
	{
		m_mahjong.clear();
		m_right_wait.clear();

		for(size_t i=0; i<m_members.size(); i++)
		{
			m_members[i]->reset();
		}

		m_last_out_player = -1;
		m_enable_tian_hu = true;
		m_history.clear();
		m_zimo_hu = false;
		m_block_time = 0;
		m_cards_end_id = 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool room_base::init_room_rules(const Json::Value &option)
	{
		m_option = option;
		set_pay_type(m_zhifu);

		m_enable_dian_pao = option["wanfa"].asInt() == 0;
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool room_base::init_room_context(const Json::Value &context)
	{
		m_dismiss_index = -1;
		if (context["is_opening"].asBool()){ //已开局
			set_open();
		}

		m_status						= (room_state)context["status"].asInt();
		m_round							= context["round"].asInt();
		m_round_total					= context["round_total"].asInt();
		m_zhifu							= context["zhifu"].asInt();
		m_renshu						= context["renshu"].asInt();
		m_diamond_pay					= context["diamond_pay"].asInt();
		m_wang							= context["wang"].asInt();
		m_idle_time						= context["idle_time"].asInt();
		m_zhuaniao						= context["zhuaniao"].asInt();
		m_beilv							= context["beilv"].asInt();
		m_enable_dian_pao				= context["enable_dian_pao"].asBool();
		m_zhuangxian					= context["zhuangxian"].asBool();

		m_init_card_num					= context["init_card_num"].asInt();

		m_next							= context["next"].asInt();
		m_time_limit					= context["time_limit"].asBool();
		m_cur_player					= context["cur_player"].asInt();
		m_last_out_player					= context["last_out"].asInt();
		m_zhuang						= context["zhuang"].asInt();
		m_last_winner					= context["last_winner"].asInt();
		m_next_zhuang					= context["next_zhuang"].asInt();
		m_cards_end_id					= context["cards_end_id"].asInt();
		m_enable_chi					= context["enable_chi"].asInt();

		m_block_time					= context["block_time"].asInt();
		m_enable_tian_hu				= context["enable_tian_hu"].asBool();
		for (Json::ArrayIndex i=0; i<context["block_json"].size(); i++)
		{
			m_block_json.push_back(context["block_json"][i]);
		}

		if(context.isMember("mahjong"))
		{
			const Json::Value& hMahjong			= context["mahjong"];
			for(int i=0; i<(int)hMahjong.size(); i++)
			{
				m_mahjong.push_back(hMahjong[i].asInt());
			}
		}

		if(context.isMember("right_wait"))
		{
			m_right_wait.index = context["right_wait"]["index"].asInt();
			m_right_wait.right = context["right_wait"]["right"].asInt();
			m_right_wait.card_id = context["right_wait"]["card_id"].asInt();
			if(context["right_wait"].isMember("hu_list"))
			{
				for(Json::ArrayIndex i=0; i<context["right_wait"]["hu_list"].size(); i++)
				{
					m_right_wait.hu_list.push_back(context["right_wait"]["hu_list"][i].asInt());
				}
			}
		}

		if(context.isMember("gangzi"))
		{
			for (Json::ArrayIndex i=0; i<context["gangzi"].size(); i++)
			{
				m_gangzi.push_back(context["gangzi"][i].asInt());
			}

		}


		if(context.isMember("niao"))
		{
			for (Json::ArrayIndex i=0; i<context["niao"].size(); i++)
			{
				m_niao.push_back(context["niao"][i].asInt());
			}

		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool room_base::init_room_members(const Json::Value &members)
	{
		if (!members.isArray()){
			return false;
		}

		//导入成员数据
		for (Json::ArrayIndex i = 0; i < members.size(); i++){
			int index = members[i]["index"].asInt();
			player_base* member = m_members[i];
			member->analy_data(members[i]);
			insert_member(member->uuid, index, member->nickname, member->sex, member->head_url);
		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::export_data(Json::Value& result) const
	{
		result["option"]						 = m_option;
		result["context"]["is_opening"]			= is_opening();
		result["context"]["status"]				= m_status;
		result["context"]["idle_time"]			= m_idle_time; //?

		result["context"]["round"]				= m_round;
		result["context"]["round_total"]		= m_round_total;
		result["context"]["zhifu"]				= m_zhifu;
		result["context"]["renshu"]				= m_renshu;
		result["context"]["diamond_pay"]		= m_diamond_pay;
		result["context"]["wang"]				= m_wang;
		result["context"]["enable_dian_pao"]	= m_enable_dian_pao;
		result["context"]["zhuangxian"]			= m_zhuangxian;

		result["context"]["init_card_num"]			= m_init_card_num;

		result["context"]["next"]				= m_next;
		result["context"]["time_limit"]			= m_time_limit;
		result["context"]["cur_player"]			= m_cur_player;
		result["context"]["last_out"]			= m_last_out_player;
		result["context"]["last_winner"]		= m_last_winner;
		result["context"]["next_zhuang"]		= m_next_zhuang;
		result["context"]["cards_end_id"]		= m_cards_end_id;
		result["context"]["enable_chi"]			= m_enable_chi;
		result["context"]["zhuaniao"]			= m_zhuaniao;
		result["context"]["beilv"]				= m_beilv;
		result["context"]["block_time"]			= m_block_time;
		result["context"]["enable_tian_hu"]		= m_enable_tian_hu;
		for(size_t i=0; i<m_block_json.size(); i++)
		{
			result["context"]["block_json"].append(m_block_json[i]);
		}

		for(size_t i=0; i<m_mahjong.size(); i++)
		{
			result["context"]["mahjong"].append(m_mahjong[i]);
		}

		if(m_right_wait.right > 0)
		{
			result["context"]["right_wait"]["index"] = m_right_wait.index;
			result["context"]["right_wait"]["card_id"] = m_right_wait.card_id;
			result["context"]["right_wait"]["right"] = m_right_wait.right;
			for(size_t i=0; i<m_right_wait.hu_list.size(); i++)
			{
				result["context"]["right_wait"]["hu_list"].append(m_right_wait.hu_list[i]);
			}
		}

		for(size_t i=0; i<m_gangzi.size(); i++)
		{
			result["context"]["gangzi"].append(m_gangzi[i]);
		}

		for(size_t i=0; i<m_niao.size(); i++)
		{
			result["context"]["niao"].append(m_niao[i]);
		}

		//导出成员数据
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == 0){
				continue;
			}
			Json::Value member;
			m_members[i]->export_data(member);
			result["members"].append(member);
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	bool room_base::import_data(const io::stringc &data)
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
	io::stringc room_base::get_options() const
	{
		io::stringc option(m_option.toFastString().c_str());

		return std::move(option);
	}

	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_create()
	{
		parent::on_create();
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_update(int delta)
	{
		parent::on_update(delta);
		//如果房间在解散中
		if (is_dismissing()){
			int index = m_dismiss_index;
			time_t dismiss_time = m_members[index]->time_agree;
			time_t time_now = time(0);
			if (time_now - dismiss_time > dismiss_time_limit){
				set_dismissed();
			}
			return;
		}

		if(m_status != room_state::room_playing)
			return;

		if(!m_block_json.empty())
		{
			m_block_time += delta;
			if(m_block_time > 500)
			{
				Json::Value& hPack=  m_block_json[0];
				m_block_time = 0;
				this->on_chu_pai(hPack["uuid"].asInt(), hPack);
				m_block_json.erase(m_block_json.begin());
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_enter(io_player::value_type player)
	{
		int index = -1; //默认用户索引(无效索引)
		time_t time_now = time(0);
		Json::Value notify;
		std::vector<int> empty_index;
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == player->get_uuid()){
				m_members[i]->time_enter  = time_now;
				m_members[i]->ipaddr      = player->get_ipaddr();
				m_members[i]->device      = player->get_device();
				m_members[i]->voice_token = player->get_voice_token();
				index = i;
			} else if (m_members[i]->uuid == 0){
				empty_index.push_back(i);
			} else { //有其他成员存在
				Json::Value member;
				member["index"]      = i;
				member["uuid"]       = m_members[i]->uuid;
				member["score"]      = m_members[i]->score;
				member["nickname"]   = m_members[i]->nickname;
				member["sex"]        = m_members[i]->sex;
				member["head_url"]   = m_members[i]->head_url;
				member["ipaddr"]     = m_members[i]->ipaddr;
				member["time_enter"] = m_members[i]->time_enter;
				member["time_ready"] = m_members[i]->time_ready;
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
			//index =  rand() % (int)empty_index.size();
			//index = empty_index[index];
			index = empty_index[0];
			//第一个加入房间的人默认准备
			if (empty_index.size() == m_members.size()){
				m_members[index]->time_ready = time_now;
			} else {
				m_members[index]->time_ready = 0;
			}

			m_members[index]->time_enter  = time_now;
			m_members[index]->uuid        = player->get_uuid();
			m_members[index]->nickname    = player->get_nickname();
			m_members[index]->sex         = player->get_sex();
			m_members[index]->head_url    = player->get_head_url();
			m_members[index]->ipaddr      = player->get_ipaddr();
			m_members[index]->device      = player->get_device();
			m_members[index]->voice_token = player->get_voice_token();
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
		notify["uuid"]        = m_members[index]->uuid;
		notify["score"]       = m_members[index]->score;
		notify["time_enter"]  = m_members[index]->time_enter;
		notify["time_ready"]  = m_members[index]->time_ready;
		notify["game_type"]   = get_game_type();
		send(user_enter_room, notify, player, 0);
		//获取用户的综合分数
		m_members[index]->health = player->get_health();
		//把当前用户的信息告诉房间内其他人
		notify.clear();
		notify["index"]       = index;
		notify["uuid"]        = m_members[index]->uuid;
		notify["score"]       = m_members[index]->score;
		notify["nickname"]    = m_members[index]->nickname;
		notify["sex"]         = m_members[index]->sex;
		notify["head_url"]    = m_members[index]->head_url;
		notify["ipaddr"]      = m_members[index]->ipaddr;
		notify["time_enter"]  = m_members[index]->time_enter;
		notify["time_ready"]  = m_members[index]->time_ready;
		notify["game_type"]   = get_game_type();
		//如果已开局则同步房间数据
		send_other(user_enter_room, notify, player, 0);
		if (is_opening()){
			sync_room_data(player);
		}
		//如果房间正在解散中
		if (is_dismissing()){
			player_base* member = m_members[m_dismiss_index];
			time_t time_dismiss = member->time_agree;
			time_t dead_line = time_dismiss + dismiss_time_limit;
			notify.clear();
			notify["dead_line"]             = dead_line;
			notify["applicant"]["index"]    = m_dismiss_index;
			notify["applicant"]["time"]     = time_dismiss;
			notify["applicant"]["uuid"]     = player->get_uuid();
			notify["applicant"]["nickname"] = player->get_nickname();
			for (size_t i = 0; i < m_members.size(); i++){
				if (m_members[i]->uuid == 0){
					continue;
				} else if (m_members[i]->time_agree){
					notify["consenter"].append(i);
				}
			}
			send(room_dismiss_request, notify, player, 0);
		}
		//如果未开局且设置了通知地址
		if (!is_opening() && !m_voice_url.empty()){
			int member_count = 0;
			for (size_t i = 0; i < m_members.size(); i++){
				if (m_members[i]->uuid == 0)
					continue;
				member_count++;
			}
			for (size_t i = 0; i < m_members.size(); i++){
				if (m_members[i]->uuid == 0){
					continue;
				}
				bool need_notify = false;
				io_player::value_type user = io_player::find(m_members[i]->uuid);
				if (user){
					need_notify = user->is_run_back();
				} else {
					need_notify = true;
				}
				if (!need_notify){
					continue;
				}
				if (m_members[i]->voice_token.empty()){
					continue;
				}
				//构建通知信息
				io::stringc info;
				io::stringc nickname = player->get_nickname();
				nickname = nickname.to_ascii();
				info.format("%s 进入房间，人数(%d/%d)", nickname.c_str(), member_count, m_members.size());
				info = info.to_utf8();
				info = http::url::encode(info).c_str();
				//构建通知 URL
				io::stringc url;
				url.format(m_voice_url.c_str(), m_members[i]->device.c_str(), m_members[i]->voice_token.c_str(), info.c_str());
				http::instance()->get_url_async(1, url);
#ifndef POKER_PUBLISH
				PRINT("%s\r\n", url.c_str());
#endif
			}
		}
		parent::on_enter(player);
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_leave(io_player::value_type player, bool is_exit)
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
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == player->get_uuid()){
				index = i;
				if (is_exit){
					m_members[i]->reset();
					m_members[i]->uuid = 0;
				} else {
					m_members[i]->time_enter = 0;
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
	void room_base::on_request(io_player::value_type player, protocol type, const Json::Value &data)
	{
		userid uuid = player->get_uuid();
		switch (type){
		case room_dismiss_request:
			on_dismiss(uuid, data);
			break;
		case room_dismiss_response:
			on_dismiss_reply(uuid, data);
			break;
		case room_ready:
			on_ready(uuid, data);
			break;
		case room_begin:
			on_round_begin(uuid, data);
			break;
		case room_broadcast:
			on_room_broadcast(uuid, data);
			break;
		}

		// 锁定状态下不处理逻辑协议
		if(!m_block_json.empty())
		{
			parent::on_request(player, type, data);
			return;
		}

		switch (type){
		case room_chu_pai:
			on_chu_pai(uuid, data);
			break;
		case room_chi_pai:
			on_chi_pai(uuid, data);
			break;
		case room_peng_pai:
			on_peng_pai(uuid, data);
			break;
		case room_gang_pai:
			on_gang_pai(uuid, data);
			break;
		case room_guo:
			on_guo(uuid, data);
			break;
		case room_hu:
			on_hu(uuid, data);
			break;

		}
		parent::on_request(player, type, data);
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_destroy(bool completed)
	{
		//统计用户的总成绩(多个游戏)
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == 0){
				continue;
			}
			userid uuid = m_members[i]->uuid;
			io_player::value_type player = io_player::find(uuid);
			if (player){
				player->set_score(m_members[i]->score);
			}
		}
		//如果需要支付且是大赢家支付
		if (m_round > 0 && m_zhifu == 3&&!is_free_time()){ //大赢家支付
			std::vector<int> big_winner;
			int max_score = 0;
			for (size_t i = 0; i < m_members.size(); i++){
				if (m_members[i]->uuid == 0){
					continue;
				}
				if (m_members[i]->score > max_score){
					big_winner.clear();
					big_winner.push_back(i);
					max_score = m_members[i]->score;
				} else if (m_members[i]->score == max_score){
					big_winner.push_back(i);
				}
			}
			for (size_t i = 0; i < big_winner.size(); i++){
				userid uuid = m_members[big_winner[i]]->uuid;
				deduct_diamond(uuid, m_diamond_pay*m_members.size()/big_winner.size());
			}
		}
		if(m_round && !completed)
			publish_result(-200, completed);
		parent::on_destroy(completed);
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_dismiss(userid uuid, const Json::Value &data)
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
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == uuid){
				set_dismiss();
				m_dismiss_index = i;
				m_members[i]->time_agree = time_now;
				break;
			}
		}
		time_t dead_line = time_now + dismiss_time_limit;
		notify["dead_line"]             = dead_line;
		notify["applicant"]["index"]    = m_dismiss_index;
		notify["applicant"]["time"]     = time_now;
		notify["applicant"]["uuid"]     = m_members[m_dismiss_index]->uuid;
		notify["applicant"]["nickname"] = m_members[m_dismiss_index]->nickname;
		notify["consenter"].append(m_dismiss_index);
		send_room(room_dismiss_request, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_dismiss_reply(userid uuid, const Json::Value &data)
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
				m_members[m_dismiss_index]->time_agree = 0;
				m_dismiss_index = -1;
			}
		} else { //同意解散
			bool is_all_agree = true;
			for (size_t i = 0; i < m_members.size(); i++){
				if (m_members[i]->uuid == 0){
					continue;
				} else if (m_members[i]->uuid == uuid){
					notify["consenter"].append(i);
					m_members[i]->time_agree = time(0);
				} else if (m_members[i]->time_agree == 0){
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
	void room_base::on_ready(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status > room_wait_ready){
			send((protocol)room_ready, notify, uuid, 1);
			return;
		}
		time_t time_now   = time(0);
		int  index_ready  = -1;
		int  count_ready  = 0;
		bool is_all_ready = true;
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == 0){
				is_all_ready = false;//所有位置都需要有人
			} else if (m_members[i]->uuid == uuid){
				index_ready = i;
				if (m_members[i]->time_ready == 0)
					count_ready++;
				m_members[i]->time_ready = time_now;
			} else if (m_members[i]->time_ready){
				count_ready++;
			} else {
				is_all_ready = false;
			}
		}

		notify.clear();
		notify["index"]      = index_ready;
		notify["uuid"]       = uuid;
		notify["time_ready"] = m_members[index_ready]->time_ready;
		notify["game_type"]   = get_game_type();
		send_room((protocol)room_ready, notify, 0);
		//如果所有人都准备且大于1人
		if (is_all_ready && count_ready > 1){
			if (m_round > 0){
				init_next_round(); //开始下一局
			} else {
				int index_first_ready = 0;
				time_t min_time_ready = 0;
				for (size_t i = 0; i < m_members.size(); i++){
					if (m_members[i]->time_ready){
						if (min_time_ready == 0){
							index_first_ready = i;
							min_time_ready = m_members[i]->time_ready;
						} else if (m_members[i]->time_ready < min_time_ready){
							index_first_ready = i;
							min_time_ready = m_members[i]->time_ready;
						}
					}
				}
				notify.clear();
			}
		}
	}
	void room_base::prepare_aglo(player_base* player)
	{
		//压入手牌
		m_algo->begin(player->hand, m_wang);

		//压入吃碰杠
		for(size_t i=0; i<player->group_cards.size();i++)
		{
			taoj_algo::set_t opened;
			opened.first = player->group_cards[i].card_id;
			switch (player->group_cards[i].type)
			{
			case group_type::type_shun:
				opened.type = taoj_algo::type_shun;
				break;
			case group_type::type_ke:
			case group_type::type_an_gang:
			case group_type::type_ming_gang:
			case group_type::type_bu_gang:
				opened.type = taoj_algo::type_ke;
				break;
			default:
				break;
			}
			m_algo->push(opened.first, opened.type);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_round_begin(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_ready){
			send((protocol)room_wait_ready, notify, uuid, 1);
			return;
		}
		int  count_ready = 0;
		bool is_all_ready = true;
		for (size_t i = 0; i < m_members.size(); i++){
			if (m_members[i]->uuid == 0){
				return;
			} else if (m_members[i]->time_ready){
				count_ready++;
			} else {
				is_all_ready = false;
			}
		}
		if (!is_all_ready){ //不是所有人都准备好了
			send((protocol)room_begin, notify, uuid, 20);
			return;
		}
		if (count_ready < 2){ //至少要有2个人准备好了才能开局
			send((protocol)room_begin, notify, uuid, 30);
			return;
		}
		init_next_round(); //初始化下一局(在这里是第一局)
	}

	////////////////////////////////////////////////////////////////////////////////
	void room_base::on_room_broadcast(userid uuid, const Json::Value &data)
	{
		send_room((protocol)room_broadcast, data, 0);
	}

	void room_base::on_chu_pai(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		int retCode = 0;
		int card_id = data["id"].asInt();
		int user_index = get_index_by_uuid(uuid);
		if(user_index != m_cur_player)
			ERROR_BREAK(1);

		player_base* player = m_members[user_index];
		if(!player->is_have_card(card_id))
			ERROR_BREAK(2);

		//没有出牌权限
		if(!player->check_right(right_type::chu))
			ERROR_BREAK(3);

		//出牌后不可能在倒地胡
		m_enable_tian_hu = false;

		//重置,过胡标志
		m_members[user_index]->guo_hu = false;

		m_last_out_player = user_index;

		player->last_out_card = card_id;
		player->last_get_card = 0;
		//移除手牌
		player->remove_card(card_id);
		player->add_out_card(card_id);

		//清除权限
		clear_all_rights();

		//广播出牌协议
		notify["index"] = user_index;
		notify["card"] = card_id;
		notify["force"] = data["force"].asBool();
		send_room((protocol)room_chu_pai, notify, 0);

		notify["type"] = history_type::chu_pai;
		//player->set_rights(notify);
		m_history.append(notify);

		//清理等待操作队列
		m_right_wait.clear();

		//检测权限
		if(!deal_chupai(user_index, card_id))
		{
			turn_to_next_player();//切换到下个玩家
			send_wait_operation();
		}

		return;
__RESULT:
		send((protocol)room_chu_pai, notify, uuid, retCode);
	}

	void room_base::on_chi_pai(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		int retCode = 0;
		int card_id = data["card"].asInt();
		int user_index = get_index_by_uuid(uuid);

		player_base* player = m_members[user_index];

		if(!m_enable_chi)
			ERROR_BREAK(1);

		//没有吃牌权限
		if(!player->check_right(right_type::chi))
			ERROR_BREAK(3);

		//吃牌牌值存在性判断
		if(!player->check_chi_enable(card_id, m_members[m_last_out_player]->last_out_card))
			ERROR_BREAK(4);

		//清理本人权限
		player->clear_rights();

		//检测吃牌请求
		if(!check_right_priority(user_index, right_type::chi, card_id)) //有比吃牌权限高的
		{
			notify["drop"] = true;
			send((protocol)room_chi_pai, notify, uuid, retCode);
			notify["index"] = user_index;
			notify["type"] = history_type::chi_pai;
			notify["show"] = true;
			player->set_rights(notify);
			m_history.append(notify);
		}

		return;
__RESULT:
		send((protocol)room_chi_pai, notify, uuid, retCode);
	}

	void room_base::on_peng_pai(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);

		player_base* player = m_members[user_index];
		//没有碰牌权限
		if(!player->check_right(right_type::peng))
			ERROR_BREAK(3);

		//是否可碰
		if(!player->check_peng_enable(m_members[m_last_out_player]->last_out_card))
			ERROR_BREAK(4);

		//清理权限
		player->clear_rights();

		//检测出牌请求
		if(!check_right_priority(user_index, right_type::peng, m_members[m_last_out_player]->last_out_card)) //碰牌失败
		{
			notify["drop"] = true;
			send((protocol)room_peng_pai, notify, uuid, retCode);

			notify["show"] = true;
			notify["type"] = history_type::peng_pai;
			notify["index"] = user_index;
			player->set_rights(notify);
			m_history.append(notify);
		}

		return;
__RESULT:
		send((protocol)room_peng_pai, notify, uuid, retCode);
	}

	void room_base::on_gang_pai(userid uuid, const Json::Value &data){
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

		m_members[m_cur_player]->guo_hu = false;

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

	void room_base::on_guo(userid uuid, const Json::Value &data)
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

	void room_base::on_hu(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		int retCode = 0;
		int user_index = get_index_by_uuid(uuid);
		player_base* player = m_members[user_index];
		if(!player->check_right(right_type::hu))
			ERROR_BREAK(1);

		player->clear_rights();
		if(m_cur_player == user_index)
		{
			do_hu(user_index, true);
			publish_result(user_index, m_round == m_round_total);
		}
		else
		{
			//检测出牌请求
			if(!check_right_priority(user_index, right_type::hu, 0)) //碰牌失败
			{
				notify["drop"] = true;
				notify["index"].append(user_index);
				send((protocol)room_hu, notify, uuid, retCode);
				notify["type"] = history_type::hu_pai;
				notify["op_index"] = user_index;
				notify["show"] = true;
				player->set_rights(notify);
				m_history.append(notify);
			}
		}
		return;
__RESULT:
		send((protocol)room_hu, notify, uuid, retCode);
	}

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	void room_base::init_next_mahjong()
	{
		//初始化牌局参数
		m_next = 0;
		m_mahjong.clear();

		for(int i=0; i < 4; i++)
		{
			for(int j=0; j<38; j++)
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
	void room_base::init_user_mahjong()
	{
		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			player->clear_hand();
		}

		for(int j=0; j<13; j++)
		{
			for(size_t i=0; i<m_members.size(); i++)
			{
				player_base* player = m_members[i];
				player->hand[m_mahjong[m_next]]++;
				player->init_hand[m_mahjong[m_next]]++; //初始手牌同步赋值,用于战绩
				m_next++;
			}
		}

#ifdef _DEBUG
		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			player->clear_hand();
		}
		m_members[0]->hand[2] = 3;
		m_members[0]->hand[1] = 1;

		group_info gf;
		gf.type = group_type::type_shun;
		gf.card_id = 24;
		gf.list.push_back(24);
		gf.list.push_back(26);
		gf.list.push_back(25);

		m_members[0]->group_cards.push_back(gf);

		group_info gf1;
		gf1.type = group_type::type_ke;
		gf1.card_id = 23;
		gf1.list.push_back(23);
		m_members[0]->group_cards.push_back(gf1);

		group_info gf2;
		gf2.type = group_type::type_ming_gang;
		gf2.card_id = 19;
		gf2.list.push_back(19);
		m_members[0]->group_cards.push_back(gf2);
		//m_members[0]->hand[3] = 1;
		//m_members[0]->hand[4] = 2;
		//m_members[0]->hand[5] = 1;
		//m_members[0]->hand[6] = 1;
		//m_members[0]->hand[7] = 2;
		//m_members[0]->hand[8] = 2;
		//m_members[0]->hand[9] = 2;


		if(m_members.size() >= 3)
		{
			m_members[2]->hand[1] = 2;
			m_members[2]->hand[2] = 2;
		}
		if(m_members.size() >= 4)
		{
			m_members[3]->hand[1] = 3;
			m_members[3]->hand[2] = 1;
		}


		m_members[1]->hand[2] = 2;
		m_members[1]->hand[1] = 2;

		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			for(int j=0; j<49; j++)
			{
				player->init_hand[j] = player->hand[j];
			}
		}



#endif // _DEBUG
	}
	////////////////////////////////////////////////////////////////////////////////
	void room_base::send_user_mahjong()
	{
		//根据不同的抢庄类型选择下一步的操作
		for (size_t i = 0; i < m_members.size(); i++){
			player_base* player = m_members[i];
			if (player->uuid == 0){
				continue; //没参与牌局
			}
			Json::Value notify;
			for (size_t j = 0; j < 49; j++){
				for(int k=0; k<player->hand[j]; k++)
				{
					notify["hand"].append(j);
				}
			}
			userid uuid = player->uuid;
			send((protocol)room_mahjongs, notify, uuid, 0);
		}
	}

	void room_base::set_room_status(room_state status)
	{
		set_dirty();
		m_idle_time = 0;
		m_status    = status;
		//广播状态改变消息
		Json::Value notify;
		notify["room_state"] = status;
		send_room((protocol)room_status_change, notify, 0);
	}


	void room_base::turn_to_next_player()
	{
		if(get_left_card_number() <= 0)
		{
			publish_result(-1, m_round >= m_round_total);
			//流局处理
			return;
		}

		//清理杠子
		m_gangzi.clear();
		m_niao.clear();

		while(true)
		{
			m_cur_player++;
			if(m_cur_player >= (int)m_members.size())
				m_cur_player = 0;
			if(m_members[m_cur_player]->uuid !=0)
				break;
		}
		m_right_wait.clear();

		//发牌
		fa_pai();

		player_base* player = m_members[m_cur_player];
		//听牌后不能胡,则自动出牌
		if(player->is_ting && player->right_list.size() < 2)//听牌后不能胡,则自动出牌o
		{
			Json::Value hPack;
			hPack["type"] = (protocol)room_chu_pai;
			hPack["id"] = player->last_get_card;
			hPack["force"] = true;
			hPack["uuid"] = player->uuid;
			m_block_json.push_back(hPack);
		}
#ifdef AUTO_PLAY_CARD
		else{
			if(player->right_list.size() < 2)
			{
				Json::Value hPack;
				hPack["type"] = (protocol)room_chu_pai;
				hPack["id"] = player->last_get_card;
				hPack["force"] = true;
				hPack["uuid"] = player->uuid;
				m_block_json.push_back(hPack);
			}
		}
#endif

	}

	void room_base::check_mo_pai_right(player_base* player)
	{
		//还原为摸牌前的手牌
		player->hand[player->last_get_card]--;
		prepare_aglo(player);

		//判胡
		if(m_algo->finish(player->last_get_card, true, player->first_operation))
			player->add_right(right_type::hu);

		player->hu_card = player->last_get_card;

		//起手胡判断
		if(m_enable_tian_hu)
		{
			if(check_qishouhu(player))
				player->add_right(right_type::hu);
		}

		//还原为摸牌后的手牌
		player->hand[player->last_get_card]++;
	}

	void room_base::check_my_turn_gang(player_base* player){

		//已经有杠权限了,取消自检
		if(player->check_right(right_type::gang))
			return;

		//检测手牌有没有4张一样的
		for(int i=0; i<49; i++)
		{
			if(i == m_wang)
				continue;

			if(player->hand[i] == 4)
			{
				player->add_right(right_type::gang);
				player->add_can_gang(i);
			}
		}


		//判断有没有能成杠的刻字
		for (size_t i=0; i<player->group_cards.size(); i++)
		{
			if(player->group_cards[i].type == group_type::type_ke)
			{
				int card_id = player->group_cards[i].card_id;
				//如果禁止3张碰后补杠,则跳过
				if(std::find(player->forbidden_gang.begin(), player->forbidden_gang.end(), card_id) != player->forbidden_gang.end())
					continue;

				if(player->hand[card_id] !=1)
					continue;


				player->add_right(right_type::gang);
				player->add_can_gang(player->group_cards[i].card_id);
			}
		}
	}

	int room_base::find_next_player()
	{
		int next_index = m_cur_player;
		while(true)
		{
			next_index++;
			if(next_index >= (int)m_members.size())
				next_index = 0;
			if(m_members[next_index]->uuid !=0)
				break;
		}

		return next_index;
	}

	//给某个人发牌
	void room_base::fa_pai()
	{
		int new_card = m_mahjong[m_next++];
#ifdef _DEBUG
		new_card = 2;
#endif // _DEBUG

		//摸牌权限判断g
		m_members[m_cur_player]->add_card(new_card);

		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			if(player->uuid == 0)
				continue;

			Json::Value notify;
			notify["index"]	= m_cur_player;
			if(i == m_cur_player)
				notify["card"] = new_card;
			send((protocol)room_fa_pai, notify, player->uuid, 0);
		}

		Json::Value hs;
		hs["index"] = m_cur_player;
		hs["card"] = new_card;
		hs["type"] = history_type::mo_pai;
		m_history.append(hs);


		//增加出牌权限
		m_members[m_cur_player]->add_right(right_type::chu);

		//检查自身杠
		check_my_turn_gang(m_members[m_cur_player]);

		//摸牌后权限处理
		check_mo_pai_right(m_members[m_cur_player]);

		//发送给当前玩家他的权限
		send_operation(m_cur_player);
		//通知其他人当前操作的玩家
		send_wait_operation(false);
	}

	void room_base::send_operation(int index)
	{
		player_base* player = m_members[index];
		if(player->uuid == 0)
			return;

		if(player->right_list.empty())
			return;

		Json::Value hs;

		Json::Value notify;
		notify["index"] = index;
		for(size_t j=0; j<player->right_list.size(); j++)
		{
			notify["rights"].append(player->right_list[j]);
		}

		notify["cph"] = player->hu_card;
		if(!player->gang_list.empty())
		{
			for(size_t i=0; i<player->gang_list.size();i++)
			{
				notify["gang"].append(player->gang_list[i]);
			}
		}

		//notify["type"] = history_type::wait_operation;
		//m_history.append(notify);
		send((protocol)room_wait_operation, notify, player->uuid, 0);
	}

	void room_base::send_wait_operation(bool all)
	{
		Json::Value hPack;
		hPack["index"] = m_cur_player;
		hPack["rights"].append(right_type::chu);
		for(size_t i=0; i<m_members.size(); i++)
		{
			if(i == m_cur_player && !all)
				continue;

			player_base* player = m_members[i];
			if(player->uuid == 0)
				continue;

			send((protocol)room_wait_operation, hPack, player->uuid, 0);
		}

	}

	bool room_base::check_player_hu(int index, int card_id, bool gang)
	{
		if(index < 0 || index >= (int)m_members.size())
			return false;


		//检测上个出牌人能不能胡
		player_base* player = m_members[index];
		prepare_aglo(player);

		bool send = true;
		if(player->check_right(right_type::hu))
			send = false;
		//判胡
		if(!check_guo_hu(index) && m_algo->finish(card_id, false, player->first_operation, gang?mj_gang::gang_qiang:mj_gang::gang_not))
		{
			player->add_right(right_type::hu);
			player->hu_card = card_id;
		}

		if(player->get_rights_count() > 0 )
		{
			if(send)
				send_operation(index);

			return true;
		}

		return false;
	}

	bool room_base::check_guo_hu(int index){
		if(index < 0 
			||index >= (int)m_members.size())
			return false;
		return m_members[index]->guo_hu;
	}

	bool room_base::check_other_hu(int except_index, int card_id, bool gang)
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
			//判胡
			if(!check_guo_hu(i) && m_algo->finish(card_id, false, player->first_operation, gang?mj_gang::gang_qiang:mj_gang::gang_not))
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

	char room_base::get_rand_dice(char &small_shaizi)
	{
		int r1 = (rand() % 6) + 1;
		int r2 = (rand() % 6) + 1;
		if (r1 < r2){
			small_shaizi = r1;
		} else {
			small_shaizi = r2;
		}
		return (r1 + r2);
	}

	int room_base::get_player_number()
	{
		int count = 0;
		for(size_t i=0; i< m_members.size(); i++)
		{
			if(m_members[i]->uuid != 0)
				count++;
		}

		return count;
	}

	int room_base::get_left_card_number(){
		return m_mahjong.size() - m_next;
	}

	int room_base::get_index_by_uuid(userid uuid)
	{
		for(size_t i=0;i<m_members.size();i++)
		{
			if(m_members[i]->uuid == uuid)
				return i;
		}

		return -1;
	}


	void room_base::do_gang(int user_index, int card_id, bool zimo, bool direct)
	{
		//杠牌后不可能在倒地胡
		m_enable_tian_hu = false
			;
		group_type gang_type = m_members[user_index]->do_gang(card_id, zimo);
		Json::Value notify;
		notify["index"] = user_index;
		notify["card"] = card_id;
		notify["type"] = gang_type;
		if(zimo)
			notify["from"] = m_cur_player;
		else
			notify["from"] = m_last_out_player;

		switch(gang_type)
		{
		case group_type::type_an_gang:
			{
				m_members[user_index]->angang_num++;
				m_members[user_index]->angang_total_num++;
			}
			break;
		case group_type::type_ming_gang:
			{
				m_members[user_index]->minggang_num++;
				m_members[user_index]->minggang_total_num++;

				m_members[m_cur_player]->diangang_num++;
				m_members[m_cur_player]->diangang_total_num++;
			}
			break;
		case group_type::type_bu_gang:
			{
				m_members[user_index]->bugang_num++;
				m_members[user_index]->bugang_total_num++;

				notify["type"] = type_ming_gang;
			}
			break;
		}


		for(size_t i=0; i<m_members.size(); i++)
		{
			player_base* player = m_members[i];
			notify["card"] = card_id;
			send((protocol)room_gang_pai, notify, player->uuid, 0);
		}

		notify["gtype"] = gang_type;
		if(gang_type == group_type::type_bu_gang)
			notify["gtype"] = type_ming_gang;

		notify["type"] = history_type::gang_pai;
		notify["show"] = direct;
		m_members[user_index]->set_rights(notify);
		m_history.append(notify);

		m_cur_player = user_index;
		if(!zimo)
		{
			if(m_last_out_player > 0)
				m_members[m_last_out_player]->remove_last_out_card();
		}
		else{

			//红中,转转抢补杠胡判断
			if(gang_type == group_type::type_bu_gang)
			{
				//转转需要点炮选项打开才能抢杠胡
				if(get_game_type() == game_type::room_type_zhong
					||(get_game_type() == game_type::room_type_zhuanzhuan && m_enable_dian_pao))
				{
					//有人抢杠胡,则打断摸牌
					if(check_other_hu(m_cur_player, card_id))
					{
						m_right_wait.gang_index = m_cur_player;
						return;
					}
				}
			}
		}



		if(get_left_card_number() < 1)
		{
			publish_result(-1, m_round >= m_round_total);
			return;
		}

		fa_pai();
	}

	void room_base::do_chi(int user_index, int begin, int out_card, bool direct)
	{
		int old_pos = m_cur_player;
		//出牌方,移除最后一个出牌区的牌
		m_members[m_last_out_player]->remove_last_out_card();
		//吃牌方进行吃牌处理
		std::vector<int>& list = m_members[user_index]->do_chi(begin, out_card);
		m_cur_player = user_index;

		Json::Value hPack;
		hPack["card"] = out_card;
		hPack["index"] = user_index;
		hPack["from"] = old_pos;

		for(size_t i=0; i<list.size(); i++)
		{
			hPack["list"].append(list[i]);
		}
		send_room((protocol)room_chi_pai, hPack, 0);
		hPack["type"] = history_type::chi_pai;
		hPack["show"] = direct;
		m_members[user_index]->set_rights(hPack);
		m_history.append(hPack);

		//检测自己的杠
		check_my_turn_gang(m_members[m_cur_player]);
		//添加出牌权限
		m_members[m_cur_player]->add_right(right_type::chu);

		//检测胡碰权限
		check_mo_pai_right(m_members[m_cur_player]);

		send_operation(m_cur_player);
		send_wait_operation();
	}

	void room_base::do_peng(int user_index, int out_card, bool direct)
	{
		int old_pos = m_last_out_player;
		m_members[m_last_out_player]->remove_last_out_card();

		m_members[user_index]->do_peng(out_card);
		m_cur_player = user_index;

		Json::Value hPack;
		hPack["id"] = out_card;
		hPack["index"] = user_index;
		hPack["from"] = old_pos;
		send_room((protocol)room_peng_pai, hPack, 0);
		hPack["type"] = history_type::peng_pai;
		hPack["show"] = direct;
		m_members[user_index]->set_rights(hPack);
		m_history.append(hPack);

		check_my_turn_gang(m_members[m_cur_player]);

		//添加出牌权限
		m_members[m_cur_player]->add_right(right_type::chu);

		//检测胡碰权限
		check_mo_pai_right(m_members[m_cur_player]);

		send_operation(m_cur_player);
		send_wait_operation();
	}

	void room_base::clear_all_rights()
	{
		for(size_t i=0; i<m_members.size(); i++)
		{
			m_members[i]->clear_rights();
		}
	}

	bool room_base::check_right_priority(int user_index, right_type type, int card_id)
	{
		bool direct = m_right_wait.index != user_index;
		if(type > right_type::none)
		{
			if(type > m_right_wait.right) //权限高,则直接替换
			{
				m_right_wait.index = user_index;
				m_right_wait.right = type;
				if(card_id)
					m_right_wait.card_id = card_id;

			}
			else if(type == m_right_wait.right)//相同权限,比较玩家座次优先级
			{
				if(check_index_priority(user_index, m_right_wait.index))
				{
					m_right_wait.index = user_index;
					m_right_wait.right = type;
					if(card_id)
						m_right_wait.card_id = card_id;
				}	
			}

			//记录有人杠过,用于抢杠胡判断
			if(m_right_wait.right == right_type::gang)
				m_right_wait.gang_index = m_right_wait.index;
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
			m_members[m_right_wait.index]->is_ting = true;
			do_gang(m_right_wait.index, m_right_wait.card_id, false, direct);
			break;
		case right_type::none:
			{
				if(m_right_wait.gang_index < 0)
				{
					//切换到下个玩家
					turn_to_next_player();
				}
			}
			break;
		case right_type::hu:
			{
				do_hu(m_right_wait.index, direct);
				publish_result(m_right_wait.index, m_round >= m_round_total);
			}
			break;
		}

		if(m_status != room_state::room_playing)
			return true;

		bool no_response = m_right_wait.index == user_index;

		if(m_right_wait.gang_index >= 0)
		{
			m_members[m_right_wait.gang_index]->is_ting = true;
			do_gang(m_right_wait.gang_index, m_right_wait.card_id, true, true);
			//摸杠 抢杠胡人点过
		}
		m_right_wait.clear();

		//如果本次处理的行为,与发起者的操作一样,如发起吃,自己吃或别人吃都会返回吃的协议,从而解决客户端协议返回需求卡死问题, 
		//否则需要给发起行为返回一个空的对应协议
		return no_response;
	}
	bool room_base::check_index_priority(int src_index, int dst_index)
	{
		if(src_index == m_cur_player)
			return true;

		while (true)
		{
			src_index++;
			if(src_index >= (int)m_members.size())
				src_index = 0;

			if(src_index == dst_index)
				return true;

			if(src_index == m_cur_player)
				break;
		}

		return false;
	}

	bool room_base::deal_chupai(int deal_pos, int card_id)
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

			if(m_enable_dian_pao)
			{
				if(!check_guo_hu(cur_pos) && m_algo->finish(card_id, false, player->first_operation))
					player->add_right(right_type::hu);

			}

			if(next_player_index == cur_pos)
			{
				//判吃
				if(m_algo->enable(card_id, taoj_algo::type_shun))
					player->add_right(right_type::chi);
			}

			//判碰
			if(m_algo->enable(card_id, taoj_algo::type_ke))
				player->add_right(right_type::peng);


			player->hu_card = card_id;

			//判杠
			if(m_algo->enable(card_id, false))
			{
				player->add_right(right_type::gang);
				player->add_can_gang(card_id);
			}


			if(player->get_rights_count() > 0)
			{
				someone_have_right = true;
				send_operation(cur_pos);
			}

		}
		return someone_have_right;
	}

	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
