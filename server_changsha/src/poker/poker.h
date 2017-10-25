

#ifndef __POKER_H_
#define __POKER_H_
////////////////////////////////////////////////////////////////////////////////
namespace poker{
////////////////////////////////////////////////////////////////////////////////
enum type{
    d_diamond     = 1,       //方(块)
    c_club        = 2,       //梅(花)
    h_heart       = 3,       //红(心)
    s_spade       = 4,       //黑(桃)
    k_joker       = 5        //杰克
};
////////////////////////////////////////////////////////////////////////////////
enum point{
    point_null    = 0,       //无效
    point_A       = 1,       //A
    point_2       = 2,       //2
    point_3       = 3,       //3
    point_4       = 4,       //4
    point_5       = 5,       //5
    point_6       = 6,       //6
    point_7       = 7,       //7
    point_8       = 8,       //8
    point_9       = 9,       //9
    point_10      = 10,      //10
    point_J       = 11,      //J
    point_Q       = 12,      //Q
    point_K       = 13,      //K
    point_J_black = 14,      //小王
    point_J_red   = 15       //大王
};
////////////////////////////////////////////////////////////////////////////////
typedef unsigned char value_t;
inline point   get_point(value_t v){return (point)((v & 0xf0) >> 4);}
inline type    get_type (value_t v){return (type)  (v & 0x0f);}
inline value_t set_value(point p, type t){return  ((p & 0x0f) << 4) | (t & 0x0f);}
inline value_t get_value(value_t v){return get_point(v) > 9 ? 10 : get_point(v);}
////////////////////////////////////////////////////////////////////////////////
//希尔排序算法
////////////////////////////////////////////////////////////////////////////////
template<typename _Ty> inline void shellsort(_Ty *p, int n){
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
} //End namespace poker
////////////////////////////////////////////////////////////////////////////////
#endif //__POKER_H_
