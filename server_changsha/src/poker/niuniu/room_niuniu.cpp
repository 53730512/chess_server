

#include <algorithm>
#include <random>
#include "protocol.h"
#include "../../io_handler.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
	////////////////////////////////////////////////////////////////////////////////
	Json::Value niuniu_options;
#ifdef _DEBUG
	const time_t dismiss_time_limit = 10;
#else
	const time_t dismiss_time_limit = 60;
#endif
	static std::random_device rd;
	static std::mt19937 mt(rd());
	static const int max_idle_time = 10000; //10秒
	const char *niuniu_options_file = "../rules/niuniu/douniu.json";
	////////////////////////////////////////////////////////////////////////////////
	game_room::game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
		: room_basic(creater, type, ruleid, number, time_now, owner_exit)
	{
		m_idle_time       = 0;
		m_round           = 0;
		m_round_total     = 0;
		m_zhifu           = 0;
		m_fanbei          = 0;
		m_diamond_pay     = 0;
		m_diamond_huanpai = 0;
		m_huan_zhuang     = 0;
		m_index_zhuang    = 0;
		m_dismiss_index   = -1;
		m_status          = room_wait_ready;
		m_next            = 0;
		m_no_flower       = false;
		m_need_payment    = false;
		m_time_limit      = false;
		m_qiang_zhuang    = false;
		m_xiazhu.clear();
		m_option.clear();
		memset(m_pokers, 0, sizeof(m_pokers));
		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].reset();
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_rules(const Json::Value &option)
	{
		if (niuniu_options.isNull()){
			FILE *fpread = fopen(niuniu_options_file, "r");
			if (!fpread){
				return false;
			}
			io::stringc content;
			while (!feof(fpread)){
				char buffer[8192];
				size_t n = fread(buffer, 1, sizeof(buffer), fpread);
				content.append(buffer, n);
			}
			fclose(fpread);
			Json::Reader reader;
			if (!reader.parse(content, niuniu_options)){
				return false;
			}
		}
		int  jushu   = option["jushu"].asInt();
		int  zhifu   = option["zhifu"].asInt();
		int  fanbei  = option["fanbei"].asInt();
		int  qiang   = option["qiangzhuang"].asInt();
		int  xiazhu  = option["xiazhu"].asInt();
		bool huanpai = option["huanpai"].asBool(); //是否可换牌
		m_time_limit = option["timeout"].asBool(); //操作限时
		m_no_flower  = option["wuhua"].asBool();   //没有花牌
		//检查是否存在该规则编号
		io::stringc rule_id;
		rule_id.format("%d", get_rule_id());
		if (!niuniu_options.isMember(rule_id)){
			return false;
		}
		//读取局数信息
		Json::Value &rules = niuniu_options[rule_id];
		if (jushu < 0 || jushu >= (int)rules["jushu"].size()){
			return false;
		}
		m_round       = 0;
		m_huan_zhuang = rules["huanzhuang"].asInt();
		m_round_total = rules["jushu"][jushu].asInt();
		m_fanbei      = rules["fanbei"][fanbei].asInt();
		//新初始化的房间默认抢庄(除了随机庄模式)
		if (m_huan_zhuang == zhuang_qiang1 || m_huan_zhuang == zhuang_qiang2){
			m_qiang_zhuang = true;
		} else {
			m_qiang_zhuang = false;
		}
		//读取支付方式信息
		if (zhifu < 0 || zhifu >= (int)rules["zhifu"].size()){
			return false;
		}
		m_zhifu = rules["zhifu"][zhifu][0].asInt();
		m_diamond_pay = rules["zhifu"][zhifu][1].asInt();
		m_diamond_pay *= (jushu + 1);
		set_pay_type(m_zhifu);
		//读取番型分数信息
		if (fanbei < 0 || fanbei >= (int)rules["fanbei"].size()){
			return false;
		}
		//读取下注分值信息
		int index = m_fanbei - 1;
		if (xiazhu < 0 || xiazhu >= (int)rules["xiazhu"][index].size()){
			return false;
		}
		for (Json::ArrayIndex i = 0; i < rules["xiazhu"][index][xiazhu].size(); i++){
			int v = rules["xiazhu"][index][xiazhu][i].asInt();
			if (v < 0){
				v *= (-1);
			}
			m_xiazhu.push_back(v);
		}
		if (qiang < 0 || qiang >= (int)rules["qiangzhuang"][index].size()){
			return false;
		}
		for (Json::ArrayIndex i = 0; i <= (Json::ArrayIndex)qiang; i++){
			int v = rules["qiangzhuang"][index][i].asInt();
			if (v < 0){
				v *= (-1);
			}
			m_qiang.push_back(v);
		}
		//读取是否允许换牌信息
		if (huanpai){
			if (rules["huanpai"][0].asInt() == 0)
				return false;
			m_diamond_huanpai = rules["huanpai"][1].asInt();
		}
		return (m_option = option), true;
	}
	////////////////////////////////////////////////////////////////////////////////
	bool game_room::init_room_context(const Json::Value &context)
	{
		m_dismiss_index = -1;
		if (context["is_opening"].asBool()){ //已开局
			set_open();
		}
		m_status          = (room_state)context["status"].asInt();
		m_round           = context["round"].asInt();
		m_round_total     = context["round_total"].asInt();
		m_zhifu           = context["zhifu"].asInt();
		m_fanbei          = context["fanbei"].asInt();
		m_diamond_pay     = context["diamond_pay"].asInt();
		m_diamond_huanpai = context["diamond_huanpai"].asInt();
		m_huan_zhuang     = context["huan_zhuang"].asInt();
		m_index_zhuang    = context["index_zhuang"].asInt();
		m_next            = context["next_index"].asInt();
		m_need_payment    = context["need_payment"].asBool();
		m_time_limit      = context["time_limit"].asBool();
		m_no_flower       = context["no_flower"].asBool();
		m_qiang_zhuang    = context["qiang_zhuang"].asBool();
		//导入牌池数据
		for (size_t i = 0; i < 54; i++){
			m_pokers[i] = context["pokers"][(int)i].asInt();
		}
		//导入抢庄倍率
		if (context.isMember("qiang") && context["qiang"].isArray()){
			m_qiang.clear();
			for (Json::ArrayIndex i = 0; i < context["qiang"].size(); i++){
				m_qiang.push_back(context["qiang"][i].asInt());
			}
		}
		//导入下注倍率
		if (context.isMember("xiazhu") && context["xiazhu"].isArray()){
			m_xiazhu.clear();
			for (Json::ArrayIndex i = 0; i < context["xiazhu"].size(); i++){
				m_xiazhu.push_back(context["xiazhu"][i].asInt());
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
			member.is_show_hand = members[i]["is_show_hand"].asBool();
			member.show_count   = members[i]["show_count"].asInt();
			member.switch_poker = members[i]["switch_poker"].asInt();
			member.switch_count = members[i]["switch_count"].asInt();
			member.switch_style = (bull_type)members[i]["switch_style"].asInt();
			member.xia_zhu      = members[i]["xia_zhu"].asInt();
			member.qiang_zhuang = members[i]["qiang_zhuang"].asInt();
			member.bull_type    = (bull_type)members[i]["bull_type"].asInt();
			for (int j = 0; j < 5; j++){
				member.hand.set_poker(j, members[i]["hand"][j].asInt());
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
		result["option"]                     = m_option;
		result["context"]["round"]           = m_round;
		result["context"]["round_total"]     = m_round_total;
		result["context"]["zhifu"]           = m_zhifu;
		result["context"]["fanbei"]          = m_fanbei;
		result["context"]["diamond_pay"]     = m_diamond_pay;
		result["context"]["diamond_huanpai"] = m_diamond_huanpai;
		result["context"]["huan_zhuang"]     = m_huan_zhuang;
		result["context"]["status"]          = m_status;
		result["context"]["index_zhuang"]    = m_index_zhuang;
		result["context"]["next_index"]      = m_next;
		result["context"]["need_payment"]    = m_need_payment;
		result["context"]["time_limit"]      = m_time_limit;
		result["context"]["is_opening"]      = is_opening();
		result["context"]["no_flower"]       = m_no_flower;
		result["context"]["qiang_zhuang"]    = m_qiang_zhuang;
		//导出牌池数据
		for (size_t i = 0; i < 54; i++){
			result["context"]["pokers"].append(m_pokers[i]);
		}
		//导出抢庄倍率
		for (size_t i = 0; i < m_qiang.size(); i++){
			result["context"]["qiang"].append(m_qiang[i]);
		}
		//导出下注倍率
		for (size_t i = 0; i < m_xiazhu.size(); i++){
			result["context"]["xiazhu"].append(m_xiazhu[i]);
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
			member["is_show_hand"] = m_members[i].is_show_hand;
			member["show_count"]   = m_members[i].show_count;
			member["switch_poker"] = m_members[i].switch_poker;
			member["switch_count"] = m_members[i].switch_count;
			member["switch_style"] = m_members[i].switch_style;
			member["xia_zhu"]      = m_members[i].xia_zhu;
			member["qiang_zhuang"] = m_members[i].qiang_zhuang;
			member["bull_type"]    = m_members[i].bull_type;
			for (int j = 0; j < 5; j++){
				poker::value_t v = m_members[i].hand.get_poker(j);
				member["hand"].append(v);
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
			result.append(score);
		}
		io::stringc output(result.toFastString().c_str());
		return std::move(output);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_create()
	{
		m_intervene = true; //默认干预输赢比率
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
		if (!is_opening() || is_completed() || is_dismissed()){
			return;
		}
		//如果选择了超时设置
		if (m_time_limit){
			m_idle_time += delta;
			if (m_idle_time >= max_idle_time + 5000){ //延迟5秒处理
				m_idle_time = 0;
				switch (m_status){
				case room_wait_ready: auto_send_ready();
					break;
				case room_wait_qiang_zhuang: auto_send_qiang();
					break;
				case room_wait_xia_zhu: auto_send_xiazhu();
					break;
				case room_wait_show_poker: auto_send_publish();
					break;
				default: break;
				}
			}
		}
		//抢庄状态(等待抢庄)
		if (m_status == room_wait_qiang_zhuang){
			if (m_qiang_zhuang){
				return;
			}
			//轮流坐庄
			if (m_huan_zhuang == zhuang_order){
				int index = -1;
				if (m_round == 1){
					for (int i = 0; i < MAX_MEMBERS; i++){
						if (m_members[i].uuid == 0){
							continue;
						}
						if (index < 0){
							index = i;
						} else if (m_members[i].time_enter < m_members[index].time_enter){
							index = i;
						}
					}
				} else {
					index = m_index_zhuang + 1;
					index %= MAX_MEMBERS;
					for (int i = 0; i < MAX_MEMBERS; i++){
						int j = (index + i) % MAX_MEMBERS;
						if (m_members[j].uuid > 0){
							index = j;
							break;
						}
					}
				}
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue;
					}
					if (m_members[i].qiang_zhuang){
						continue;
					}
					Json::Value notify;
					if (i == index){
						notify["value"] = m_qiang[0];
					} else {
						notify["value"] = -1;
					}
					on_qiang_zhuang(m_members[i].uuid, notify);
				}
			}
			//如果随机当庄
			else if (m_huan_zhuang == zhuang_random){
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue;
					}
					if (m_members[i].qiang_zhuang){
						continue;
					}
					Json::Value notify;
					notify["value"] = m_qiang[0];
					on_qiang_zhuang(m_members[i].uuid, notify);
				}
			}
			//牛牛上庄
			else if (m_huan_zhuang == zhuang_niuniu){
				int index = -1;
				if (m_round == 1){
					for (int i = 0; i < MAX_MEMBERS; i++){
						if (m_members[i].uuid == 0){
							continue;
						}
						if (index < 0){
							index = i;
						} else if (m_members[i].time_enter < m_members[index].time_enter){
							index = i;
						}
					}
				} else {
					index = m_index_zhuang;
				}
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue;
					}
					if (m_members[i].qiang_zhuang){
						continue;
					}
					Json::Value notify;
					if (i == index){
						notify["value"] = m_qiang[0];
					} else {
						notify["value"] = -1;
					}
					on_qiang_zhuang(m_members[i].uuid, notify);
				}
			}
		}
		//下注状态(等待下注)
		else if (m_status == room_wait_xia_zhu){
			if (m_xiazhu.size() == 1){ //只有一种选择
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue;
					}
					if (m_members[i].xia_zhu){
						continue;
					}
					Json::Value notify;
					notify["value"] = m_xiazhu[0];
					on_xia_zhu(m_members[i].uuid, notify);
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
			index = empty_index[index];
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
		notify["time_limit"]  = max_idle_time / 1000;
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
		case niuniu_ready:
			on_ready(uuid, data);
			break;
		case niuniu_round_begin:
			on_round_begin(uuid, data);
			break;
		case niuniu_qiang_zhuang:
			on_qiang_zhuang(uuid, data);
			break;
		case niuniu_xia_zhu:
			on_xia_zhu(uuid, data);
			break;
		case niuniu_huan_pai:
			on_huan_pai(uuid, data);
			break;
		case niuniu_publish_hand:
			on_publish_hand(uuid, data);
			break;
		case niuniu_room_broadcast:
			on_room_broadcast(uuid, data);
			break;
		case niuniu_status_timeout:
			on_status_timeout(uuid, data);
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
	void game_room::auto_send_ready()
	{
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_status != room_wait_ready){
				return;
			}
			if (m_members[i].uuid == 0){
				continue;
			}
			if (m_members[i].time_ready == 0){
				Json::Value notify;
				on_ready(m_members[i].uuid, notify);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_ready(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status > room_wait_begin){
			send((protocol)niuniu_ready, notify, uuid, 1);
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
		send_room((protocol)niuniu_ready, notify, 0);
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
				set_room_status(room_wait_begin);
				userid uuid = m_members[index_first_ready].uuid;
				send((protocol)niuniu_begin, notify, uuid, 0);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_round_begin(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_begin){
			send((protocol)niuniu_round_begin, notify, uuid, 1);
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
			send((protocol)niuniu_round_begin, notify, uuid, 20);
			return;
		}
		if (count_ready < 2){ //至少要有2个人准备好了才能开局
			send((protocol)niuniu_round_begin, notify, uuid, 30);
			return;
		}
		init_next_round(); //初始化下一局(在这里是第一局)
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::auto_send_qiang()
	{
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_status != room_wait_qiang_zhuang){
				return;
			}
			if (m_members[i].uuid == 0){
				continue;
			}
			if (m_members[i].qiang_zhuang == 0){
				Json::Value notify;
				notify["value"] = -1;
				on_qiang_zhuang(m_members[i].uuid, notify);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_qiang_zhuang(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_qiang_zhuang){
			send((protocol)niuniu_qiang_zhuang, notify, uuid, 1);
			return;
		}
		int qiang_zhuang = data["value"].asInt();
		if (qiang_zhuang == 0){
			send((protocol)niuniu_qiang_zhuang, notify, uuid, 2);
			return;
		} else if (qiang_zhuang > 0){
			bool finded = false;
			for (size_t i = 0; i < m_qiang.size(); i++){
				if (qiang_zhuang == m_qiang[i]){
					finded = true;
					break;
				}
			}
			if (!finded){
				send((protocol)niuniu_qiang_zhuang, notify, uuid, 3);
				return;
			}
		}
		int qiang_index = -1;
		int qiang_max_value = 0;
		std::vector<int> qiang_max_array;
		std::vector<int> qiang_all_array;
		bool is_all_qiang = true;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			qiang_all_array.push_back(i);
			if (m_members[i].uuid == uuid){
				if (m_members[i].qiang_zhuang != 0){
					break;
				}
				qiang_index = i;
				m_members[i].qiang_zhuang = qiang_zhuang;
			} else if (m_members[i].qiang_zhuang == 0){
				is_all_qiang = false;
			}
			//统计最大的抢庄用户索引列表
			int value = m_members[i].qiang_zhuang;
			if (value > qiang_max_value){
				qiang_max_value = value;
				qiang_max_array.clear();
				qiang_max_array.push_back(i);
			} else if (value == qiang_max_value){
				qiang_max_array.push_back(i);
			}
		}
		if (qiang_index < 0){
			send((protocol)niuniu_qiang_zhuang, notify, uuid, 4);
			return;
		}
		if (m_qiang_zhuang){
			notify.clear();
			notify["index"] = qiang_index;
			notify["uuid"]  = uuid;
			notify["value"] = qiang_zhuang;
			send_room((protocol)niuniu_qiang_zhuang, notify, 0);
		}
		//如果所有人都抢过庄了
		if (is_all_qiang){
			set_room_status(room_wait_xia_zhu);
			//如果不是抢庄模式则抢庄结束后设置为不抢庄
			if (m_huan_zhuang != zhuang_qiang1 &&
				m_huan_zhuang != zhuang_qiang2){
					m_qiang_zhuang = false;
			}
			notify.clear();
			size_t qiang_count = qiang_max_array.size();
			//根据抢庄情况选择庄
			if (qiang_count == 1){ //就一个最大值
				m_index_zhuang = qiang_max_array[0];
			} else if (!qiang_count){ //没人抢庄
				size_t size = qiang_all_array.size();
				for (size_t i = 0; i < size; i++){
					notify["index"].append(qiang_all_array[i]);
				}
				int rand_value = rand();
				rand_value %= qiang_all_array.size();
				m_index_zhuang = qiang_all_array[rand_value];
			} else { //从抢庄的人里随机挑选庄
				size_t size = qiang_max_array.size();
				for (size_t i = 0; i < size; i++){
					notify["index"].append(qiang_max_array[i]);
				}
				int rand_value = rand();
				rand_value %= qiang_max_array.size();
				m_index_zhuang = qiang_max_array[rand_value];
			}
			if (m_members[m_index_zhuang].qiang_zhuang < 0){
				m_members[m_index_zhuang].qiang_zhuang = 1;
			}
			notify["zhuang_index"] = m_index_zhuang;
			notify["value"] = m_members[m_index_zhuang].qiang_zhuang;
			send_room((protocol)niuniu_random_select, notify, 0);
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::auto_send_xiazhu()
	{
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_status != room_wait_xia_zhu){
				return;
			}
			if (m_members[i].uuid == 0){
				continue;
			}
			if (i == m_index_zhuang){
				continue;
			}
			if (m_members[i].xia_zhu == 0){
				Json::Value notify;
				notify["value"] = m_xiazhu[0];
				on_xia_zhu(m_members[i].uuid, notify);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_xia_zhu(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_xia_zhu){
			send((protocol)niuniu_xia_zhu, notify, uuid, 1);
			return;
		}
		int xia_zhu = data["value"].asInt();
		if (xia_zhu <= 0){
			send((protocol)niuniu_qiang_zhuang, notify, uuid, 2);
			return;
		} else if (xia_zhu > 0){
			bool finded = false;
			for (size_t i = 0; i < m_xiazhu.size(); i++){
				if (xia_zhu == m_xiazhu[i]){
					finded = true;
					break;
				}
			}
			if (!finded){
				send((protocol)niuniu_xia_zhu, notify, uuid, 3);
				return;
			}
		}
		int  xiazhu_index  = -1;
		int  poker_count   = 0;
		bool is_all_xiazhu = true;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			if (m_huan_zhuang != zhuang_unknow){
				if (m_index_zhuang == i){ //庄不下注
					continue;
				}
			}
			if (m_members[i].uuid == uuid){
				if (m_members[i].xia_zhu != 0){
					break;
				}
				xiazhu_index = i;
				poker_count = m_members[i].show_count;
				m_members[i].xia_zhu = xia_zhu;
			} else if (m_members[i].xia_zhu == 0){
				is_all_xiazhu = false;
			}
		}
		if (xiazhu_index < 0){
			send((protocol)niuniu_xia_zhu, notify, uuid, 4);
			return;
		}
		if (m_xiazhu.size() > 1){
			notify.clear();
			notify["index"] = xiazhu_index;
			notify["uuid"]  = uuid;
			notify["value"] = xia_zhu;
			send_room((protocol)niuniu_xia_zhu, notify, 0);
		}
		//如果所有人都下过注了
		if (is_all_xiazhu){
			set_room_status(room_wait_show_poker);
			if (poker_count == 4){
				for (int i = 0; i < MAX_MEMBERS; i++){
					if (m_members[i].uuid == 0){
						continue;
					}
					//计算用户牌型
					m_members[i].show_count = 5;
					poker::value_t v = m_members[i].hand.get_poker(4);
					//发送用户的第5张牌
					notify.clear();
					notify["id"]    = poker::get_point(v);
					notify["type"]  = poker::get_type(v);
					notify["style"] = m_members[i].bull_type;
					userid uuid = m_members[i].uuid;
					send((protocol)niuniu_show_five, notify, uuid, 0);
				}
			} else {
				send_user_poker();  //给用户发送手牌信息
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_huan_pai(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_show_poker){
			send((protocol)niuniu_huan_pai, notify, uuid, 1);
			return;
		}
		io_player::value_type player = io_player::find(uuid);
		if (!player){
			send((protocol)niuniu_huan_pai, notify, uuid, 2);
			return;
		}
		//判断是否允许换牌
		int dec_diamonds = m_diamond_huanpai;
		if (dec_diamonds <= 0){
			send((protocol)niuniu_huan_pai, notify, uuid, 3);
			return;
		}
		//如果不是通比牛牛模式
		if (m_huan_zhuang != zhuang_unknow){
			if (m_index_zhuang == player->get_index()){
				dec_diamonds *= 2;
			}
		}
		//目前打一折促销
		dec_diamonds = (int)(dec_diamonds * 0.1f);
		//如果用户钻石数不够
		if (is_free_time()){
			dec_diamonds = 0; //免费时段
		} else {
			if (player->get_diamonds_rest() < dec_diamonds){
				send((protocol)niuniu_huan_pai, notify, uuid, 4);
				return;
			}
		}
		//开始换牌
		int switch_index = -1;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			} else if (m_members[i].uuid == uuid){
				if (m_members[i].switch_count == 0){
					switch_index = i;
					poker::value_t new_value = m_pokers[m_next++];
					m_members[i].switch_count++;
					m_members[i].switch_poker = new_value;
					//计算新换的牌的牌型
					poker::value_t v = m_members[i].hand.get_poker(4);
					m_members[i].hand.set_poker(4, new_value);
					m_members[i].switch_style = get_bull_style(i);
					m_members[i].hand.set_poker(4, v);
					//扣除玩家10(庄20)个钻石
					deduct_diamond(uuid, dec_diamonds);
				}
				break;
			}
		}
		if (switch_index < 0){
			send((protocol)niuniu_huan_pai, notify, uuid, 3);
			return;
		}
		poker::value_t v = m_members[switch_index].switch_poker;
		notify.clear();
		notify["index"]   = switch_index;
		notify["uuid"]    = uuid;
		notify["style"]   = m_members[switch_index].switch_style;
		notify["id"]      = poker::get_point(v);
		notify["type"]    = poker::get_type(v);
		notify["diamond"] = dec_diamonds;
		send((protocol)niuniu_huan_pai, notify, uuid, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::auto_send_publish()
	{
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_status != room_wait_show_poker){
				return;
			}
			if (m_members[i].uuid == 0){
				continue;
			}
			if (m_members[i].is_show_hand == false){
				Json::Value notify;
				bool exchange = false;
				if (m_members[i].switch_poker){
					if (m_members[i].switch_style > m_members[i].bull_type){
						exchange = true;
					} else if (m_members[i].switch_style == m_members[i].bull_type){
						if (m_members[i].switch_poker > m_members[i].hand.get_poker(4)){
							exchange = true;
						}
					}
				}
				notify["huanpai"] = exchange;
				on_publish_hand(m_members[i].uuid, notify);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_publish_hand(userid uuid, const Json::Value &data)
	{
		Json::Value notify;
		if (m_status != room_wait_show_poker){
			send((protocol)niuniu_publish_hand, notify, uuid, 1);
			return;
		}
		bool huanpai = data["huanpai"].asBool();
		if (huanpai){
			if (m_diamond_huanpai == 0){
				send((protocol)niuniu_publish_hand, notify, uuid, 2);
				return;
			}
		}
		int  show_hand_index = -1;
		bool is_all_show_hand = true;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			} else if (m_members[i].uuid == uuid){
				if (m_members[i].is_show_hand){
					break;
				}
				if (huanpai){
					poker::value_t v = m_members[i].switch_poker;
					if (v == 0){
						break;
					}
					m_members[i].is_show_hand = true;
					m_members[i].hand.set_poker(4, v);
					m_members[i].bull_type = get_bull_style(i);
				}
				show_hand_index = i;
				m_members[i].is_show_hand = true;
			} else if (m_members[i].is_show_hand == false){
				is_all_show_hand = false;
			}
		}
		if (show_hand_index < 0){
			send((protocol)niuniu_publish_hand, notify, uuid, 3);
			return;
		}
		//回应用户结果
		room_member &user = m_members[show_hand_index];
		notify.clear();
		notify["index"] = show_hand_index;
		notify["uuid"]  = uuid;
		if (!m_diamond_huanpai){ //如果可换牌
			for (int i = 0; i < 5; i++){
				Json::Value value;
				poker::value_t v = user.hand.get_poker(i);
				value["id"]      = poker::get_point(v);
				value["type"]    = poker::get_type(v);
				notify["hand"].append(value);
			}
			notify["style"] = m_members[show_hand_index].bull_type;
		}
		send_room((protocol)niuniu_publish_hand, notify, 0);
		//如果所有人都亮牌了
		if (is_all_show_hand){
			if (m_round == 1){ //第一轮比赛结束
				m_need_payment = true;
			}
			publish_result(is_completed()); //公布比赛结果
			//切换房间状态
			if (m_round >= m_round_total){
				set_completed(); //设置比赛结束标记
				set_room_status(room_completed);
			} else {
				set_room_status(room_wait_ready);
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_room_broadcast(userid uuid, const Json::Value &data)
	{
		send_room((protocol)niuniu_room_broadcast, data, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::on_status_timeout(userid uuid, const Json::Value &data)
	{
		if (m_idle_time < max_idle_time)
			return;
		m_idle_time = max_idle_time + 5000;
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_round()
	{
		if (!is_opening()){ //如果没有开局
			set_open(); //设置房间已开局标记
		}
		//过滤没有准备的人(不参加牌局)
		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].switch_count = 0;
			if (m_members[i].time_ready){
				m_members[i].time_ready = 0;
			}
		}

		static bool is_set_rand = false;
		if(!is_set_rand)
		{
			srand((unsigned int)time(0));
			is_set_rand = true;
		}

		m_round++;
		init_next_poker();  //初始化牌池
		init_user_poker();  //初始化用户手牌
		//切换房间状态
		if (m_huan_zhuang == zhuang_unknow){
			for (int i = 0; i < MAX_MEMBERS; i++){
				if (m_members[i].uuid == 0)
					continue;
				m_members[i].qiang_zhuang = 1;
			}
			set_room_status(room_wait_xia_zhu);
		} else {
			set_room_status(room_wait_qiang_zhuang);
		}
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue; //没参与牌局
			}
			Json::Value notify;
			notify["round"]        = m_round;
			notify["round_total"]  = m_round_total;
			notify["qiang_zhuang"] = m_qiang_zhuang;
			userid uuid = m_members[i].uuid;
			send((protocol)niuniu_round_begin, notify, uuid, 0);
		}
		if (m_huan_zhuang == zhuang_qiang1){
			send_user_poker();  //给用户发送手牌信息
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::send_user_poker()
	{
		//根据不同的抢庄类型选择下一步的操作
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue; //没参与牌局
			}
			//判断应该发几张牌
			m_members[i].show_count = 4;
			m_members[i].bull_type  = get_bull_style(i);
			if (m_members[i].qiang_zhuang != 0 ||
				m_members[i].xia_zhu != 0){
					m_members[i].show_count = 5;
			}
			Json::Value notify;
			if (m_members[i].show_count == 5){
				notify["style"] = m_members[i].bull_type;
			}
			for (int j = 0; j < m_members[i].show_count; j++){
				poker::value_t v = m_members[i].hand.get_poker(j);
				Json::Value card;
				card["id"]   = poker::get_point(v);
				card["type"] = poker::get_type(v);
				notify["hand"].append(card);
			}
			userid uuid = m_members[i].uuid;
			send((protocol)niuniu_pokers, notify, uuid, 0);
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::init_next_poker()
	{
		//初始化牌局参数
		m_next = 0;
		memset(m_pokers, 0, sizeof(m_pokers));
		//初始化牌池数据
		std::vector<poker::value_t> temp_pokers;
		int max_poker_point = poker::point_J_black;
		if (m_no_flower){
			max_poker_point = poker::point_10;
		}
		for (int i = poker::point_A; i < max_poker_point; i++){
			for (int j = poker::d_diamond; j < poker::k_joker; j++){
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
		if (!m_intervene){
			for (int i = 0; i < MAX_MEMBERS; i++){
				m_members[i].init();
				if (m_members[i].uuid == 0){
					continue;
				}
				for (int j = 0; j < 5; j++){
					m_members[i].hand.set_poker(j, m_pokers[m_next++]);
				}
			}
			return;
		}
		//把用户按照成绩排序
		struct __user_node{
			int   index;
			float value;
			inline bool operator<(const __user_node &n){
				return value < n.value;
			}
		} user[MAX_MEMBERS];
		int index = 0;
		for (int i = 0; i < MAX_MEMBERS; i++){
			m_members[i].init();
			if (m_members[i].uuid == 0){
				continue;
			}
			int j = index++;
			user[j].index  = i;
			user[j].value  = (float)m_members[i].score;
			user[j].value += (float)m_members[i].health * 2.0f;
			user[j].value += (float)(rand() % 100) / 100.0f;
			//判断用户是否是代理
			io_player::value_type player = io_player::find(m_members[i].uuid);
			if (player){
				if (player->get_type() > 0){
					int level = player->get_level();
					if (level > 0){
						user[j].value *= level;
					}
				}
			}
		}
		poker::shellsort<__user_node>(user, index);
		//初始化用户数付牌并按照大小排序
		poker_hand hand[10];
		std::vector<int> unused;
		for (int i = 0; i < index; i++){
			unused.push_back(i);
			for (int j = 0; j < 5; j++){
				hand[i].set_poker(j, m_pokers[m_next++]);
			}
		}
		struct __bull_node{
			int index;
			int value;
			inline bool operator<(const __bull_node &n){
				return value < n.value;
			}
		} bull[10];
		for (int i = 0; i < index; i++){
			bull[i].index = i;
			bull_type t   = get_bull_style(hand[i]);
			bull[i].value = get_bull_value(t);
		}
		poker::shellsort<__bull_node>(bull, index);
		//让用户按照健康值选择牌
		for (int i = 0; i < index; i++){
			int unused_size = (int)unused.size();
			int begin_index = 0;
			int range_count = 3;
			if (unused_size > range_count){
				begin_index = unused_size - range_count;
			} else if (unused_size < range_count){
				range_count = unused_size;
			}
			int poker_index = begin_index + (rand() % range_count);
			int j = poker_index; //index - i - 1;
			TRACE("POKER: %d(%d)(%.2f) -> %d\r\n", user[i].index, i, user[i].value, unused[j]);
			poker_hand &old = m_members[user[i].index].hand;
			poker_hand &now = hand[bull[unused[j]].index];
			unused.erase(unused.begin() + j);
			for (int k = 0; k < 5; k++){
				old.set_poker(k, now.get_poker(k));
			}
		}
	}
	////////////////////////////////////////////////////////////////////////////////
	bull_type game_room::get_bull_style(int index)
	{
		return get_bull_style(m_members[index].hand);
	}
	////////////////////////////////////////////////////////////////////////////////
	bull_type game_room::get_bull_style(const poker_hand &hand)
	{
		if (hand.is_bomb()){
			return bull_bomb;
		} else if (m_no_flower && hand.is_five_big()){
			return bull_5_big;
		} else if (hand.is_five_small()){
			return bull_5_small;
		} else if (m_no_flower && hand.is_hulu_niu()){
			return bull_gourd;
		} else if (m_no_flower && hand.is_order_niu()){
			return bull_order;
		} else if (hand.is_gold()){
			return bull_gold;
		} else if (hand.is_silver()){
			return bull_silver;
		}
		return hand.is_have_bull(m_no_flower);
	}
	////////////////////////////////////////////////////////////////////////////////
	int game_room::get_bull_value(bull_type type)
	{
		if (m_fanbei == 1){ //普通模式
			switch(type){
			case bull_null:    return 1;
			case bull_one:     return 1;
			case bull_one2:    return 1;
			case bull_two:     return 1;
			case bull_two2:    return 1;
			case bull_three:   return 1;
			case bull_three2:  return 1;
			case bull_four:    return 1;
			case bull_four2:   return 1;
			case bull_five:    return 1;
			case bull_five2:   return 1;
			case bull_six:     return 1;
			case bull_six2:    return 1;
			case bull_seven:   return 2;
			case bull_seven2:  return 2;
			case bull_eight:   return 2;
			case bull_eight2:  return 2;
			case bull_nine:    return 2;
			case bull_nine2:   return 2;
			case bull_niuniu:  return 3;
			case bull_niuniu2: return 3;
			case bull_silver:  return 4;
			case bull_order:   return 4;
			case bull_gold:    return 5;
			case bull_gourd:   return 5;
			case bull_5_small: return 6;
			case bull_5_big:   return 6;
			case bull_bomb:    return 10;
			}
		} else if (m_fanbei = 2){ //扫雷模式
			switch(type){
			case bull_null:    return 1;
			case bull_one:     return 1;
			case bull_one2:    return 1;
			case bull_two:     return 2;
			case bull_two2:    return 2;
			case bull_three:   return 3;
			case bull_three2:  return 3;
			case bull_four:    return 4;
			case bull_four2:   return 4;
			case bull_five:    return 5;
			case bull_five2:   return 5;
			case bull_six:     return 6;
			case bull_six2:    return 6;
			case bull_seven:   return 7;
			case bull_seven2:  return 7;
			case bull_eight:   return 8;
			case bull_eight2:  return 8;
			case bull_nine:    return 9;
			case bull_nine2:   return 9;
			case bull_niuniu:  return 10;
			case bull_niuniu2: return 10;
			case bull_silver:  return 12;
			case bull_order:   return 12;
			case bull_gold:    return 15;
			case bull_gourd:   return 15;
			case bull_5_small: return 18;
			case bull_5_big:   return 18;
			case bull_bomb:    return 20;
			}
		}
		return 1;
	}
	////////////////////////////////////////////////////////////////////////////////
	int game_room::get_user_score(int index, int other)
	{
		int result = 0;
		room_member &user   = m_members[index];
		room_member &zhuang = m_members[other];
		if (user.bull_type > zhuang.bull_type){
			result = get_bull_value(user.bull_type);
		} else if (user.bull_type < zhuang.bull_type){
			result = get_bull_value(zhuang.bull_type);
			result *= (-1);
		}
		if (result == 0){
			if (zhuang.bull_type == bull_null){
				result = 1;
			} else {
				result = get_bull_value(zhuang.bull_type);
			}
			bool is_gold_niu = (zhuang.bull_type > bull_null && zhuang.bull_type < bull_silver);
			if (is_gold_niu){
				is_gold_niu = (zhuang.bull_type % 2 == 0);
			}
			if (!is_gold_niu){
				if (user.hand.get_max_value() < zhuang.hand.get_max_value()){
					result *= (-1);
				}
			} else {
				if (user.hand.get_gold_value() < zhuang.hand.get_gold_value()){
					result *= (-1);
				}
			}
		}
		result *= user.xia_zhu;
		result *= zhuang.qiang_zhuang;
		return result;
	}
	////////////////////////////////////////////////////////////////////////////////
	int game_room::get_winner_index()
	{
		int index = -1;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			if (index < 0 || get_user_score(i, index) > 0){
				index = i;
			}
		}
		return index;
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
		send_room((protocol)niuniu_status_change, notify, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
	void game_room::publish_result(bool completed)
	{
		Json::Value notify;
		notify["number"]      = get_number();
		notify["round"]       = m_round;
		notify["round_total"] = m_round_total;
		std::vector<int> niuniu_array;
		//确定其他用户和谁比牌
		int zhuang_score = 0;
		int zhuang_index = m_index_zhuang;
		if (m_huan_zhuang == zhuang_unknow){
			zhuang_index = get_winner_index();
		}
		notify["zhuang"]  = zhuang_index;
		int  member_count = 0;
		bool need_payment = m_need_payment;
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			member_count++;
			room_member &user = m_members[i];
			if (need_payment && !is_free_time()){
				if (m_round == 1 && m_zhifu == 1){ //AA支付
					m_need_payment = false;
					deduct_diamond(user.uuid, m_diamond_pay);
				}
			}
			Json::Value member;
			if (i != zhuang_index){
				member["index"]  = i;
				member["uuid"]   = user.uuid;
				member["style"]  = user.bull_type;
				member["qiang"]  = user.qiang_zhuang;
				member["xiazhu"] = user.xia_zhu;
				//如果是牛牛换庄
				if (m_huan_zhuang == zhuang_niuniu){
					if (user.bull_type == bull_niuniu){
						m_qiang_zhuang = false;
						niuniu_array.push_back(i);
					}
				}
				for (int j = 0; j < 5; j++){
					Json::Value node;
					poker::value_t v = user.hand.get_poker(j);
					node["id"]       = poker::get_point(v);
					node["type"]     = poker::get_type(v);
					member["hand"].append(node);
				}
				int this_score = get_user_score(i, zhuang_index);
				if (this_score > 0){
					user.health++;
				} else if (this_score < 0){
					user.health--;
				}
				user.score     += this_score;
				zhuang_score   -= this_score;
				member["score"] = this_score;
				member["score_total"] = user.score;
				notify["result"].append(member);
			}
		}
		if (need_payment && !is_free_time()){
			if (m_round == 1 && m_zhifu == 2){ //房主支付
				m_need_payment = false;
				int payment = m_diamond_pay * member_count;
				deduct_diamond(get_creater(), payment);
			}
		}
		Json::Value member;
		room_member &zhuang  = m_members[zhuang_index];
		member["index"]      = zhuang_index;
		member["uuid"]       = zhuang.uuid;
		member["style"]      = zhuang.bull_type;
		member["qiang"]      = zhuang.qiang_zhuang;
		member["xiazhu"]     = zhuang.xia_zhu;
		//如果是牛牛换庄
		if (m_huan_zhuang == zhuang_niuniu){
			if (zhuang.bull_type == bull_niuniu){
				m_qiang_zhuang = false;
				niuniu_array.push_back(zhuang_index);
			}
		}
		for (int j = 0; j < 5; j++){
			Json::Value node;
			poker::value_t v = zhuang.hand.get_poker(j);
			node["id"]       = poker::get_point(v);
			node["type"]     = poker::get_type(v);
			member["hand"].append(node);
		}
		if (zhuang_score > 0){
			zhuang.health++;
		} else if (zhuang_score < 0){
			zhuang.health--;
		}
		zhuang.score         += zhuang_score;
		member["score"]       = zhuang_score;
		member["score_total"] = zhuang.score;
		notify["result"].append(member);
		send_room((protocol)niuniu_publish_result, notify, 0);
		//如果出现了牛牛牌
		if (niuniu_array.size() == 1){
			m_index_zhuang = niuniu_array[0];
		} else if (niuniu_array.size() > 1){
			int max_index = 0;
			for (size_t i = 1; i < niuniu_array.size(); i++){
				int i_now = niuniu_array[i];
				int i_max = niuniu_array[max_index];
				poker::value_t v1 = m_members[i_now].hand.get_max_value();
				poker::value_t v2 = m_members[i_max].hand.get_max_value();
				if (v1 > v2){
					max_index = (int)i;
				}
			}
			m_index_zhuang = niuniu_array[max_index];
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
		if (m_huan_zhuang != zhuang_unknow){
			notify["zhuang"] = m_index_zhuang;
		}
		notify["qiang_zhuang"] = m_qiang_zhuang;
		if (m_time_limit){
			int delta = (max_idle_time - m_idle_time) / 1000;
			notify["end_time"] = time(0) + delta;
		}
		for (int i = 0; i < MAX_MEMBERS; i++){
			if (m_members[i].uuid == 0){
				continue;
			}
			Json::Value member;
			member["index"]        = i;
			member["uuid"]         = m_members[i].uuid;
			member["is_show_hand"] = m_members[i].is_show_hand;
			member["show_count"]   = m_members[i].show_count;
			member["xia_zhu"]      = m_members[i].xia_zhu;
			member["qiang_zhuang"] = m_members[i].qiang_zhuang;
			//如果用户已经亮牌且不可换牌
			if (m_members[i].uuid == player->get_uuid() ||
				(m_members[i].is_show_hand && !m_diamond_huanpai) ||
				(m_members[i].is_show_hand && m_status == room_wait_ready))
			{
				member["bull_type"] = m_members[i].bull_type;
				for (int j = 0; j < m_members[i].show_count; j++){
					Json::Value card;
					poker::value_t v = m_members[i].hand.get_poker(j);
					card["id"] = poker::get_point(v);
					card["type"] = poker::get_type(v);
					member["hand"].append(card);
				}
			}
			if (m_members[i].uuid == player->get_uuid()){
				Json::Value card;
				poker::value_t v = m_members[i].switch_poker;
				member["switch_poker"]["id"]   = poker::get_point(v);
				member["switch_poker"]["type"] = poker::get_type(v);
				member["switch_count"] = m_members[i].switch_count;
				member["switch_style"] = m_members[i].switch_style;
			}
			notify["members"].append(member);
		}
		send((protocol)niuniu_sync_data, notify, player, 0);
	}
	////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
