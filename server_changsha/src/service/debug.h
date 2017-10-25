

#ifndef __DEBUG_DEFINE_H_
#define __DEBUG_DEFINE_H_
////////////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
#include <libnpx.hpp>
////////////////////////////////////////////////////////////////////////////////
#define PRINT(fmt, ...) {printf(fmt, __VA_ARGS__);}
#ifndef TRACE
#ifdef _DEBUG
#define TRACE(fmt, ...) {printf(fmt, __VA_ARGS__);}
#else
#define TRACE(fmt, ...) void(0)
#endif
#endif
////////////////////////////////////////////////////////////////////////////////
#ifndef NPX_WINDOWS
#ifndef __int64
#define __int64 long long
#endif
inline int _vscprintf(const char *format, va_list pargs){
    int retval; 
    va_list argcopy;
    va_copy(argcopy, pargs); 
    retval = vsnprintf(0, 0, format, argcopy); 
    va_end(argcopy); 
    return retval;
}
#endif
////////////////////////////////////////////////////////////////////////////////
namespace console{
////////////////////////////////////////////////////////////////////////////////
inline void log(const char *format, ...){
    char cache[8192] = {0};
    char *errinfo = cache;
    if (format){
        va_list args;
        va_start(args, format);
        size_t bytes = _vscprintf(format, args) + 1;
        if (bytes > sizeof(cache)){
            errinfo = (char*)malloc(bytes);
            if (!errinfo)
                return;
        }
        vsprintf(errinfo, format, args);
        va_end(args);
    }
    time_t time_now = time(0);
    struct tm *ptm = localtime(&time_now);
    io::stringc content;
    content.format("[%02d:%02d:%02d] %s\r\n"
        , ptm->tm_hour
        , ptm->tm_min
        , ptm->tm_sec
        , errinfo
        );
    if (errinfo != cache){
        free(errinfo);
    }
    if (!content.is_utf8()){
        content = content.to_utf8();
    }
    io::stringc filename;
    filename.format("./~error/%02d%02d%02d.log"
        , ptm->tm_year + 1900
        , ptm->tm_mon + 1
        , ptm->tm_mday
        );
    npx_make_dir("./~error");
    FILE *fpwrite = fopen(filename.c_str(), "a");
    if (fpwrite){
        fwrite(content.c_str(), 1, content.size(), fpwrite);
        fclose(fpwrite);
    }
}
////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////
#endif //__DEBUG_DEFINE_H_
