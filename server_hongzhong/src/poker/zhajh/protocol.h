

#ifndef _ZHJH_PROTOCOL_H_
#define _ZHJH_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../io_protocol.h"
////////////////////////////////////////////////////////////////////////////////
enum zhajh_protocol{
    zhajh_ready			= room_min_protocol + 1,
    zhajh_begin,
	zhajh_wait_operation,		//�ȴ�����
	zhajh_gen_zhu,				//��ע
	zhajh_jia_zhu,				//��ע
	zhajh_kan_pai,				//����
	zhajh_bi_pai,				//����
	zhajh_qi_pai,				//����
	zhajh_sync_data,			//ͬ������
	zhajh_status_change,		//״̬�ı�
	zhajh_publish_result,		//����
	zhajh_room_broadcast = room_min_protocol + 99,		//�㲥
};
////////////////////////////////////////////////////////////////////////////////
#endif //__NIUNIU_PROTOCOL_H_
