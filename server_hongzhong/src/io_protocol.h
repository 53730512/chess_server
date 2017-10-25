

#ifndef __IO_PROTOCOL_H_
#define __IO_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
enum protocol{
    user_keepalive              = 0x0000, //心跳协议
    user_auth                   = 0x0001, //认证协议
    user_auth_repeat            = 0x0002, //重复登录
    user_create_room            = 0x0003, //创建房间
    user_enter_room             = 0x0004, //进入房间
    user_leave_room             = 0x0005, //离开房间
    user_get_package            = 0x0006, //通知用户收到礼包
    user_get_payment            = 0x0007, //通知用户收到充值
    room_dismissed              = 0x0008, //房间销毁通知
    room_dismiss_request        = 0x0009, //解散房间申请
    room_dismiss_response       = 0x000A, //解散房间应答
    user_gold_change            = 0x000B, //通知金币数改变
    user_diamond_change         = 0x000C, //通知钻石数改变
    load_player_data            = 0x0101, //加载玩家数据
    load_player_data_ret        = 0x0102, //加载玩家数据结果
    user_activation             = 0x0103, //激活用户
    user_offline                = 0x0104, //用户离线
    user_cache_reset            = 0x0105, //重置在线缓存
    user_cache_reset_ret        = 0x0106, //重置在线缓存回应
    load_player_package         = 0x0107, //加载用户礼包
    load_player_package_ret     = 0x0108, //加载用户礼包回应
    load_player_payment         = 0x0109, //加载充值数据
    load_player_payment_ret     = 0x010A, //加载充值数据回应
    user_auto_enter             = 0x010B, //要求用户自动加入房间
    user_payment_sign           = 0x010C, //充值签名
    user_payment_callback       = 0x010D, //充值回调通知
    user_bind_promoter          = 0x010E, //绑定邀请码
    user_wechat_shared          = 0x010F, //分享微信朋友圈
    load_player_zhanji          = 0x0110, //加载战绩列表
    load_zhanji_round           = 0x0111, //加载战绩数据
    user_set_voice_token        = 0x0112, //设置voicetoken
    dec_offline_diamonds        = 0x0ffb, //扣除离线钻石
    user_is_run_back            = 0x0ffc, //切换前后台
    user_online_repeat          = 0x0ffd, //用户重复在线
    save_player_data            = 0x0ffe, //保存角色数据
    request_invalid             = 0x0fff, //无效请求
    room_min_protocol           = 0x1000  //最小的房间协议号
};
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_PROTOCOL_H_
