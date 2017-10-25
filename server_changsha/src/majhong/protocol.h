#ifndef _ROOM_PROTOCOL_H_
#define _ROOM_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include "../io_protocol.h"
////////////////////////////////////////////////////////////////////////////////
enum room_protocol{
	room_broadcast = room_min_protocol + 99,		//广播
	room_ready,										//准备 
	room_begin,										//开始
	room_status_change,								//状态改变
	room_mahjongs,									//发送手牌
	room_wait_operation,							//等待操作
	room_fa_pai,									//发牌
	room_chu_pai,
	room_chi_pai,
	room_peng_pai,									//碰牌
	room_gang_pai,									//杠牌
	room_guo,										//过
	room_hu,										//胡牌
	room_sync_data,									//同步数据
	room_publish_result,							//结算
};

enum taojiang_protocol{
	taojiang_gang_zi= room_min_protocol + 1000 ,				//杠子
	taojiang_shaizi,											//掷骰子
};
////////////////////////////////////////////////////////////////////////////////
#endif 
