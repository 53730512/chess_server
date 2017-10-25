

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
    bull_one2    = 2,          //金牌牛一
    bull_two     = 3,          //牛二
    bull_two2    = 4,          //金牌牛二
    bull_three   = 5,          //牛三
    bull_three2  = 6,          //金牌牛三
    bull_four    = 7,          //牛四
    bull_four2   = 8,          //金牌牛四
    bull_five    = 9,          //牛五
    bull_five2   = 10,         //金牌牛五
    bull_six     = 11,         //牛六
    bull_six2    = 12,         //金牌牛六
    bull_seven   = 13,         //牛七
    bull_seven2  = 14,         //金牌牛七
    bull_eight   = 15,         //牛八
    bull_eight2  = 16,         //金牌牛八
    bull_nine    = 17,         //牛九
    bull_nine2   = 18,         //金牌牛九
    bull_niuniu  = 19,         //牛牛
    bull_niuniu2 = 20,         //金牌牛牛
    bull_silver  = 21,         //银牛
    bull_gold    = 22,         //金牛
    bull_order   = 23,         //顺子牛*
    bull_gourd   = 24,         //葫芦牛*
    bull_5_small = 25,         //全小牛
    bull_5_big   = 26,         //全大牛*
    bull_bomb    = 27          //炸弹牛
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
    bool is_five_big() const{ //全大
        poker::value_t value = 0;
        for (int i = 0; i < 5; i++)
            value += poker::get_value(card[i]);
        return (value >= 40);
    }
    bool is_five_small() const{ //全小
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
    bool is_hulu_niu() const{ //葫芦
        for (int i = 0; i < 3; i++){
            for (int j = i + 1; j < 5; j++){
                for (int k = j + 1; k < 5; k++){
                    poker::value_t v1 = poker::get_value(card[i]);
                    poker::value_t v2 = poker::get_value(card[j]);
                    poker::value_t v3 = poker::get_value(card[k]);
                    if (v1 != v2 || v2 != v3 || v1 != v3){
                        continue;
                    }
                    poker::value_t v4 = 0;
                    poker::value_t v5 = 0;
                    for (int m = 0; m < 5; m++){
                        if (m == i || m == j || m == k){
                            continue;
                        }
                        if (v4 == 0){
                            v4 = poker::get_value(card[m]);
                        } else {
                            v5 = poker::get_value(card[m]);
                        }
                    }
                    if (v4 == v5){
                        return true;
                    }
                }
            }
        }
        return false;
    }
    bool is_order_niu() const{ //顺子
        poker::value_t data[5];
        for (int i = 0; i < 5; i++){
            data[i] = poker::get_value(card[i]);
        }
        poker::shellsort<poker::value_t>(data, 5);
        for (int i = 1; i < 5; i++){
            if (data[i] - data[i - 1] != 1){
                return false;
            }
        }
        return true;
    }
    //牛值(0为无牛，10为牛牛)
    bull_type is_have_bull(bool no_flower) const{
        bull_type result = bull_null;
        for (int i = 0; i < 3; i++){
            for (int j = i + 1; j < 5; j++){
                for (int k = j + 1; k < 5; k++){
                    poker::value_t v1 = poker::get_value(card[i]);
                    poker::value_t v2 = poker::get_value(card[j]);
                    poker::value_t v3 = poker::get_value(card[k]);
                    bool is_gold_niu = false;
                    if (no_flower){
                        is_gold_niu = (v1 == v2 && v2 == v3);
                    }
                    if (!is_gold_niu){ //金牌牛
                        if ((v1 + v2 + v3) % 10){
                            continue;
                        }
                    }
                    poker::value_t v4 = 0;
                    for (int m = 0; m < 5; m++){
                        if (m == i || m == j || m == k)
                            continue;
                        v4 += poker::get_value(card[m]);
                    }
                    int niu = (v4 % 10) ? (v4 % 10) : 10;
                    niu = is_gold_niu ? niu * 2 : niu * 2 - 1;
                    if (niu > result){
                        result = (bull_type)niu;
                    }
                }
            }
        }
        return result;
    }
    poker::value_t get_max_value() const{
        poker::value_t max_value = 0;
        for (int i = 0; i < 5; i++){
            if (card[i] > max_value){
                max_value = card[i];
            }
        }
        return max_value;
    }
    poker::value_t get_gold_value() const{ //有3张一样的
        for (int i = 0; i < 3; i++){
            for (int j = i + 1; j < 5; j++){
                for (int k = j + 1; k < 5; k++){
                    poker::value_t v1 = poker::get_value(card[i]);
                    poker::value_t v2 = poker::get_value(card[j]);
                    poker::value_t v3 = poker::get_value(card[k]);
                    if (v1 == v2 || v2 == v3){
                        return v1;
                    }
                }
            }
        }
        return 0;
    }
    inline poker_hand(){clear();}
    inline void clear(){memset(card, 0, sizeof(card));}
    inline poker::value_t get_poker(int i) const{return card[i];}
    inline void set_poker(int i, poker::value_t v){card[i] = v;}
private:
    poker::value_t card[5];
};
////////////////////////////////////////////////////////////////////////////////
} //End namespace niuniu
////////////////////////////////////////////////////////////////////////////////
#endif //__ALGO_DOUNIU_H_
