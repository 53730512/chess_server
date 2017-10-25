

#ifndef __ALGO_DOUNIU_H_
#define __ALGO_DOUNIU_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../poker/poker.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
enum bull_type{
    bull_null    = 0,          //无牛
    bull_one     = 1,          //牛一
    bull_two     = 2,          //牛二
    bull_three   = 3,          //牛三
    bull_four    = 4,          //牛四
    bull_five    = 5,          //牛五
    bull_six     = 6,          //牛六
    bull_seven   = 7,          //牛七
    bull_eight   = 8,          //牛八
    bull_nine    = 9,          //牛九
    bull_douniu  = 10,         //牛牛
    bull_silver  = 11,         //银牛
    bull_gold    = 12,         //金牛
    bull_5_small = 13,         //五小牛
    bull_bomb    = 14          //炸弹牛
};
////////////////////////////////////////////////////////////////////////////////
class poker_hand{
public:
    bool is_bomb() const{ //炸弹
        for (int i = 0; i < 5; i++){
            for (int j = i + 1; j < 5; j++){
                for (int k = j + 1; k < 5; k++){
                    for (int m = k + 1; m < 5; m++){
                        poker::value_t v1 = poker::get_point(card[i]);
                        poker::value_t v2 = poker::get_point(card[j]);
                        poker::value_t v3 = poker::get_point(card[k]);
                        poker::value_t v4 = poker::get_point(card[m]);
                        if (v1 == v2 && v2 == v3 && v3 == v4){
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    bool is_five_small() const{ //五小
        poker::value_t value = 0;
        for (int i = 0; i < 5; i++)
            value += poker::get_value(card[i]);
        return (value <= 10);
    }
    bool is_gold() const{ //金牛
        int count = 0;
        for (int i = 0; i < 5; i++){
            poker::point point = poker::get_point(card[i]);
            if (poker::get_value(card[i]) == 10 && point > poker::point_10)
                count++;
        }
        return (count == 5);
    }
    bool is_silver() const{ //银牛
        int count = 0;
        for (int i = 0; i < 5; i++){
            if (poker::get_value(card[i]) == 10)
                count++;
        }
        return (count == 5);
    }
    //牛值(0为无牛，10为牛牛)
    bull_type is_have_bull(int select[3]){
        bull_type result = bull_null;
        for (int i = 0; i < 3; i++){
            for (int j = i + 1; j < 5; j++){
                for (int k = j + 1; k < 5; k++){
                    poker::value_t v1 = poker::get_value(card[i]);
                    poker::value_t v2 = poker::get_value(card[j]);
                    poker::value_t v3 = poker::get_value(card[k]);
                    if ((v1 + v2 + v3) % 10){
                        continue;
                    }
                    poker::value_t v4 = 0;
                    for (int m = 0; m < 5; m++){
                        if (m == i || m == j || m == k)
                            continue;
                        v4 += poker::get_value(card[m]);
                    }
                    int niu = (v4 % 10) ? (v4 % 10) : 10;
                    if (niu > result){
                        result    = (bull_type)niu;
                        select[0] = i;
                        select[1] = j;
                        select[2] = k;
                    }
                }
            }
        }
        return result;
    }
    poker::point get_max_point(poker::type &type) const{
        poker::point max_point = poker::point_A;
        for (int i = 0; i < 5; i++){
            poker::point point = poker::get_point(card[i]);
            if (point > max_point){
                max_point = point;
                type = poker::get_type(card[i]);
            }
        }
        return max_point;
    }
    inline poker_hand(){clear();}
    inline void clear(){memset(card, 0, sizeof(card));}
private:
    poker::value_t card[5];
};
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ALGO_DOUNIU_H_
