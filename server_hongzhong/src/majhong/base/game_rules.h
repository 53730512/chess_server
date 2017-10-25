#ifndef __MJ_RULES_H
#define __MJ_RULES_H

#define MJ_TYPE_WAN     0   //万(1-9)
#define MJ_TYPE_TONG    1   //桶(1-9)
#define MJ_TYPE_TIAO    2   //条(1-9)
#define MJ_TYPE_ZI      3   //字(东南西北中发白)
#define MJ_TYPE_HUA     4   //花(春夏秋冬梅兰竹菊)(暂不支持)

//#define AUTO_PLAY_CARD  1
namespace mj_base{

	enum mj_card_value{
		wan_1=1,
		wan_2,
		wan_3,
		wan_4,
		wan_5,
		wan_6,
		wan_7,
		wan_8,
		wan_9,
		tong_1 =11,
		tong_2,
		tong_3,
		tong_4,
		tong_5,
		tong_6,
		tong_7,
		tong_8,
		tong_9,
		tiao_1=21,
		tiao_2,
		tiao_3,
		tiao_4,
		tiao_5,
		tiao_6,
		tiao_7,
		tiao_8,
		tiao_9,
		dong=31,
		nan,
		xi,
		bei,
		zhong,
		fa,
		bai
	};
	//权限类型
	enum right_type{
		none,
		chu =1,
		chi,
		peng,
		gang,
		hu,
	};

	//组牌类型
	enum group_type{
		type_none,
		type_shun,			//顺子
		type_ke,			//刻子
		type_ming_gang,		//明杠
		type_an_gang,		//暗杠
		type_bu_gang,		//补杠
	};

	enum history_type
	{
		hs_none=0,
		wait_operation =1, //等待操作
		mo_pai,				//摸牌
		guo,				//过
		chu_pai,			//出牌
		peng_pai,			//碰牌
		chi_pai,			//吃_牌
		gang_pai,			//杠牌
		hu_pai,				//胡牌
	};
}
#endif
