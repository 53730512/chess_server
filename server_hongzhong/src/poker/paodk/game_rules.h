

#ifndef __ALGO_PAODK_H_
#define __ALGO_PAODK_H_

#include "../../poker/poker.h"
namespace paodk{
	enum CARD_GROUP_TYPE {
		NONE,
		SINGLE, //����
		PAIRS,  //����
		PROGRESSION,//˳��
		THREE,      //����
		FOUR,       //�Ĵ�
		BOMB,   //ը��
	};

} //End namespace paodk
#endif //__ALGO_PAODK_H_
