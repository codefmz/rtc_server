#ifndef _PLATFORM_COMPAT_H_
#define _PLATFORM_COMPAT_H_

#include <cstdio>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

#endif
