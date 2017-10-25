

#ifndef __ALGO_PAODK_H_
#define __ALGO_PAODK_H_

#include "../../poker/poker.h"
namespace paodk{
	enum CARD_GROUP_TYPE {
		NONE,
		SINGLE, //单牌
		PAIRS,  //对子
		PROGRESSION,//顺子
		THREE,      //三代
		FOUR,       //四带
		BOMB,   //炸弹
	};

} //End namespace paodk
#endif //__ALGO_PAODK_H_
