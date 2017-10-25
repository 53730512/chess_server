

#ifndef __NPX_OS_H_
#define __NPX_OS_H_

////////////////////////////////////////////////////////////////////////////////
#if defined(__APPLE__) && defined(__GNUC__)
#define NPX_MACX
#elif defined(__MACOSX__)
#define NPX_MACX
#elif defined(macintosh)
#define NPX_MAC9
#elif defined(__CYGWIN__)
#define NPX_CYGWIN
#elif defined(WIN64) || defined(_WIN64) || defined(__WIN64__) 
#define NPX_WIN64
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define NPX_WIN32
#elif defined(__sun) || defined(sun)
#define NPX_SOLARIS
#elif defined(hpux) || defined(__hpux)
#define NPX_HPUX
#elif defined(__linux__) || defined(__linux)
#define NPX_LINUX
#elif defined(__FreeBSD__)
#define NPX_FREEBSD
#elif defined(__NetBSD__)
#define NPX_NETBSD
#elif defined(__OpenBSD__)
#define NPX_OPENBSD
#else
#error "ERROR: Unsupported operating system."
#endif
////////////////////////////////////////////////////////////////////////////////
#if defined(NPX_MAC9) || defined(NPX_MACX)
#define NPX_APPLE
#endif
#if defined(NPX_FREEBSD) || defined(NPX_NETBSD) || defined(NPX_OPENBSD)
#define NPX_BSD4
#endif
#if defined(NPX_WIN32) || defined(NPX_WIN64)
#define NPX_WINDOWS
#endif
////////////////////////////////////////////////////////////////////////////////

#endif //__NPX_OS_H_
