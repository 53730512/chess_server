

#ifndef __ROOM_BASIC_H_
#define __ROOM_BASIC_H_
////////////////////////////////////////////////////////////////////////////////
#include "io_player.h"
////////////////////////////////////////////////////////////////////////////////
enum game_type{
	room_type_douniu    = 1, //斗牛(牛牛)
	room_type_paodekuai = 2, //跑得快
	room_type_zhajinhua = 3,  //诈金花
	room_type_doudizhu  = 4, //斗地主
	room_type_taojiang  = 100,
	room_type_zhuanzhuan= 101,//转转麻将
	room_type_zhong = 102,//转转麻将
};
////////////////////////////////////////////////////////////////////////////////
#define MIN_ROOM_NUMBER 100000
#ifdef _DEBUG
#define MAX_ROOM_NUMBER 100999
#else
#define MAX_ROOM_NUMBER 999999
#endif
////////////////////////////////////////////////////////////////////////////////
class room_basic
	: public std::enable_shared_from_this<room_basic>{
		friend class io_player;
		virtual bool update_data(const io::stringc &data);
		virtual bool import_data(const io::stringc &data){return true;}
		virtual io::stringc export_data() const{return io::stringc();}
		virtual io::stringc export_score() const{return io::stringc();}
public:
	virtual ~room_basic();
	virtual io::stringc get_options() const{return io::stringc();};
	typedef std::shared_ptr<room_basic> value_type;
	friend value_type create_room(userid, game_type, time_t);
	void send_room(protocol type, const Json::Value &data, int errcode);
	void send(protocol type, const Json::Value &data, io_player::value_type player, int errcode);
	void send(protocol type, const Json::Value &data, userid target, int errcode);
	void send_other(protocol type, const Json::Value &data, io_player::value_type exclude, int errcode);
	void send_other(protocol type, const Json::Value &data, userid exclude, int errcode = 0);
	bool is_member(userid user_id) const;
	bool in_the_room(userid user_id) const;
	bool init_from_data(const io::stringc &data);
public:
	inline void        set_dirty()       {m_is_dirty  = true;}
	inline void        set_open()        {m_opening   = true;}
	inline void        set_dismissed()   {m_dismissed = true;}
	inline void        set_dismiss()     {m_dismiss   = true;}
	inline void        set_completed()   {m_completed = true;}
	inline void        set_continue()    {m_dismiss   = false;}
	inline int         get_number()      const{return m_number;}
	inline time_t      get_create_time() const{return m_time_create;}
	inline bool        is_opening()      const{return m_opening;}
	inline bool        is_dismissing()   const{return m_dismiss;}
	inline bool        is_dismissed()    const{return m_dismissed;}
	inline bool        is_completed()    const{return m_completed;}
	inline bool        is_owner_exit()   const{return m_owner_exit;}
	inline game_type   get_game_type()   const{return m_type;}
	inline int         get_rule_id()     const{return m_rule_id;}
	inline int         get_pay_type()    const{return m_pay_type;}
	inline userid      get_creater()     const{return m_creater;}
	inline __int64     get_record_id()   const{return m_record_id;}
	inline void        set_pay_type(int type){m_pay_type = type;}
public:
	static bool        is_free_time();
	static void        write_to_cache();
	static void        read_from_cache();
	static void        update(int delta);
	static value_type  find(int number);
	static value_type  find(userid user_id);
	static size_t      get_room_count();
	static int         pop_new_number();
	static void        push_old_number(int number);
	static void        submit_gold(userid userid, int count);
	static void        submit_diamond(userid userid, int count);
protected:
	virtual void       on_create ();
	virtual void       on_update (int delta);
	virtual void       on_enter  (io_player::value_type player);
	virtual void       on_leave  (io_player::value_type player, bool is_exit);
	virtual void       on_request(io_player::value_type player, protocol type, const Json::Value &data);
	virtual void       on_destroy(bool completed);
protected:
	room_basic(userid, game_type, int, int, time_t, bool);
	void delete_member (userid user_id, bool is_exit);
	void delete_member (io_player::value_type player, bool is_exit);
	void insert_member (io_player::value_type player);
	void insert_member (userid user_id, int index, const io::stringc &nickname, const io::stringc sex, const io::stringc &head_url);
	void insert_record (int round, int total, int visit, const io::stringc &data);
	void deduct_gold   (userid user_id, int count);
	void deduct_gold   (io_player::value_type player, int count);
	void deduct_diamond(userid user_id, int count);
	void deduct_diamond(io_player::value_type player, int count);
private:
	typedef struct{
		userid         uuid;
		int            index;
		time_t         enter_time;
		io::stringc    nickname;
		io::stringc    sex;
		io::stringc    head_url;
	} member_t;
	std::map<userid, member_t> m_members;
private:
	bool               m_is_dirty;    //存在脏数据标记
	bool               m_opening;     //已开局
	bool               m_dismiss;     //房间在解散中
	bool               m_dismissed;   //房间已解散
	bool               m_completed;   //房间已结束
	int                m_elapse;      //流逝的时间(毫秒)
	int                m_pay_type;    //支付类型
	__int64            m_record_id;   //记录ID
	time_t             m_time_idle;   //房间空闲的时间
	const bool         m_owner_exit;  //是否允许房主退出
	const userid       m_creater;     //房间的创建者
	const game_type    m_type;        //房间类型(游戏类型)
	const int          m_rule_id;     //比赛规则ID(规则类型)
	const int          m_number;      //房间号
	const time_t       m_time_create; //房间的创建时间
};
////////////////////////////////////////////////////////////////////////////////
template<typename _Ty>
inline room_basic::value_type create_room(
	userid             creater,       //创建者uuid
	game_type          type,          //游戏类型
	int                ruleid,        //规则类型
	time_t             time_create,   //创建时间
	bool               owner_exit,    //所有者是否可以退出房间
	int                room_number    //房间号
	){
		return room_basic::value_type(
			new _Ty(creater, type, ruleid, room_number, time_create, owner_exit)
			);
}
////////////////////////////////////////////////////////////////////////////////
template<typename _Ty>
inline room_basic::value_type create_room(
	userid             creater,       //创建者uuid
	game_type          type,          //游戏类型
	int                ruleid,        //规则类型
	time_t             time_create,   //创建时间
	bool               owner_exit     //所有者是否可以退出房间
	){
		room_basic::value_type new_room;
		int room_number = room_basic::pop_new_number();
		if (room_number){
			new_room = create_room<_Ty>(
				creater, type, ruleid, time_create, owner_exit, room_number
				);
			if (!new_room){
				room_basic::push_old_number(room_number);
			}
		}
		return new_room;
}
////////////////////////////////////////////////////////////////////////////////
#endif //__ROOM_BASIC_H_
