#define UNIX

#define SHELL_Q		'\''
#define GPDATADIR "/usr/local/lib/pari/galdata"
#define GPMISCDIR "/usr/local/lib/pari"

#define PARI_BYTE_ORDER    1234
#define NOEXP2	/* Otherwise elliptic.t:11 rounds differetly, and fails */

#define USE_GETRUSAGE 1

#define HAS_DLOPEN

#define PARI_DOUBLE_FORMAT 1

#define DL_DFLT_NAME	NULL

#define PARI_VERSION_CODE	131844
#define PARI_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))
#define PARI_VERSION_SHIFT	8

