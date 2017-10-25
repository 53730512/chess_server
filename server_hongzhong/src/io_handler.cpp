

#include "io_handler.h"
#include "io_config.h"
#include "io_database.h"
////////////////////////////////////////////////////////////////////////////////
io_handler* io_handler::get_instance()
{
	static std::auto_ptr<io_handler> __instance;
	if (!__instance.get())
		__instance.reset(new io_handler());
	return __instance.get();
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::close(http::channal id)
{
	http::close(id);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::close(http_context context)
{
	http::close(context);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::send(http_context context, const std::string &data)
{
	if (!context) //无效会话
		return;
	http::send(data, context);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::send(http_context context, int protocol, const Json::Value &data, int errcode)
{
	Json::Value notify;
	notify["type"]  = protocol;
	notify["error"] = errcode;
	notify["data"]  = data;
	send(context, notify.toFastString());
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::send(http::channal id, int protocol, const Json::Value &data, int errcode)
{
	send(http::get_session(id), protocol, data, errcode);
}
////////////////////////////////////////////////////////////////////////////////
bool io_handler::send_to_cache(int protocol, const Json::Value &data, int errcode)
{
	if (!m_session_cache)
		return false;
	return send(m_session_cache, protocol, data, errcode), true;
}
////////////////////////////////////////////////////////////////////////////////
io_type io_handler::get_io_type(http_context context)
{
	return (io_type)(__int64)context->context;
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_request(http_context context)
{
	http::set_timeout(300);
	const std::string method = http::get_query_type();
	http::session_type type = http::get_session_type();
	switch (type){
	case http::type_http:
		if (method == "GET"){
			on_http_get(context);  //HTTP GET请求
		} else if (method == "POST") {
			on_http_post(context); //HTTP POST请求
		}
		break;
	case http::type_web_socket:
		on_ws_request(context);    //Websocket请求
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_create(const void *context)
{
	database::init();
	m_title_idle = 0;
	m_mem_inited = false;
	m_run_type = (io_type)(int)context;
	if (m_run_type != type_internal){
		room_basic::read_from_cache();
		m_run_type = type_player;
		context = (const void*)type_internal;

		const char *host = config::get("session_server");
		const char *port = config::get("session_server_port");
		http::connect(host, (unsigned short)atoi(port), context);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_update(int delta)
{
	if (m_run_type == type_player){
		io_player::update(delta);
		room_basic::update(delta);

		m_title_idle += delta;
		if (m_title_idle >= 1000){
			m_title_idle = 0;
			size_t room_count = room_basic::get_room_count();
			size_t player_count = io_player::get_player_count();
			HWND hwnd = GetConsoleWindow();
			if (hwnd){
				io::stringc title;
				title.format("房间数: %d  在线数: %d", (int)room_count, (int)player_count);
				SetWindowTextA(hwnd, title.c_str());
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_timer(http_context context)
{
	io_type type = get_io_type(context);
	if (type == type_player){
		io_player::value_type player = io_player::find(context);
		if (player && player->get_uuid()){
			player->on_timer();
		}
	} else {
		time_t tm_now  = time(0);
		auto iter = m_online_users.begin();
		for (; iter != m_online_users.end();){
			time_t tm_last = iter->second.last_active;
			if (tm_now - tm_last > 60){
				m_online_users.erase(iter++);
			} else {
				iter++; //条件不满足，指向下面一个结点
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_connect(http_context context)
{
	on_ws_connect(context);
	console::log("* Connect to server successed.\r\n");
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_accept(http_context context)
{
	if (!http::verify_url(context)){
		close(context);
		return;
	}
	http::uri &uri = context->request.uri;
	io::stringc argv1 = uri.get_param("context").c_str();
	if (argv1.empty()){
		context->context = (void*)type_player;
	} else {
		context->context = (void*)atoi(argv1.c_str());
	}
	on_ws_accept(context);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_error(http_context context, int code)
{
	io_type type = get_io_type(context);
	if (type == type_player){
		io_player::value_type player = io_player::find(context);
		if (player && player->get_uuid()){
			player->on_error(code);
		}
		io_player::destroy(player);
	} else {
		if (context->status != http::conn_accepted){
			m_session_cache.reset();
			http::reconnect(context); //重建会话
			const char *host    = context->ipaddr.c_str();
			unsigned short port = context->port;
			console::log("* Reconnecting to remote: %s:%d...\r\n", host, port);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_timeout(http_context context)
{
	io_type type = get_io_type(context);
	if (type == type_player){
		io_player::value_type player = io_player::find(context);
		if (player && player->get_uuid()){
			player->on_timeout();
		}
	} else {

	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_destroy()
{
	if (m_run_type == type_player){
		room_basic::write_to_cache();
		reset_user_cache();
		PRINT("* Server exit completed...\r\n");
	}
	if (m_mem_inited){
		database mem;
		mem.free_memory_table();
	}
	database::release();
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_ws_connect(http_context context)
{
	io_type type = get_io_type(context);
	if (type == type_internal){
		m_session_cache = context;
		PRINT("* The session connected to %s:%d\r\n"
			, context->ipaddr.c_str()
			, context->port
			);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_ws_accept(http_context context)
{
	io_type type = get_io_type(context);
	if (type == type_player){
		//启用数据包压缩
		http::set_compress(true);
		io_player::value_type player = io_player::find(context);
		if (!player){
			player = io_player::create(context);
		}
	} else {
		if (!m_mem_inited){
			database mem;
			m_mem_inited = mem.init_memory_table();
		}
		//格式化时间
		time_t time_now = time(0);
		struct tm *ptm = localtime(&time_now);
		io::stringc log_time;
		log_time.format("%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		//打印日志
		PRINT("* [%s] The server connected from %s:%d\r\n"
			, log_time.c_str()
			, context->ipaddr.c_str()
			, context->port
			);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_ws_request(http_context context)
{
	io_type type = get_io_type(context);
	if (type == type_player){
		//前台连接请求
		io_player::value_type player = io_player::find(context);
		if (player){
			const io::stringc data(http::get_query_data().c_str());
			player->get_uuid() ? player->on_request(data) : player->on_login(data);
		}
	} else if (type == type_internal){
		//后台连接请求
		on_parse_data(context);
	} else {
		close(context); //无效websocket连接请求
		std::string url = context->request.uri.url;
		console::log("* Invalid WebSocket URL: %s\r\n", url.c_str());
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_http_get(http_context context)
{
	const http::uri &uri = context->request.uri;
	std::string hash = hash::md5(uri.url.c_str());
	if (hash == "3aafd66c1ddcaa2b92d88df0682d9c62"){ ///interface/callback/payment/
		on_callback_payment(uri);
	} else if (hash == "9136ab061b3eab12a11ebb19cefe74be"){ ///interface/diamond/give/
		on_diamond_give(uri);
	} else if (hash == "99ac554500c0d6f79934eb750c606662"){ ///interface/request/uuid/
		on_uuid_by_hashid(uri);
	} else if (hash == "25c74aa73e33097ed5a092a2d56abe87"){ ///interface/request/hashid/
		on_hashid_by_uuid(uri);
	} else {
		http::send("<b>Service OK</b>", context);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_http_post(http_context context)
{
	std::string data = http::get_query_data();
	http::send("<b>Service OK</b>", context);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_parse_data(http_context context)
{
	io::stringc data(http::get_query_data().c_str());
	Json::Value packet;
	Json::Reader reader;
	if (!reader.parse(data, packet)){
		return;
	}
	const Json::Value &request = packet["data"];
	protocol type  = (protocol)packet["type"].asInt();
	protocol error = (protocol)packet["error"].asInt();
	//处理不同的协议类型
	switch (type){
	case load_player_data:
		on_load_player(request);
		break;
	case user_activation:
		on_active_user(request);
		break;
	case user_offline:
		on_offline_user(request);
		break;
	case user_cache_reset:
		on_reset_cache(request);
		break;
	case save_player_data:
		on_save_player(request);
		break;
	case load_player_package:
		on_load_package(request);
		break;
	case load_player_payment:
		on_load_payment(request);
		break;
	case user_diamond_change:
		on_diamond_change(request);
		break;
	case dec_offline_diamonds:
		on_offline_diamond(request);
		break;
	case user_online_repeat:
		on_active_user_ret(request, error);
		break;
	case load_player_data_ret:
		on_load_player_ret(request, error);
		break;
	case user_cache_reset_ret:
		on_reset_cache_ret(request, error);
		break;
	case load_player_package_ret:
		on_load_package_ret(request, error);
		break;
	case load_player_payment_ret:
		on_load_payment_ret(request, error);
		break;
	case user_payment_callback:
		on_callback_payment(request, error);
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////
int io_handler::register_user(userid uuid, const io::stringc &unionid, const io::stringc &nickname, const io::stringc &ip)
{
	int error = 0;
	int sid   = get_server_id();
	auto iter = m_online_users.find(uuid);
	if (iter == m_online_users.end()){
		online_t user;
		user.id = get_server_id();
		user.last_active = time(0);
		m_online_users[uuid] = user;
	} else if (sid != iter->second.id){
		time_t tm_now  = time(0);
		time_t tm_last = iter->second.last_active;
		if (tm_now - tm_last < 60){
			return 113;
		}
		m_online_users.erase(iter);
		online_t user;
		user.id = get_server_id();
		user.last_active = tm_now;
		m_online_users[uuid] = user;
	} else {
		iter->second.last_active = time(0);
	}
	if (error == 0){
		database db;
		db.insert_new_session(uuid, unionid, nickname, ip);
		TRACE("* Player online, UID:%u\r\n", uuid);
	}
	return error;
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::reset_user_cache()
{
	if (m_run_type != type_player){
		return;
	}
	//断开所有用户连接
	io_player::clear();
	PRINT("* Clear online user and online cache...\r\n");
	while (io_player::get_player_count()){
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	//通知清除在线缓存
	Json::Value notify;
	if (send_to_cache(user_cache_reset, notify, 0)){
		m_exit_signal.wait(5000); //等待退出信号
	}
}
////////////////////////////////////////////////////////////////////////////////
int io_handler::get_server_id()
{
	return http::get_channal_id();
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_save_player(const Json::Value &data)
{
	database db;
	db.save_player_data(data);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_player(const Json::Value &data)
{
	http::channal channal = data["channal"].asInt();
	io::stringc pfid      = data["pfid"].asString().c_str();
	io::stringc unionid   = data["unionid"].asString().c_str();
	io::stringc nickname  = data["nickname"].asString().c_str();
	io::stringc ipaddr    = data["ipaddr"].asString().c_str();

	database db;
	Json::Value content;
	int error = db.load_player_data(pfid, unionid, content) ? 0 : 110;
	if (!error && !content.isMember("uuid")){
		error = db.init_player_data(data) ? 0 : 111;
		if (!error){
			error = db.load_player_data(pfid, unionid, content) ? 0 : 112;
		}
	}
	userid uuid = content["uuid"].asInt();
	if (!error){
		error = register_user(uuid, unionid, nickname, ipaddr);
		if (error){
			content.clear();
		}
	}
	//返回登录结果给game
	Json::Value notify;
	notify["channal"] = channal;
	notify["content"] = content;
	send(http::get_cur_session(), load_player_data_ret, notify, error);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_offline_diamond(const Json::Value &data)
{
	userid uuid = data["uuid"].asInt();
	int count = data["count"].asInt();
	if (uuid > 0 && count > 0){
		database db;
		db.dec_offline_diamond(uuid, count);
		PRINT("* 因用户(%d)不在线，离线扣除 %d 钻.\r\n", (int)uuid, count);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_package(const Json::Value &data)
{
	http::channal channal = data["channal"].asInt();
	int package           = data["package"].asInt();
	int occasion          = data["occasion"].asInt();
	time_t time_create    = data["createtime"].asUInt();
	//加载礼包数据
	Json::Value content;
	database db;
	if (!db.load_package_data(package, time_create, content, 0)){
		return;
	}
	//返回礼包结果
	Json::Value notify;
	notify["channal"] = channal;
	notify["content"] = content;
	send(http::get_cur_session(), load_player_package_ret, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_payment(const Json::Value &data)
{
	http::channal channal = data["channal"].asInt();
	userid uuid           = data["uuid"].asInt();
	int payment           = data["payment"].asInt();
	//加载礼包数据
	Json::Value content;
	database db;
	if (!db.load_payment_data(uuid, payment, content)){
		return;
	}
	//返回礼包结果
	Json::Value notify;
	notify["channal"] = channal;
	notify["content"] = content;
	send(http::get_cur_session(), load_player_payment_ret, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_diamond_change(const Json::Value &data)
{
	userid uuid = data["uuid"].asInt();
	int count   = data["count"].asInt();
	int remain  = data["remain"].asInt();
	console::log("* The user(%d) uses %d diamonds, and the remaining %d.\r\n", uuid, count, remain);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_active_user(const Json::Value &data)
{
	userid uuid          = data["uuid"].asInt();
	io::stringc unionid  = data["unionid"].asString().c_str();
	io::stringc nickname = data["nickname"].asString().c_str();
	io::stringc ipaddr   = data["ipaddr"].asString().c_str();
	//
	auto iter = m_online_users.find(uuid);
	if (iter != m_online_users.end()){
		int sid = get_server_id();
		if (sid != iter->second.id){
			//用户重复在线
			Json::Value notify;
			notify["uuid"] = uuid;
			send(http::get_session(iter->second.id), user_online_repeat, notify, 0);
			return;
		}
		iter->second.id = sid;
		iter->second.last_active = time(0);
		TRACE("* Player keep-alive, UID:%u\r\n", uuid);
	} else {
		register_user(uuid, unionid, nickname, ipaddr);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_active_user_ret(const Json::Value &data, int error)
{
	userid uuid = data["uuid"].asUInt();
	io_player::value_type player = io_player::find(uuid);
	if (player){
		player->set_uuid(0);
		player->close();
		TRACE("* Player online-repeat, UID:%u\r\n", uuid);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_reset_cache(const Json::Value &data)
{
	int sid = get_server_id();
	auto iter = m_online_users.begin();
	for (; iter != m_online_users.end();){
		if (iter->second.id == sid){
			m_online_users.erase(iter++);
		} else {
			iter++; //条件不满足，指向下面一个结点
		}
	}
	Json::Value notify;
	send(http::get_cur_session(), user_cache_reset_ret, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_offline_user(const Json::Value &data)
{
	database db;
	userid uuid = data["uuid"].asInt();
	db.delete_old_session(uuid);

	auto iter = m_online_users.find(uuid);
	if (iter != m_online_users.end()){
		int sid = get_server_id();
		if (sid != iter->second.id){
			return;
		}
		TRACE("* Player offline, UID:%u\r\n", uuid);
		m_online_users.erase(iter);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_reset_cache_ret(const Json::Value &data, int error)
{
	m_exit_signal.set();
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_player_ret(const Json::Value &data, int error)
{
	http::channal channal = data["channal"].asUInt();
	io_player::value_type player = io_player::find(channal);
	if (player){
		player->user_login_reply(data["content"], error);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_package_ret(const Json::Value &data, int error)
{
	http::channal channal = data["channal"].asUInt();
	io_player::value_type player = io_player::find(channal);
	if (player){
		player->user_package_data(data["content"], error);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_load_payment_ret(const Json::Value &data, int error)
{
	http::channal channal = data["channal"].asUInt();
	io_player::value_type player = io_player::find(channal);
	if (player){
		player->user_payment_data(data["content"], error);
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_callback_payment(const http::uri &uri)
{
	std::string order_id  = uri.get_param("id");
	std::string str_count = uri.get_param("count");
	std::string str_uuid  = uri.get_param("user");
	std::string str_sign  = uri.get_param("sign");
	std::string str_type  = uri.get_param("type");
	std::string str_money = uri.get_param("money");
	std::string str_gift  = uri.get_param("gift");

	io::stringc data;
	data += order_id;
	data += str_count;
	data += str_uuid;
	data += str_money;
	data += HMAC_MD5_KEY;
	std::string hash = hash::md5(data);

	database db;
	Json::Value result;
	userid uuid = (userid)_atoll(str_uuid.c_str());
	userid gift = (userid)_atoll(str_gift.c_str());
#ifndef _DEBUG
	//判断签名是否正确
	if (hash != str_sign){
		result["error"] = "Invalid signature";
		result["user"]  = str_uuid;
		result["order"] = order_id;
		http::send(result.toFastString(), http::get_cur_session());
		return;
	}
	//判断用户是否存在
	if (!db.user_is_exist(uuid)){
		result["error"] = "Invalid user";
		result["user"]  = str_uuid;
		result["order"] = order_id;
		http::send(result.toFastString(), http::get_cur_session());
		return;
	}
#endif
	io::stringc pfid("wechat");
	int count = atoi(str_count.c_str());
	int money = atoi(str_money.c_str());
	//格式化时间
	time_t time_now = time(0);
	struct tm *ptm = localtime(&time_now);
	io::stringc log_time;
	log_time.format("%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	//如果订单已存在
	if (db.order_is_exist(pfid, order_id.c_str())){
		PRINT("* [%s] 充值订单号重复(已忽略): %s\r\n", log_time.c_str(), order_id.c_str());
	} else {
		if (!db.init_payment_data(uuid, pfid, order_id.c_str(), money, 0, count, gift)){
			result["error"] = "Invalid database";
			result["user"]  = str_uuid;
			result["order"] = order_id;
			http::send(result.toFastString(), http::get_cur_session());
			return;
		}
		auto iter = m_online_users.find(uuid);
		if (iter != m_online_users.end()){
			//返回充值结果给game
			Json::Value notify;
			notify["uuid"] = uuid;
			send(iter->second.id, user_payment_callback, notify, 0);
		}
		PRINT("* [%s] 用户(%d)充值成功: ￥%.2f元(%d钻)\r\n", log_time.c_str(), uuid, (float)money / 100.0f, count);
	}
	result.clear();
	result["user"]  = uuid;
	result["count"] = count;
	result["order"] = order_id;
	http::send(result.toFastString(), http::get_cur_session());
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_callback_payment(const Json::Value data, int error)
{
	userid uuid = (userid)data["uuid"].asInt();
	io_player::value_type player = io_player::find(uuid);
	if (player){
		player->load_payment_data();
	}
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_diamond_give(const http::uri &uri)
{
	Json::Value result;
	std::string str_uuid  = uri.get_param("user");
	std::string str_count = uri.get_param("count");
	userid uuid = (userid)_atoll(str_uuid.c_str());
	int count = atoi(str_count.c_str());

	io::stringc temp;
	temp.format("%u-%s-%s-%d"
		, (unsigned int)time(0)
		, str_uuid.c_str()
		, str_count.c_str()
		, rand()
		);
	database db;
	io::stringc order_id = hash::md5(temp);
	if (!db.init_payment_data(uuid, "xrgame", order_id.c_str(), 0, 0, count, 0)){
		result["error"] = "Invalid database";
		result["user"]  = str_uuid;
		result["order"] = order_id;
		http::send(result.toFastString(), http::get_cur_session());
		return;
	}
	auto iter = m_online_users.find(uuid);
	if (iter != m_online_users.end()){
		//返回充值结果给game
		Json::Value notify;
		notify["uuid"] = uuid;
		send(iter->second.id, user_payment_callback, notify, 0);
	}
	result["user"]  = str_uuid;
	result["order"] = order_id;
	http::send(result.toFastString(), http::get_cur_session());
	//格式化时间
	time_t time_now = time(0);
	struct tm *ptm = localtime(&time_now);
	io::stringc log_time;
	log_time.format("%02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	PRINT("* [%s] 免费赠送用户(%u) %d 个钻石成功.\r\n", log_time.c_str(), uuid, count);
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_uuid_by_hashid(const http::uri &uri)
{
	std::string str_hashid = uri.get_param("hashid");
	unsigned int hashid = (unsigned int)_atoll(str_hashid.c_str());
	database db;
	userid uuid = 0;
	bool result = db.hashid_to_uuid(hashid, uuid);
	if (uuid <= 0){
		result = false;
	}
	Json::Value notify;
	notify["uuid"]   = uuid;
	notify["result"] = result;
	http::send(notify.toFastString(), http::get_cur_session());
}
////////////////////////////////////////////////////////////////////////////////
void io_handler::on_hashid_by_uuid(const http::uri &uri)
{
	std::string str_uuid = uri.get_param("uuid");
	userid uuid = (userid)_atoll(str_uuid.c_str());
	database db;
	unsigned int hashid = 0;
	bool result = db.uuid_to_hashid(uuid, hashid);
	if (hashid <= 0){
		result = false;
	}
	Json::Value notify;
	notify["hashid"] = hashid;
	notify["result"] = result;
	http::send(notify.toFastString(), http::get_cur_session());
}
////////////////////////////////////////////////////////////////////////////////
