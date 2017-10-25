

#ifndef __MAHJONG_INCLUDED_H_
#define __MAHJONG_INCLUDED_H_
////////////////////////////////////////////////////////////////////////////////
/* char hand[38] = {
X, 0, 0, 0, 0, 0, 0, 0, 0, 0, //万(01-09)
X, 0, 0, 0, 0, 0, 0, 0, 0, 0, //筒(11-19)
X, 0, 0, 0, 0, 0, 0, 0, 0, 0, //条(21-29)
X, 0, 0, 0, 0, 0, 0, 0, X, X, //东南西北中发白(31-37)
X, 0, 0, 0, 0, 0, 0, 0        //春夏秋冬梅兰竹菊
}; */
////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <algorithm>
#include <string>
#include <stdio.h>
////////////////////////////////////////////////////////////////////////////////
#ifndef sprintf
#define sprintf sprintf_s
#endif
////////////////////////////////////////////////////////////////////////////////
template<typename _Ty>
inline void shell_sort(_Ty *p, int n){
	_Ty t;
	int i, j, k = (n >> 1);
	while (k > 0){
		for (i = k; i < n; i++){
			t = p[i];
			j = i - k;
			while (j >= 0 && t < p[j]){
				p[j + k] = p[j];
				j = j - k;
			}
			p[j + k] = t;
		}
		k >>= 1;
	}
}
////////////////////////////////////////////////////////////////////////////////
struct mahjong{
	enum mj_mode{
		enum_hu   = 0,     //判胡
		enum_ting = 1      //判听
	};
	enum mj_type{
		type_shun = 0,     //顺子
		type_ke   = 1      //刻子
	};
};
////////////////////////////////////////////////////////////////////////////////
class generic : public mahjong{
public:
	typedef struct __set{
		int type;          //牌组类型
		int first;         //首张牌值
		inline bool operator<(const __set &r){
			if (type < r.type)
				return true;
			return (first < r.first);
		}
		inline __set& operator=(const __set &r){
			type = r.type;
			first = r.first;
			return (*this);
		}
	} set_t;
	typedef struct __form{
		int     jiang;     //将牌牌值
		int     count;     //顺或刻的个数
		set_t   sets[4];   //顺牌或刻牌
		int     value;     //牌型番值
		int     rascal;    //癞子使用数
		__int64 context;   //附加上下文
		inline void sort(){
			shell_sort<set_t>(sets, count);
		}
		bool have(mj_type type, int v) const{
			int t, x, n = count;
			for (int i = 0; i < n; i++){
				t = sets[i].type;
				if (t != type){
					continue;
				}
				x = sets[i].first;
				if (x == v){
					return true;
				}
			}
			return false;
		}
		inline bool operator<(const __form &r){
			return (jiang < r.jiang);
		}
	} form_t;
public:
	//构造函数, 参数为癞子牌值
	inline generic(const void *context = 0)
		: m_context(context){
			clear(), ghost(0);
	}
	//添加一组明牌数据(吃碰杠的牌)
	bool push(int v, mj_type type){
		if (m_form.count > 3){
			return false;
		}
		if (type != type_ke && type != type_shun){
			return false;
		}
		int g = ghost();
		if (type == type_ke){
			if (v == g){
				m_used += 3;
			}
		} else {
			for (int i = v; i < v + 3; i++){
				if (i == g){
					m_used++;
				}
			}
		}
		int i = m_form.count++;
		m_form.sets[i].type  = type;
		m_form.sets[i].first = v;
		return true;
	}
	//清除 push 的明牌
	virtual void clear(){
		m_used = m_reserved = 0;
		memset(&m_form, 0, sizeof(m_form));
	}
	//获取对应的胡牌牌型指针
	const form_t* get(int i){
		int n = (int)count();
		return (i < 0 || i >= n) ? 0 : &m_checked[i];
	}
	const form_t* optimal() const{
		return &m_result;
	}
	//对胡牌结果集进行排序
	void sort(){
		auto begin = m_checked.begin();
		auto end   = m_checked.end();
		std::sort(begin, end);
	}
	//解析手牌是否听牌
	bool tingpai(int hand[38]){
		int table[38];
		memcpy(table, hand, sizeof(table));
		reset(0, false);
		//优化数据
		optimization(table);
		if (m_rest % 3 != 1){
			return 0;
		}
		m_hand = hand;
		take_next_set(table, enum_ting);
		return m_ready;
	}
	//解析手牌是否可以胡指定牌
	bool hupai(int hand[38], int v, bool zimo = false, bool recursion = false){
		int table[38];
		memcpy(table, hand, sizeof(table));
		//把最后一张牌放入手牌
		table[v]++;
		reset(v, recursion);
		//优化数据
		optimization(table);
		if (m_rest % 3 != 2){
			return 0;
		}
		if (v == ghost() && zimo == false){
			m_reserved = 1;
		}
		m_hand = hand;
		take_next_set(table, enum_hu);
		return (m_result.value > 0);
	}
	//打印牌型数据
	void print(const form_t *p){
		if (p == 0){
			return;
		}
		int v = p->jiang;
		int n = p->count;
		std::string info;

		char data[32];
		sprintf(data, "[%02d %02d]", v, v);
		info += data;

		for (int i = 0; i < n; i++){
			v = p->sets[i].first;
			int t = p->sets[i].type;
			switch (t){
			case type_ke:
				sprintf(data, "-[%02d %02d %02d]", v, v, v);
				break;
			case type_shun:
				sprintf(data, "-[%02d %02d %02d]", v, v + 1, v + 2);
				break;
			}
			info += data;
		}
		printf("%s [%02d] 癞子:%02d 番值:%d 番型: %lld\r\n"
			, info.c_str()
			, m_last
			, ghost()
			, p->value
			, p->context
			);
	}
	//设置癞子牌牌值
	inline void ghost(int v){m_ghost = v;}
	//获取癞子牌牌值
	inline int  ghost() const{return m_ghost;}
	//获取胡牌牌型数
	inline int  count() const{return (int)m_checked.size();}
	//获取使用的癞子数
	inline int  used() const{return m_used;}
	//获取保留的癞子数
	inline int  reserved() const{return m_reserved;}
	//获取上下文
	inline const void* context() const{return m_context;}
public:
	static bool valid(int v){
		return (v > 0 && v < 38 && v % 10);
	}
	static bool chi(int hand[38], int v){
		if (!valid(v) || v > 29){
			return false;
		}
		int data[38];
		memcpy(data, hand, sizeof(data));
		data[v]++;

		int t = v / 10 * 10;
		int m = v % 10;
		if (m == 0){
			return false;
		}
		int a = m-2;
		if(a < 1 )
			a= 1;

		int b = m+2;
		if(b>9)
			b=9;
		a += t;
		b += t;

		int *p = data;
		for (int i = a; i <= b-2; i++){
			if (p[i] && p[i + 1] && p[i + 2]){
				return true;
			}
		}
		return false;
	}
	static bool peng(int hand[38], int v){
		return (valid(v) && hand[v] > 1);
	}
	static bool gang(int hand[38], int v){
		return (valid(v) && hand[v] > 2);
	}
protected:
	//重载该函数可以对胡牌结果进行二次确认
	//可胡返回对应的番值，否则返回0表示不能胡牌
	virtual int confirm(const int hand[38], form_t &r, int v){
		return true;
	}
protected:
	bool confirm(form_t &r){
		r.sort(); //排序牌组
		return (confirm(m_hand, r, m_last) > 0);
	}
	bool confirm(form_t &r, int v, int x){
		int g = ghost();
		form_t t = r;
		if (v && x == 0){ //单钓将
			if (v == g){
				return true;
			}
			t.jiang = v;
			return confirm(t);
		}
		int n = t.count++;
		if (v == x){ //凑刻子
			t.sets[n].first = v;
			t.sets[n].type  = type_ke;
			return confirm(t);
		}
		if (v == g || x == g){
			int w = (v == g) ? x : v;
			t.sets[n].first = w;
			t.sets[n].type  = type_ke;
			return confirm(t);
		}
		if (x - v == 1){
			if (v % 10 == 1 || x % 10 == 9){
				t.sets[n].first = v;
				t.sets[n].type  = type_shun;
				return confirm(t);
			}
			t.sets[n].first = v - 1;
			t.sets[n].type  = type_shun;
			if (!confirm(t)){
				t.sets[n].first = x - 1;
				t.sets[n].type  = type_shun;
				return confirm(t);
			}
			return true;
		}
		t.sets[n].first = v;
		t.sets[n].type  = type_shun;
		return confirm(t);
	}
	int get_card_count(const int table[38]){
		int rest = 0;
		register int i;
		for (i = 0; i < 38; i++) rest += table[i];
		return rest;
	}
	void optimization(const int table[38]){
		m_begin = 1, m_end = 1;
		for (int i = 1; i < 38; i++){
			if (table[i] == 0){
				continue;
			}
			if (!m_rest && i > 2){
				m_begin = i - 2;
			}
			m_end = i + 1;
			m_type[i / 10] += table[i];
			m_rest += table[i];
		}
	}
	void push_jiang(int table[38], int v){
		form_t &r = m_form;
		if (r.jiang == 0){
			m_rest -= 2;
			table[v] -= 2;
			r.jiang = v;
		}
	}
	void pop_jiang(int table[38]){
		form_t &r = m_form;
		if (r.jiang > 0){
			m_rest += 2;
			table[r.jiang] += 2;
			r.jiang = 0;
		}
	}
	void push_to_set(int table[38], int v, bool shun){
		m_rest -= 3;
		form_t &r = m_form;
		int i = r.count++;
		r.sets[i].first = v;
		if (shun){
			r.sets[i].type = type_shun;
			table[v]--;
			table[v + 1]--;
			table[v + 2]--;
		} else {
			r.sets[i].type = type_ke;
			table[v] -= 3;
		}
	}
	void pop_from_set(int table[38]){
		m_rest += 3;
		form_t &r = m_form;
		const int i  = --r.count;
		const int v  = r.sets[i].first;
		const int t  = r.sets[i].type;
		if (t == type_ke){
			table[v] += 3;
		} else {
			table[v]++;
			table[v + 1]++;
			table[v + 2]++;
		}
	}
	void push_result(int table[38]){
		form_t r = m_form;
		r.sort();
		int n = sizeof(r) - sizeof(int) * 4;
		for (int i = 0; i < count(); i++){
			if (memcmp(&r, get(i), n) == 0){
				return;
			}
		}
		r.value = confirm(m_hand, r, m_last);
		if (r.value > m_value){
			m_result = r;
			m_value  = r.value;
		}
		m_checked.push_back(r);
	}
	bool is_tingpai(int table[38], int n){
		int v[2] = {0};
		register int i;
		for (i = 1; i < 38; i++){
			if (v[n - 1]){
				break;
			}
			int m = table[i];
			if (m == 0){
				continue;
			}
			for (int j = 0; j < m; j++){
				if (v[0] == 0){
					v[0] = i;
				} else {
					v[1] = i;
				}
			}
		}
		form_t &r = m_form;
		if (n == 1){
			return confirm(r, v[0], 0);
		}
		//如果剩余两张牌值相差小于3
		if (v[1] - v[0] < 3){
			if (confirm(r, v[0], v[1])){
				return true;
			}
		}
		//如果剩余两张中有任何一个是癞子
		int g = ghost();
		if (v[0] == g || v[1] == g){
			if (confirm(r, v[0], v[1])){
				return true;
			}
		}
		return false;
	}
	bool is_continue(int mode){
		if (mode == enum_hu)
			return (m_recursion || m_checked.empty());
		return !m_ready;
	}
	void take_next_set(int table[38], int mode){
		if (mode == enum_hu){ //检查胡牌
			if (m_rest == 0){
				push_result(table);
				return;
			}
		} else if (m_rest < 3) { //检查听牌
			if (!m_ready)
				m_ready = is_tingpai(table, m_rest);
			return; //停止继续检查
		}
		register int i;
		for (i = m_begin; i < m_end; i++){
			if (i % 10 == 0){ //无效值
				continue;
			}
			//尝试取刻牌
			if (table[i]){
				try_to_take_ke(table, i, mode);
				if (!is_continue(mode)){
					break;
				}
			}
			//尝试取将牌
			int j = m_form.jiang;
			if (j == 0 && table[i]){
				try_to_take_jiang(table, i, mode);
				if (!is_continue(mode)){
					break;
				}
			}
			//只有牌值小于8的才能凑顺
			if (i < 30 && i % 10 < 8){
				//判断手上是否有该类型的牌
				int t = i / 10;
				if (m_type[t] == 0){
					i = (t + 1) * 10;
					continue;
				}
				try_to_take_shun(table, i, mode);
				if (!is_continue(mode)){
					break;
				}
			}
		}
	}
	void try_to_take_shun(int table[38], int i, int mode){
		register int j;
		int v[3], x = 0;
		for (j = 0; j < 3; j++){
			v[j] = table[i + j] ? 1 : 0;
			x += v[j];
		}
		if (x == 0){
			return;
		}
		if (x == 3){
			push_to_set(table, i, true);
			take_next_set(table, mode);
			pop_from_set(table);
			return;
		}
		int g = ghost();
		if (g == 0){
			return;
		}
		//判断可用癞子数是否足够
		int r = reserved();
		for (j = 0; !r && j < 3; j++){
			if (i + j == g){
				r = 1;
				break;
			}
		}
		int n = 3 - x;
		if (table[g] < n + r){
			return;
		}
		//尝试用癞子凑顺子
		for (j = i; j < i + 3; j++){
			if (table[j] == 0){
				table[j]++;
				table[g]--;
			}
		}
		m_form.rascal += n;
		try_to_take_shun(table, i, mode);
		m_form.rascal -= n;
		for (j = 0; j < 3; j++){
			if (v[j] == 0){
				table[i + j]--;
				table[g]++;
			}
		}
	}
	void try_to_take_ke(int table[38], int i, int mode){
		int j = table[i];
		if (j >= 3){
			push_to_set(table, i, false);
			take_next_set(table, mode);
			pop_from_set(table);
			return;
		}
		int g = ghost();
		if (g == 0 || i == g){
			return;
		}
		//判断可用癞子数是否足够
		int n = 3 - j;
		if (table[g] - reserved() < n){
			return;
		}
		//3张癞子本来就是一副刻牌
		table[i] += n;
		table[g] -= n;
		m_form.rascal += n;
		try_to_take_ke(table, i, mode);
		m_form.rascal -= n;
		table[g] += n;
		table[i] -= n;
	}
	void try_to_take_jiang(int table[38], int i, int mode){
		int j = table[i];
		if (j >= 2){
			push_jiang(table, i);
			take_next_set(table, mode);
			pop_jiang(table);
			return;
		}
		int g = ghost();
		if (g == 0 || i == g){
			return;
		}
		//判断可用癞子数是否足够
		int n = 2 - j;
		if (table[g] - reserved() < n){
			return;
		}
		//2张癞子本来就是一副将牌
		table[i] += n;
		table[g] -= n;
		m_form.rascal += n;
		try_to_take_jiang(table, i, mode);
		m_form.rascal -= n;
		table[g] += n;
		table[i] -= n;
	}
	void reset(int v, bool recursion){
		m_reserved  = 0;
		m_rest      = 0;
		m_value     = 0;
		m_hand      = 0;
		m_begin     = 1;
		m_end       = 38;
		m_last      = v;
		m_ready     = false;
		m_recursion = recursion;
		m_checked.clear();
		m_checked.reserve(10);
		memset(m_type, 0, sizeof(m_type));
		memset(&m_result, 0, sizeof(m_result));
	}
private:
	int    m_ghost;     //癞子牌牌值
	int    m_used;      //已吃碰杠的癞子数
	int    m_reserved;  //当本牌的癞子数量
	int    m_rest;      //剩余手牌数量
	int    m_begin;     //循环开始
	int    m_end;       //循环结束
	int    m_type[4];   //牌类型数组
	int    m_last;      //最后一张牌牌值
	int    m_value;     //最优番值
	int*   m_hand;      //手牌指针
	bool   m_ready;     //已听牌标记
	bool   m_recursion; //是否穷举结果
	form_t m_form;      //当前牌型组合
	form_t m_result;    //最优胡牌牌型
	const void* m_context; //附带上下文
	std::vector<form_t> m_checked;
};
////////////////////////////////////////////////////////////////////////////////
#endif //__MAHJONG_INCLUDED_H_
////////////////////////////////////////////////////////////////////////////////
