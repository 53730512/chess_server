

#include "io_handler.h"
#include "io_database.h"
////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
#pragma comment(lib, "sqlnet.lib")
#endif
////////////////////////////////////////////////////////////////////////////////
database::database()
{

}
////////////////////////////////////////////////////////////////////////////////
database::~database()
{
	close();
}
////////////////////////////////////////////////////////////////////////////////
bool database::init()
{
	return sqlnet3::startup();
}
////////////////////////////////////////////////////////////////////////////////
void database::release()
{
	sqlnet3::cleanup();
}
////////////////////////////////////////////////////////////////////////////////
bool database::open_player_db()
{
	if (m_db.is_open()){
		m_db.close();
	}
	const char *host = config::get("user_database");
	if (!host){
		host = "127.0.0.1";
	}
	const char *strc = config::get("user_database_port");
	if (!strc){
		strc = "8433";
	}
	unsigned short port = (unsigned short)atoi(strc);
	int result = m_db.open(host, port, "poker_users", "<auto>", 6, SQLNET_OPEN_COMPRESS);
	if (!result){
		PRINT("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
	}
	return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
bool database::open_memory_db()
{
	if (m_db.is_open()){
		m_db.close();
	}
	const char *host = config::get("user_database");
	if (!host){
		host = "127.0.0.1";
	}
	const char *strc = config::get("user_database_port");
	if (!strc){
		strc = "8433";
	}
	unsigned short port = (unsigned short)atoi(strc);
	int result = m_db.open(host, port, 0, "<auto>", 6, SQLNET_OPEN_COMPRESS);
	if (!result){
		PRINT("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
	}
	return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
bool database::open_majiang_db()
{
	if (m_db.is_open()){
		m_db.close();
	}
	const char *host = config::get("tjmj_database");
	if (!host){
		host = "127.0.0.1";
	}
	const char *strc = config::get("tjmj_database_port");
	if (!strc){
		strc = "8433";
	}
	unsigned short port = (unsigned short)atoi(strc);
	int result = m_db.open(host, port, "player", "<auto>", 6, SQLNET_OPEN_COMPRESS);
	if (!result){
		PRINT("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
	}
	return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
bool database::open_record_db()
{
	if (m_db.is_open()){
		m_db.close();
	}
	const char *host = config::get("record_database");
	if (!host){
		host = "127.0.0.1";
	}
	const char *strc = config::get("record_database_port");
	if (!strc){
		strc = "8433";
	}
	unsigned short port = (unsigned short)atoi(strc);
	int result = m_db.open(host, port, "poker_record", "<auto>", 6, SQLNET_OPEN_COMPRESS);
	if (!result){
		PRINT("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
	}
	return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
bool database::open_room_db()
{
	if (m_db.is_open()){
		m_db.close();
	}
	const char *host = config::get("room_database");
	if (!host){
		host = "127.0.0.1";
	}
	const char *strc = config::get("room_database_port");
	if (!strc){
		strc = "8433";
	}
	unsigned short port = (unsigned short)atoi(strc);
	int result = m_db.open(host, port, "poker_rooms", "<auto>", 6, SQLNET_OPEN_COMPRESS);
	if (!result){
		PRINT("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
	}
	return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
void database::close()
{
	m_db.close();
}
////////////////////////////////////////////////////////////////////////////////
bool database::user_is_exist(userid uuid)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select [uuid] from [user_account] where [uuid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return (!m_db.is_eof());
}
////////////////////////////////////////////////////////////////////////////////
bool database::hashid_to_uuid(unsigned int hashid, userid &uuid)
{
	uuid = 0;
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select [uuid] from [user_account] where [hashid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (__int64)hashid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (!m_db.is_eof())
		uuid = m_db.get_int("uuid");
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::uuid_to_hashid(userid uuid, unsigned int &hashid)
{
	hashid = 0;
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select [hashid] from [user_account] where [uuid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (!m_db.is_eof())
		hashid = m_db.get_int("hashid");
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::order_is_exist(const io::stringc &pfid, const io::stringc &orderid)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select [id] from [user_payment] where [pfid] = ? and [orderid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, pfid.c_str());
	m_db.bind(index++, orderid.c_str());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return (!m_db.is_eof());
}
////////////////////////////////////////////////////////////////////////////////
bool database::dec_offline_diamond(userid uuid, int count)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("update [user_account] set [diamond] = [diamond] - ?, [diamond_used] = [diamond_used] + ? where [uuid] = ?");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, count);
	m_db.bind(index++, count);
	m_db.bind(index++, (int)uuid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return (!m_db.is_eof());
}
////////////////////////////////////////////////////////////////////////////////
bool database::init_player_data(const Json::Value &data)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("insert into [user_account]([hashid],[pfid],[unionid],[nickname],[sex],[device],[idfa],[country],[province],[city],[ipaddr],[head_url]) values(?,?,?,?,?,?,?,?,?,?,?,?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	userid uuid = data["hashid"].asUInt();
	m_db.bind(index++, (__int64)uuid);
	m_db.bind(index++, data["pfid"].asCString());
	m_db.bind(index++, data["unionid"].asCString());
	m_db.bind(index++, data["nickname"].asCString());
	m_db.bind(index++, data["sex"].asCString());
	m_db.bind(index++, data["device"].asCString());
	m_db.bind(index++, data["idfa"].asCString());
	m_db.bind(index++, data["country"].asCString());
	m_db.bind(index++, data["province"].asCString());
	m_db.bind(index++, data["city"].asCString());
	m_db.bind(index++, data["ipaddr"].asCString());
	m_db.bind(index++, data["head_url"].asCString());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_player_data(const io::stringc &pfid, const io::stringc &unionid, Json::Value &output)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select * from [user_account] where [pfid] = ? and [unionid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, pfid.c_str());
	m_db.bind(index++, unionid.c_str());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (m_db.is_eof()){
		return true;
	}
	output["uuid"]       = m_db.get_int("uuid");
	output["type"]       = m_db.get_int("type");
	output["quality"]    = m_db.get_int("quality");
	output["level"]      = m_db.get_int("level");
	output["score"]      = m_db.get_int("score");
	output["health"]     = m_db.get_int("health");
	output["package"]    = m_db.get_int("package");
	output["status"]     = m_db.get_int("status");
	output["promoter"]   = m_db.get_int("promoter");
	output["payment"]    = m_db.get_int("payment");
	output["createtime"] = (time_t)m_db.get_int("tm_create");
	output["gold"].append(m_db.get_int("gold"));
	output["gold"].append(m_db.get_int("gold_used"));
#ifdef _DEBUG
	output["diamond"].append(1000);
	output["diamond"].append(0);
#else
	output["diamond"].append(m_db.get_int("diamond"));
	output["diamond"].append(m_db.get_int("diamond_used"));
#endif
	//如果没绑定邀请码
	if (!output["promoter"].asInt()){
		if (config::get("tjmj_database")){
			int promoter = 0;
			io::stringc temp;
			temp.format("%s%s", pfid.c_str(), unionid.c_str());
			unsigned int hash_id = hash::chksum32(temp);
			if (load_tjmj_promoter(hash_id, promoter)){
				if (promoter > 0){
					output["promoter"] = promoter;
				}
			}
		}
	}
	//构造扩展数据
	Json::Reader reader;
	std::string reserved(m_db.get_text("reserved"));
	reader.parse(reserved, output["extended"]);
	return close(), true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_tjmj_promoter(unsigned int hash_id, int &promoter)
{
	promoter = 0;
	if (!open_majiang_db()){
		return false;
	}
	io::stringc sql("select [promoter] from [room_player] where [uuid] = ? limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (__int64)hash_id);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (m_db.is_eof()){
		return true;
	}
	promoter = m_db.get_int("promoter");
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::save_tjmj_promoter(unsigned int hash_id, int promoter)
{
	if (!open_majiang_db()){
		return false;
	}
	io::stringc sql("update [room_player] set [promoter] = ? where [uuid] = ? and [promoter] = 0");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, promoter);
	m_db.bind(index++, (__int64)hash_id);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::init_memory_table()
{
	if (!open_memory_db()){
		return false;
	}
	io::stringc sql("select * from [sqlite_master] where [type] = 'table' and [tbl_name] = 'user_session' limit 1");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (!m_db.is_eof()){
		return true;
	}
	io::stringc sql1("CREATE TABLE user_session (uuid INTEGER NOT NULL PRIMARY KEY, unionid VARCHAR NOT NULL, nickname VARCHAR NOT NULL, ipaddr VARCHAR NOT NULL)");
	io::stringc sql2("CREATE INDEX idx_user_session_uuid ON user_session (unionid)");
	io::stringc sql3("CREATE INDEX idx_user_session_nickname ON user_session (nickname)");
	if (m_db.exec(sql1.c_str())){
		m_db.exec(sql2.c_str());
		m_db.exec(sql3.c_str());
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::free_memory_table()
{
	if (!open_memory_db())
		return false;
	return m_db.exec("drop table user_session") ? true : false;
}
////////////////////////////////////////////////////////////////////////////////
bool database::insert_new_session(userid uuid, const io::stringc &unionid, const io::stringc &nickname, const io::stringc &ipaddr)
{
	if (!open_memory_db()){
		return false;
	}
	io::stringc sql("insert into [user_session]([uuid],[unionid],[nickname],[ipaddr]) values(?, ?, ?, ?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, unionid.c_str());
	m_db.bind(index++, nickname.c_str());
	m_db.bind(index++, ipaddr.c_str());
	return (m_db.execute() ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
bool database::delete_old_session(userid uuid)
{
	if (!open_memory_db()){
		return false;
	}
	io::stringc sql("delete from [user_session] where [uuid] = ?");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::save_player_data(const Json::Value &data)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("update [user_account] set [nickname]=?, [sex]=?, [device]=?, [idfa]=?, [country]=?, [province]=?, [city]=?, [ipaddr]=?, [head_url]=?, [quality]=?, [score]=?, [health]=?, [package]=?, [promoter]=?, [gold]=?, [gold_used]=?, [diamond]=?, [diamond_used]=?, [payment]=?, [tm_update]=?, [reserved]=? where [uuid] = ?");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, data["nickname"].asCString());
	m_db.bind(index++, data["sex"].asCString());
	m_db.bind(index++, data["device"].asCString());
	m_db.bind(index++, data["idfa"].asCString());
	m_db.bind(index++, data["country"].asCString());
	m_db.bind(index++, data["province"].asCString());
	m_db.bind(index++, data["city"].asCString());
	m_db.bind(index++, data["ipaddr"].asCString());
	m_db.bind(index++, data["head_url"].asCString());
	m_db.bind(index++, data["quality"].asInt());
	m_db.bind(index++, data["score"].asInt());
	m_db.bind(index++, data["health"].asInt());
	m_db.bind(index++, data["package"].asInt());
	m_db.bind(index++, data["promoter"].asInt());
	m_db.bind(index++, data["gold"][0].asInt());
	m_db.bind(index++, data["gold"][1].asInt());
	m_db.bind(index++, data["diamond"][0].asInt());
	m_db.bind(index++, data["diamond"][1].asInt());
	m_db.bind(index++, data["payment"].asInt());
	m_db.bind(index++, time(0));
	std::string reserved = data["extended"].toFastString();
	m_db.bind(index++, reserved.c_str());
	m_db.bind(index++, data["uuid"].asInt());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	//把邀请码同步到麻将帐号库
	int promoter = data["promoter"].asInt();
	if (promoter > 0 && config::get("tjmj_database")){
		io::stringc temp(data["pfid"].asString().c_str());
		temp += data["unionid"].asString();
		unsigned int hash_id = hash::chksum32(temp);
		save_tjmj_promoter(hash_id, promoter);
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_package_data(int minid, time_t user_create, Json::Value &output, int occasion)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select * from [user_package] where [id] > ? and [user_create] > ? and [occasion] = ? order by [id]");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, minid);
	m_db.bind(index++, user_create);
	m_db.bind(index++, occasion);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	while(!m_db.is_eof()){
		int id       = m_db.get_int("id");
		int diamond  = m_db.get_int("diamond");
		int gold     = m_db.get_int("gold");
		int occasion = m_db.get_int("occasion");
		Json::Value value;
		value["id"]       = id;
		value["diamond"]  = diamond;
		value["gold"]     = gold;
		value["occasion"] = occasion;
		output.append(value);
		m_db.move_next();
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_payment_data(userid uuid, int payid, Json::Value &output)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("select * from [user_payment] where [uuid] = ? and [status] = 0 and [id] > ? order by [id]");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, payid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int last_id = 0;
	while(!m_db.is_eof()){
		int id           = m_db.get_int("id");
		int money        = m_db.get_int("money");
		int gold         = m_db.get_int("gold");
		int diamond      = m_db.get_int("diamond");
		time_t time      = m_db.get_int64("tm_create");
		last_id          = id;
		Json::Value value;
		value["id"]      = id;
		value["money"]   = money;
		value["time"]    = time;
		value["gold"]    = gold;
		value["diamond"] = diamond;
		output.append(value);
		m_db.move_next();
	}
	if (last_id > 0){
		sql = "update [user_payment] set [status] = 1, [tm_obtain] = ? where [uuid] = ? and [status] = 0 and id <= ?";
		if (!m_db.prepair(sql.c_str())){
			printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
			output.clear();
			return false;
		}
		index = 1;
		m_db.bind(index++, time(0));
		m_db.bind(index++, (int)uuid);
		m_db.bind(index++, last_id);
		if (!m_db.execute()){
			printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
			output.clear();
			return false;
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::init_payment_data(userid uuid, const io::stringc &pfid, const io::stringc &orderid, int money, int gold, int diamond, userid gift)
{
	if (!open_player_db()){
		return false;
	}
	io::stringc sql("insert into [user_payment]([uuid],[pfid],[orderid],[money],[gold],[diamond],[reserved]) values(?,?,?,?,?,?,?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	Json::Value reserved;
	if (gift != 0){
		reserved["giver"] = uuid;
		uuid = gift;
	}
	int index = 1;
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, pfid.c_str());
	m_db.bind(index++, orderid.c_str());
	m_db.bind(index++, money);
	m_db.bind(index++, gold);
	m_db.bind(index++, diamond);
	m_db.bind(index++, reserved.toFastString().c_str());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_created_room()
{
	if (!open_room_db()){
		return false;
	}
	io::stringc sql("select * from [room_interim]");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	while (!m_db.is_eof()){
		int number          = m_db.get_int("number");
		int game            = m_db.get_int("game");
		int rule            = m_db.get_int("rule");
		userid creater      = m_db.get_int("creater");
		bool exit           = m_db.get_int("exit") ? true : false;
		io::stringc context = m_db.get_text("context");
		time_t time_create  = m_db.get_int64("tm_create");

		room_basic::value_type room;
		switch (game){
		case room_type_douniu:
			room = create_room<niuniu::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		case room_type_paodekuai:
			room = create_room<paodk::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		case room_type_zhajinhua:
			room = create_room<zhajh::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		case room_type_taojiang:
			room = create_room<taojiang_mj::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		case room_type_zhuanzhuan:
			room = create_room<zhuanzhuan_mj::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		case room_type_zhong:
			room = create_room<zhong_mj::game_room>(creater, (game_type)game, rule, time_create, exit, number);
			break;
		}
		if (room)
			room->init_from_data(context);
		m_db.move_next();
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::save_created_room(const room_basic::value_type room, const io::stringc &data)
{
	if (!open_room_db()){
		return false;
	}
	io::stringc sql("update [room_interim] set [context] = ? where [number] = ?");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, data.c_str());
	m_db.bind(index++, room->get_number());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::init_created_room(const room_basic::value_type room, const io::stringc &data)
{
	if (!open_room_db()){
		return false;
	}
	io::stringc sql("insert into [room_interim]([number],[game],[rule],[creater],[exit],[context]) values(?,?,?,?,?,?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)room->get_number());
	m_db.bind(index++, (int)room->get_game_type());
	m_db.bind(index++, (int)room->get_rule_id());
	m_db.bind(index++, (int)room->get_creater());
	m_db.bind(index++, (int)room->is_owner_exit() ? 1 : 0);
	m_db.bind(index++, data.c_str());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::free_created_room(const room_basic::value_type room)
{
	if (!open_room_db()){
		return false;
	}
	io::stringc sql("delete from [room_interim] where [number] = ?");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)room->get_number());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::init_room_record(const room_basic::value_type room, const Json::Value &members)
{
	if (!open_record_db()){
		return false;
	}
	io::stringc sql("insert into [room_record]([id],[game_type],[number],[creater],[options],[members],[member1],[member2],[member3],[member4],[member5],[member6]) values(?,?,?,?,?,?,?,?,?,?,?,?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	int creater = (int)room->get_creater();
	m_db.bind(index++, room->get_record_id());
	m_db.bind(index++, room->get_game_type());
	m_db.bind(index++, room->get_number());
	m_db.bind(index++, creater);
	m_db.bind(index++, room->get_options().c_str());
	m_db.bind(index++, members.toFastString().c_str());
	for (int i = 0 ; i < 6; i++){
		if (i < (int)members.size()){
			m_db.bind(index++, members[i]["uuid"].asInt());
		} else {
			m_db.bind(index++, 0);
		}
	}
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::create_room_round(__int64 record_id, int round, int total, int vitis_code, const io::stringc &data, const io::stringc &score)
{
	if (!open_record_db()){
		return false;
	}
	io::stringc sql("insert into [room_round]([id],[round],[round_total],[visit_code],[content]) values(?,?,?,?,?)");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, record_id);
	m_db.bind(index++, round);
	m_db.bind(index++, total);
	m_db.bind(index++, vitis_code);
	m_db.bind(index++, data.c_str());
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	sql = "update [room_record] set [score] = ? where [id] = ?";
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	index = 1;
	m_db.bind(index++, score.c_str());
	m_db.bind(index++, record_id);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_room_record(int game, userid uuid, Json::Value &record)
{
	if (!open_record_db()){
		return false;
	}
	io::stringc sql("select * from [room_record] where [game_type] = ? and ([member1] = ? or [member2] = ? or [member3] = ? or [member4] = ? or [member5] = ? or [member6] = ?) order by [tm_create] desc limit 10");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, (int)game);
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, (int)uuid);
	m_db.bind(index++, (int)uuid);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	record.clear();
	while (!m_db.is_eof()){
		Json::Value info;
		io::stringc strid;
		strid.format("%llu", m_db.get_int64("id"));
		info["id"]      = strid;
		info["number"]  = m_db.get_int("number");
		info["creater"] = m_db.get_int("creater");
		info["time"]    = m_db.get_int64("tm_create");
		std::string options = m_db.get_text("options");
		std::string members = m_db.get_text("members");
		std::string score   = m_db.get_text("score");
		Json::Reader reader;
		reader.parse(options, info["options"]);
		reader.parse(members, info["members"]);
		reader.parse(score, info["score"]);
		record.append(info);
		m_db.move_next();
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool database::load_room_round(int game, __int64 record_id, Json::Value &record)
{
	if (!open_record_db()){
		return false;
	}
	io::stringc sql("select * from [room_round] where [id] = ? order by [round]");
	if (!m_db.prepair(sql.c_str())){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	int index = 1;
	m_db.bind(index++, record_id);
	if (!m_db.execute()){
		printf("%s: %s\r\n", __FUNCTION__, m_db.get_lasterr());
		return false;
	}
	record.clear();
	while (!m_db.is_eof()){
		Json::Value info;
		io::stringc strid;
		strid.format("%llu", m_db.get_int64("id"));
		info["id"]          = strid;
		info["visit_code"]  = m_db.get_int64("visit_code");
		info["time"]        = m_db.get_int64("tm_create");
		std::string content = m_db.get_text("content");
		Json::Reader reader;
		reader.parse(content, info["content"]);
		record.append(info);
		m_db.move_next();
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
