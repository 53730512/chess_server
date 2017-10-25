

#include "io_handler.h"
#include "io_config.h"
#include "io_database.h"
#include "service/unicode.h"
////////////////////////////////////////////////////////////////////////////////
size_t players_count = 0;
std::map<userid, http::channal> userid_mapping;
std::map<http::channal, io_player::value_type> player_mapping;


////////////////////////////////////////////////////////////////////////////////
inline void login_log(const char *format, ...){
	char cache[8192] = {0};
	char *errinfo = cache;
	if (format){
		va_list args;
		va_start(args, format);
		size_t bytes = _vscprintf(format, args) + 1;
		if (bytes > sizeof(cache)){
			errinfo = (char*)malloc(bytes);
			if (!errinfo)
				return;
		}
		vsprintf(errinfo, format, args);
		va_end(args);
	}
	time_t time_now = time(0);
	struct tm *ptm = localtime(&time_now);
	io::stringc content;
	content.format("[%02d:%02d:%02d] %s\r\n"
		, ptm->tm_hour
		, ptm->tm_min
		, ptm->tm_sec
		, errinfo
		);
	if (errinfo != cache){
		free(errinfo);
	}
	if (!content.is_utf8()){
		content = content.to_utf8();
	}
	io::stringc filename;
	filename.format("./~login/%02d%02d%02d.log"
		, ptm->tm_year + 1900
		, ptm->tm_mon + 1
		, ptm->tm_mday
		);
	npx_make_dir("./~login");
	FILE *fpwrite = fopen(filename.c_str(), "a");
	if (fpwrite){
		fwrite(content.c_str(), 1, content.size(), fpwrite);
		fclose(fpwrite);
	}
}
////////////////////////////////////////////////////////////////////////////////
Json::Value io_player::data::to_json_object() const
{
	Json::Value data;
	data["status"]           = status;
	data["type"]             = type;
	data["uuid"]             = uuid;
	data["level"]            = level;
	data["promoter"]         = promoter;
	data["score"]            = score;
	data["health"]           = health;
	data["quality"]          = quality;
	data["createtime"]       = createtime;
	data["package"]          = package;
	data["payment"]          = payment;
	data["device"]           = device;
	data["idfa"]             = idfa;
	data["pfid"]             = pfid;
	data["unionid"]          = unionid;
	data["nickname"]         = nickname;
	data["emoji_name"]       = emoji_name;
	data["sex"]              = sex;
	data["country"]          = country;
	data["province"]         = province;
	data["city"]             = city;
	data["head_url"]         = head_url;
	data["extended"]         = extended;
	data["gold"].append(gold[currency_rest]);
	data["gold"].append(gold[currency_used]);
	data["gold"].append(gold[currency_total]);
	data["diamond"].append(diamond[currency_rest]);
	data["diamond"].append(diamond[currency_used]);
	data["diamond"].append(diamond[currency_total]);
	return std::move(data);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::data::from_json_object(const Json::Value &data)
{
	status                   = (user_state)data["status"].asInt();
	type                     = (user_type)data["type"].asInt();
	createtime               = (time_t)data["createtime"].asInt();
	uuid                     = data["uuid"].asUInt();
	level                    = data["level"].asInt();
	promoter                 = data["promoter"].asInt();
	score                    = data["score"].asInt();
	health                   = data["health"].asInt();
	quality                  = data["quality"].asInt();
	package                  = data["package"].asInt();
	payment                  = data["payment"].asInt();
	device                   = data["device"].asString().c_str();
	idfa                     = data["idfa"].asString().c_str();
	pfid                     = data["pfid"].asString().c_str();
	unionid                  = data["unionid"].asString().c_str();
	nickname                 = data["nickname"].asString().c_str();
	emoji_name               = data["emoji_name"].asString().c_str();
	sex                      = data["sex"].asString().c_str();
	country                  = data["country"].asString().c_str();
	province                 = data["province"].asString().c_str();
	city                     = data["city"].asString().c_str();
	head_url                 = data["head_url"].asString().c_str();
	extended                 = data["extended"];
	gold[currency_rest]      = data["gold"][currency_rest].asInt();
	gold[currency_used]      = data["gold"][currency_used].asInt();
	gold[currency_total]     = data["gold"][currency_total].asInt();
	diamond[currency_rest]   = data["diamond"][currency_rest].asInt();
	diamond[currency_used]   = data["diamond"][currency_used].asInt();
	diamond[currency_total]  = data["diamond"][currency_total].asInt();
}
////////////////////////////////////////////////////////////////////////////////
unsigned int io_player::data::get_hash() const
{
	Json::Value value = to_json_object();
	io::stringc result(value.toFastString().c_str());
	return hash::chksum32(result);
}
////////////////////////////////////////////////////////////////////////////////
io_player::io_player(http_context context)
	: m_context(context)
{
	m_data.status                   = user_state_ok;
	m_data.type                     = user_type_user;
	m_data.uuid                     = 0;
	m_data.level                    = 0;
	m_data.promoter                 = 0;
	m_data.score                    = 0;
	m_data.quality                  = 0;
	m_data.health                   = 0;
	m_data.createtime               = 0;
	m_data.package                  = 0;
	m_data.payment                  = 0;
	m_data.gold[currency_rest]      = 0;
	m_data.gold[currency_used]      = 0;
	m_data.gold[currency_total]     = 0;
	m_data.diamond[currency_rest]   = 0;
	m_data.diamond[currency_used]   = 0;
	m_data.diamond[currency_total]  = 0;
	m_data.ipaddr                   = context->real_ipaddr;
	m_index                         = 0;
	m_room_number                   = 0;
	m_run_back                      = false;
	m_destroyed                     = false;
	m_new_user                      = false;
	m_in_the_room                   = false;
	m_hash_id                       = 0;
	m_data_hash                     = 0;
	m_time_login                    = 0;
	m_time_online                   = 0;
}
////////////////////////////////////////////////////////////////////////////////
size_t io_player::get_player_count()
{
	return players_count;
}
////////////////////////////////////////////////////////////////////////////////
void io_player::clear()
{
	auto iter = player_mapping.begin();
	for (; iter != player_mapping.end(); iter++){
		iter->second->close();
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::update(int delta)
{
	auto iter = player_mapping.begin();
	for (; iter != player_mapping.end(); iter++){
		iter->second->on_update(delta);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::broadcast(protocol type, const Json::Value &data)
{
	auto iter = player_mapping.begin();
	for (; iter != player_mapping.end(); iter++){
		iter->second->send(type, data, 0);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::close()
{
	handler()->close(m_context);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::destroy(value_type player)
{
	if (player && !player->m_destroyed){
		userid uuid = player->get_uuid();
		if (uuid){
			player->on_destroy();
			userid_mapping.erase(uuid);
		}
		players_count--;
		player->m_destroyed = true;
		player_mapping.erase(player->m_context->id);
	}
}
////////////////////////////////////////////////////////////////////////////////
io_player::value_type io_player::create(http_context context)
{
	value_type player(new io_player(context));
	if (player){
		players_count++;
		player_mapping[context->id] = player;
		player->on_create();
	}
	return player;
}
////////////////////////////////////////////////////////////////////////////////
void io_player::set_uuid(userid uuid)
{
	m_data.uuid = uuid;
	if (uuid > 0){
		userid_mapping[uuid] = m_context->id;
	}
}
////////////////////////////////////////////////////////////////////////////////
io_player::value_type io_player::find(userid uuid)
{
	auto iter = userid_mapping.find(uuid);
	return (iter != userid_mapping.end()) ? find(iter->second) : io_player::value_type();
}
////////////////////////////////////////////////////////////////////////////////
io_player::value_type io_player::find(http::channal id)
{
	auto iter = player_mapping.find(id);
	return (iter != player_mapping.end()) ? iter->second : io_player::value_type();
}
////////////////////////////////////////////////////////////////////////////////
io_player::value_type io_player::find(http_context context)
{
	return context ? find(context->id) : io_player::value_type();
}
////////////////////////////////////////////////////////////////////////////////
void io_player::send(protocol type, const Json::Value &data, int errcode)
{
	handler()->send(m_context, type, data, errcode);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::set_score(int score)
{
	m_data.score += score;
	if (score > 0){ //赢了
		m_data.health += 1;
	} else if (score < 0) { //输了
		m_data.health -= 1;
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::inc_golds(int count)
{
	m_data.gold[currency_rest]  += count;
	m_data.gold[currency_total] += count;
	Json::Value notify;
	notify["count"]  = count;
	notify["remain"] = m_data.gold[currency_rest];
	send(user_gold_change, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::dec_golds(int count)
{
	if (count == 0){ //没改变
		return;
	}
	m_data.gold[currency_used] += count;
	m_data.gold[currency_rest] -= count;
	Json::Value notify;
	notify["count"]  = (-1 * count);
	notify["remain"] = m_data.gold[currency_rest];
	send(user_gold_change, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::set_golds(int rest_count, int used_count)
{
	m_data.gold[currency_used]  = 0;
	m_data.gold[currency_rest]  = 0;
	m_data.gold[currency_total] = 0;

	m_data.gold[currency_used]  = used_count;
	m_data.gold[currency_rest]  = rest_count;
	m_data.gold[currency_total] = used_count + rest_count;
}
////////////////////////////////////////////////////////////////////////////////
void io_player::inc_diamonds(int count)
{
	m_data.diamond[currency_rest]  += count;
	m_data.diamond[currency_total] += count;
	Json::Value notify;
	notify["count"]  = count;
	notify["remain"] = m_data.diamond[currency_rest];
	send(user_diamond_change, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::dec_diamonds(int count)
{
	if (count == 0){ //没改变
		return;
	}
	m_data.diamond[currency_used] += count;
	m_data.diamond[currency_rest] -= count;
	//通知用户钻石改变
	Json::Value notify;
	notify["count"]  = (-1 * count);
	notify["remain"] = m_data.diamond[currency_rest];
	send(user_diamond_change, notify, 0);
	//通知中心钻石改变
	notify.clear();
	notify["uuid"]   = get_uuid();
	notify["count"]  = count;
	notify["remain"] = get_diamonds_rest();
	handler()->send_to_cache(user_diamond_change, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::set_diamonds(int rest_count, int used_count)
{
	m_data.diamond[currency_used]  = 0;
	m_data.diamond[currency_rest]  = 0;
	m_data.diamond[currency_total] = 0;

	m_data.diamond[currency_used]  = used_count;
	m_data.diamond[currency_rest]  = rest_count;
	m_data.diamond[currency_total] = used_count + rest_count;
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_create()
{

}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_update(int delta)
{

}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_timeout()
{

}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_error(int error)
{
	int room_number = get_room_number();
	if (room_number && is_in_the_room()){
		room_basic::value_type room = room_basic::find(room_number);
		if (room){
			room->on_leave(shared_from_this(), false);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_timer()
{
	if (m_time_login == 0){
		return;
	}
	if (m_data_hash){ //有hash值
		unsigned int hash = m_data.get_hash();
		if (m_data_hash != hash){
			write_to_cache();
			m_data_hash = hash;
		}
	}
	if ((++m_time_online) % 10 == 0){
		Json::Value notify;
		notify["uuid"]     = get_uuid();
		notify["unionid"]  = get_unionid();
		notify["nickname"] = get_nickname();
		notify["ipaddr"]   = get_ipaddr();
		handler()->send_to_cache(user_activation, notify, 0);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_destroy()
{
	if (m_time_login == 0){
		return;
	}
	if (m_data_hash){ //有hash值
		unsigned int hash = m_data.get_hash();
		if (m_data_hash != hash){
			write_to_cache();
			m_data_hash = hash;
		}
	}
	Json::Value notify;
	notify["uuid"] = get_uuid();
	handler()->send_to_cache(user_offline, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::wechat_login(unsigned int utc, bool rlogin)
{
	if (m_token.size() != 32){
		return false;
	}
	io::stringc data;
	data.format("%s%s", get_unionid().c_str(), HMAC_MD5_KEY);
	std::string hash = hash::md5(data.c_str(), (int)data.size());
	return (m_token == hash);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::xrgame_login(unsigned int utc, bool rlogin)
{
#ifndef POKER_PUBLISH
	return true;
#else
	return false;
#endif
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::player_login(unsigned int utc, bool rlogin)
{
	bool result = false;
	if (m_data.pfid == "wechat"){
		result = wechat_login(utc, rlogin);
	} else if (m_data.pfid == "xrgame"){
		result = xrgame_login(utc, rlogin);
	}
	return result;
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_login(const io::stringc &data)
{
	Json::Value packet;
	Json::Reader reader;
	//解析请求数据
	if (!reader.parse(data, packet)){
		console::log("JSON parsing failed: %s", data.c_str());
		send_error_reply(user_auth, 101);
		return;
	}
	//检查数据格式
	if (!packet.isObject()){
		console::log("JSON is not an object: %s", data.c_str());
		send_error_reply(user_auth, 102);
		return;
	}
	//检查登录协议号
	protocol type = (protocol)packet["type"].asInt();
	if (type == user_keepalive){
		return;
	}
	if (type != user_auth){
		console::log("User is not logged: %s", data.c_str());
		send_error_reply(type, 103);
		return;
	}
	//检查协议内容
	Json::Value &request = packet["data"];
	if (!request.isObject()){
		console::log("DATA is not an object: %s", data.c_str());
		send_error_reply(user_auth, 104);
		return;
	}
	//过滤emoji符号
	io::stringc nickname;
	std::string wx_name = request["nickname"].asString();
	nickname = wx_name.c_str();
	if (!nickname.is_utf8()){
		nickname = nickname.to_utf8();
	}
	nickname.strrstr("#", "*");
	io::stringc emoji_name;
	io::stringc no_emoji_name;
	size_t len = 0;
	while (len < nickname.size()){
		io::stringc c = nickname.get_char(len);
		if (c.size() < 4){
			emoji_name += c;
			no_emoji_name += c;
		} else {
			unsigned long unicode_char = 0;
			unsigned char *input = (unsigned char*)c.c_str();
			int n = unicode::from_utf8_char(input, &unicode_char);
			if (n > 0){
				io::stringc emoji;
				emoji.format("\\u%05x", unicode_char);
				emoji_name += emoji;
			}
		}
	}
	if (no_emoji_name.empty()){
		no_emoji_name = "[empty]";
	}
	m_data.nickname   = std::move(no_emoji_name);
	m_data.emoji_name = std::move(emoji_name);
	//读取请求参数
	m_data.device     = request["device"].asString().c_str();
	m_data.idfa       = request["idfa"].asString().c_str();
	m_data.pfid       = request["pfid"].asString().c_str();
	m_data.unionid    = request["unionid"].asString().c_str();
	m_data.sex        = request["sex"].asString().c_str();
	m_data.country    = request["country"].asString().c_str();
	m_data.province   = request["province"].asString().c_str();
	m_data.city       = request["city"].asString().c_str();
	m_data.head_url   = request["head_url"].asString().c_str();
	//读取附加参数
	m_run_back        = false;
	m_token           = request["token"].asString().c_str();
	m_voice_token     = request["voice_token"].asString().c_str();
	unsigned int time_utc = request["time"].asUInt();
	bool rlogin = request["rlogin"].asBool();
	//检查参数合法性
	if (m_data.pfid.empty() || m_data.unionid.empty()){
		console::log("pfid or unionid is empty: %s", data.c_str());
		send_error_reply(user_auth, 105);
		return;
	}
	if (m_token.empty() || time_utc == 0){
		console::log("token or time is empty: %s", data.c_str());
		send_error_reply(user_auth, 106);
		return;
	}
	//登录验证
	if (!player_login(time_utc, rlogin)){
		console::log("User login failed: %s", data.c_str());
		send_error_reply(user_auth, 107);
		return;
	}
	//提取账号数据
	if (!load_from_cache()){
		console::log("Load from cache failed: %s", data.c_str());
		send_error_reply(user_auth, 108);
		return;
	}

	//保存登录日志
	login_log("name:%s unionid=%s token=%s", m_data.nickname.c_str(), m_data.unionid.c_str(), m_token.c_str());
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_request(const io::stringc &data)
{
	//检查用户登录状态
	if (get_uuid() == 0){ //未登录
		console::log("User is not logged: %s", data.c_str());
		send_error_reply(request_invalid, 201);
		return;
	}
	//解析请求数据
	Json::Value packet;
	Json::Reader reader;
	if (!reader.parse(data, packet)){
		console::log("JSON parsing failed: %s", data.c_str());
		send_error_reply(request_invalid, 202);
		return;
	}
	//检查数据格式
	if (!packet.isObject()){
		console::log("JSON is not an object: %s", data.c_str());
		send_error_reply(request_invalid, 203);
		return;
	}
	//检查是否是登录请求
	protocol type = (protocol)packet["type"].asInt();
	if (type == user_auth){
		console::log("User is already logged: %s", data.c_str());
		send_error_reply(type, 204);
		return;
	}
	//检查协议内容
	Json::Value &request  = packet["data"];
	if (!request.isObject()){
		console::log("DATA is not an object: %s", data.c_str());
		send_error_reply(type, 205);
		return;
	}
	//解析用户请求
	parse_request(type, request);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::parse_request(protocol type, const Json::Value &data)
{
#ifdef NPX_WINDOWS
	__try{
#endif
		switch (type){
		case user_keepalive:       on_keep_alive(data);
			break;
		case user_create_room:     on_create_room(data);
			break;
		case user_enter_room:      on_enter_room(data);
			break;
		case user_leave_room:      on_leave_room(data);
			break;
		case user_bind_promoter:   on_bind_promoter(data);
			break;
		case user_wechat_shared:   on_wechat_shared(data);
			break;
		case load_player_zhanji:   on_load_zhanji(data);
			break;
		case load_zhanji_round:    on_load_zhanji_round(data);
			break;
		case user_payment_sign:    on_payment_sign(data);
			break;
		case user_is_run_back:     on_run_back_notify(data);
			break;
		case user_set_voice_token: on_set_voice_token(data);
			break;
		default: parse_unknown(type, data);
			break;
		}
		if (type != user_keepalive && type != user_is_run_back){
			m_run_back = false; //收到非心跳请求则切换为前台
		}
#ifdef NPX_WINDOWS
	}__except(EXCEPTION_EXECUTE_HANDLER){
		console::log("Handling user request exceptions: protocol = %d.", type);
		return;
	}
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_player::parse_unknown(protocol type, const Json::Value &data)
{
	Json::Value notify;
	if (!is_in_the_room()){
		send_error_reply(type, 901);
		return;
	}
	int room_number = get_room_number();
	room_basic::value_type room = room_basic::find(room_number);
	if (!room){
		send_error_reply(type, 902);
		return;
	}
	room->on_request(shared_from_this(), type, data);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::write_to_cache()
{
	if (get_uuid() == 0){
		return false; //用户还没有完成数据加载，绝对不能保存
	}
	Json::Value notify = m_data.to_json_object();
	notify["ipaddr"] = http::get_remote_addr();
	return handler()->send_to_cache(save_player_data, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::load_from_cache()
{
	//计算hash_id
	io::stringc temp;
	temp.format("%s%s", get_pfid().c_str(), get_unionid().c_str());
	m_hash_id = hash::chksum32(temp);
	//加载角色数据
	Json::Value notify;
	notify["channal"]  = m_context->id;
	notify["hashid"]   = get_hash_id();
	notify["pfid"]     = get_pfid();
	notify["unionid"]  = get_unionid();
	notify["nickname"] = get_nickname();
	notify["sex"]      = get_sex();
	notify["device"]   = get_device();
	notify["idfa"]     = get_idfa();
	notify["country"]  = get_country();
	notify["province"] = get_province();
	notify["city"]     = get_city();
	notify["head_url"] = get_head_url();
	notify["ipaddr"]   = http::get_remote_addr();
	return handler()->send_to_cache(load_player_data, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::load_package_data()
{
	Json::Value notify;
	notify["channal"]    = m_context->id;
	notify["package"]    = get_package_id();
	notify["occasion"]   = 0;
	notify["createtime"] = get_create_time();
	return handler()->send_to_cache(load_player_package, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::load_payment_data()
{
	Json::Value notify;
	notify["channal"] = m_context->id;
	notify["uuid"]    = get_uuid();
	notify["payment"] = get_payment();
	return handler()->send_to_cache(load_player_payment, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
bool io_player::today_is_shared()
{
	Json::Value extended = get_extended();
	if (!extended.isMember("wechat_shared")){
		return false;
	}
	Json::Value &wechat = extended["wechat_shared"];
	if (!wechat.isMember("time")){
		return false;
	}
	time_t time_last = wechat["time"].asUInt();
	struct tm *ptm = localtime(&time_last);
	int last_day = ptm->tm_year * ptm->tm_yday;

	time_t time_now = time(0);
	ptm = localtime(&time_now);
	int this_day = ptm->tm_year * ptm->tm_yday;
	return (this_day <= last_day);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_keep_alive(const Json::Value &data)
{
	send(user_keepalive, data, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_create_room(const Json::Value &data)
{
	Json::Value notify;
	//判断用户是否已经加入其他房间
	room_basic::value_type room = room_basic::find(get_uuid());
	if (room){
		send_error_reply(user_create_room, 301);
		return;
	}
	//判断用户钻石是否够用
	userid uuid = get_uuid();
	int diamond_rest = get_diamonds_rest();
	if (diamond_rest < 0){ //欠费
		send_error_reply(user_create_room, 302);
		return;
	} else if (diamond_rest == 0 && !room_basic::is_free_time()){
		send_error_reply(user_create_room, 302);
		return;
	}
	//根据类型实例化对应类
	game_type game     = (game_type)data["game"].asInt();
	game_type type     = (game_type)data["type"].asInt();
	bool creater_leave = data["owner_exit"].asBool();
	switch (game){
	case room_type_douniu:
		//创建斗牛房间
		room = create_room<niuniu::game_room>(uuid, game, type, 0, creater_leave);
		break;
	case room_type_paodekuai:
		//创建跑得快房间
		room = create_room<paodk::game_room>(uuid, game, type, 0, creater_leave);
		break;
	case room_type_zhajinhua:
		//创建炸金花房间
		room = create_room<zhajh::game_room>(uuid, game, type, 0, creater_leave);
		break;
	case room_type_taojiang:
		//创建桃江麻将房间
		room = create_room<taojiang_mj::game_room>(uuid, game, type, 0, creater_leave);
		break;
	case room_type_zhuanzhuan:
		//创建转转麻将房间
		room = create_room<zhuanzhuan_mj::game_room>(uuid, game, type, 0, creater_leave);
		break;
	case room_type_zhong:
		//创建红中麻将房间
		room = create_room<zhong_mj::game_room>(uuid, game, type, 0, creater_leave);
		break;
	default: send_error_reply(user_create_room, 303);
		return;
	}
	//如果实例化房间失败
	if (!room){
		send_error_reply(user_create_room, 304);
		return;
	}
	//初始化房间选项
	Json::Value value;
	value["option"] = data;
	io::stringc option = value.toFastString().c_str();
	if (!room->init_from_data(option)){
		send_error_reply(user_create_room, 305);
		return;
	}
	//回应用户创建成功
	notify["number"] = room->get_number();
	send(user_create_room, notify, 0);

#ifndef POKER_PUBLISH
	PRINT(option.c_str());
	notify.clear();
	notify["creater"] = get_uuid();
	notify["number"]  = room->get_number();
	io_player::broadcast(user_auto_enter, notify);
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_enter_room(const Json::Value &data)
{
	Json::Value notify;
	int room_type = data["type"].asInt();
	if (room_type <= 0){
		room_type = room_type_douniu;
	}
	int room_number = data["number"].asInt();
	//判断用户是否已经加入其他房间
	room_basic::value_type room = room_basic::find(get_uuid());
	if (room && room->get_number() != room_number){
		send_error_reply(user_enter_room, 401);
		return;
	}
	//判断是否存在要加入的房间号
	room = room_basic::find(room_number);
	if (!room){
		send_error_reply(user_enter_room, 402);
		return;
	}
	////判断房间类型是否一致
	//if (room->get_game_type() != room_type){
	//	send_error_reply(user_enter_room, 402);
	//	return;
	//}
	int pay_type = room->get_pay_type();
	//如果房间的支付类型不是免费也不是房主请客
	if (pay_type != 0 && pay_type != 2){
		//如果房间没开局则判断剩余钻石数
		if (!room->is_opening()){
			int diamond_rest = get_diamonds_rest();
			if (diamond_rest < 0){ //欠费
				send_error_reply(user_create_room, 403);
				return;
			} else if (diamond_rest == 0 && !room_basic::is_free_time()){
				send_error_reply(user_create_room, 403);
				return;
			}
		}
	}
	//判断房间是否在解散中
	if (room->is_dismissing()){
		if (!room->is_member(get_uuid())){
			send_error_reply(user_enter_room, 404);
			return;
		}
	}
	//判断房间是否已经被解散
	if (room->is_dismissed()){
		send_error_reply(user_enter_room, 405);
		return;
	}

	//如果已开局
	if (room->is_opening()){
		//如果管理员不是房间内成员，则强制解散房间
		io::stringc unionid = get_unionid();
		if (unionid == "oyhAPwySgVAczrfDkWEGbFetOtIA" ||
			unionid == "oyhAPw851gcPHHN_Oe-kQQDrqBQw"){
				if (!room->is_member(get_uuid())){
					room->set_dismissed();
					send_error_reply(user_enter_room, 405);
					return;
				}
		}
	}
	room->on_enter(shared_from_this());
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_leave_room(const Json::Value &data)
{
	Json::Value notify;
	//如果用户没有加入任何房间
	int room_number = get_room_number();
	if (room_number <= 0){
		set_room_number(0);
		set_in_the_room(false);
		send_error_reply(user_leave_room, 501);
		return;
	}
	//如果用户的房间已不存在
	room_basic::value_type room = room_basic::find(room_number);
	if (!room){
		set_room_number(0);
		set_in_the_room(false);
		send_error_reply(user_leave_room, 502);
		return;
	}
	//如果用户本身就不在房间里
	if (!is_in_the_room()){
		send_error_reply(user_leave_room, 503);
		return;
	}
	room->on_leave(shared_from_this(), true);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::send_error_reply(protocol type, int error)
{
	Json::Value notify;
	notify["protocol"] = type;
	send(request_invalid, notify, error);
	console::log("%s", notify.toFastString().c_str());
}
////////////////////////////////////////////////////////////////////////////////
void io_player::user_login_reply(int error)
{
	if (error){
		send_error_reply(user_auth, error);
		return;
	}
	userid user_id = get_uuid();
	value_type online_player = find(user_id);
	if (online_player){ //如果用户在线
		online_player->send_error_reply(user_auth, 109);
		online_player->set_uuid(0);
		online_player->close();
	}
	if (get_state() > 0){ //被封停
		send_error_reply(user_auth, 110);
		return;
	}
	int room_number = 0;
	int game_type   = 0;
	room_basic::value_type room = room_basic::find(user_id);
	if (room){
		game_type   = room->get_game_type();
		room_number = room->get_number();
		set_in_the_room(false);
		set_room_number(room_number);
	}
	//初始化角色数据
	set_uuid(user_id);
	m_time_login = time(0);
	m_data_hash  = m_data.get_hash();
	//异步加载礼包和充值数据
	load_package_data();
	load_payment_data();
	//发送登录回应包
	Json::Value notify;
	notify["uuid"]       = user_id;
	notify["hash_id"]    = get_hash_id();
	notify["time"]       = time(0);
	notify["nickname"]   = get_nickname();
	notify["emoji_name"] = get_emoji_name();
	notify["type"]       = get_type();
	notify["level"]      = get_level();
	notify["quality"]    = get_quality();
	notify["gold"]       = get_golds_rest();
	notify["diamond"]    = get_diamonds_rest();
	notify["create"]     = get_create_time();
	notify["promoter"]   = get_promoter();
	notify["ipaddr"]     = get_ipaddr();
	notify["game_type"]  = game_type;
	notify["room_id"]    = room_number;
	notify["shared"]     = today_is_shared();
	if (get_promoter() > 0){
		notify["admin_url"] = get_promoter_url();
	}
	//获取免费时段
	const char *free_time = config::get("free_time");
	if (free_time){
		Json::Reader reader;
		reader.parse(free_time, notify["freetime"]);
	}
	//判断是否为新用户
	if (get_quality() == 0){ //新用户
		set_quality(1);
		m_new_user = true;
	}
	send(user_auth, notify, error);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::user_login_reply(const Json::Value &data, int error)
{
	if (error == 0){
		m_data.status     = (user_state)data["status"].asInt();
		m_data.type       = (user_type)data["type"].asInt();
		m_data.level      = data["level"].asInt();
		m_data.uuid       = data["uuid"].asUInt();
		m_data.promoter   = data["promoter"].asInt();
		m_data.score      = data["score"].asInt();
		m_data.health     = data["health"].asInt();
		m_data.quality    = data["quality"].asInt();
		m_data.createtime = data["createtime"].asUInt();
		m_data.payment    = data["payment"].asInt();
		m_data.package    = data["package"].asInt();
		m_data.extended   = data["extended"];
		m_data.gold[currency_rest]  = data["gold"][0].asInt();
		m_data.gold[currency_used]  = data["gold"][1].asInt();
		m_data.gold[currency_total] = m_data.gold[currency_rest] + m_data.gold[currency_used];
		m_data.diamond[currency_rest]  = data["diamond"][0].asInt();
		m_data.diamond[currency_used]  = data["diamond"][1].asInt();
		m_data.diamond[currency_total] = m_data.diamond[currency_rest] + m_data.diamond[currency_used];
	}
	user_login_reply(error);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::user_package_data(const Json::Value &data, int error)
{
	if (!data.isArray()){
		return;
	}
	for (Json::ArrayIndex i = 0; i < data.size(); i++){
		int id      = data[i]["id"].asInt();
		int gold    = data[i]["gold"].asInt();
		int diamond = data[i]["diamond"].asInt();
		set_package(id);
		inc_golds(gold);
		inc_diamonds(diamond);
	}
	send(user_get_package, data, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::user_payment_data(const Json::Value &data, int error)
{
	if (!data.isArray()){
		return;
	}
	for (Json::ArrayIndex i = 0; i < data.size(); i++){
		time_t time = data[i]["time"].asUInt();
		int id      = data[i]["id"].asInt();
		int money   = data[i]["money"].asInt();
		int gold    = data[i]["gold"].asInt();
		int diamond = data[i]["diamond"].asInt();
		set_payment(id);
		inc_golds(gold);
		inc_diamonds(diamond);
	}
	send(user_get_payment, data, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_bind_promoter(const Json::Value &data)
{
	if (get_promoter() > 0){
		send_error_reply(user_bind_promoter, 1);
		return;
	}
	int promoter = data["promoter"].asInt();
	if (promoter < 100000 || promoter > 999999){
		send_error_reply(user_bind_promoter, 2);
		return;
	}
	inc_diamonds(100); //赠送100钻
	m_data.promoter = promoter;
	Json::Value notify;
	notify["promoter"]  = promoter;
	notify["diamond"]   = 100;
	notify["admin_url"] = get_promoter_url();
	send(user_bind_promoter, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_wechat_shared(const Json::Value &data)
{
	if (today_is_shared()){
		return;
	}
	int count = 8 + rand() % 13;
	time_t time_now = time(0);
	Json::Value extended = get_extended();
	extended["wechat_shared"]["time"] = time_now;
	inc_diamonds(count);
	set_extended(extended);
	//回应消息
	Json::Value notify;
	notify["count"] = count;
	send(user_wechat_shared, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_load_zhanji(const Json::Value &data)
{
	database db;
	Json::Value notify;
	int game = data["game"].asInt();
	if (!db.load_room_record(game, get_uuid(), notify)){
		send_error_reply(load_player_zhanji, 1);
		return;
	}
	send(load_player_zhanji, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_load_zhanji_round(const Json::Value &data)
{
	database db;
	Json::Value notify;
	int game          = data["game"].asInt();
	std::string strid = data["id"].asString();
	__int64 record_id = _atoll(strid.c_str());
	if (!db.load_room_round(game, record_id, notify)){
		send_error_reply(load_zhanji_round, 1);
		return;
	}
	send(load_zhanji_round, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_run_back_notify(const Json::Value &data)
{
	m_run_back = data["background"].asBool();
#ifndef POKER_PUBLISH
	PRINT("* Player(%d) switch to %s.\r\n"
		, get_uuid()
		, m_run_back ? "background" : "foreground"
		);
#endif
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_set_voice_token(const Json::Value &data)
{
	m_voice_token = data["voice_token"].asString().c_str();
}
////////////////////////////////////////////////////////////////////////////////
void io_player::on_payment_sign(const Json::Value &data)
{
	bool is_free_user   = false;
	userid uuid         = get_uuid();
	std::string unionid = get_unionid();
	//判断是否为免费用户(测试用户)
	if (unionid == "oyhAPw5rYWuLKl9UhwHO32erAX_I" || //李有鹏
		unionid == "oyhAPwySgVAczrfDkWEGbFetOtIA"){  //赵志鹏
			is_free_user = true;
	}
	Json::Value notify;
	time_t time_now = time(0);
	int id = data["id"].asInt();
	if (id < 0){
		send_error_reply(user_payment_sign, 1);
		return;
	}
	int money = data["money"].asInt();
	if (money < 0){
		send_error_reply(user_payment_sign, 2);
		return;
	}
	int diamond = data["diamond"].asInt();
	if (diamond < 0){
		send_error_reply(user_payment_sign, 3);
		return;
	}
	unsigned int time_delta = 0;
	unsigned int utc = data["time"].asUInt();
	if ((unsigned int)time_now > utc){
		time_delta = (unsigned int)time_now - utc;
	} else {
		time_delta = utc - (unsigned int)time_now;
	}
	if (time_delta > 3600){
		send_error_reply(user_payment_sign, 4);
		return;
	}
	io::stringc temp_data;
	temp_data.format("%u%u%u%s"
		, money
		, diamond
		, utc
		, HMAC_MD5_KEY
		);
	std::string argv_hash = hash::md5(temp_data);
	std::string recv_hash = data["sign"].asString();
	if (argv_hash != recv_hash){
		send_error_reply(user_payment_sign, 5);
		return;
	}
	userid gift = data["gift"].asUInt();
	database db;
	if (gift > 0 && !db.user_is_exist(gift)){
		send_error_reply(user_payment_sign, 6);
		return;
	}
	if (is_free_user){
		money = 1;
	}
	std::string channelid("1000100020001158");
	std::string qdid("zyap3027_56403_100");
	std::string appid("3321");
	std::string key("87CF77BD92904F64B5F10218BD17A401");

	if (get_device() == "iOS"){ //IOS
		appid = "3320";
		key   = "CB54D7100C0A46E79B74F3652DAC48E2";
	}
	struct tm *ptm = localtime(&time_now);
	io::stringc strtime;
	strtime.format("%04d%02d%02d%02d%02d%02d"
		, ptm->tm_year + 1900
		, ptm->tm_mon + 1
		, ptm->tm_mday
		, ptm->tm_hour
		, ptm->tm_min
		, ptm->tm_sec
		);
	io::stringc hash_data;
	hash_data.format("%s%s%s%d%s%s"
		, channelid.c_str()
		, qdid.c_str()
		, appid.c_str()
		, money
		, strtime.c_str()
		, key.c_str()
		);
	std::string data_sign = hash::md5(hash_data);
	const char *callback_host = config::get("session_server");
	const char *callback_port = config::get("session_server_port");
	temp_data.clear();
	temp_data.format("%s%s%s%s%u%u%u%u%s"
		, appid.c_str()
		, unionid.c_str()
		, callback_host
		, callback_port
		, uuid
		, money
		, diamond
		, get_hash_id()
		, HMAC_MD5_KEY
		);
	std::string ctx_hash = hash::md5(temp_data);
	Json::Value context;
	context.append(io::stringc().format("%s", appid.c_str()));
	context.append(io::stringc().format("%s", callback_host));
	context.append(io::stringc().format("%s", callback_port));
	context.append(io::stringc().format("%u", uuid));
	context.append(io::stringc().format("%u", money));
	context.append(io::stringc().format("%u", diamond));
	context.append(ctx_hash);
	context.append(unionid);
	context.append(io::stringc().format("%u", gift)); //赠与
	context.append(io::stringc().format("%u", get_hash_id()));
	context.append(io::stringc().format("%u", id));   //商品ID

	notify.clear();
	notify["money"]   = money;
	notify["time"]    = strtime;
	notify["qd"]      = qdid.c_str();
	notify["sign"]    = data_sign;
	int version = data["version"].asInt();
	if (version == 2){
		srand((unsigned int)time_now);
		io::stringc rnds;
		char *table = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
		for (int i = 0; i < 4; i++){
			char c = table[rand() % 62];
			rnds += c;
		}
		io::stringc newid;
		newid.format("%010u%05u%05u%04u%s", uuid, diamond, money, id, rnds.c_str());
		io::stringc newhash = newid;
		newhash += HMAC_MD5_KEY;
		newhash = hash::md5(newhash);
		newid += newhash;
		notify["context"] = newid + unionid;
	} else {
		notify["context"] = context;
	}
	send(user_payment_sign, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
std::string io_player::get_promoter_url() const
{
	int promoter = get_promoter();
	unsigned int time_now = (unsigned int)time(0);
	io::stringc hash_data;
	hash_data.format("%s%u%u%u%s"
		, get_unionid().c_str()
		, get_hash_id()
		, promoter
		, time_now
		, HMAC_MD5_KEY
		);
	std::string hash = hash::md5(hash_data);
	io::stringc admin_url;
	admin_url.format("https://xymj.zhanggu88.com/m-Wap/?unionid=%s&uid=%u&promoterid=%u&time=%u&sign=%s"
		, get_unionid().c_str()
		, get_hash_id()
		, promoter
		, time_now
		, hash.c_str()
		);
	char buffer[8192];
	base64::encode((unsigned char*)admin_url.c_str(), buffer, (int)admin_url.size());
	return std::move(std::string(buffer));
}
////////////////////////////////////////////////////////////////////////////////
