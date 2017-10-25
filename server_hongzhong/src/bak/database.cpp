

#include "database.h"
#include "io_handler.h"
////////////////////////////////////////////////////////////////////////////////
#ifdef NPX_WINDOWS
#pragma comment(lib, "sqlnet.lib")
#endif
////////////////////////////////////////////////////////////////////////////////
static Json::Value config;
static unsigned int time_begin = 0;
static std::string remote_db, local_db;
static const unsigned short db_port = 8433;
////////////////////////////////////////////////////////////////////////////////
bool init_database()
{
    char *filename = "./config.json";
    FILE *fp = fopen(filename, "r");
    if (fp){
        char buffer[256];
        size_t n = fread(buffer, 1, sizeof(buffer) - 1, fp);
        buffer[n] = 0;
        fclose(fp);

        Json::Reader reader;
        if (reader.parse(buffer, config)){
            remote_db  = config["remote"].asString();
            local_db   = config["local"].asString();
            time_begin = config["time_begin"].asUInt();
        }
    }
    if (remote_db.empty()){
        remote_db = "127.0.0.1";
    }
    if (local_db.empty()){
        local_db = "127.0.0.1";
    }
    return sqlnet3::startup();
}
////////////////////////////////////////////////////////////////////////////////
void free_database()
{
    sqlnet3::cleanup();
}
////////////////////////////////////////////////////////////////////////////////
bool open_database(sqlnet3 &db, const char *name)
{
    const char *host = "127.0.0.1";
    if (name == "remote"){
        host = remote_db.c_str();
    } else if (name == "local") {
        host = local_db.c_str();
    }
    int result = db.open(host, db_port, "player", "<auto>", 6, SQLNET_OPEN_COMPRESS);
    if (!result){
        printf("%s: %s\r\n", __FUNCTION__, db.get_lasterr());
    }
    return (result ? true : false);
}
////////////////////////////////////////////////////////////////////////////////
void close_database(sqlnet3 &db)
{
    db.close();
}
////////////////////////////////////////////////////////////////////////////////
bool copy_player_data()
{
    sqlnet3 remote, local, insert;
    if (!open_database(remote, "remote")){
        printf("无法打开远程数据库...\r\n");
        return false;
    }
    if (!open_database(local, "local") || !open_database(insert, "local")){
        printf("无法打开本地数据库...\r\n");
        return false;
    }
    std::string sql = "select * from [room_player] where [time_update] > ? order by [time_update]";
    if (!remote.prepair(sql.c_str())){
        printf("%s: %s\r\n", __FUNCTION__, remote.get_lasterr());
        return false;
    }
    sql = "update [room_player] set [type] = ?, [level] = ?, [promoter] = ?, [state] = ?, [score] = ?, [nickname] = ?, [hint_card] = ?, [hint_time] = ?, [group] = ?, [expired] = ?, [joined] = ?, [payforid] = ?, [room_card] = ?, [time_update] = ?, [jsonext] = ? where [uuid] = ?";
    if (!local.prepair(sql.c_str())){
        printf("%s: %s\r\n", __FUNCTION__, local.get_lasterr());
        return false;
    }
    sql = "insert into [room_player]([uuid],[server_id],[type],[level],[promoter],[state],[score],[device],[pfid],[account],[sex],[nickname],[city],[hint_card],[hint_time],[group],[expired],[joined],[payforid],[room_card],[tm_year],[tm_mon],[tm_day],[tm_hour],[tm_min],[tm_sec],[time_create],[time_update],[jsonext]) values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    if (!insert.prepair(sql.c_str())){
        printf("%s: %s\r\n", __FUNCTION__, insert.get_lasterr());
        return false;
    }
    remote.bind_int64(1, (__int64)time_begin);
    if (!remote.execute()){
        printf("%s: %s\r\n", __FUNCTION__, remote.get_lasterr());
        return false;
    }
    while (!remote.is_eof()){
        int index = 1;
        io::stringc nickname(remote.get_text ("nickname"));
        nickname = nickname.to_ascii();
        local.reset();
        local.bind_int  (index++, remote.get_int  ("type"));
        local.bind_int  (index++, remote.get_int  ("level"));
        local.bind_int  (index++, remote.get_int  ("promoter"));
        local.bind_int  (index++, remote.get_int  ("state"));
        local.bind_int  (index++, remote.get_int  ("score"));
        local.bind_text (index++, remote.get_text ("nickname"));
        local.bind_int  (index++, remote.get_int  ("hint_card"));
        local.bind_int64(index++, remote.get_int64("hint_time"));
        local.bind_int  (index++, remote.get_int  ("group"));
        local.bind_int64(index++, remote.get_int64("expired"));
        local.bind_int64(index++, remote.get_int64("joined"));
        local.bind_int64(index++, remote.get_int64("payforid"));
        local.bind_int  (index++, remote.get_int  ("room_card"));
        local.bind_int64(index++, remote.get_int64("time_update"));
        local.bind_text (index++, remote.get_text ("jsonext"));
        local.bind_int64(index++, remote.get_int64("uuid"));
        if (!local.execute()){
            printf("%s: %s\r\n", __FUNCTION__, local.get_lasterr());
            break;
        }
        if (local.get_effect() == 1){
            printf("%s [UPDATE]\r\n", nickname.c_str());
        } else {
            index = 1;
            insert.reset();
            insert.bind_int64(index++, remote.get_int64("uuid"));
            insert.bind_int  (index++, remote.get_int  ("server_id"));
            insert.bind_int  (index++, remote.get_int  ("type"));
            insert.bind_int  (index++, remote.get_int  ("level"));
            insert.bind_int  (index++, remote.get_int  ("promoter"));
            insert.bind_int  (index++, remote.get_int  ("state"));
            insert.bind_int  (index++, remote.get_int  ("score"));
            insert.bind_text (index++, remote.get_text ("device"));
            insert.bind_text (index++, remote.get_text ("pfid"));
            insert.bind_text (index++, remote.get_text ("account"));
            insert.bind_text (index++, remote.get_text ("sex"));
            insert.bind_text (index++, remote.get_text ("nickname"));
            insert.bind_text (index++, remote.get_text ("city"));
            insert.bind_int  (index++, remote.get_int  ("hint_card"));
            insert.bind_int64(index++, remote.get_int64("hint_time"));
            insert.bind_int  (index++, remote.get_int  ("group"));
            insert.bind_int64(index++, remote.get_int64("expired"));
            insert.bind_int64(index++, remote.get_int64("joined"));
            insert.bind_int64(index++, remote.get_int64("payforid"));
            insert.bind_int  (index++, remote.get_int  ("room_card"));
            insert.bind_int  (index++, remote.get_int  ("tm_year"));
            insert.bind_int  (index++, remote.get_int  ("tm_mon"));
            insert.bind_int  (index++, remote.get_int  ("tm_day"));
            insert.bind_int  (index++, remote.get_int  ("tm_hour"));
            insert.bind_int  (index++, remote.get_int  ("tm_min"));
            insert.bind_int  (index++, remote.get_int  ("tm_sec"));
            insert.bind_int64(index++, remote.get_int64("time_create"));
            insert.bind_int64(index++, remote.get_int64("time_update"));
            insert.bind_text (index++, remote.get_text ("jsonext"));
            if (!insert.execute()){
                printf("%s: %s\r\n", __FUNCTION__, insert.get_lasterr());
                break;
            } else {
                printf("%s [INSERT]\r\n", nickname.c_str());
            }
        }
        time_begin = (unsigned int)remote.get_int64("time_update");
        remote.move_next();
    }
    close_database(remote);
    close_database(local);
    config["time_begin"] = time_begin;
    std::string content = config.toFastString();
    FILE *fpw = fopen("./config.json", "w");
    if (fpw){
        fwrite(content.c_str(), 1, content.size(), fpw);
        fclose(fpw);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
