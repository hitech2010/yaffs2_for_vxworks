/*由yaffs2 ydirectenv.h和yvxenv.h合并得来*/
#ifndef _VXEXTRAS_H_
#define _VXEXTRAS_H_

#include "vxWorks.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "yaffs_hweight.h"

void yaffs_bug_fn(const char *file_name, int line_no);

#define BUG() do { yaffs_bug_fn(__FILE__, __LINE__); } while (0)

typedef unsigned long loff_t;

/*typedef unsigned long mode_t;*/
typedef unsigned long dev_t;
/*typedef unsigned long off_t;*/
typedef unsigned long ino_t;



#ifndef Y_LOFF_T
#define Y_LOFF_T loff_t
#endif



/*extern void TickWatchDog();*/

#define	S_ISSOCK(mode)	((mode & S_IFMT) == S_IFSOCK)	/* fifo special */

#include "assert.h"
#define YBUG() assert(1)

#define YCHAR char
#define YUCHAR unsigned char
#define _Y(x) x

#define yaffs_strcat(a, b)	strcat(a, b)
#define yaffs_strcpy(a,b)    strcpy(a,b)
#define yaffs_strncpy(a,b,c) strncpy(a,b,c)
#define yaffs_strnlen(s, m)	strnlen(s, m)

#ifdef CONFIG_YAFFS_CASE_INSENSITIVE
#define yaffs_strcmp(a, b)	strcasecmp(a, b)
#define yaffs_strncmp(a, b, c)	strncasecmp(a, b, c)
#else
#define yaffs_strcmp(a, b)	strcmp(a, b)
#define yaffs_strncmp(a, b, c)	strncmp(a, b, c)
#endif

/*#define yaffs_strncmp(a,b,c) strncmp(a,b,c)*/

#define hweight8(x)	yaffs_hweight8(x)
#define hweight32(x)	yaffs_hweight32(x)

#define sort(base, n, sz, cmp_fn, swp) qsort(base, n, sz, cmp_fn)

#define yaffs_strlen(s)	     strlen(s)
#define yaffs_sprintf	     sprintf
#define yaffs_toupper(a)     toupper(a)

#ifdef NO_inline
#define inline
#else
#define inline __inline__
#endif


#define kmalloc(x, flags) malloc(x)
#define kfree(x)   free(x)
#define vmalloc(x) malloc(x)
#define vfree(x)   free(x)

#define cond_resched()  do {} while (0)

#define YYIELD()  do{taskDelay(1);}while(0);


#ifndef Y_DUMP_STACK
#define Y_DUMP_STACK() do { } while (0)
#endif

#define YAFFS_PATH_DIVIDERS  "/"

/*#define YINFO(s) YPRINTF(( __FILE__ " %d %s\n",__LINE__,s))*/
/*#define YALERT(s) YINFO(s)*/
#define yaffs_trace(msk, fmt, ...) do { \
	if (yaffs_trace_mask & (msk)) \
		printf("yaffs: " fmt "\n", ##__VA_ARGS__); \
} while (0)

#define TENDSTR "\n"
#define TSTR(x) x
#define TOUT(p) printf p


#define YAFFS_LOSTNFOUND_NAME		"lost+found"
#define YAFFS_LOSTNFOUND_PREFIX		"obj"

#include "yaffscfg.h"

#define Y_CURRENT_TIME yaffsfs_CurrentTime()
#define Y_TIME_CONVERT(x) x

#define YAFFS_ROOT_MODE				0666
#define YAFFS_LOSTNFOUND_MODE		0666

#define yaffs_SumCompare(x,y) ((x) == (y))

#include "yaffs_list.h"

#include "yaffsfs.h"
#endif
