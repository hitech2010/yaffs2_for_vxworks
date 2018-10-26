#ifndef _vxLIB_H_
#define _vxLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "yaffs_guts.h"
#include "iosLib.h"
#include "semLib.h"
#include "yaffsfs.h"

#define OK	 	0
#define ERROR	(-1)

typedef struct YAFFS_FILE_DESC * YAFFS_FILE_DESC_ID;
typedef struct YAFFS_VOLUME_DESC * YAFFS_VOLUME_DESC_ID;
/* Volumn descriptor */

struct YAFFS_VOLUME_DESC{
	DEV_HDR	devHdr;	/* i/o system device header */

	/* TODO: does system needs these ? */
	unsigned int	magic;	/* control magic number */
	BOOL	mounted;	/* volumn mounted */

 char *diskname;

	SEM_ID devSem;
	SEM_ID shortSem;

	struct yaffs_dev* pYaffsDev;
	YAFFS_FILE_DESC_ID pFdList;	/* file descriptor lists */
	int maxFiles;
} ;

/* 
 * File descriptor 
 *  copy from yaffsfs_Handle
 *  yaffs_Object acts as DOS_FILE_HDL
 */
	/*typedef struct __opaque yaffs_DIR;*/
struct YAFFS_FILE_DESC
{
	unsigned char  inUse;		/* this file descriptor is in use */
	unsigned char  readOnly;	/* this file descriptor is readonly */
	unsigned char  append;		/* append only	*/
	unsigned char  exclusive;	/* exclusive	*/
	unsigned long position;		/* current position in file	*/
	YAFFS_VOLUME_DESC_ID pVolDesc;	/* yaffs volume pointer */


/*	struct yaffs_DIR diry;*/
	char path[YAFFS_MAX_NAME_LENGTH+1];
	char oripath[YAFFS_MAX_NAME_LENGTH+1];

	int yfd;
	struct yaffs_obj * obj;	/* the object */
	struct yaffs_DIR *ydir;
};

/*************************************************************************/
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#define YAFFS_NFILES_DEFAULT	20
#define YAFFS_DFLT_DIR_MASK 0777
#define YAFFS_DFLT_FILE_MASK 0666

#define M_yaffs2Lib	(200<<16)	/* See vwModNum.h */

#define S_yaffs2Lib_32BIT_OVERFLOW		(M_yaffs2Lib |  1)
#define S_yaffs2Lib_DISK_FULL			(M_yaffs2Lib |  2)
#define S_yaffs2Lib_FILE_NOT_FOUND		(M_yaffs2Lib |  3)
#define S_yaffs2Lib_NO_FREE_FILE_DESCRIPTORS 	(M_yaffs2Lib |  4)
#define S_yaffs2Lib_NOT_FILE			(M_yaffs2Lib |  5)
#define S_yaffs2Lib_NOT_SAME_VOLUME	        (M_yaffs2Lib |  6)
#define S_yaffs2Lib_NOT_DIRECTORY		(M_yaffs2Lib |  7)
#define S_yaffs2Lib_DIR_NOT_EMPTY		(M_yaffs2Lib |  8)
#define S_yaffs2Lib_FILE_EXISTS			(M_yaffs2Lib |  9)
#define S_yaffs2Lib_INVALID_PARAMETER		(M_yaffs2Lib | 10)
#define S_yaffs2Lib_NAME_TOO_LONG		(M_yaffs2Lib | 11)
#define S_yaffs2Lib_UNSUPPORTED			(M_yaffs2Lib | 12)
#define S_yaffs2Lib_VOLUME_NOT_AVAILABLE		(M_yaffs2Lib | 13)
#define S_yaffs2Lib_INVALID_NUMBER_OF_BYTES	(M_yaffs2Lib | 14)
#define S_yaffs2Lib_ILLEGAL_NAME			(M_yaffs2Lib | 15)
#define S_yaffs2Lib_CANT_DEL_ROOT		(M_yaffs2Lib | 16)
#define S_yaffs2Lib_READ_ONLY			(M_yaffs2Lib | 17)
#define S_yaffs2Lib_ROOT_DIR_FULL		(M_yaffs2Lib | 18)
#define S_yaffs2Lib_NO_LABEL			(M_yaffs2Lib | 19)
#define S_yaffs2Lib_NO_CONTIG_SPACE		(M_yaffs2Lib | 20)
#define S_yaffs2Lib_FD_OBSOLETE			(M_yaffs2Lib | 21)
#define S_yaffs2Lib_DELETED			(M_yaffs2Lib | 22)
#define S_yaffs2Lib_INTERNAL_ERROR		(M_yaffs2Lib | 23)
#define S_yaffs2Lib_WRITE_ONLY			(M_yaffs2Lib | 24)
#define S_yaffs2Lib_ILLEGAL_PATH			(M_yaffs2Lib | 25)
#define S_yaffs2Lib_ACCESS_BEYOND_EOF		(M_yaffs2Lib | 26)
#define S_yaffs2Lib_DIR_READ_ONLY		(M_yaffs2Lib | 27)
#define S_yaffs2Lib_UNKNOWN_VOLUME_FORMAT	(M_yaffs2Lib | 28)
#define S_yaffs2Lib_ILLEGAL_CLUSTER_NUMBER	(M_yaffs2Lib | 29)

#define fsize_t unsigned long


typedef struct find_diry_return{
	struct yaffs_obj* robj;
	char rstr[YAFFS_MAX_NAME_LENGTH+1];
	char rname[YAFFS_MAX_NAME_LENGTH+1];
	char rrest[YAFFS_MAX_NAME_LENGTH+1];
	char flag;
}FIND_DIRY_RETURN; 

typedef struct
 {
 long val[2];                    /* file system id type */
 } fsid_t;
struct statfs
    {
    long f_type;                    /* type of info, zero for now */
    long f_bsize;                   /* fundamental file system block size */
    long f_blocks;                  /* total blocks in file system */
    long f_bfree;                   /* free block in fs */
    long f_bavail;                  /* free blocks avail to non-superuser */
    long f_files;                   /* total file nodes in file system */
    long f_ffree;                   /* free file nodes in fs */
    fsid_t f_fsid;                  /* file system id */
    long f_spare[7];                /* spare for later */
    };


struct stat
    {
    unsigned long       st_dev;     /* Device ID number */
    unsigned long       st_ino;     /* File serial number */
    int      st_mode;    /* Mode of file */
    unsigned long     st_nlink;   /* Number of hard links to file */
    unsigned short       st_uid;     /* User ID of file */
    unsigned short       st_gid;     /* Group ID of file */
    unsigned long       st_rdev;    /* Device ID if special file */
    long long       st_size;    /* File size in bytes */
    unsigned int      st_atime;   /* Time of last access */
    unsigned int     st_mtime;   /* Time of last modification */
    unsigned int      st_ctime;   /* Time of last status change */
    long   st_blksize; /* File system block size */
    long    st_blocks;  /* Number of blocks containing file */
    unsigned char       st_attrib;  /* DOSFS only - file attributes */
    int         st_reserved1;  /* reserved for future use */
    int         st_reserved2;  /* reserved for future use */
    int         st_reserved3;  /* reserved for future use */
    int         st_reserved4;  /* reserved for future use */
    };


struct dirent		/* dirent */
    {
    ino_t	d_ino;                  /* file serial number */
    char	d_name [256]; /* file name, null-terminated */
    };

typedef struct		/* DIR */
    {
    long 	  dd_cookie;		/* filesys-specific marker within dir */
    int		  dd_fd;		/* file descriptor for open directory */
    BOOL	  dd_eof;		/* readdir EOF flag */
    struct dirent dd_dirent;		/* obtained directory entry */
    SEM_ID	  dd_lock;		/* lock for readdir_r */
    } DIR;

typedef struct		/* DIRPLUS */
    {
	long	  dd_cookie;	/* filesys-specific marker within dir */
    int		  dd_fd;		/* file descriptor for open directory */
    BOOL	  dd_eof;		/* readdir EOF flag */
    struct dirent dd_dirent;		/* obtained directory entry */
    SEM_ID	  dd_lock;		/* lock for readdir_r */
	struct stat dd_stat;
    } DIRPLUS;


/*************************************************************************/


#ifdef __cplusplus
}
#endif

#endif
