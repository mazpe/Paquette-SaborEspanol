/*  This file was created by Configure. Any change made to it will be lost
 *  next time Configure is run. */
#ifndef __CONFIG_H__
#define __CONFIG_H__
#define UNIX
#define GPHELP "/usr/local/bin/gphelp"
#define GPDATADIR "/usr/local/share/pari"
#define SHELL_Q '\''

#define PARIVERSION "GP/PARI CALCULATOR Version 2.3.4 (released)"
#ifdef __cplusplus
# define PARIINFO "C++ i686 running linux (ix86 kernel) 32-bit version"
#else
# define PARIINFO "i686 running linux (ix86 kernel) 32-bit version"
#endif
#define PARI_VERSION_CODE 131844
#define PARI_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define PARI_VERSION_SHIFT 8

#define PARI_DOUBLE_FORMAT 1
#define GCC_VERSION "4.2.3 (Ubuntu 4.2.3-2ubuntu7)"
#define ASMINLINE

/*  Location of GNU gzip program (enables reading of .Z and .gz files). */
#define GNUZCAT
#define ZCAT "/bin/gzip -dc"

#define USE_GETRUSAGE 1
#define HAS_SIGACTION
#define HAS_DLOPEN
#define STACK_CHECK
#define HAS_VSNPRINTF
#define HAS_TIOCGWINSZ
#define HAS_STRFTIME
#define HAS_STAT
#endif
