/*从direct/test-frame/yaffs_osglue.c 中来结合yaffscfg.c
 */

#include "yaffscfg.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"
#include "yaffs_trace.h"
#include <assert.h>

#include <errno.h>
#include <unistd.h>

unsigned yaffs_trace_mask =

	/*YAFFS_TRACE_SCAN |*/
/*	YAFFS_TRACE_GC |*/   /*  yaffs: GC Selected block*/
	/*YAFFS_TRACE_ERASE |*/
	YAFFS_TRACE_ERROR |
/*	YAFFS_TRACE_TRACING |*/
	/*YAFFS_TRACE_ALLOCATE |*/
	/*YAFFS_TRACE_BAD_BLOCKS |*/
/*	YAFFS_TRACE_VERIFY |*/
/*	YAFFS_TRACE_MTD|*/
	/*YAFFS_TRACE_CHECKPOINT |*/
	0;


/*
 * yaffsfs_SetError() and yaffsfs_GetError()
 * Do whatever to set the system error.
 * yaffsfs_GetError() just fetches the last error.
 */
int strnlen (const char* s, int maxlen)
{
	size_t len = 0;
	
	while ((len <= maxlen) && (*s))
	{
		s++;
		len++;
	}
	
	return len;
}

/***********************************************************************************
下面是几个简单的测试函数，可有可无
************************************************************************************/
void make_a_file(const char *yaffsName,char bval,int sizeOfFile)/*//创建文件*/
{
	int outh;
	int i;
	unsigned char buffer[100];

	outh = yaffs_open(yaffsName, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);

	memset(buffer,bval,100);

	do{
		i = sizeOfFile;
		if(i > 100) i = 100;
		sizeOfFile -= i;

		yaffs_write(outh,buffer,i);

	} while (sizeOfFile > 0);

	yaffs_close(outh);
}

int dump_file_data(char *fn)/*//读出文件*/
{
	int h;
	int i = 0;
	int ok = 1;
	unsigned char b;

	h = yaffs_open(fn, O_RDWR,0);


	printf("%s\n",fn);
	while(yaffs_read(h,&b,1)> 0)
	{
		printf("%02x",b);
		i++;
		if(i > 32)
		{
		   printf("\n");
		   i = 0;;
		 }
	}
	printf("\n");
	yaffs_close(h);
	return ok;
}

static int yaffsfs_lastError;

void yaffsfs_SetError(int err)
{
/*	//Do whatever to set error*/
	yaffsfs_lastError = err;
	errno = err;
}

int yaffsfs_GetLastError(void)
{
	return yaffsfs_lastError;
}

/*
 * yaffsfs_CheckMemRegion()
 * Check that access to an address is valid.
 * This can check memory is in bounds and is writable etc.
 *
 * Returns 0 if ok, negative if not.
 */
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	(void) size;
	(void) write_request;

	if(!addr)
		return -1;
	return 0;
}

/*
 * yaffsfs_Lock()
 * yaffsfs_Unlock()
 * A single mechanism to lock and unlock yaffs. Hook up to a mutex or whatever.
 * Here are two examples, one using POSIX pthreads, the other doing nothing.
 *
 * If we use pthreads then we also start a background gc thread.
 */

#include <pthread.h>

SEM_ID mutex1;
/*static pthread_t bc_gc_thread;*/

void yaffsfs_Lock(void)
{
/*	pthread_mutex_lock( &mutex1 );*/
	semTake(mutex1, WAIT_FOREVER);

}

void yaffsfs_Unlock(void)
{
/*	pthread_mutex_unlock( &mutex1 );*/
	semGive(mutex1);
}

static void *bg_gc_func(void *dummy)
{
	struct yaffs_dev *dev;
	int urgent = 0;
	int result;
	int next_urgent;

	(void)dummy;

	/* Sleep for a bit to allow start up */
	sleep(2);


	while (1) {
		/* Iterate through devices, do bg gc updating ungency */
		yaffs_dev_rewind();
		next_urgent = 0;

		while ((dev = yaffs_next_dev()) != NULL) {
			result = yaffs_do_background_gc_reldev(dev, urgent);
			if (result > 0)
				next_urgent = 1;
		}

		urgent = next_urgent;

		if (next_urgent)
			sleep(1);
		else
			sleep(5);
	}

	/* Don't ever return. */
	return NULL;
}

void yaffsfs_LockInit(void)
{
	mutex1= semMCreate ( SEM_Q_PRIORITY | SEM_DELETE_SAFE );
}

#else

void yaffsfs_Lock(void)
{
}

void yaffsfs_Unlock(void)
{
}

void yaffsfs_LockInit(void)
{
}

/*
 * yaffsfs_CurrentTime() retrns a 32-bit timestamp.
 *
 * Can return 0 if your system does not care about time.
 */

u32 yaffsfs_CurrentTime(void)
{
	return time(NULL);
}


/*
 * yaffsfs_malloc()
 * yaffsfs_free()
 *
 * Functions to allocate and free memory.
 */

#ifdef CONFIG_YAFFS_TEST_MALLOC

static int yaffs_kill_alloc = 0;
static size_t total_malloced = 0;
static size_t malloc_limit = 0 & 6000000;

void *yaffsfs_malloc(size_t size)
{
	void * this;
	if(yaffs_kill_alloc)
		return NULL;
	if(malloc_limit && malloc_limit <(total_malloced + size) )
		return NULL;

	this = malloc(size);
	if(this)
		total_malloced += size;
	return this;
}

#else

void *yaffsfs_malloc(size_t size)
{
	return malloc(size);
}

#endif

void yaffsfs_free(void *ptr)
{
	free(ptr);
}

void yaffsfs_OSInitialisation(void)
{
	yaffsfs_LockInit();
}

/*
 * yaffs_bug_fn()
 * Function to report a bug.
 */

void yaffs_bug_fn(const char *file_name, int line_no)
{
	printf("yaffs bug detected %s:%d\n",
		file_name, line_no);
/*	assert(0);*/
}

static int yflash2_Initialise(struct yaffs_dev *dev)
{
	(void) dev;
#if 0
	CheckInit();
#endif
	return YAFFS_OK;
}

