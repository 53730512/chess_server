

#ifndef __ALGO_DOUNIU_H_
#define __ALGO_DOUNIU_H_
////////////////////////////////////////////////////////////////////////////////
#include "../../poker/poker.h"
////////////////////////////////////////////////////////////////////////////////
namespace niuniu{
////////////////////////////////////////////////////////////////////////////////
enum bull_type{
    bull_null    = 0,          //��ţ
    bull_one     = 1,          //ţһ
    bull_one2    = 2,          //����ţһ
    bull_two     = 3,          //ţ��
    bull_two2    = 4,          //����ţ��
    bull_three   = 5,          //ţ��
    bull_three2  = 6,          //����ţ��
    bull_four    = 7,          //ţ��
    bull_four2   = 8,          //����ţ��
    bull_five    = 9,          //ţ��
    bull_five2   = 10,         //����ţ��
    bull_six     = 11,         //ţ��
    bull_six2    = 12,         //����ţ��
    bull_seven   = 13,         //ţ��
    bull_seven2  = 14,         //����ţ��
    bull_eight   = 15,         //ţ��
    bull_eight2  = 16,         //����ţ��
    bull_nine    = 17,         //ţ��
    bull_nine2   = 18,         //����ţ��
    bull_niuniu  = 19,         //ţţ
    bull_niuniu2 = 20,         //����ţţ
    bull_silver  = 21,         //��ţ
    bull_gold    = 22,         //��ţ
    bull_order   = 23,         //˳��ţ*
    bull_gourd   = 24,         //��«ţ*
    bull_5_small = 25,         //ȫСţ
    bull_5_big   = 26,         //ȫ��ţ*
    bull_bomb    = 27          //ը��ţ
};
////////////////////////////////////////////////////////////////////////////////
class poker_hand{
public:
    bool is_bomb() const{ //ը��
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
    bool is_five_big() const{ //ȫ��
        poker::value_t value = 0;
        for (int i = 0; i < 5; i++)
            value += poker::get_value(card[i]);
        return (value >= 40);
    }
    bool is_five_small() const{ //ȫС
        poker::value_t value = 0;
        for (int i = 0; i < 5; i++)
            value += poker::get_value(card[i]);
        return (value <= 10);
    }
    bool is_gold() const{ //��ţ
        int count = 0;
        for (int i = 0; i < 5; i++){
            poker::point point = poker::get_point(card[i]);
            if (poker::get_value(card[i]) == 10 && point > poker::point_10)
                count++;
        }
        return (count == 5);
    }
    bool is_silver() const{ //��ţ
        int count = 0;
        for (int i = 0; i < 5; i++){
            if (poker::get_value(card[i]) == 10)
                count++;
        }
        return (count == 5);
    }
    bool is_hulu_niu() const{ //��«
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
    bool is_order_niu() const{ //˳��
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
    //ţֵ(0Ϊ��ţ��10Ϊţţ)
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
                    if (!is_gold_niu){ //����ţ
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
    poker::value_t get_gold_value() const{ //��3��һ����
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
