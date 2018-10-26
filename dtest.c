/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2011 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>

#include "yaffsfs.h"

#include "yaffs_guts.h" /* Only for dumping device innards */

extern int yaffs_trace_mask;

void dumpDir(const char *dname);

void dump_directory_tree_worker(const char *dname,int recursive)
{
	yaffs_DIR *d;
	struct yaffs_dirent *de;
	struct yaffs_stat s;
	char str[1000];

	d = yaffs_opendir(dname);

	if(!d)
	{
		printf("opendir failed\n");
	}
	else
	{
		while((de = yaffs_readdir(d)) != NULL)
		{
			sprintf(str,"%s/%s",dname,de->d_name);

			yaffs_lstat(str,&s);

			printf("%s inode %d length %d mode %X ",
				str, s.st_ino, (int)s.st_size, s.st_mode);
			switch(s.st_mode & S_IFMT)
			{
				case S_IFREG: printf("data file"); break;
				case S_IFDIR: printf("directory"); break;
				case S_IFLNK: printf("symlink -->");
							  if(yaffs_readlink(str,str,100) < 0)
								printf("no alias");
							  else
								printf("\"%s\"",str);
							  break;
				default: printf("unknown"); break;
			}

			printf("\n");

			if((s.st_mode & S_IFMT) == S_IFDIR && recursive)
				dump_directory_tree_worker(str,1);

		}

		yaffs_closedir(d);
	}

}


void dumpDir(const char *dname)                                                                 /*Y*/
{	dump_directory_tree_worker(dname,0);
	printf("\n");
	printf("Free space in %s is %d\n\n",dname,(int)yaffs_freespace(dname));
}

void make_a_file1(const char *yaffsName,char bval,int sizeOfFile)
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
void make_pattern_file(char *fn,int size)   /**//**/
{
	int outh;
	int marker;
	int i;
	outh = yaffs_open(fn, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
	yaffs_lseek(outh,size-1,SEEK_SET);
	yaffs_write(outh,"A",1);

	for(i = 0; i < size; i+=256)
	{
		marker = ~i;
		yaffs_lseek(outh,i,SEEK_SET);
		yaffs_write(outh,&marker,sizeof(marker));
	}
	yaffs_close(outh);

}
int check_pattern_file(char *fn)
{
	int h;
	int marker;
	int i;
	int size;
	int ok = 1;

	h = yaffs_open(fn, O_RDWR,0);
	size = yaffs_lseek(h,0,SEEK_END);

	for(i = 0; i < size; i+=256)
	{
		yaffs_lseek(h,i,SEEK_SET);
		yaffs_read(h,&marker,sizeof(marker));
		ok = (marker == ~i);
		if(!ok)
		{
		   printf("pattern check failed on file %s, size %d at position %d. Got %x instead of %x\n",
					fn,size,i,marker,~i);
		}
	}
	yaffs_close(h);
	return ok;
}
int dump_file_data1(char *fn)
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



void dump_file(const char *fn)
{
	int i;
	int size;
	int h;

	h = yaffs_open(fn,O_RDONLY,0);
	if(h < 0)
	{
		printf("*****\nDump file %s does not exist\n",fn);
	}
	else
	{
		size = yaffs_lseek(h,0,SEEK_SET);
		printf("*****\nDump file %s size %d\n",fn,size);
		for(i = 0; i < size; i++)
		{

		}
	}
}
void create_file_of_size(const char *fn,int syze)
{
	int h;
	int n;
	int result;
	int iteration = 0;
	char xx[200];

	h = yaffs_open(fn, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);

	while (syze > 0)
	{
		sprintf(xx,"%s %8d",fn,iteration);
		n = strlen(xx);
		result = yaffs_write(h,xx,n);
		if(result != n){
			printf("Wrote %d, should have been %d. syze is %d\n",result,n,syze);
			syze = 0;
		} else
			syze-=n;
		iteration++;
	}
	yaffs_close (h);
}

void verify_file_of_size(const char *fn,int syze)
{
	int h;
	int result;

	char xx[200];
	char yy[200];
	int l;

	int iterations = (syze + strlen(fn) -1)/ strlen(fn);

	h = yaffs_open(fn, O_RDONLY, S_IREAD | S_IWRITE);

	while (iterations > 0)
	{
		sprintf(xx,"%s %8d",fn,iterations);
		l = strlen(xx);

		result = yaffs_read(h,yy,l);
		if (result)
			printf("result in line %d is %d", __LINE__, result);
		yy[l] = 0;

		if(strcmp(xx,yy)){
			printf("=====>>>>> verification of file %s failed near position %lld\n",fn,(long long)yaffs_lseek(h,0,SEEK_CUR));
		}
		iterations--;
	}
	yaffs_close (h);
}

void create_resized_file_of_size(const char *fn,int syze1,int reSyze, int syze2)
{
	int h;

	int iterations;

	h = yaffs_open(fn, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);

	iterations = (syze1 + strlen(fn) -1)/ strlen(fn);
	while (iterations > 0)
	{
		yaffs_write(h,fn,strlen(fn));
		iterations--;
	}

	yaffs_ftruncate(h,reSyze);

	yaffs_lseek(h,0,SEEK_SET);
	iterations = (syze2 + strlen(fn) -1)/ strlen(fn);
	while (iterations > 0)
	{
		yaffs_write(h,fn,strlen(fn));
		iterations--;
	}

	yaffs_close (h);
}


void do_some_file_stuff(const char *path)
{

	char fn[100];

	sprintf(fn,"%s/%s",path,"f1");
	create_file_of_size(fn,10000);

	sprintf(fn,"%s/%s",path,"fdel");
	create_file_of_size(fn,10000);
	yaffs_unlink(fn);

	sprintf(fn,"%s/%s",path,"f2");

	create_resized_file_of_size(fn,10000,3000,4000);
}



char xxzz[2000];

void yaffs_device_flush_test(const char *path)
{
	char fn[100];
	int h;
	int i;

	yaffs_start_up("yaffs");

	yaffs_mount(path);

	do_some_file_stuff(path);

/*	// Open and add some data to a few files*/
	for(i = 0; i < 10; i++) {

		sprintf(fn,"%s/ff%d",path,i);

		h = yaffs_open(fn, O_CREAT | O_RDWR | O_TRUNC, S_IWRITE | S_IREAD);
		yaffs_write(h,xxzz,2000);
		yaffs_write(h,xxzz,2000);
	}
	yaffs_unmount(path);

	yaffs_mount(path);
}



void short_scan_test(const char *path, int fsize, int niterations)
{
	int i;
	char fn[100];

	sprintf(fn,"%s/%s",path,"f1");

	yaffs_start_up("yaffs");
	for(i = 0; i < niterations; i++)
	{
		printf("\n*****************\nIteration %d\n",i);
		yaffs_mount(path);
		printf("\nmount: Directory look-up of %s\n",path);
		dumpDir(path);
		make_a_file1(fn,1,fsize);
		yaffs_unmount(path);
	}
}

