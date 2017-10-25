

#ifndef __MJ_AGO_BASE_H_
#define __MJ_AGO_BASE_H_
////////////////////////////////////////////////////////////////////////////////
//用法范例如下
////////////////////////////////////////////////////////////////////////////////
/*
//假设hand为手牌
int hand[38];
memset(hand, 0, sizeof(hand));
hand[3] = 3; //3个三万
hand[4] = 3; //3个四万
hand[5] = 1; //1个五万

//假设opened为吃碰杠的牌
taojiang::set_t opened[4];
memset(opened, 0, sizeof(opened));
//碰了一对一万
opened[0].first = 1;
opened[0].type  = taojiang::type_ke;
//吃了个三四五顺
opened[1].first = 3;
opened[1].type  = taojiang::type_shun;

taojiang test;
//初始化手牌(必须)
test.begin(hand, 4);
//压入吃碰杠的牌(必须)
test.push(opened[0].first, opened[0].type);
test.push(opened[1].first, opened[1].type);

//判断是否胡5万
bool result = test.finish(5);
if (result){
//获取番值
int value = test.value();
//获取番型
int style = test.style();
}
//判断是否听牌
result = test.finish();
//判断是否可以吃二万
result = test.enable(2, taojiang::type_shun);
//判断是否可以碰三万
result = test.enable(3, taojiang::type_ke);
//判断是否可以杠三万
result = test.enable(3, true);
*/
////////////////////////////////////////////////////////////////////////////////
#include "../mj.h"
////////////////////////////////////////////////////////////////////////////////
enum mj_style{
	hu_ping     = 0x0001, //平胡*
	hu_peng     = 0x0002, //碰碰胡*
	hu_jiang    = 0x0004, //将将胡*
	hu_qing     = 0x0008, //清一色*
	hu_tian     = 0x0010, //天胡
	hu_tiantian = 0x0020, //天天胡
	hu_di       = 0x0040, //地胡
	hu_didi     = 0x0080, //地地胡
	hu_daodi    = 0x0100, //倒地胡**
	hu_heitian  = 0x0200, //黑天胡
	hu_duidui   = 0x0400, //七小对*
	hu_ganghua  = 0x0800, //杠上花
	hu_gangpao  = 0x1000, //杠上炮
	hu_qiang    = 0x2000, //抢杠胡
	hu_ying     = 0x4000,  //硬庄
	hu_zimo     = 0x8000  //自摸
};


////////////////////////////////////////////////////////////////////////////////
enum mj_gang{
	gang_not    = 0,      //杠无关
	gang_hua    = 1,      //杠上花
	gang_pao    = 2,      //杠上炮
	gang_qiang  = 3       //抢杠胡
};
////////////////////////////////////////////////////////////////////////////////
class algo_base : public generic{
protected:
	set_t   m_used[5];
	int     m_value;
	int     m_style;
	int     m_ding_count;
	int     m_rascal_count;
	int*    m_hand;
	mj_gang m_gang;
	bool    m_zimo;
	bool    m_double;
public:
	inline algo_base(const void* context = 0)
		: generic(context){
	}

	virtual void clear()
	{
		generic::clear();
		m_hand = 0;
		memset(m_used, 0, sizeof(m_used));
	}
	//初始化手牌环境
	bool begin(int hand[38], int rascal = 0){
		clear();
		if (rascal && valid(rascal)){
			ghost(rascal); //设置癞子牌牌值
		}
		for (int i = 0; i < 38; i++){
			if (hand[i] == 0)
				continue;
			if (!valid(i)){
				return false;
			}
		}
		return (m_hand = hand), true;
	}

	//添加明牌牌组
	bool push(int v, int type){
		return push(v, (mj_type)type);
	}

	//添加明牌牌组
	bool push(int v, mj_type type){
		if (m_hand == 0 || !valid(v)){
			return false;
		}
		for (int i = 0; i < 4; i++){
			if (m_used[i].first){
				continue;
			}
			m_used[i].type  = type;
			m_used[i].first = v;
			break;
		}
		return generic::push(v, type);
	}
	//判断是否听牌
	virtual bool is_ting() = 0;
	//判断是否胡牌
	virtual bool finish(int v, bool zimo = false, bool first = false, mj_gang gang = gang_not)= 0;
	//判断是否可吃碰
	virtual bool enable(int v, mj_type type){
		if (type == type_shun)
			return chi(m_hand, v);
		return peng(m_hand, v);
	}
	//判断是否可杠牌
	virtual bool enable(int v, bool zimo) = 0;
	//获取番值
	inline int value() const{return m_value;}
	//获取番型
	inline int style() const{return m_style;}

protected:
	void build(int hand[38], int data[38]){
		set_t *p = m_used;
		while (p->first){
			if (p->type == type_ke){
				data[p->first] += 3;
			} else {
				for (int i = 0; i < 3; i++){
					data[p->first + i] += 1;
				}
			}
			p++; //移动到下个数据区
		}
	}
	virtual void init(int v, bool zimo, mj_gang gang) = 0;
	int confirm(const int hand[38], form_t &r, int v) = 0;
};
////////////////////////////////////////////////////////////////////////////////
#endif //__MJ_TAOJIANG_H_
////////////////////////////////////////////////////////////////////////////////
