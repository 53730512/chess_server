

#include "protocol.h"
#include "../../io_handler.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
Json::Value niuniu_options;
const char *niuniu_options_file = "../rules/niuniu/douniu.json";
////////////////////////////////////////////////////////////////////////////////
game_room::game_room(userid creater, game_type type, int ruleid, int number, time_t time_now, bool owner_exit)
    : room_basic(creater, type, ruleid, number, time_now, owner_exit)
{
    m_round           = 0;
    m_round_total     = 0;
    m_zhifu           = 0;
    m_fanbei          = 0;
    m_diamond_pay     = 0;
    m_diamond_huanpai = 0;
    m_dismiss_index   = -1;
    m_status          = room_wait_ready;
    m_xiazhu.clear();
    m_option.clear();
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
    int  xiazhu  = option["xiazhu"].asInt();
    bool huanpai = option["huanpai"].asBool();
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
    m_round = 0;
    m_round_total = rules["jushu"][jushu].asInt();
    //读取支付方式信息
    if (zhifu < 0 || zhifu >= (int)rules["zhifu"].size()){
        return false;
    }
    m_zhifu = rules["zhifu"][zhifu][0].asInt();
    m_diamond_pay = rules["zhifu"][zhifu][1].asInt();
    //读取番型分数信息
    if (fanbei < 0 || fanbei >= (int)rules["fanbei"].size()){
        return false;
    }
    m_fanbei = rules["fanbei"][zhifu].asInt();
    //读取下注分值信息
    if (xiazhu < 0 || xiazhu >= (int)rules["xiazhu"].size()){
        return false;
    }
    for (Json::ArrayIndex i = 0; i < rules["xiazhu"][xiazhu].size(); i++){
        m_xiazhu.push_back(rules["xiazhu"][xiazhu][i].asInt());
    }
    //读取是否允许换牌信息
    if (huanpai && rules["huanpai"][0].asInt() == 0){
        return false;
    }
    m_diamond_huanpai = rules["huanpai"][1].asInt();
    return (m_option = option), true;
}
////////////////////////////////////////////////////////////////////////////////
bool game_room::init_room_context(const Json::Value &context)
{
    m_round           = context["round"].asInt();
    m_round_total     = context["round_total"].asInt();
    m_zhifu           = context["zhifu"].asInt();
    m_fanbei          = context["fanbei"].asInt();
    m_diamond_pay     = context["diamond_pay"].asInt();
    m_diamond_huanpai = context["diamond_huanpai"].asInt();
    m_status          = (room_state)context["status"].asInt();
    if (context.isMember("xiazhu") && context["xiazhu"].isArray()){
        for (Json::ArrayIndex i = 0; i < context["xiazhu"].size(); i++){
            m_xiazhu.push_back(context["xiazhu"][i].asInt());
        }
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool game_room::init_room_members(const Json::Value &members)
{
    if (members.isArray()){
        for (Json::ArrayIndex i = 0; i < members.size(); i++){
            int index = members["index"].asInt();
            m_members[index].uuid       = members["uuid"].asUInt();
            m_members[index].nickname   = members["nickname"].asString().c_str();
            m_members[index].sex        = members["sex"].asString().c_str();
            m_members[index].head_url   = members["head_url"].asString().c_str();
            m_members[index].ipaddr     = members["ipaddr"].asString().c_str();
            m_members[index].score      = members["score"].asInt();
            m_members[index].time_ready = members["time_ready"].asUInt();
            m_members[index].time_agree = 0;
            m_members[index].time_enter = 0;
        }
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
io::stringc game_room::export_data() const
{
    Json::Value result;
    result["context"]["round"]       = m_round;
    result["context"]["round_total"] = m_round_total;
    result["context"]["zhifu"]       = m_zhifu;
    result["context"]["fanbei"]      = m_fanbei;
    result["context"]["diamond_pay"] = m_diamond_pay;
    result["context"]["diamond_huanpai"] = m_diamond_huanpai;
    result["context"]["status"]      = m_status;
    for (size_t i = 0; i < m_xiazhu.size(); i++){
        result["context"]["xiazhu"].append(m_xiazhu[i]);
    }
    for (int i = 0; i < MAX_MEMBERS; i++){
        if (m_members[i].uuid == 0){
            continue;
        }
        Json::Value member;
        member["index"]      = i;
        member["uuid"]       = m_members[i].uuid;
        member["nickname"]   = m_members[i].nickname;
        member["sex"]        = m_members[i].sex;
        member["head_url"]   = m_members[i].head_url;
        member["ipaddr"]     = m_members[i].ipaddr;
        member["score"]      = m_members[i].score;
        member["time_ready"] = m_members[i].time_ready;
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
        init_room_rules(init["option"]);
    }
    if (init.isMember("context")){
        init_room_context(init["context"]);
    }
    if (init.isMember("members")){
        init_room_members(init["members"]);
    }
    //在这里写你的代码
    return true;
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_create()
{
    parent::on_create();
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_update(int delta)
{
    //在这里写你的代码
    parent::on_update(delta);
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
            m_members[i].time_enter = time_now;
            m_members[i].ipaddr = player->get_ipaddr();
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
            send(user_enter_room, notify, player, 405);
            return; //已开局
        }
        if (empty_index.empty()){
            send(user_enter_room, notify, player, 406);
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
        m_members[index].time_enter = time_now;
        m_members[index].uuid       = player->get_uuid();
        m_members[index].nickname   = player->get_nickname();
        m_members[index].sex        = player->get_sex();
        m_members[index].head_url   = player->get_head_url();
        m_members[index].ipaddr     = player->get_ipaddr();
    }
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
    send(user_enter_room, notify, player, 0);
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
    send_other(user_enter_room, notify, player, 0);
    //如果房间正在解散中
    if (is_dismissing() && m_dismiss_index >= 0){
        room_member &member = m_members[m_dismiss_index];
        time_t time_dismiss = member.time_agree;
        time_t remain = time_now - time_dismiss;
        remain = (remain > 60) ? 0 : (60 - remain);
        notify.clear();
        notify["remain"]                = remain;
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
    parent::on_enter(player);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_leave(io_player::value_type player, bool is_exit)
{
    Json::Value notify;
    if (is_exit){
        //已开局任何人不能退出房间
        if (is_opening()){
            send(user_leave_room, notify, player, 505);
            return;
        }
        //如果不允许创建者离开房间
        if (!is_owner_exit()){
            if (get_creater() == player->get_uuid()){
                send(user_leave_room, notify, player, 506);
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
    if (is_exit){
        notify["index"] = index;
        notify["uuid"]  = player->get_uuid();
        send_room(user_leave_room, notify, 0); //广播给所有人用户离开房间
    }
    parent::on_leave(player, is_exit);
}
void game_room::on_request(io_player::value_type player, protocol type, const Json::Value &data)
{
    switch (type){
    case room_dismiss_request:
        on_dismiss(player, data);
        break;
    case room_dismiss_response:
        on_dismiss_reply(player, data);
        break;
    case niuniu_ready:
        on_ready(player, data);
        break;
    case niuniu_round_begin:
        on_round_begin(player, data);
        break;
    }
    parent::on_request(player, type, data);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_destroy(bool completed)
{
    //在这里写你的代码
    parent::on_destroy(completed);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_dismiss(io_player::value_type player, const Json::Value &data)
{
    Json::Value notify;
    if (is_dismissing() || is_dismissed()){
        send(room_dismiss_request, notify, player, 601);
        return;
    }
    if (!is_opening()){
        if (get_creater() == player->get_uuid()){
            set_dismissed();
        } else {
            //未开局时非房主不能解散房间
            send(room_dismiss_request, notify, player, 603);
        }
        return;
    }
    time_t time_now = time(0);
    for (int i = 0; i < MAX_MEMBERS; i++){
        if (m_members[i].uuid == player->get_uuid()){
            m_dismiss_index = i;
            m_members[i].time_agree = time_now;
            set_dismiss();
            break;
        }
    }
    notify["remain"]                = 60;
    notify["applicant"]["index"]    = m_dismiss_index;
    notify["applicant"]["time"]     = time_now;
    notify["applicant"]["uuid"]     = player->get_uuid();
    notify["applicant"]["nickname"] = player->get_nickname();
    notify["consenter"].append(m_dismiss_index);
    send_room(room_dismiss_request, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_dismiss_reply(io_player::value_type player, const Json::Value &data)
{
    Json::Value notify;
    bool agree = data["agree"].asBool();
    if (!is_dismissing()){
        send(room_dismiss_request, notify, player, 701);
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
            } else if (m_members[i].uuid == player->get_uuid()){
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
    notify["responder"] = player->get_uuid();
    notify["agree"]     = agree;
    notify["dismissed"] = is_dismissed();
    send_room(room_dismiss_response, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_ready(io_player::value_type player, const Json::Value &data)
{
    Json::Value notify;
    if (m_status != room_wait_ready){
        send((protocol)niuniu_ready, notify, player, 1);
        return;
    }
    time_t time_now  = time(0);
    int  index_ready = -1;
    int  count_ready = 0;
    bool is_all_ready = true;
    for (int i = 0; i < MAX_MEMBERS; i++){
        if (m_members[i].uuid == 0){
            continue;
        } else if (m_members[i].uuid == player->get_uuid()){
            index_ready = i;
            m_members[i].time_ready = time_now;
            if (m_members[i].time_ready == 0){
                count_ready++;
            }
        } else if (m_members[i].time_ready){
            count_ready++;
        } else {
            is_all_ready = false;
        }
    }
    if (is_all_ready && count_ready > 1){
        if (m_round == 0){ //房间还未开局
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
            userid uuid = m_members[index_first_ready].uuid;
            notify.clear();
            m_status = room_wait_begin;
            send((protocol)niuniu_begin, notify, uuid, 0);
        } else { //其他局
            init_next_round(); //开始下一局
        }
    }
    notify.clear();
    notify["index"]      = index_ready;
    notify["uuid"]       = player->get_uuid();
    notify["time_ready"] = m_members[index_ready].time_ready;
    send_room((protocol)niuniu_ready, notify, 0);
}
////////////////////////////////////////////////////////////////////////////////
void game_room::on_round_begin(io_player::value_type player, const Json::Value &data)
{
    Json::Value notify;
    if (m_status != room_wait_begin){
        send((protocol)niuniu_round_begin, notify, player, 1);
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
        send((protocol)niuniu_round_begin, notify, player, 1);
        return;
    }
    if (count_ready < 2){ //至少要有2个人准备好了才能开局
        send((protocol)niuniu_round_begin, notify, player, 2);
        return;
    }
    init_next_round(); //初始化下一局(在这里是第一局)
}
////////////////////////////////////////////////////////////////////////////////
void game_room::init_next_round()
{
    //如果当前局已经是最后一局了
    if (m_round >= m_round_total){
        set_completed();
        m_status = room_completed;
        return;
    } else if (m_round == 0){
        set_open(); //设置房间已开局标记
    }
    for (int i = 0; i < MAX_MEMBERS; i++){
        m_members[i].hand.clear();
        m_members[i].switch_count = 0;
    }
    m_round += 1; //增加当前局数计数器
    m_status = room_wait_qiang_zhuang;
}
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
