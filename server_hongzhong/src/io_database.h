

#ifndef __IO_DATABASE_H_
#define __IO_DATABASE_H_
////////////////////////////////////////////////////////////////////////////////
#include <inc/sqlnet.h>
#include "room_basic.h"
////////////////////////////////////////////////////////////////////////////////
class database{
    sqlnet3 m_db;
    bool open_memory_db();
    bool open_room_db();
    bool open_player_db();
    bool open_record_db();
    bool open_majiang_db();
public:
    database();
    virtual ~database();
    virtual void close();
    bool user_is_exist(userid uuid);
    bool hashid_to_uuid(unsigned int hashid, userid &uuid);
    bool uuid_to_hashid(userid uuid, unsigned int &hashid);
    bool order_is_exist(const io::stringc &pfid, const io::stringc &orderid);
    bool dec_offline_diamond(userid uuid, int count);
    bool init_player_data (const Json::Value &data);
    bool save_player_data (const Json::Value &data);
    bool load_player_data (const io::stringc &pfid, const io::stringc &unionid, Json::Value &output);
    bool load_package_data(int minid, time_t user_create, Json::Value &output, int occasion = 0);
    bool load_payment_data(userid uuid, int payid, Json::Value &output);
    bool init_payment_data(userid uuid, const io::stringc &pfid, const io::stringc &orderid, int money, int gold, int diamond, userid gift);
public:
    bool init_memory_table();
    bool free_memory_table();
    bool insert_new_session(userid uuid, const io::stringc &unionid, const io::stringc &nickname, const io::stringc &ipaddr);
    bool delete_old_session(userid uuid);
    bool load_tjmj_promoter(unsigned int hash_id, int &promoter);
    bool save_tjmj_promoter(unsigned int hash_id, int promoter);
public:
    bool load_created_room();
    bool save_created_room(const room_basic::value_type room, const io::stringc &data);
    bool init_created_room(const room_basic::value_type room, const io::stringc &data);
    bool free_created_room(const room_basic::value_type room);
public:
    bool init_room_record(const room_basic::value_type room, const Json::Value &members);
    bool create_room_round(__int64 record_id, int round, int total, int vitis_code, const io::stringc &data, const io::stringc &score);
    bool load_room_record(int game, userid uuid, Json::Value &record);
    bool load_room_round(int game, __int64 record_id, Json::Value &record);
public:
    static bool init();
    static void release();
};
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_DATABASE_H_
