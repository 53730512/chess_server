

#ifndef __SQLNET_H_
#define __SQLNET_H_

////////////////////////////////////////////////////////////////////////////////

#if !defined(_MSC_VER) && !defined(__NANO_H_)
typedef long long __int64;
#endif

typedef unsigned char boolean;

typedef void (*sqlnet_callback)(
    const char *name,         //���ݿ���
    int         op,           //��Ϊ
    const char *data,         //���ݼ�
    int         size,         //���ݼ��ֽ���
    __int64     syncid        //ͬ��ID
    );
typedef int sqlnet_hdbc;      //���ݿ����Ӿ����

////////////////////////////////////////////////////////////////////////////////

//��Ч�� sqlnet_connect ����ֵ
#define SQLNET_INVALID_HANDLE (-1)
#define SQLNET_INVALID_INDEX  (-1)
#define SQLNET_VERSION        "1.0.7.0"
#define SQLNET_VERSION_NUMBER 1007000
#define SQLNET_WAIT_FOREVER   0xffffffff

//���ݿ�����ѡ��
#define SQLNET_OPEN_COMPRESS  0x00000001
#define SQLNET_CREATE_ALWAYS  0x00000002

////////////////////////////////////////////////////////////////////////////////

enum SQLNET_TYPE
{
    SQLNET_INTEGER            = 1,
    SQLNET_FLOAT              = 2,
    SQLNET_TEXT               = 3,
    SQLNET_BLOB               = 4,
    SQLNET_NULL               = 5
};

enum SQLNET_ERROR
{
    SQLNET_OK                 =  0,
    SQLNET_ERROR              =  1,
    SQLNET_INTERNAL           =  2,
    SQLNET_PERM               =  3,
    SQLNET_ABORT              =  4,
    SQLNET_BUSY               =  5,
    SQLNET_LOCKED             =  6,
    SQLNET_NOMEM              =  7,
    SQLNET_READONLY           =  8,
    SQLNET_INTERRUPT          =  9,
    SQLNET_IOERR              = 10,
    SQLNET_CORRUPT            = 11,
    SQLNET_NOTFOUND           = 12,
    SQLNET_FULL               = 13,
    SQLNET_CANTOPEN           = 14,
    SQLNET_PROTOCOL           = 15,
    SQLNET_EMPTY              = 16,
    SQLNET_SCHEMA             = 17,
    SQLNET_TOOBIG             = 18,
    SQLNET_CONSTRAINT         = 19,
    SQLNET_MISMATCH           = 20,
    SQLNET_MISUSE             = 21,
    SQLNET_NOLFS              = 22,
    SQLNET_AUTH               = 23,
    SQLNET_FORMAT             = 24,
    SQLNET_RANGE              = 25,
    SQLNET_NOTADB             = 26,
    SQLNET_NOTICE             = 27,
    SQLNET_WARNING            = 28,
    SQLNET_ROW                = 100,
    SQLNET_DONE               = 101,
    SQLNET_NO_OPEN            = 1001,
    SQLNET_NO_PREPAIR         = 1002,
    SQLNET_NO_DATA            = 1003,
    SQLNET_TIMEOUT            = 1004
};

////////////////////////////////////////////////////////////////////////////////

#ifndef SQLNET_API
#define SQLNET_API
#endif

#ifndef SQLNET_CDECL
#define SQLNET_CDECL
#endif

#ifndef SQLNET_STDCALL
#define SQLNET_STDCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////

/*
 ����: ��ʼ�� sqlnet ���ݿ⣬�����ڳ�������ʱ������һ��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL sqlnet_open();

/*
 ����: ����һ��Ψһ��32λ��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_id32(int *pid);            //���ڽ��������32λ��

/*
 ����: ����һ��Ψһ��64λ��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_id64(__int64 *pid);        //���ڽ��������64λ��

/*
 ����: ����һ��Ψһ��64λ��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_add_policy(
    const char *ipaddr            //������ʵ�IP
);

/*
 ����: ���� sqlnet �����ڱ��ؽ���һ�� sqlne t������
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_run(
    unsigned int port,            //���ؼ����� TCP �˿ں�
    const char *host = 0,         //���ؼ����� TCP ��ַ�����Ϊ0�����ʾ�������е�ַ
    const char *key = 0,          //���ݿ�������Կ
    int key_size = 0,             //���ݿ�������Կ�ֽ���
    int options = 0,              //���ݿ�����ѡ��, SQLNET_OPEN_COMPRESS or SQLNET_CREATE_ALWAYS
    int cache_size = 8000,        //���ݿ��ڴ�ҳ����
    bool transaction = true,      //�Ƿ񱣴�������־
    sqlnet_callback callback = 0  //������־�ص�����
);

/*
 ����: ��Զ�̻򱾵ص� sqlnet ���ݿ⣬����һ�������
 ����: ������ֵ��ʾ�ɹ�������Ϊ SQLNET_INVALID_HANDLE
*/
SQLNET_API sqlnet_hdbc SQLNET_STDCALL
sqlnet_connect(
    const char *host,             //Զ������ TCP ��ַ������
    unsigned int port,            //Զ������ TCP �˿ں�
    const char *database,         //���ݿ����Ƶ��ַ���
    const char *key = 0,          //���ݿ�������Կ
    int key_size = 0,             //���ݿ�������Կ�ֽ���
    int options = 0               //���ݿ�����ѡ��, SQLNET_OPEN_COMPRESS or SQLNET_CREATE_ALWAYS
);

/*
 ����: ���ú������ִ��ʱ��(����), Ĭ��Ϊ������ʱ
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_timeout(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    unsigned int ms               //��ʱʱ�������
);

/*
 ����: ����sqlnet��������
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_call(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *data              //������������ַ���
);

/*
 ����: ��ȡ���һ�δ����������Ϣ
 ����: ���������ַ���ָ��
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_last_error(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ��ȡ���һ�δ����ID
 ����: ����ID
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_last_error_code(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �޸����ݿ��û�����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_pwd(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *newpwd            //�µ����ݿ�����
);

/*
 ����: �� sqlnet ����������һ������̽������
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_ping(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int timeout                   //��ʱ������
);

/*
 ����: ����һ�� sqlnet ����ͬ��������־
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_sync(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *data,             //������־����
    int size,                     //�����ֽ���
    __int64 syncid                //������־���к�
);

/*
 ����: ִ��һ�� SQL ���
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_execute(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *sqlstr            //Ҫִ�е�SQL���
);

/*
 ����: ����һ�����񣬲�֧������Ƕ��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_begin(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ύһ���Ѵ���������
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_commit(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ع�һ���Ѵ�������
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_rollback(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ر�һ���򿪵����ݿ�����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_close(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ر�һ���򿪵����ݿ����Ӳ����٣����������ӳ�
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_destory(
    sqlnet_hdbc hdbc
);

/*
 ����: ��ȡ���Ӱ��ļ�¼������
 ����: Ӱ�������
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_effect(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ��ȡ��� insert ���� id
 ����: idֵ
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_last_insert_id(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: Ԥ����һ�� SQL ���
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_prepair(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *sqlstr            //Ҫִ�е�SQL���
);

/*
 ����: ��������Ѱ󶨵Ĳ���
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_reset(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ر�һ���Ѵ��ڵĽ����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_release(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ִ��һ������õ�SQL��䣨sqlnet_prepair��
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_step(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ��ȡ���ִ��ʱ��ĺ�����
 ����: ����ִ��ʱ�䣬��λΪ����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_runtime(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ��ȡ�ֶε��ֽ���
 ����: �ֶε��ֽ���, 0��ʾNULL
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_bytes(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ�ֶε��ֽ���
 ����: �ֶε��ֽ���, 0��ʾNULL
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_bytes_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ����ƣ���Сд����
);

/*
 ����: ��ȡ�ֶε�����
 ����: �ֶε����ͱ�ţ��ο� SQLNET_TYPE ˵��
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_type(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ�ֶε�����
 ����: �ֶε����ͱ�ţ��ο� SQLNET_TYPE ˵��
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_type_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ����ƣ���Сд����
);

/*
 ����: ��ȡ�ֶε�����
 ����: �ֶε������ַ���ָ�룬�ձ�ʾ�����ڵ��ֶλ����
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_column_name(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ���ص��ֶ���
 ����: �ֶε�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_count(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �ж��Ƿ񵽽������β��
 ����: ��0��ʾ����β��������û�е���
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_eof(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: �����ƶ������ָ�뵽��һ����¼
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_move_next(
    sqlnet_hdbc hdbc              //���� sqlnet_connect �ķ���ֵ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻�0
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_get_int(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻�0
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_get_int_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ����ƣ���Сд����
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻�0
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_get_int64(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻�0
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_get_int64_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ�����, ��Сд����
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻� 0.0f
*/
SQLNET_API double SQLNET_STDCALL
sqlnet_get_double(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�����ݿ��ֵΪ NULL �򷵻� 0.0f
*/
SQLNET_API double SQLNET_STDCALL
sqlnet_get_double_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ�����, ��Сд����
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�������򷵻ؿ��ַ���
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_get_text(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�������򷵻ؿ��ַ���
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_get_text_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ�����, ��Сд����
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�������򷵻ؿ�����
*/
SQLNET_API const void* SQLNET_STDCALL
sqlnet_get_blob(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //�ֶ������ţ���1��ʼ
);

/*
 ����: ��ȡ��ǰ��¼���ֶ�ֵ
 ����: ȡ�õ�ֵ�������򷵻ؿ�����
*/
SQLNET_API const void* SQLNET_STDCALL
sqlnet_get_blob_byname(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    const char *name              //�ֶ�����, ��Сд����
);

/*
 ����: Ϊ׼���õ�SQL���󶨿ղ���
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_null(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n                         //���������ţ���1��ʼ
);

/*
 ����: Ϊ׼���õ�SQL����int����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_int(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n,                        //���������ţ���1��ʼ
    int value                     //Ҫ�󶨵�����
);

/*
 ����: Ϊ׼���õ�SQL����__int64����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_int64(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n,                        //���������ţ���1��ʼ
    __int64 value                 //Ҫ�󶨵�����
);

/*
 ����: Ϊ׼���õ�SQL����double����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_double(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n,                        //���������ţ���1��ʼ
    double value                  //Ҫ�󶨵�����
);

/*
 ����: Ϊ׼���õ�SQL����string����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_text(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n,                        //���������ţ���1��ʼ
    const char *value             //Ҫ�󶨵�����
);

/*
 ����: Ϊ׼���õ�SQL����binary����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_blob(
    sqlnet_hdbc hdbc,             //���� sqlnet_connect �ķ���ֵ
    int n,                        //���������ţ���1��ʼ
    const void *value,            //Ҫ�󶨵�����
    int size,                     //Ҫ�󶨵������ֽ���
    bool compress                 //Ҫ�󶨵������ֽ���
);

/*
 ����: �ͷ����е� sqlnet �������ݣ������ڳ����˳�ʱ����
 ����: SQLNET_OK ��ʾ�ɹ�������ֵΪ�����
*/
SQLNET_API int SQLNET_STDCALL sqlnet_finalize();

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

class sqlnet3{
public:
    static bool startup(){
        return (sqlnet_open() == SQLNET_OK);
    }
    static void cleanup(){
        sqlnet_finalize();
    }
    static bool startup(unsigned short port, const char *host = 0, const char *key = 0, int key_size = 0, int options = 0, int cache_size = 8000, bool transaction = true, sqlnet_callback callback = 0){
        if (!startup())
            return false;

        int ret = sqlnet_run(port, host, key, key_size, options, cache_size, transaction, callback);
        return (ret == SQLNET_OK);
    }
    static bool add_policy(const char *ip){
        return (sqlnet_add_policy(ip) == SQLNET_OK);
    }
    static int id_32(){
        int id = 0;
        return (sqlnet_id32(&id) == SQLNET_OK) ? id : 0;
    }
    static __int64 id_64(){
        __int64 id = 0;
        return (sqlnet_id64(&id) == SQLNET_OK) ? id : 0;
    }

public:
    virtual void close(){
        m_hdbc = (sqlnet_close(m_hdbc), SQLNET_INVALID_HANDLE);
    }
    inline void destory(){
        m_hdbc = (sqlnet_destory(m_hdbc), SQLNET_INVALID_HANDLE);
    }
    inline sqlnet3() : m_error(0)
        , m_hdbc(SQLNET_INVALID_HANDLE){
    }
    inline boolean open(const char *host, unsigned short port, const char *database, const char *key = 0, int key_size = 0, int options = 0){
        m_hdbc = sqlnet_connect(host, port, database, key, key_size, options);
        return (m_hdbc != SQLNET_INVALID_HANDLE);
    }
    inline const char* get_lasterr(){
        return sqlnet_last_error(m_hdbc);
    }
    inline const char* get_name(int n){
        return sqlnet_column_name(m_hdbc, n);
    };
    inline int get_errcode() const{
        return m_error;
    }
    inline int get_effect(){
        return sqlnet_effect(m_hdbc);
    }
    inline int get_runtime(){
        return sqlnet_runtime(m_hdbc);
    }
    inline int get_bytes(int n){
        return sqlnet_column_bytes(m_hdbc, n);
    }
    inline int get_bytes(const char *name){
        return sqlnet_column_bytes_byname(m_hdbc, name);
    }
    inline int get_bytes_by_id(int n){
        return get_bytes(n);
    }
    inline int get_bytes_by_name(const char *name){
        return get_bytes(name);
    }
    inline int get_type(int n){
        return sqlnet_column_type(m_hdbc, n);
    }
    inline int get_type(const char *name){
        return sqlnet_column_type_byname(m_hdbc, name);
    }
    inline int get_type_by_id(int n){
        return get_type(n);
    }
    inline int get_type_by_name(const char *name){
        return get_type(name);
    }
    inline int get_columns(){
        return sqlnet_column_count(m_hdbc);
    }
    inline boolean set_timeout(unsigned int ms){
        return (sqlnet_timeout(m_hdbc, ms) == SQLNET_OK);
    }
    inline boolean is_open() const{
        return (m_hdbc != SQLNET_INVALID_HANDLE);
    }
    inline boolean set_pwd(const char *pwdstr){
        m_error = sqlnet_pwd(m_hdbc, pwdstr);
        return (m_error == SQLNET_OK);
    }
    inline boolean ping(int timeout = 1000){
        m_error = sqlnet_ping(m_hdbc, timeout);
        return (m_error == SQLNET_OK);
    }
    inline boolean sync(const char *data, int size, __int64 syncid){
        m_error = sqlnet_sync(m_hdbc, data, size, syncid);
        return (m_error == SQLNET_OK);
    }
    inline boolean call(const char *strcmd){
        m_error = sqlnet_call(m_hdbc, strcmd);
        return (m_error == SQLNET_OK);
    }
    inline boolean execute(const char *sqlstr){
        m_error = sqlnet_execute(m_hdbc, sqlstr);
        return (m_error == SQLNET_OK);
    }
    inline boolean exec(const char *sqlstr){
        return execute(sqlstr);
    }
    inline boolean begin(){
        m_error = sqlnet_begin(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean commit(){
        m_error = sqlnet_commit(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean rollback(){
        m_error = sqlnet_rollback(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean prepair(const char *sqlstr){
        m_error = sqlnet_prepair(m_hdbc, sqlstr);
        return (m_error == SQLNET_OK);
    }
    inline boolean reset(){
        m_error = sqlnet_reset(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean release(){
        m_error = sqlnet_release(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean execute(){
        m_error = sqlnet_step(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    inline boolean step(){
        return execute();
    }
    inline boolean is_eof(){
        return sqlnet_eof(m_hdbc) ? true : false;
    }
    inline boolean move_next(){
        m_error = sqlnet_move_next(m_hdbc);
        return (m_error == SQLNET_OK);
    }
    virtual ~sqlnet3(){close();}

public:
    inline boolean bind(int n){
        m_error = sqlnet_bind_null(m_hdbc, n);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_null(int n){
        return bind(n);
    }
    inline boolean bind(int n, int v){
        m_error = sqlnet_bind_int(m_hdbc, n, v);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_int(int n, int v){
        return bind(n, v);
    }
    inline boolean bind(int n, __int64 v){
        m_error = sqlnet_bind_int64(m_hdbc, n, v);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_int64(int n, __int64 v){
        return bind(n, v);
    }
    inline boolean bind(int n, double v){
        m_error = sqlnet_bind_double(m_hdbc, n, v);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_double(int n, double v){
        return bind(n, v);
    }
    inline boolean bind(int n, const char *v){
        m_error = sqlnet_bind_text(m_hdbc, n, v);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_text(int n, const char *v){
        return bind(n, v);
    }
    inline boolean bind(int n, const void *v, int s, int compress = 0){
        m_error = sqlnet_bind_blob(m_hdbc, n, v, s, compress ? true : false);
        return (m_error == SQLNET_OK);
    }
    inline boolean bind_blob(int n, const void *v, int s, int compress = 0){
        return bind(n, v, s, compress);
    }
    inline int get_int(int n){
        return sqlnet_get_int(m_hdbc, n);
    }
    inline int get_int(const char *name){
        return sqlnet_get_int_byname(m_hdbc, name);
    }
    inline int get_int_by_id(int n){
        return get_int(n);
    }
    inline int get_int_by_name(const char *name){
        return get_int(name);
    }
    inline __int64 get_int64(int n){
        return sqlnet_get_int64(m_hdbc, n);
    }
    inline __int64 get_int64(const char *name){
        return sqlnet_get_int64_byname(m_hdbc, name);
    }
    inline __int64 get_int64_by_id(int n){
        return get_int64(n);
    }
    inline __int64 get_int64_by_name(const char *name){
        return get_int64(name);
    }
    inline double get_double(int n){
        return sqlnet_get_double(m_hdbc, n);
    }
    inline double get_double(const char *name){
        return sqlnet_get_double_byname(m_hdbc, name);
    }
    inline double get_double_by_id(int n){
        return get_double(n);
    }
    inline double get_double_by_name(const char *name){
        return get_double(name);
    }
    inline const char* get_text(int n){
        return sqlnet_get_text(m_hdbc, n);
    }
    inline const char* get_text(const char *name){
        return sqlnet_get_text_byname(m_hdbc, name);
    }
    inline const char* get_text_by_id(int n){
        return get_text(n);
    }
    inline const char* get_text_by_name(const char *name){
        return get_text(name);
    }
    inline const char* get_blob(int n){
        return (char*)sqlnet_get_blob(m_hdbc, n);
    }
    inline const char* get_blob(const char *name){
        return (char*)sqlnet_get_blob_byname(m_hdbc, name);
    }
    inline const char* get_blob_by_id(int n){
        return get_blob(n);
    }
    inline const char* get_blob_by_name(const char *name){
        return get_blob(name);
    }

private:
    int m_error;
    sqlnet_hdbc m_hdbc;
};

#endif //__cplusplus

////////////////////////////////////////////////////////////////////////////////

#if defined(LUA_VERSION) && defined(ctor) //for fflua.
inline void sqlnet_for_lua(lua_State* lua)
{
    ff::fflua_register_t<sqlnet3, ctor()>(lua, "sqlnet")
        .def(&sqlnet3::open,               "open")
        .def(&sqlnet3::close,              "close")
        .def(&sqlnet3::set_timeout,        "set_timeout")
        .def(&sqlnet3::is_open,            "is_open")
        .def(&sqlnet3::get_lasterr,        "get_lasterr")
        .def(&sqlnet3::get_name,           "get_name")
        .def(&sqlnet3::get_errcode,        "get_errcode")
        .def(&sqlnet3::get_effect,         "get_effect")
        .def(&sqlnet3::get_runtime,        "get_runtime")
        .def(&sqlnet3::get_bytes_by_name,  "get_bytes")
        .def(&sqlnet3::get_bytes_by_id,    "get_bytes_by_id")
        .def(&sqlnet3::get_type_by_name,   "get_type")
        .def(&sqlnet3::get_type_by_id,     "get_type_by_id")
        .def(&sqlnet3::get_columns,        "get_columns")
        .def(&sqlnet3::exec,               "exec")
        .def(&sqlnet3::begin,              "begin")
        .def(&sqlnet3::commit,             "commit")
        .def(&sqlnet3::rollback,           "rollback")
        .def(&sqlnet3::prepair,            "prepair")
        .def(&sqlnet3::reset,              "reset")
        .def(&sqlnet3::release,            "release")
        .def(&sqlnet3::step,               "step")
        .def(&sqlnet3::is_eof,             "is_eof")
        .def(&sqlnet3::move_next,          "move_next")
        .def(&sqlnet3::bind_null,          "bind_null")
        .def(&sqlnet3::bind_int,           "bind_int")
        .def(&sqlnet3::bind_double,        "bind_double")
        .def(&sqlnet3::bind_text,          "bind_text")
        .def(&sqlnet3::bind_blob,          "bind_blob")
        .def(&sqlnet3::get_int_by_name,    "get_int")
        .def(&sqlnet3::get_int_by_id,      "get_int_by_id")
        .def(&sqlnet3::get_double_by_name, "get_double")
        .def(&sqlnet3::get_double_by_id,   "get_double_by_id")
        .def(&sqlnet3::get_text_by_name,   "get_text")
        .def(&sqlnet3::get_text_by_id,     "get_text_by_id")
        .def(&sqlnet3::get_blob_by_name,   "get_blob")
        .def(&sqlnet3::get_blob_by_id,     "get_blob_by_id");
}
#endif

////////////////////////////////////////////////////////////////////////////////

#endif //__SQLNET_H_
