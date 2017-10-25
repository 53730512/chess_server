

#ifndef __MJ_TAOJIANG_H_
#define __MJ_TAOJIANG_H_
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
#include "../base/algo_base.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class taoj_algo : public algo_base{
	int     m_ding_count;
public:
	inline taoj_algo(const void* context = 0)
		: algo_base(context){
	}

	//判断是否听牌
	bool is_ting() override{
		int g = ghost();
		int v = g;
		if (v == 0){
			for (int i = 0; i < 38; i++){
				if (i % 10 == 0){
					continue;
				}
				if (m_hand[i] == 0){
					v = i;
					break;
				}
			}
			ghost(v);
		}
		bool result = finish(v, true, false, gang_not);
		if (g == 0){
			ghost(0);
		}
		if (!result){
			return false;
		}
		int mask = 0;
		int mode = style();
		mask |= mode & hu_ping;
		mask |= mode & hu_peng;
		mask |= mode & hu_jiang;
		mask |= mode & hu_qing;
		mask |= mode & hu_duidui;
		return (mask > 0);
	}
	//判断是否胡牌
	bool finish (int v, bool zimo = false, bool first = false, mj_gang gang = gang_not) override{
		if (m_hand == 0 || !valid(v)){
			return false;
		}

		//初始化内部数据
		init(v, zimo, gang);

		//3个癞子只能自摸，不能要炮
		if (!zimo && m_rascal_count >= 3){
			return false;
		}
		//如果为7对胡牌则判断附加番型
		switch (gang){
		case gang_hua:   //杠上花
			m_value += 2;
			m_style |= hu_ganghua;
			break;
		case gang_pao:   //杠上炮
			m_value += 2;
			m_style |= hu_gangpao;
			break;
		case gang_qiang: //抢杠胡
			m_value += 2;
			m_style |= hu_qiang;
			break;
		}
		m_double = true;
		bool is_duidui = is_double(m_hand, v);
		if (is_duidui){
			m_value += 2;
			m_style |= hu_duidui;
			if (is_jiang_hu(m_hand, v)){
				m_value += 2;
				m_style |= hu_jiang;
			}
			if (is_qingyise(m_hand, v)){
				m_value += 2;
				m_style |= hu_qing;
			}
			if (is_tian_tian(m_hand, v)){
				m_value += 4;
				m_style |= hu_tiantian;
			} else if (is_tian_hu(m_hand, v)){
				m_value += 2;
				m_style |= hu_tian;
			}
			if (is_didi_hu(m_hand, v)){
				m_value += 4;
				m_style |= hu_didi;
			}
			if (m_double != false){
				m_value *= 2;
				m_style |= hu_ying;
			}
		}
		//如果普通模式也胡牌则判断番型大小
		m_double = true;
		bool is_normal = hupai(m_hand, v, zimo, true);
		if (is_normal){
			const form_t* hu_form = optimal();
			if (hu_form->value > m_value){
				m_value = (int)hu_form->value;
				m_style = (int)hu_form->context;
			}
		}
		//如果已胡牌则返回
		if (is_duidui || is_normal){
			return true;
		}
		//如果基础两种牌型不胡则判断特殊番型
		if (first){
			m_value = m_style = 0;
			if (is_heitian(m_hand, v)){
				m_value = m_style = 0;
				m_value += 2;
				m_style |= hu_heitian;
				return true;
			}
		}
		m_double = true;
		if (is_jiang_hu(m_hand, v)){
			m_value += 2;
			m_style |= hu_jiang;
			if (m_double != false){
				m_value *= 2;
				m_style |= hu_ying;
			}
		} else {
			m_value = m_style = 0;
		}
		if (is_didi_hu(m_hand, v)){
			m_value += 4;
			m_style |= hu_didi;
		} else if (is_di_hu(m_hand, v)){
			m_value += 2;
			m_style |= hu_di;
		}
		return (m_value > 0 && m_style > 0);
	}

	//判断是否可杠牌
	bool enable(int v, bool zimo) override{
		bool result = gang(m_hand, v);
		if (!result){
			//如果不是自摸且不听牌
			if (!zimo || !is_ting()){
				return false;
			}
			set_t *p = m_used;
			while (p->first){
				if (p->type != type_ke){
					continue;
				}
				if (p->first == v){
					result = true;
					break;
				}
				p++; //移动到下个数据区
			}
			return result;
		}
		//模拟用户杠牌后判听
		int hand[38];
		memcpy(hand, m_hand, sizeof(hand));
		hand[v] = 0;

		taoj_algo clone;
		clone.begin(hand, ghost());
		clone.push(v, mahjong::type_ke);
		set_t *p = m_used;
		while (p->first){
			clone.push(p->first, p->type);
			p++;
		}
		return clone.is_ting();
	}
private:
	void init(int v, bool zimo, mj_gang gang) override{
		int g = ghost();
		int n = m_hand[g];
		if ((zimo || gang)&& v == g){
			n += 1;
		}
		m_rascal_count = n;
		int d = ding_wang();
		n = m_hand[d];
		if (zimo && v == d){
			n += 1;
		}
		m_zimo   = zimo;
		m_gang   = gang;
		m_ding_count = n;
		m_style = m_value = 0;
	}

	//判断牌值是否为258值
	inline bool is_258(int v){
		v %= 10;
		return (v == 2 || v == 5 || v == 8);
	}
	//附加番型(天胡)
	bool is_tian_hu(int hand[38], int hu){
		if (!zimo() || m_gang)
			return false;
		return (m_rascal_count == 3);
	}
	//附加番型(天天胡)
	bool is_tian_tian(int hand[38], int hu){
		if (!zimo() || m_gang)
			return false;
		return (m_rascal_count == 4);
	}
	//特殊番型(地胡)
	bool is_di_hu(int hand[38], int hu){
		if (!zimo() || m_gang){
			return false;
		}
		int d = ding_wang();
		int n = (hu == d) ? 2 : 3;
		return (hand[d] == n);
	}
	//特殊番型(地地胡)
	bool is_didi_hu(int hand[38], int hu){
		if (!zimo() || m_gang){
			return false;
		}
		int d = ding_wang();
		int n = (hu == d) ? 3 : 4;
		return (hand[d] == n);
	}
	bool is_qingyise(int hand[38], int hu){
		int data[38];
		memcpy(data, hand, sizeof(data));
		data[hu]++;
		build(hand, data);

		int t[4] = {0};
		int g = ghost();
		for (int i = 0; i < 38; i++){
			if (i == g){
				continue;
			}
			if (data[i] == 0)
				continue;
			t[i / 10] = 1;
		}
		//如果有保留的癞子
		if (reserved() || used()){
			t[g / 10] = 1;
		}
		int index = -1;
		for (int i = 0; i < 4; i++){
			if (t[i] > 0){
				if (index >= 0){
					return false;
				} else {
					index = i;
				}
			}
		}
		//看看是否使用了癞子
		if (m_rascal_count > 0){
			if (g / 10 != index){
				m_double = false;
			}
		}
		return true;
	}
	//特殊番型(将将胡)
	bool is_jiang_hu(int hand[38], int hu){
		int data[38];
		memcpy(data, hand, sizeof(data));
		data[hu]++;
		build(hand, data);

		int g = ghost();
		for (int i = 0; i < 38; i++){
			if (i == g){
				continue;
			}
			if (data[i] && !is_258(i)){
				return false;
			}
		}
		//如果有保留的癞子
		if (reserved() || used()){
			if (!is_258(g)){
				//保留的癞子不是258
				return false;
			}
		}
		if (m_rascal_count > 0)
			m_double = is_258(g);
		return true;
	}

	//特殊番型(七小对)
	bool is_double(int hand[38], int hu){
		int data[38];
		memcpy(data, hand, sizeof(data));
		data[hu]++;

		int n = 0;
		int m = 0, g = ghost();
		for (int i = 0; i < 38; i++){
			n += data[i];
			if (i == g){
				continue;
			}
			if (data[i] % 2 == 1){
				m++;
			} else {
				continue;
			}
		}
		if (n != 14){
			return false;
		}
		n = m_rascal_count;
		int r = reserved() + used();
		if (m + r > n){
			return false;
		}
		if ((n - m - r) % 2){
			return false;
		}
		if (m_rascal_count > 0)
			m_double = (m == 0);
		return true;
	}

	//特殊番型(黑天胡)
	bool is_heitian(int hand[38], int hu){
		if (!zimo() || m_gang){
			return false;
		}
		int data[38];
		memcpy(data, hand, sizeof(data));
		data[hu]++;

		int n = 0, g = ghost();
		for (int i = 0; i < 38; i++){
			if (data[i] == 0){
				continue;
			}
			n += data[i];
			if (i == g){
				return false;
			}
			if (is_258(i)){
				return false;
			}
			if (data[i] > 2){
				return false;
			}
			if (i % 10 < 8){
				if (data[i + 1] && data[i + 2]){
					return false;
				}
			}
		}
		return (n == 14);
	}
private:
	//是否为自摸
	bool zimo() const{
		return (m_gang || m_zimo);
	}
	//清除明牌牌组
	virtual void clear(){
		algo_base::clear();

	}
	//获取定王牌牌值
	int ding_wang() const{
		int g = ghost();
		return ((g - 1)%10) ? g - 1 : g + 8;
	}
	//是否为天胡
	bool is_tian_hu(const form_t &r){
		if (!zimo())
			return false;
		return (m_rascal_count == 3);
	}
	//是否为天天胡
	bool is_tian_tian(const form_t &r){
		if (!zimo())
			return false;
		return (m_rascal_count == 4);
	}
	//是否为地胡
	bool is_di_hu(const form_t &r){
		int d = ding_wang();
		if (m_ding_count < 3 || !zimo()){
			return false;
		}
		return r.have(type_ke, d);
	}
	//是否为地地胡
	bool is_didi_hu(const form_t &r){
		int d = ding_wang();
		if (m_ding_count < 4 || !zimo()){
			return false;
		}
		return r.have(type_ke, d);
	}
	//是否为碰碰胡
	bool is_peng_hu(const form_t &r){
		bool result = true;
		for (int i = 0; i < r.count; i++){
			if (r.sets[i].type != type_ke){
				result = false;
			}
		}
		return result;
	}
	//是否为清一色
	bool is_qingyise(const form_t &r){
		int g = ghost();
		int t[4] = {0};
		int n = r.count;
		for (int i = 0; i < n; i++){
			int m = r.sets[i].type;
			int v = r.sets[i].first;
			if (m == type_ke){
				if (v != g)
					t[v / 10] = 1;
				continue;
			}
			for (int j = 0; j < 3; j++){
				if (v + j == g)
					continue;
				t[(v + j) / 10] = 1;
			}
		}
		if (r.jiang != g){
			t[r.jiang / 10] = 1;
		}
		//如果有保留的癞子
		if (reserved() || used()){
			t[g / 10] = 1;
		}
		int index = -1;
		for (int i = 0; i < 4; i++){
			if (t[i] > 0){
				if (index >= 0){
					return false;
				} else {
					index = i;
				}
			}
		}
		//看看是否使用了癞子
		if (m_rascal_count > 0){
			if (g / 10 != index){
				m_double = false;
			}
		}
		return true;
	}
	//是否为将将胡
	bool is_jiang_hu(const form_t &r){
		int v, t;
		int g = ghost();
		for (int i = 0; i < r.count; i++){
			t = r.sets[i].type;
			if (t != type_ke){
				return false;
			}
			v = r.sets[i].first;
			if (v == g){
				continue;
			}
			if (!is_258(v)){
				return false;
			}
		}
		v = r.jiang;
		if (v != g && !is_258(v)){
			return false;
		}
		//如果有保留的癞子
		if (reserved() || used()){
			if (!is_258(g)){
				//保留的癞子不是258
				return false;
			}
		}
		if (m_rascal_count)
			m_double = is_258(g);
		return true;
	}
	//是否为硬庄
	bool is_yingzhuang(const form_t &r){
		if (ghost() == 0)
			return false;
		return (m_double && r.rascal == 0);
	}
private:
	int confirm(const int hand[38], form_t &r, int v) override{
		m_double = true;
		int value = 0, style = 0;
		if (is_qingyise(r)){
			value += 2;
			style |= hu_qing;
		}
		if (is_peng_hu(r)){
			value += 2;
			style |= hu_peng;
		} else if (value == 0) {
			int g = ghost();
			int j = r.jiang;
			bool is_ping = false;
			if (!is_258(j)){
				if ((j == g) ? zimo() : false){
					value = 1;
					is_ping = true;
					m_double = false;
				}
			} else {
				if (r.rascal ? zimo() : true){
					value = 1;
					is_ping = true;
				}
			}
			if (is_ping == false){
				m_double = true;
				return 0;
			}
			style |= hu_ping;
		}
		switch (m_gang){
		case gang_hua:
			value += 2;
			style |= hu_ganghua;
			break;
		case gang_pao:
			value += 2;
			style |= hu_gangpao;
			break;
		case gang_qiang:
			value += 2;
			style |= hu_qiang;
			break;
		}
		if (is_jiang_hu(r)){
			value += 2;
			style |= hu_jiang;
		}
		if (is_didi_hu(r)){
			value += 4;
			style |= hu_didi;
		} else if (is_di_hu(r)){
			value += 2;
			style |= hu_di;
		}
		if (is_tian_tian(r)){
			value += 4;
			style |= hu_tiantian;
		} else if (is_tian_hu(r)){
			value += 2;
			style |= hu_tian;
		}
		int d = style & hu_di;
		if (d == 0){
			d = style & hu_didi;
		}
		int t = style & hu_tian;
		if (t == 0){
			t = style & hu_tiantian;
		}
		//天地胡带平不计算平胡
		if (m_gang || (style & hu_jiang) || (t && d)){
			if (style & hu_ping){
				value -= 1;
				style ^= hu_ping;
			}
		}
		if (is_yingzhuang(r)){
			value *= 2;
			style |= hu_ying;
		}
		m_double = true;
		return (r.context = style), value;
	}
};
////////////////////////////////////////////////////////////////////////////////
#endif //__MJ_TAOJIANG_H_
////////////////////////////////////////////////////////////////////////////////
