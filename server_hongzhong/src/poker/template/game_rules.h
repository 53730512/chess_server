

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
    bull_two     = 2,          //ţ��
    bull_three   = 3,          //ţ��
    bull_four    = 4,          //ţ��
    bull_five    = 5,          //ţ��
    bull_six     = 6,          //ţ��
    bull_seven   = 7,          //ţ��
    bull_eight   = 8,          //ţ��
    bull_nine    = 9,          //ţ��
    bull_douniu  = 10,         //ţţ
    bull_silver  = 11,         //��ţ
    bull_gold    = 12,         //��ţ
    bull_5_small = 13,         //��Сţ
    bull_bomb    = 14          //ը��ţ
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
    bool is_five_small() const{ //��С
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
    //ţֵ(0Ϊ��ţ��10Ϊţţ)
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
