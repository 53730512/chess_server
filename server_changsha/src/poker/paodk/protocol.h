

#ifndef __NIUNIU_PROTOCOL_H_
#define __NIUNIU_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../io_protocol.h"
////////////////////////////////////////////////////////////////////////////////
enum paodk_protocol{
    paodk_ready          = room_min_protocol + 1,
    paodk_begin          = room_min_protocol + 2,
    paodk_pokers         = room_min_protocol + 3,
    paodk_round_begin    = room_min_protocol + 4,
    paodk_wait_play		 = room_min_protocol + 5,
    paodk_play_card		 = room_min_protocol + 6,
    paodk_bomb_score        = room_min_protocol + 7,
    paodk_publish_result = room_min_protocol + 11,
    paodk_sync_data      = room_min_protocol + 12,
    paodk_status_change  = room_min_protocol + 13,
    paodk_qie_pai = room_min_protocol + 14,
	paodk_wait_qie_pai = room_min_protocol + 15,
    paodk_room_broadcast = room_min_protocol + 99
};
////////////////////////////////////////////////////////////////////////////////
#endif //__NIUNIU_PROTOCOL_H_
