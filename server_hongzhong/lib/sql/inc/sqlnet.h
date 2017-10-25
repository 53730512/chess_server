

#ifndef __SQLNET_H_
#define __SQLNET_H_

////////////////////////////////////////////////////////////////////////////////

#if !defined(_MSC_VER) && !defined(__NANO_H_)
typedef long long __int64;
#endif

typedef unsigned char boolean;

typedef void (*sqlnet_callback)(
    const char *name,         //数据库名
    int         op,           //行为
    const char *data,         //数据集
    int         size,         //数据集字节数
    __int64     syncid        //同步ID
    );
typedef int sqlnet_hdbc;      //数据库连接句柄号

////////////////////////////////////////////////////////////////////////////////

//无效的 sqlnet_connect 返回值
#define SQLNET_INVALID_HANDLE (-1)
#define SQLNET_INVALID_INDEX  (-1)
#define SQLNET_VERSION        "1.0.7.0"
#define SQLNET_VERSION_NUMBER 1007000
#define SQLNET_WAIT_FOREVER   0xffffffff

//数据库连接选项
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
 功能: 初始化 sqlnet 数据库，必须在程序启动时被调用一次
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL sqlnet_open();

/*
 功能: 生成一个唯一的32位数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_id32(int *pid);            //用于接收输出的32位数

/*
 功能: 生成一个唯一的64位数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_id64(__int64 *pid);        //用于接收输出的64位数

/*
 功能: 生成一个唯一的64位数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_add_policy(
    const char *ipaddr            //允许访问的IP
);

/*
 功能: 运行 sqlnet 服务，在本地建立一个 sqlne t服务器
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_run(
    unsigned int port,            //本地监听的 TCP 端口号
    const char *host = 0,         //本地监听的 TCP 地址，如果为0，则表示监听所有地址
    const char *key = 0,          //数据库连接密钥
    int key_size = 0,             //数据库连接密钥字节数
    int options = 0,              //数据库连接选项, SQLNET_OPEN_COMPRESS or SQLNET_CREATE_ALWAYS
    int cache_size = 8000,        //数据库内存页数量
    bool transaction = true,      //是否保存事务日志
    sqlnet_callback callback = 0  //事务日志回调函数
);

/*
 功能: 打开远程或本地的 sqlnet 数据库，返回一个句柄号
 返回: 正整数值表示成功，否则为 SQLNET_INVALID_HANDLE
*/
SQLNET_API sqlnet_hdbc SQLNET_STDCALL
sqlnet_connect(
    const char *host,             //远程主机 TCP 地址或域名
    unsigned int port,            //远程主机 TCP 端口号
    const char *database,         //数据库名称的字符串
    const char *key = 0,          //数据库连接密钥
    int key_size = 0,             //数据库连接密钥字节数
    int options = 0               //数据库连接选项, SQLNET_OPEN_COMPRESS or SQLNET_CREATE_ALWAYS
);

/*
 功能: 设置函数最大执行时间(毫秒), 默认为永不超时
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_timeout(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    unsigned int ms               //超时时间毫秒数
);

/*
 功能: 运行sqlnet控制命令
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_call(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *data              //控制命令语句字符串
);

/*
 功能: 获取最后一次错误的描述信息
 返回: 错误描述字符串指针
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_last_error(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 获取最后一次错误的ID
 返回: 错误ID
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_last_error_code(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 修改数据库用户密码
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_pwd(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *newpwd            //新的数据库密码
);

/*
 功能: 给 sqlnet 服务器发送一个网络探测请求
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_ping(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int timeout                   //超时毫秒数
);

/*
 功能: 向另一个 sqlnet 服务同步事务日志
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_sync(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *data,             //事务日志数据
    int size,                     //数据字节数
    __int64 syncid                //事务日志序列号
);

/*
 功能: 执行一条 SQL 语句
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_execute(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *sqlstr            //要执行的SQL语句
);

/*
 功能: 开启一个事务，不支持事务嵌套
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_begin(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 提交一个已创建的事务
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_commit(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 回滚一个已创建事务
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_rollback(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 关闭一个打开的数据库连接
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_close(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 关闭一个打开的数据库连接并销毁，不进入连接池
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_destory(
    sqlnet_hdbc hdbc
);

/*
 功能: 获取语句影响的记录行数量
 返回: 影响的行数
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_effect(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 获取最后 insert 语句的 id
 返回: id值
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_last_insert_id(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 预编译一条 SQL 语句
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_prepair(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *sqlstr            //要执行的SQL语句
);

/*
 功能: 清除所有已绑定的参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_reset(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 关闭一个已存在的结果集
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_release(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 执行一个编译好的SQL语句（sqlnet_prepair）
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_step(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 获取语句执行时间的毫秒数
 返回: 语句的执行时间，单位为毫秒
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_runtime(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 获取字段的字节数
 返回: 字段的字节数, 0表示NULL
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_bytes(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取字段的字节数
 返回: 字段的字节数, 0表示NULL
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_bytes_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称，大小写敏感
);

/*
 功能: 获取字段的类型
 返回: 字段的类型编号，参考 SQLNET_TYPE 说明
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_type(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取字段的类型
 返回: 字段的类型编号，参考 SQLNET_TYPE 说明
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_type_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称，大小写敏感
);

/*
 功能: 获取字段的名称
 返回: 字段的名称字符串指针，空表示不存在的字段或错误
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_column_name(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取返回的字段数
 返回: 字段的数量
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_column_count(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 判断是否到结果集的尾部
 返回: 非0表示到达尾部，否则没有到达
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_eof(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 向下移动结果集指针到下一条记录
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_move_next(
    sqlnet_hdbc hdbc              //调用 sqlnet_connect 的返回值
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回0
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_get_int(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回0
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_get_int_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称，大小写敏感
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回0
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_get_int64(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回0
*/
SQLNET_API __int64 SQLNET_STDCALL
sqlnet_get_int64_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称, 大小写敏感
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回 0.0f
*/
SQLNET_API double SQLNET_STDCALL
sqlnet_get_double(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，数据库该值为 NULL 则返回 0.0f
*/
SQLNET_API double SQLNET_STDCALL
sqlnet_get_double_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称, 大小写敏感
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，错误则返回空字符串
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_get_text(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，错误则返回空字符串
*/
SQLNET_API const char* SQLNET_STDCALL
sqlnet_get_text_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称, 大小写敏感
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，错误则返回空数据
*/
SQLNET_API const void* SQLNET_STDCALL
sqlnet_get_blob(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //字段索引号，从1开始
);

/*
 功能: 获取当前记录的字段值
 返回: 取得的值，错误则返回空数据
*/
SQLNET_API const void* SQLNET_STDCALL
sqlnet_get_blob_byname(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    const char *name              //字段名称, 大小写敏感
);

/*
 功能: 为准备好的SQL语句绑定空参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_null(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n                         //参数索引号，从1开始
);

/*
 功能: 为准备好的SQL语句绑定int参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_int(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n,                        //参数索引号，从1开始
    int value                     //要绑定的数据
);

/*
 功能: 为准备好的SQL语句绑定__int64参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_int64(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n,                        //参数索引号，从1开始
    __int64 value                 //要绑定的数据
);

/*
 功能: 为准备好的SQL语句绑定double参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_double(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n,                        //参数索引号，从1开始
    double value                  //要绑定的数据
);

/*
 功能: 为准备好的SQL语句绑定string参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_text(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n,                        //参数索引号，从1开始
    const char *value             //要绑定的数据
);

/*
 功能: 为准备好的SQL语句绑定binary参数
 返回: SQLNET_OK 表示成功，其他值为错误号
*/
SQLNET_API int SQLNET_STDCALL
sqlnet_bind_blob(
    sqlnet_hdbc hdbc,             //调用 sqlnet_connect 的返回值
    int n,                        //参数索引号，从1开始
    const void *value,            //要绑定的数据
    int size,                     //要绑定的数据字节数
    bool compress                 //要绑定的数据字节数
);

/*
 功能: 释放所有的 sqlnet 对象及数据，必须在程序退出时调用
 返回: SQLNET_OK 表示成功，其他值为错误号
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
