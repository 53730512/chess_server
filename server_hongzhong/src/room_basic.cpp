

#include <algorithm>
#include "io_handler.h"
#include "io_database.h"
////////////////////////////////////////////////////////////////////////////////
bool is_rand_inited = false;
std::list<int> free_room_numbers;
std::map<userid, int> joined_mapping;
std::map<int, room_basic::value_type> room_mapping;
////////////////////////////////////////////////////////////////////////////////
room_basic::room_basic(userid creater, game_type type, int ruleid, int number, time_t now, bool owner_exit)
    : m_owner_exit(owner_exit)
    , m_creater(creater)
    , m_type(type)
    , m_rule_id(ruleid)
    , m_number(number)
    , m_elapse(0)
    , m_time_idle(0)
    , m_time_create(now ? now : time(0))
{
    m_pay_type   = 0; //免费
    m_opening    = m_is_dirty = false;
    m_dismissed  = m_dismiss  = m_completed = false;
    m_time_idle  = (time(0) - m_time_create);
    
    io::stringc id;
    id.format("%u%u", get_number(), (unsigned int)m_time_create);
    m_record_id = _atoll(id.c_str());
}
////////////////////////////////////////////////////////////////////////////////
room_basic::~room_basic()
{

}
////////////////////////////////////////////////////////////////////////////////
int room_basic::pop_new_number()
{
    int room_number = 0;
    if (free_room_numbers.size() > 0){
        room_number = free_room_numbers.front();
        free_room_numbers.pop_front();
    }
    return room_number;
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::push_old_number(int number)
{
    free_room_numbers.push_back(number);
}
////////////////////////////////////////////////////////////////////////////////
room_basic::value_type room_basic::find(int number)
{
    auto iter = room_mapping.find(number);
    return (iter != room_mapping.end()) ? iter->second : room_basic::value_type();
}
////////////////////////////////////////////////////////////////////////////////
room_basic::value_type room_basic::find(userid user_id)
{
    auto iter = joined_mapping.find(user_id);
    int room_number = (iter != joined_mapping.end()) ? iter->second : 0;
    return find(room_number);
}
////////////////////////////////////////////////////////////////////////////////
size_t room_basic::get_room_count()
{
    return room_mapping.size();
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::update(int delta)
{
    auto iter = room_mapping.begin();
    for (; iter != room_mapping.end();){
        bool dismissed = iter->second->is_dismissed();
        bool completed = iter->second->is_completed();
        if (dismissed || completed){
            database db;
            db.free_created_room(iter->second);
            iter->second->on_destroy(completed);
            room_mapping.erase(iter++);
        } else {
            iter->second->on_update(delta);
            //如果房间被设置为脏模式则自动保存数据
            if (iter->second->m_is_dirty){
                io::stringc data = iter->second->export_data();
                if (!data.empty()){
                    iter->second->update_data(data);
                }
            }
            iter++; //不满足条件，移动到下个节点
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::submit_gold(userid userid, int count)
{
    //添加代码
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::submit_diamond(userid userid, int count)
{
    //添加代码
    Json::Value notify;
    notify["uuid"]  = userid;
    notify["count"] = count;
    handler()->send_to_cache(dec_offline_diamonds, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::write_to_cache()
{
    PRINT("* Save room data to database...\r\n");
    auto iter = room_mapping.begin();
    for (; iter != room_mapping.end(); iter++){
        io::stringc data = iter->second->export_data();
        if (!data.empty()){
            iter->second->update_data(data);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::read_from_cache()
{
    database db;
    if (!db.load_created_room()){
        PRINT("* Can't load room created from database.\r\n");
        return;
    }
    //初始化空闲房间号
    std::vector<int> number_list;
    for (int i = MIN_ROOM_NUMBER; i < MAX_ROOM_NUMBER + 1; i++){
        number_list.push_back(i);
    }
    //随机打乱房间号
    //srand((unsigned int)time(0));
    std::random_shuffle(number_list.begin(), number_list.end());
    //过滤掉已用过的房间号
    auto iter = number_list.begin();
    for (; iter != number_list.end(); iter++){
        if (!find(*iter)){
            push_old_number(*iter);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
bool room_basic::update_data(const io::stringc &data)
{
    database db;
    m_is_dirty = !db.save_created_room(shared_from_this(), data);
    return (m_is_dirty == false);
}
////////////////////////////////////////////////////////////////////////////////
bool room_basic::is_member(userid user_id) const
{
    return (m_members.find(user_id) != m_members.end());
}
////////////////////////////////////////////////////////////////////////////////
bool room_basic::is_free_time()
{
    const char *free_time = config::get("free_time");
    if (!free_time){
        return false;
    }
    Json::Value tm_free;
    Json::Reader reader;
    if (!reader.parse(free_time, tm_free) || !tm_free.isArray()){
        return false;
    }
    time_t time_now = time(0);
    const struct tm *ptm = localtime(&time_now);
    for (Json::ArrayIndex i = 0; i < tm_free.size(); i++){
        int begin = tm_free[i][0].asInt();
        int end   = tm_free[i][1].asInt();
        if (ptm->tm_hour >= begin && ptm->tm_hour < end){
            return true;
        }
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::insert_member(userid user_id, int index, const io::stringc &nickname, const io::stringc sex, const io::stringc &head_url)
{
    if (user_id){
        io_player::value_type player = io_player::find(user_id);
        if (player){
            player->set_index(index);
            insert_member(player);
        } else {
            member_t member;
            member.uuid            = user_id;
            member.index           = index;
            member.enter_time      = 0;
            member.nickname        = nickname;
            member.sex             = sex;
            member.head_url        = head_url;
            m_members[member.uuid] = member;
            joined_mapping[member.uuid] = get_number();
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::insert_member(io_player::value_type player)
{
    if (player){
        member_t member;
        player->set_in_the_room(true);
        player->set_room_number(get_number());
        member.uuid            = player->get_uuid();
        member.index           = player->get_index();
        member.enter_time      = time(0);
        member.nickname        = player->get_nickname();
        member.sex             = player->get_sex();
        member.head_url        = player->get_head_url();
        m_members[member.uuid] = member;
        joined_mapping[member.uuid] = get_number();
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::delete_member(userid user_id, bool is_exit)
{
    if (user_id){
        io_player::value_type player = io_player::find(user_id);
        if (player){
            delete_member(player, is_exit);
        } else {
            if (!is_exit){
                m_members[user_id].enter_time = 0;
            } else {
                m_members.erase(user_id);
                joined_mapping.erase(user_id);
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::delete_member(io_player::value_type player, bool is_exit)
{
    if (player){
        player->set_in_the_room(false);
        userid user_id = player->get_uuid();
        if (!is_exit){
            m_members[user_id].enter_time = 0;
        } else {
            player->set_room_number(0);
            m_members.erase(user_id);
            joined_mapping.erase(user_id);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::deduct_gold(io_player::value_type player, int count)
{
    if (player){
        player->dec_golds(count);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::deduct_gold(userid user_id, int count)
{
    io_player::value_type player = io_player::find(user_id);
    if (player){
        deduct_gold(player, count);  //在线扣除
    } else {
        submit_gold(user_id, count); //离线扣除
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::deduct_diamond(io_player::value_type player, int count)
{
    if (player){
        player->dec_diamonds(count);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::deduct_diamond(userid user_id, int count)
{
    io_player::value_type player = io_player::find(user_id);
    if (player){
        deduct_diamond(player, count);  //在线扣除
    } else {
        submit_diamond(user_id, count); //离线扣除
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::insert_record(int round, int total, int visit, const io::stringc &data)
{
    if (round <= 1){
        Json::Value members;
        for (auto iter = m_members.begin(); iter != m_members.end(); iter++){
            Json::Value member;
            member["uuid"]     = iter->second.uuid;
            member["index"]    = iter->second.index;
            member["nickname"] = iter->second.nickname;
            member["sex"]      = iter->second.sex;
            member["head_url"] = iter->second.head_url;
            members.append(member);
        }
        database db;
        db.init_room_record(shared_from_this(), members);
    }
    database db;
    io::stringc score = export_score();
    db.create_room_round(get_record_id(), round, total, visit, data, score);
}
////////////////////////////////////////////////////////////////////////////////
bool room_basic::in_the_room(userid user_id) const
{
    auto iter = m_members.find(user_id);
    return (iter != m_members.end() && iter->second.enter_time > 0) ? true : false;
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::send(protocol type, const Json::Value &data, io_player::value_type player, int errcode)
{
    if (player){
        if (!errcode){
            player->send(type, data, errcode);
        } else {
            player->send_error_reply(type, errcode);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::send(protocol type, const Json::Value &data, userid target, int errcode)
{
    send(type, data, io_player::find(target), errcode);
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::send_room(protocol type, const Json::Value &data, int errcode)
{
    for (auto iter = m_members.begin(); iter != m_members.end(); iter++){
        send(type, data, iter->first, errcode);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::send_other(protocol type, const Json::Value &data, userid exclude, int errcode)
{
    for (auto iter = m_members.begin(); iter != m_members.end(); iter++){
        if (iter->first == exclude)
            continue;
        send(type, data, iter->first, errcode);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::send_other(protocol type, const Json::Value &data, io_player::value_type exclude, int errcode)
{
    if (exclude){
        send_other(type, data, exclude->get_uuid(), errcode);
    } else {
        send_other(type, data, 0, errcode);
    }
}
////////////////////////////////////////////////////////////////////////////////
bool room_basic::init_from_data(const io::stringc &data)
{
    if (!import_data(data)){
        return false;
    }
    if (free_room_numbers.size()){
        database db;
        if (!db.init_created_room(shared_from_this(), data)){
            return false;
        }
    }
    //将房间加入列表
    room_mapping[get_number()] = shared_from_this();
    return on_create(), true;
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_create()
{
    if (is_rand_inited == false){
        is_rand_inited = true;
        srand((unsigned int)time(0));
    }
    int room_number = get_number();
    for (auto iter = m_members.begin(); iter != m_members.end(); iter++){
        userid user_id = iter->first;
        joined_mapping[user_id] = room_number;
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_update(int delta)
{
    m_elapse += delta;
    if (m_elapse >= 60000){ //1分钟
        m_elapse = 0;
        m_time_idle += 60;
        if (m_time_idle >= 24 * 3600){
            set_dismissed(); //超过24小时无任何操作则解散房间
        } else if (m_members.empty() && m_time_idle >= 1800){
            set_dismissed(); //空房间30分钟自动解散
        }
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_enter(io_player::value_type player)
{
    if (player){
        insert_member(player);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_leave(io_player::value_type player, bool is_exit)
{
    if (player){
        delete_member(player, is_exit);
    }
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_request(io_player::value_type player, protocol type, const Json::Value &data)
{
    m_time_idle = 0;
}
////////////////////////////////////////////////////////////////////////////////
void room_basic::on_destroy(bool completed)
{
    int room_number = get_number();
    if (!completed){ //提前解散了
        Json::Value notify;
        notify["number"] = room_number;
        send_room(room_dismissed, notify, 0);
    }
    push_old_number(room_number);
}
////////////////////////////////////////////////////////////////////////////////
