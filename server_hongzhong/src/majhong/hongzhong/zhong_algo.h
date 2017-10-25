

#ifndef __MJ_ZHONG_H_
#define __MJ_ZHONG_H_
////////////////////////////////////////////////////////////////////////////////
#include "../base/algo_base.h"
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class zhong_algo : public algo_base{
	int     m_ding_count;
public:
	inline zhong_algo(const void* context = 0)
		: algo_base(context){
	}

	//判断是否听牌
	bool is_ting() override{
		return false;
	}

	//重载吃碰判断函数
	virtual bool enable(int v, mj_type type) override{
		if (type == type_shun)//转转不允许吃牌
			return false;
		else if(v == ghost())
			return false;

		return peng(m_hand, v);
	}

	//判断是否胡牌
	bool finish (int v, bool zimo = false, bool first = false, mj_gang gang = gang_not) override{
		if (m_hand == 0 || !valid(v)){
			return false;
		}
		//初始化内部数据
		init(v, zimo, gang);
		m_value = 0;
		m_style = 0;
		//如果为7对胡牌则判断附加番型
		bool bwin = is_double(m_hand, v) || hupai(m_hand, v, zimo, true);
		if (bwin){
			if(zimo)
			{
				m_value = 2;
				m_style |= hu_zimo;
			}
			else
			{
				m_value = 1;
				m_style |= hu_ping;
			}
		}
		return bwin;
	}

	//判断是否可杠牌
	bool enable(int v, bool zimo) override{
		if(v == ghost())
			return false;

		bool result = gang(m_hand, v);
		if (!result){
			//如果不是自摸且不听牌
			if (!zimo ){
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

		return result;
	}
private:
	void init(int v, bool zimo, mj_gang gang) override{
		int g = ghost();
		int n = m_hand[g];
		if ((zimo || gang)&& v == g){
			n += 1;
		}
		m_rascal_count = n;
		m_zimo   = zimo;
		m_gang   = gang;
		m_style = m_value = 0;
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
private:
	//是否为自摸
	bool zimo() const{
		return (m_gang || m_zimo);
	}
	//清除明牌牌组
	virtual void clear(){
		algo_base::clear();

	}
private:
	int confirm(const int hand[38], form_t &r, int v) override{
		return 1;
	}

};
////////////////////////////////////////////////////////////////////////////////
#endif //__MJ_TAOJIANG_H_
////////////////////////////////////////////////////////////////////////////////
