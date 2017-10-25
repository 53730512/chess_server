

#ifndef __ALGO_ZHJH_H_
#define __ALGO_ZHJH_H_

#include "../../poker/poker.h"
namespace zhajh{
	enum CARD_GROUP_TYPE {
		NONE,
		COMMON,			//散牌
		PAIRS,			//对子
		SHUNZI,			//顺子
		JINHUA,			//金花
		SHUNJIN,		//顺金
		BAOZI,			//豹子
		SPECIAL,		//235
	};

} //End namespace paodk
#endif //__ALGO_PAODK_H_
