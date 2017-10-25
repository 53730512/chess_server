

#ifndef __NIUNIU_PROTOCOL_H_
#define __NIUNIU_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../io_protocol.h"
////////////////////////////////////////////////////////////////////////////////
enum niuniu_protocol{
    niuniu_ready          = room_min_protocol + 1,
    niuniu_begin          = room_min_protocol + 2,
    niuniu_pokers         = room_min_protocol + 3,
    niuniu_round_begin    = room_min_protocol + 4,
    niuniu_qiang_zhuang   = room_min_protocol + 5,
    niuniu_random_select  = room_min_protocol + 6,
    niuniu_xia_zhu        = room_min_protocol + 7,
    niuniu_show_five      = room_min_protocol + 8,
    niuniu_huan_pai       = room_min_protocol + 9,
    niuniu_publish_hand   = room_min_protocol + 10,
    niuniu_publish_result = room_min_protocol + 11,
    niuniu_sync_data      = room_min_protocol + 12,
    niuniu_status_change  = room_min_protocol + 13,
    niuniu_status_timeout = room_min_protocol + 14,
    niuniu_room_broadcast = room_min_protocol + 99
};
////////////////////////////////////////////////////////////////////////////////
#endif //__NIUNIU_PROTOCOL_H_
