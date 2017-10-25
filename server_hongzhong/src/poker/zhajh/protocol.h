

#ifndef _ZHJH_PROTOCOL_H_
#define _ZHJH_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../io_protocol.h"
////////////////////////////////////////////////////////////////////////////////
enum zhajh_protocol{
    zhajh_ready			= room_min_protocol + 1,
    zhajh_begin,
	zhajh_wait_operation,		//等待操作
	zhajh_gen_zhu,				//跟注
	zhajh_jia_zhu,				//加注
	zhajh_kan_pai,				//看牌
	zhajh_bi_pai,				//比牌
	zhajh_qi_pai,				//弃牌
	zhajh_sync_data,			//同步数据
	zhajh_status_change,		//状态改变
	zhajh_publish_result,		//结算
	zhajh_room_broadcast = room_min_protocol + 99,		//广播
};
////////////////////////////////////////////////////////////////////////////////
#endif //__NIUNIU_PROTOCOL_H_
