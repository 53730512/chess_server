

#ifndef __IO_PROTOCOL_H_
#define __IO_PROTOCOL_H_
////////////////////////////////////////////////////////////////////////////////
enum protocol{
    user_keepalive              = 0x0000, //����Э��
    user_auth                   = 0x0001, //��֤Э��
    user_auth_repeat            = 0x0002, //�ظ���¼
    user_create_room            = 0x0003, //��������
    user_enter_room             = 0x0004, //���뷿��
    user_leave_room             = 0x0005, //�뿪����
    user_get_package            = 0x0006, //֪ͨ�û��յ����
    user_get_payment            = 0x0007, //֪ͨ�û��յ���ֵ
    room_dismissed              = 0x0008, //��������֪ͨ
    room_dismiss_request        = 0x0009, //��ɢ��������
    room_dismiss_response       = 0x000A, //��ɢ����Ӧ��
    user_gold_change            = 0x000B, //֪ͨ������ı�
    user_diamond_change         = 0x000C, //֪ͨ��ʯ���ı�
    load_player_data            = 0x0101, //�����������
    load_player_data_ret        = 0x0102, //����������ݽ��
    user_activation             = 0x0103, //�����û�
    user_offline                = 0x0104, //�û�����
    user_cache_reset            = 0x0105, //�������߻���
    user_cache_reset_ret        = 0x0106, //�������߻����Ӧ
    load_player_package         = 0x0107, //�����û����
    load_player_package_ret     = 0x0108, //�����û������Ӧ
    load_player_payment         = 0x0109, //���س�ֵ����
    load_player_payment_ret     = 0x010A, //���س�ֵ���ݻ�Ӧ
    user_auto_enter             = 0x010B, //Ҫ���û��Զ����뷿��
    user_payment_sign           = 0x010C, //��ֵǩ��
    user_payment_callback       = 0x010D, //��ֵ�ص�֪ͨ
    user_bind_promoter          = 0x010E, //��������
    user_wechat_shared          = 0x010F, //����΢������Ȧ
    load_player_zhanji          = 0x0110, //����ս���б�
    load_zhanji_round           = 0x0111, //����ս������
    user_set_voice_token        = 0x0112, //����voicetoken
    dec_offline_diamonds        = 0x0ffb, //�۳�������ʯ
    user_is_run_back            = 0x0ffc, //�л�ǰ��̨
    user_online_repeat          = 0x0ffd, //�û��ظ�����
    save_player_data            = 0x0ffe, //�����ɫ����
    request_invalid             = 0x0fff, //��Ч����
    room_min_protocol           = 0x1000  //��С�ķ���Э���
};
////////////////////////////////////////////////////////////////////////////////
#endif //__IO_PROTOCOL_H_
