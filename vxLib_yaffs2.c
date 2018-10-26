#include "vxWorks.h"
#include "yaffs_guts.h"
#include "vxLib_yaffs2.h"
#include "yaffsfs.h"

int yaffsDrvNum = ERROR;

#define TTRACE TTRACE1
#define TTRACE1 printf("%d:%s called\n",__LINE__, __func__);
#define TTRACE2 logMsg("%d:%s called\n",__LINE__, __func__,0,0,0,0);
/*#define ERRORTRACE logMsg("error %d:%s called\n",__LINE__, __func__,0,0,0,0);*/
#define ERRORTRACE(str,args...) printf(str,##args);

#define YAFFSDEBUGFLAG 0
#if YAFFSDEBUGFLAG
    #define YAFFSPRINT(str,args...) printf(str,##args)
#else
    #define YAFFSPRINT(str,args...) ((void)0)
#endif


/*-----------------------------------------------------------------------------------------------*/
LOCAL YAFFS_FILE_DESC_ID yaffsFdGet(YAFFS_VOLUME_DESC_ID pVolDesc)
{
	int i;
	semTake(pVolDesc->devSem, WAIT_FOREVER);
	for(i=0;i<pVolDesc->maxFiles;i++)
	{
		if(!pVolDesc->pFdList[i].inUse)
		{
			memset(&pVolDesc->pFdList[i],0,sizeof(pVolDesc->pFdList[i]));
			pVolDesc->pFdList[i].inUse=1;
			pVolDesc->pFdList[i].pVolDesc=pVolDesc;
			semGive(pVolDesc->devSem);
			return &(pVolDesc->pFdList[i]);
		}
	}
	semGive(pVolDesc->devSem);
	ERRORTRACE("error Fd get!\n")
	return NULL;
}

/* yaffsFdFree - Free a file descriptor
 *
 * This routine marks a file descriptor as free and decreases
 * reference count of a referenced file handle.
 *
 * RETURNS: N/A.
 */

LOCAL void yaffsFdFree(YAFFS_FILE_DESC_ID pFd)
{
	YAFFS_VOLUME_DESC_ID pVolDesc = pFd->pVolDesc;
	/*assert(pFd!=NULL);*/
	if(pFd!=NULL)
	{
		semTake(pVolDesc->devSem, WAIT_FOREVER);
		pFd->inUse=0;
		pFd->obj=NULL;
		pFd->ydir = NULL;
		pFd->yfd = 0;
		memset(pFd->path,0,YAFFS_MAX_NAME_LENGTH+1);
		semGive(pVolDesc->devSem);
	}
}

LOCAL struct yaffs_obj * yaffsFindDirectory(YAFFS_VOLUME_DESC_ID pVolDesc, 
	                struct yaffs_obj *startDir, const YCHAR *path,YCHAR **name,FIND_DIRY_RETURN * ret)
{
	struct yaffs_obj *dir=NULL;
	struct yaffs_obj* obj=NULL;
	YCHAR *restOfPath;
	YCHAR str[YAFFS_MAX_NAME_LENGTH+1];
	YCHAR getpath[YAFFS_MAX_NAME_LENGTH+1]={0};

	strncpy(getpath,path,strlen(path));
	int i;
	
	if(startDir)
	{
		dir = startDir;
		restOfPath = (YCHAR *)getpath;
	}
	else
	{
		dir = pVolDesc->pYaffsDev->root_dir;
		restOfPath = (YCHAR *)getpath;
	}
	while(dir)
	{
		while(*restOfPath && strchr(YAFFS_PATH_DIVIDERS, *restOfPath))                    
		{
			restOfPath++; 
		}

		*name = restOfPath;
		i = 0;
												                                                                 

		while(*restOfPath && !strchr(YAFFS_PATH_DIVIDERS, *restOfPath))                                                  
		{
			if (i < YAFFS_MAX_NAME_LENGTH)
			{
				str[i] = *restOfPath;
				str[i+1] = '\0';
				i++;
			}
			restOfPath++;
		}									                                                                     
		if(!*restOfPath)
		{
			ret->robj = dir;/*最后找到的Obj*/
			memset(ret->rstr,0,YAFFS_MAX_NAME_LENGTH+1);
			memset(ret->rname,0,YAFFS_MAX_NAME_LENGTH+1);
			memset(ret->rrest,0,YAFFS_MAX_NAME_LENGTH+1);
			memcpy(ret->rstr,str,strlen(str));
			memcpy(ret->rname,*name,strlen(*name));
			strncpy(ret->rrest,restOfPath,strlen(restOfPath));
			ret->flag = 1;
/*			printf("rstr:%s-rname:%s-rrest:%s-flag:%d\n",ret->rstr,ret->rname,ret->rrest,ret->flag);*/
			return dir;
		}
		else
		{
			if(yaffs_strcmp(str,_Y(".")) == 0)
			{

			}
			else if(yaffs_strcmp(str,_Y("..")) == 0)
			{
				dir = dir->parent;
			}
			else
			{
                                obj = dir; 
				dir = yaffs_find_by_name(dir,str);
				if(dir && dir->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY)
				{
					dir = NULL;
				}
				if(!dir)
				{
					ret->robj = obj;/*最后找到的Obj*/
					memset(ret->rstr,0,YAFFS_MAX_NAME_LENGTH+1);
					memset(ret->rname,0,YAFFS_MAX_NAME_LENGTH+1);
					memset(ret->rrest,0,YAFFS_MAX_NAME_LENGTH+1);
					memcpy(ret->rstr,str,strlen(str));
					memcpy(ret->rname,*name,strlen(*name));
					strncpy(ret->rrest,restOfPath,strlen(restOfPath));
					ret->flag = 2;
/*printf("rstr:%s-rname:%s-rrest:%s-flag:%d\n",ret->rstr,ret->rname,ret->rrest,ret->flag);*/
					return obj;
				}
			}
		}
	}
	return NULL;
}

LOCAL struct yaffs_obj * yaffsFindObject(YAFFS_VOLUME_DESC_ID pVolDesc,struct yaffs_obj *relativeDirectory,
	   	                                                                          const YCHAR *path)
{
	struct yaffs_obj *dir = NULL;
	struct yaffs_obj *obj = NULL;
	YCHAR *name;
	YCHAR str[YAFFS_MAX_NAME_LENGTH+1];

		dir=yaffsFindDirectory(pVolDesc, relativeDirectory, path, &name,NULL);
/*		printf("%s - path:%s - name:%s - %d   %d\n",__FUNCTION__,path,name,dir,dir->variant_type);*/
		if (dir && (yaffs_strcmp(name, _Y("..")) == 0)) {
			dir = dir->parent;
			obj = dir;
		} else if (dir && (yaffs_strcmp(name, _Y(".")) == 0))
			{obj = dir;}
		else if (dir && *name)				/*xxx/io/test 或xxx/io/test.txt   dir为io,name为test或test.txt*/
			{
			strncpy(str,name,strlen(name));
			str[strlen(name)] = '\0';
			obj= yaffs_find_by_name(dir, str);
			/*printf("%d\n",obj);*/
		}
		else
	       obj= dir;                                   /*name为空,如/yaffs/hels/hejd/io/   dir为io,name 为空*/
		return obj;

}


LOCAL struct yaffs_obj * yaffsSetupDirectory(YAFFS_VOLUME_DESC_ID pVolDesc, 
	                                                    const YCHAR *path,int mode, struct yaffs_obj *getDir1,int cate,FIND_DIRY_RETURN *ret)
{
	struct yaffs_obj*parent=NULL;
	struct yaffs_obj *dir=NULL;
	YCHAR *name;
	struct yaffs_obj *getDir=NULL;

	dir = yaffsFindDirectory(pVolDesc,NULL,path,&name,ret);
	while(strlen(ret->rrest)>0)
	{	
		dir = ret->robj;
		if(ret->robj && strlen(ret->rstr)>0)/*在path 中找到了最近的一级obj，restpath不为空也就是后面还有path*/
		{
			parent=yaffs_create_dir(dir,ret->rstr,mode|YAFFS_DFLT_DIR_MASK,0,0);/*根据找到的obj,obj下一级目录str,建立str 的obj*/
		}
		if(!parent)
		{
			printf("make diry:%s error!\n ",ret->rstr);
			getDir = NULL;
			return NULL;
		}
		dir = yaffsFindDirectory(pVolDesc,parent,ret->rrest,&name,ret);  /*以上一次建立的str的obj为基obj,restoppath 为path 重复*/
	}
	
	/*建立最后一个目录/文件*/	
	if(cate == 1)
	{	
		if(ret->robj && strlen(ret->rstr)>0)/*在path 中找到了最近的一级obj，restpath不为空也就是后面还有path*/
		{
			parent=yaffs_create_dir(ret->robj,ret->rstr,mode|YAFFS_DFLT_DIR_MASK,0,0);/*根据找到的obj,obj下一级目录str,建立str 的obj*/
		}
		if(!parent)
		{
			printf("make last diry:%s error!\n ",ret->rstr);
			getDir = NULL;
			return NULL;
		}
		getDir = parent;
	}
	if(cate == 2)
	{
		if(ret->robj && strlen(ret->rstr)>0)/*在path 中找到了最近的一级obj，restpath不为空也就是后面还有path*/
		{
			/*根据找到的obj,obj下一级名称str,建立str 的obj*/
			parent=yaffs_create_file(ret->robj,ret->rstr,mode|YAFFS_DFLT_FILE_MASK,0,0);
		}
		if(!parent)
		{
			printf("make last diry:%s error!\n ",ret->rstr);
			getDir = NULL;
			return NULL;
		}
		getDir = parent;
	}
}


LOCAL YAFFS_FILE_DESC_ID yaffsOpen(YAFFS_VOLUME_DESC_ID	pVolDesc,char	*pPath,int flags,int mode)
{
/*	TTRACE;*/
	YAFFSPRINT("\n==>>%s - %s - %x - %d - %d\n\n",__FUNCTION__,pPath,flags,mode,strlen(pPath));

	YAFFS_FILE_DESC_ID pFd=NULL;
	struct yaffs_obj*obj = NULL;
	BOOL devSemOwned;
	BOOL error=FALSE;
	BOOL alreadyOpen=0;
	BOOL alreadyExclusive=0;
	BOOL openDenied= 0;

	YCHAR relpath[YAFFS_MAX_NAME_LENGTH+1];
	int fd; 
	int  readRequested;
	int  writeRequested;
	int i;
	int rwflags = flags & (O_RDWR | O_RDONLY | O_WRONLY);    /*201*/

	struct find_diry_return retv;

if(strcmp(pPath,"./")==0)
	sprintf(relpath,"%s%s","/hd/",pPath);
else
	sprintf(relpath,"%s%s","/hd",pPath);


if(pVolDesc == NULL || pVolDesc->magic != YAFFS_MAGIC){
		TTRACE;
		return NULL;
	}
	if(pPath == NULL || strlen(pPath) > PATH_MAX){
		TTRACE;
		return NULL;
	}

	if(0 == pVolDesc->pYaffsDev->is_mounted){                         /*没挂载尝试挂载*/
		if(yaffsVolMount(pVolDesc,pVolDesc->diskname)== -1)
			goto ret;
	}

	/* only regular file and directory creation are supported */
	pFd=yaffsFdGet(pVolDesc);
	if(pFd==NULL){
		ERRORTRACE("error Fd get!\n");
		goto ret;
	}

	semTake(pVolDesc->devSem, WAIT_FOREVER);
	devSemOwned = TRUE;

	obj=yaffsFindObject(pVolDesc, NULL, pPath);/*查找最后一级的obj*/

	if(obj)/*the path located dir/file is build before,than get the path's obj.put it into the */
	{
			if(obj->variant_type == YAFFS_OBJECT_TYPE_DIRECTORY)
			{/*目录*/
				if ( rwflags != O_RDONLY) 
				{
				/*printf("get 1 %d\n",rwflags);*/
					openDenied = 1;
					/*yaffsfs_SetError(-EISDIR);*/
					error=TRUE;
				}
				else
				{
					pFd->yfd = -1;
					pFd->ydir = yaffs_opendir(relpath);
					if(pFd->ydir==NULL)
					{
						printf("ERROR1:open exist dir error.\n");
						error=TRUE;
					}
					pFd->obj = obj;
					alreadyOpen=TRUE;
				}
			}
			else if(obj && obj->variant_type == YAFFS_OBJECT_TYPE_FILE)
			{
				for(i=0;i<pVolDesc->maxFiles;i++)
				{      
					if(pFd!=&pVolDesc->pFdList[i] && pVolDesc->pFdList[i].inUse && obj==pVolDesc->pFdList[i].obj)
					{
						alreadyOpen = TRUE;
						if(pVolDesc->pFdList[i].exclusive) alreadyExclusive = TRUE;
					}
				}
				if(((flags&O_EXCL)&&alreadyOpen)||alreadyExclusive) openDenied = TRUE;
	                     
				if((flags&O_EXCL) && (flags&O_CREAT))
				{
					printf("open denied for flag have both o_excl/o_creat!\n");
					openDenied = TRUE;
				}
			}
			else
			{
				printf("DEBUG:the obj found in yaffs is not support-type=%d\n",obj->variant_type);
			}
	}
	else/*not find the obj,that means the path/file not build before,if flag is O_CREAT.than build the path/file*/
	{
		if(flags & O_CREAT)
		{
			if(mode&FSTAT_DIR){
				obj = yaffsSetupDirectory(pVolDesc, pPath,mode,NULL,1,&retv);/*根据path建立目录*/
				if(obj==NULL){printf("%d create diry error!\n",__LINE__);obj=NULL;error=TRUE;}
			}
			else{
				obj  = yaffsSetupDirectory(pVolDesc,  pPath,mode,NULL,2,&retv);/*根据path 建立中间的目录obj以及最后的文件*/
				if(obj==NULL){printf("%d create file error!\n",__LINE__);obj=NULL;error=TRUE;}
			}
		}
		else
		{	
			error=TRUE;
		}
	}

	if(obj && !openDenied && !error )
	{
		if(obj->variant_type == YAFFS_OBJECT_TYPE_FILE)
		{	
			fd  = 0;
			fd = yaffs_open(relpath,flags,mode);
			if(fd>=0){pFd->yfd = fd;/*printf("yaffs_open %s-%d-%d-%d!\n",relpath,flags,mode,fd);*/}
			else {
				printf("yaffs_open %s-%d-%d-%d error!\n",relpath,flags,mode,fd);
				error =TRUE;
			}
		}
		if(obj->variant_type == YAFFS_OBJECT_TYPE_DIRECTORY && !alreadyOpen)
		{	
			pFd->yfd = -1;
			pFd->ydir = yaffs_opendir(relpath);
			if(pFd->ydir==NULL)
			{
				printf("ERROR2:open exist dir error.\n");
				error=TRUE;
			}	
		}
		memset(pFd->path,0,YAFFS_MAX_NAME_LENGTH+1);
		memcpy(pFd->path,relpath,strlen(relpath));
		memset(pFd->oripath,0,YAFFS_MAX_NAME_LENGTH+1);
		memcpy(pFd->oripath,pPath,strlen(pPath));
		pFd->obj=obj;
		pFd->inUse=1;
		if(flags&(O_WRONLY|O_RDWR))
			pFd->readOnly=0;
		else
			pFd->readOnly=1;

		if(flags&O_APPEND)
			pFd->append==1;
		else
			pFd->append==0;
		
		if(flags&O_EXCL)
			pFd->exclusive=1;
		else
			pFd->exclusive=0;
		pFd->position=0;
		if((flags&O_TRUNC)&& !pFd->readOnly) yaffs_resize_file(obj, 0);
	}
ret:
	if(devSemOwned)
	{
		semGive(pVolDesc->devSem);
	}
	if(error||openDenied)
	{
		if(pFd!=NULL) yaffsFdFree(pFd);
		return (void *)ERROR;
	}
	return pFd;
}

/*
 * yaffsDelete - delete a yaffs file/directory
 */
LOCAL STATUS yaffsDelete(YAFFS_VOLUME_DESC_ID	pVolDesc,char	*pPath)
{
/*	YAFFSPRINT("\n==>>%s - %s    \n\n",__FUNCTION__,pPath);*/

	int result = -1;
	struct yaffs_obj *dir=NULL;
	struct yaffs_obj *obj=NULL;
	char *name;
	YCHAR relpath[YAFFS_MAX_NAME_LENGTH+1];
	YAFFS_FILE_DESC_ID pFd;

	sprintf(relpath,"%s%s","/hd",pPath);

	semTake(pVolDesc->devSem, WAIT_FOREVER);
	
	obj = yaffsFindObject(pVolDesc, NULL, pPath);
	dir = yaffsFindDirectory(pVolDesc, NULL, pPath, &name,NULL);
	if(!dir){
		ERRORTRACE("ERROR-%s:path is not value!\n",__FUNCTION__);
	}else if(!obj){
		ERRORTRACE("ERROR-%s:path  obj not found!\n",__FUNCTION__);
	}else{
		if(obj->variant_type==YAFFS_OBJECT_TYPE_FILE)
			result = yaffs_unlink(relpath);
		if(obj->variant_type==YAFFS_OBJECT_TYPE_DIRECTORY)
			result = yaffs_rmdir(relpath);
		if(result != 0 && obj->variant_type == YAFFS_OBJECT_TYPE_DIRECTORY){
			/*errnoSet(S_yaffs2Lib_DIR_NOT_EMPTY);*/
			ERRORTRACE("ERROR-%s:Removing DIR_NOT_EMPTY!\n",__FUNCTION__);
		}
	}

	semGive(pVolDesc->devSem);
	if(result == 0)
		return OK;
	else
		return ERROR;
}

/* unmount a yaffs volume */
STATUS yaffsVolUnmount(YAFFS_VOLUME_DESC_ID pVolDesc){
	struct yaffs_dev* dev=NULL;
	semTake(pVolDesc->devSem, WAIT_FOREVER);
	dev=pVolDesc->pYaffsDev;
	if(dev->is_mounted){
		yaffs_unmount(pVolDesc->diskname);
	}	
	semGive(pVolDesc->devSem);
	return OK;
}


/*
 * yaffsClose - close a yaffs file
 *
 * This routine closes the specified yaffs file.
 * If file reference is smaller than zero and marked unlinked, the file will be deleted
 *
 * RETURNS: OK, or ERROR if directory couldn't be flushed
 * or entry couldn't be found.
 *
 * ERRNO:
 * S_yaffs2Lib_INVALID_PARAMETER
 * S_yaffs2Lib_DELETED
 * S_yaffs2Lib_FD_OBSOLETE
 */
LOCAL STATUS yaffsClose(YAFFS_FILE_DESC_ID pFd)
{
/*TTRACE;*/
	YAFFSPRINT("\n==>>%s - fd:%d - type:%d - path:%s\n\n",__FUNCTION__,pFd->yfd,pFd->obj->variant_type,pFd->path);
	int r = -1;
	YAFFS_VOLUME_DESC_ID pVolDesc;
	if(pFd==NULL||pFd==(void *)ERROR||!pFd->inUse||pFd->pVolDesc->magic!=YAFFS_MAGIC){
		assert(FALSE);
		/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}

	assert(pFd-pFd->pVolDesc->pFdList<pFd->pVolDesc->maxFiles);
	pVolDesc=pFd->pVolDesc;

	semTake(pVolDesc->devSem, WAIT_FOREVER);

	if((pFd->yfd)>=0 && (pFd->obj->variant_type == YAFFS_OBJECT_TYPE_FILE))
	{	
		r = yaffs_close(pFd->yfd);
		if(r!=0)
		{
			ERRORTRACE("ERROR1-%s:yaffs_close error!\n",__FUNCTION__);
			semGive(pVolDesc->devSem);
			return ERROR;
		}
	}
	if( (pFd->yfd)<0 && (pFd->obj->variant_type == YAFFS_OBJECT_TYPE_DIRECTORY))
	{
		r = yaffs_closedir(pFd->ydir);
		if(r!=0)
		{
			ERRORTRACE("ERROR2-%s:yaffs_closedir error!\n",__FUNCTION__);
			semGive(pVolDesc->devSem);
			return ERROR;
		}
	}
	/* free the file descriptor */
	yaffsFdFree(pFd);
	semGive(pVolDesc->devSem);
	return OK;
}

/* yaffsCreate - create a yaffs file
 *
 * this routine creats a file with the specific name and opens it
 * with specified flags.
 * If the file already exists, it is truncated to zero length, but not
 * actually created.
 * A yaffs file descriptor is initialized for the file.
 *
 * RETURNS: Pointer to yaffs file descriptor
 * or ERROR if error in create.
 */

LOCAL YAFFS_FILE_DESC_ID yaffsCreate(YAFFS_VOLUME_DESC_ID	pVolDesc,char	*pName,int	flags)
{
/*TTRACE;*/
/*	YAFFSPRINT("\n==>>%s - %s -F: %x\n\n",__FUNCTION__,pName,flags);*/

	/* create file via yaffsOpen */
	return yaffsOpen(pVolDesc, pName, flags|O_CREAT|O_TRUNC, S_IFREG);

	/* TODO: maybe update date info here. see dosFsLib.c */
}

/*******************************************************************************
 *
 * yaffsRead - read a file.
 *
 * This routine reads the requested number of bytes from a file.
 *
 * RETURNS: number of bytes successfully read, or 0 if EOF,
 * or ERROR if file is unreadable.
 */


LOCAL int yaffsRead(YAFFS_FILE_DESC_ID pFd, char *pBuf, int maxBytes)
{
/*TTRACE;*/
	/*YAFFSPRINT("\n==>>%s - FD:%d - MB: %d\n\n",__FUNCTION__,pFd,maxBytes);*/

	struct yaffs_obj *obj=NULL;
	YAFFS_VOLUME_DESC_ID pVolDesc;
	int pos=0;
	int nRead = -1;
	unsigned int maxRead;

	if(pFd==NULL){
	   	/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR1-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}
	pVolDesc=pFd->pVolDesc;
	obj=pFd->obj;
	if(pVolDesc==NULL||obj==NULL){
		/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR2-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}

	semTake(pVolDesc->devSem, WAIT_FOREVER);
	/* Get max readable bytes */
	pos=pFd->position;
	if(yaffs_get_obj_length(obj) > pos){
		maxRead=yaffs_get_obj_length(obj)-pos;
	}
	else{
		maxRead = 0;
	}	

	/* sorry we have only maxRead bytes */
	if(maxBytes>maxRead){
		maxBytes=maxRead;
	}

	if(maxBytes>0){
		nRead=yaffs_file_rd(obj,pBuf,pos,maxBytes);
		if(nRead>=0){
			pFd->position = pos+nRead;
		}else{
			/*YBUG();*/
			ERRORTRACE("ERROR-%s:yaffs_file_rd error!\n",__FUNCTION__);
		}
	}else{
		nRead=0;
	}
	semGive(pVolDesc->devSem);
	
	if(nRead>=0)
		return nRead;
	else
		return ERROR;
	/*return (nRead>=0?nRead:ERROR);*/
}

/*
 * yaffsWrite - write a file.
 *
 * This routine writes the requested number of bytes to a file.
 *
 * RETURNS: number of bytes successfully written,
 * or ERROR if file is unwritable.
 */

LOCAL int yaffsWrite(YAFFS_FILE_DESC_ID pFd, char *pBuf, int maxBytes){
/*	TTRACE;*/
/*	YAFFSPRINT("\n==>>%s - %d - %d-type:%d-path:%s\n\n",__FUNCTION__,pFd,maxBytes,pFd->obj->variant_type,pFd->path);*/
	int i;
	struct yaffs_obj *obj=NULL;
	YAFFS_VOLUME_DESC_ID pVolDesc;
	int pos=0;
	int nWritten = -1;
	unsigned int writeThrough=0;

	if(pFd==NULL){
	   	/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR1-%s:INVALID_PARAMETER!\n",__FUNCTION__);		
		return ERROR;
	}
	
	pVolDesc=pFd->pVolDesc;
	obj=pFd->obj;
	if(pVolDesc==NULL||obj==NULL){
/*		errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR2-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}
	semTake(pVolDesc->devSem, WAIT_FOREVER);
	if(pFd->append){
		pos=yaffs_get_obj_length(obj);
	}else{
		pos=pFd->position;
	}
	nWritten=yaffs_do_file_wr(obj,pBuf,pos,maxBytes,writeThrough);
	/*nWritten=yaffs_write(pFd->yfd,pBuf,maxBytes);
	    nWritten = yaffs_pwrite(pFd->yfd,pBuf,maxBytes,pos);
	*/
	if(nWritten>=0){
		pFd->position = pos+nWritten;
	}else{
	ERRORTRACE("ERROR-%s:yaffs_do_file_wr error!\n",__FUNCTION__);
	}
	/*printf("\n==>>%s - %d - %d-type:%d-path:%s  %d\n\n",__FUNCTION__,pFd,maxBytes,pFd->obj->variant_type,pFd->path,nWritten);*/
	semGive(pVolDesc->devSem);

	if(nWritten>=0)
		return nWritten;
	else
		return ERROR;
/*	return(nWritten>=0)?nWritten:ERROR;*/
}

/*******************************************************************************
*
* yaffsRename - change name of yaffs file
*
* This routine changes the name of the specified file to the specified
* new name.
*
* RETURNS: OK, or ERROR if pNewPath already in use, 
* or unable to write out new directory info.
*
* ERRNO:
* S_yaffs2Lib_INVALID_PARAMETER
* S_yaffs2Lib_NOT_SAME_VOLUME
*/

LOCAL STATUS yaffsRename(YAFFS_FILE_DESC_ID pFd, char *pNewName){
/*	TTRACE;*/
/*	YAFFSPRINT("\n==>>%s -FD:%d - NewName:%s \n\n",__FUNCTION__,pFd,pNewName );*/
	struct yaffs_obj *olddir,*newdir;
	char oldname[YAFFS_MAX_NAME_LENGTH+1];
	char newname[YAFFS_MAX_NAME_LENGTH+1];
	STATUS result=-1;
	if(pFd==NULL ||pFd->path==NULL ||pNewName==NULL){
/*		errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
                ERRORTRACE("ERROR-%s:INVALID_PARAMETER!\n",__FUNCTION__);	
		return ERROR;
	}
	sprintf(newname,"%s%s","/hd/",pNewName);
	result = yaffs_rename(pFd->path,newname);
/*	printf("yaffsrename %s,%s,%d\n",pFd->path,newname,result);*/
	return result;
}

/***************************************************************************
 *
 * yaffsSeek - change file's current character position
 *
 * This routine sets the specified file's current character position to
 * the specified position.  This only changes the pointer, doesn't affect
 * the hardware at all.
 *
 * If the new offset pasts the end-of-file (EOF), attempts to read data
 * at this location will fail (return 0 bytes).
 *
 * RETURNS: OK, or ERROR if invalid file position.
 *
 * ERRNO:
 * S_yaffsLib_NOT_FILE
 *
 */

LOCAL STATUS yaffsSeek(YAFFS_FILE_DESC_ID pFd, fsize_t newPos){
	/*YAFFSPRINT("\n==>>%s -FD:%d - newPos:%s \n\n",__FUNCTION__,pFd->yfd,newPos);*/
	struct yaffs_obj *obj;
	if(newPos>=0){
		pFd->position=newPos;
		if(pFd->yfd>=0)
		{
			yaffs_lseek(pFd->yfd,newPos,SEEK_SET);	
		}	
	}else{
		return ERROR;
	}
	return OK;
}

/*******************************************************************************
 *
 * dosFsStatGet - get file status (directory entry data)
 *
 * This routine is called via an ioctl() call, using the FIOFSTATGET
 * function code.  The passed stat structure is filled, using data
 * obtained from the directory entry which describes the file.
 *
 * RETURNS: OK or ERROR if disk access error.
 *
 */
LOCAL STATUS yaffsStatGet(YAFFS_FILE_DESC_ID pFd, struct stat *pStat){
	struct yaffs_obj *obj;
	struct yaffs_stat s;
	if(pFd==NULL||pFd->obj==NULL ||pFd->path==NULL | pStat==NULL ){
		/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}
	bzero ((char *) pStat, sizeof (struct stat));
	yaffs_lstat(pFd->path,&s);
	
	pStat->st_dev=(unsigned long)s.st_dev;
	pStat->st_nlink=(unsigned long)s.st_nlink;                                              /*2*/
	pStat->st_size=s.st_size;                                                                      /*5*/
	pStat->st_blksize =0;/* (long)s.st_blksize;*/
	pStat->st_blocks=s.st_size;/*(long)s.st_blocks;*/
/*	pStat->st_ino=(unsigned long)s.st_ino;*/
	pStat->st_mode=pFd->obj->yst_mode & ~S_IFMT;/*(int)(s.st_mode& S_IFMT);*/                                        /*1*/
	if(pFd->obj->variant_type==YAFFS_OBJECT_TYPE_DIRECTORY){
			pStat->st_mode |= S_IFDIR;
		}else if(pFd->obj->variant_type==YAFFS_OBJECT_TYPE_SYMLINK){
			pStat->st_mode |= S_IFLNK;
		}else if(pFd->obj->variant_type==YAFFS_OBJECT_TYPE_FILE){
			pStat->st_mode |= S_IFREG;
		}
	pStat->st_uid=(unsigned short)s.st_uid;                                                  /*3 4*/
	pStat->st_gid=(unsigned short)s.st_gid;
/*	pStat->st_rdev = (unsigned long)s.st_rdev;*/
	pStat->st_atime=(unsigned int)s.yst_atime;
	pStat->st_ctime=(unsigned int)s.yst_ctime;
	pStat->st_mtime=315532800;
#if 0
	printf("\n%s\nst_dev:%d\nst_nlink:%d\nst_size:%d\nst_blksize:%d\nst_blocks:%d\nst_ino:%d\nst_mode:%x\n"
		"st_uid:%d\nst_gid:%d\nst_rdev:%d\nst_atime:%d\nst_ctime:%d\nst_mtime:%d\n\n",pFd->path,pStat->st_dev,pStat->st_nlink,pStat->st_size,pStat->st_blksize,pStat->st_blocks,
		pStat->st_ino,pStat->st_mode,pStat->st_uid,pStat->st_gid,pStat->st_rdev,pStat->st_atime,
		pStat->st_ctime,pStat->st_mtime);
#endif
	return OK;
}

/******************************************************************************
 *
 * yaffsFSStatGet - get statistics about entire file system
 *
 * Used by fstatfs() and statfs() to get information about a file
 * system.  Called through yaffsIoctl() with FIOFSTATFSGET code.
 *
 * RETURNS:  OK.
 */
LOCAL STATUS yaffsFsStatGet(YAFFS_FILE_DESC_ID pFd, struct statfs *pStat){
	struct yaffs_dev *dev;

	if(pFd==NULL||pFd->obj==NULL||pFd->obj->my_dev==NULL||pStat==NULL){
		/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}
	dev=pFd->obj->my_dev;
	pStat->f_type=0;
	pStat->f_bsize=4096;
	pStat->f_blocks=8192*64;
	pStat->f_bfree=yaffs_count_free_chunks(dev);
	pStat->f_bavail=pStat->f_bfree;
	pStat->f_files=-1;	/* not supported */
	pStat->f_ffree=-1;

	return OK;
}

/*******************************************************************************
 *
 * yaffsIoctl - do device specific control function
 *
 * This routine performs the following ioctl functions.
 *
 * RETURNS: OK or function specific 
 * or ERROR if function failed or driver returned error
 *
 */
/*struct opaque_structure*/
struct yaffs_DIR * oldydir;
int dirflag=0;

LOCAL STATUS yaffsIoctl(
		YAFFS_FILE_DESC_ID	pFd,
		int	function,
		int arg){
/*TTRACE;*/
	STATUS retVal;
	fsize_t buf64=0;
	YAFFS_VOLUME_DESC_ID pVolDesc;

	if(pFd==NULL && function!=FIOUNMOUNT){
		/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
		ERRORTRACE("ERROR1-%s:INVALID_PARAMETER!\n",__FUNCTION__);
		return ERROR;
	}
	
	pVolDesc=pFd->pVolDesc;

	switch(function)
	{
		case FIORENAME:
		case FIOMOVE:
			retVal = yaffsRename(pFd, (char *)arg);
			goto ioctlRet;
		case FIOSEEK:                                                 /*7*/
			{
				buf64=(fsize_t)arg;
				/* TODO: vxworks uses fsize_t, yaffs uses off_t */
				retVal = yaffsSeek(pFd, (fsize_t)arg);
				goto ioctlRet;
			}
		case FIOSEEK64:
			{
				buf64=*(fsize_t*)arg;
				retVal = yaffsSeek(pFd, (fsize_t)arg);
				goto ioctlRet;
			}
		case FIOWHERE:
			{
				retVal = pFd->position;
				goto ioctlRet;
			}
		case FIOWHERE64:
			{
				if((void *)arg == NULL){
					errnoSet(S_yaffs2Lib_INVALID_PARAMETER);
					retVal=ERROR;
				} else {
					*(fsize_t *)arg = (pFd->position);
					retVal=OK;
				}
				goto ioctlRet;
			}
		case FIOSYNC:
			{
				struct yaffs_dev * dev = pFd->obj->my_dev;
				semTake(pVolDesc->devSem,WAIT_FOREVER);
				if(dev->is_mounted)
				{
					yaffs_flush_whole_cache(dev,1);
					yaffs_checkpoint_save(dev);
					retVal = OK;
				}
				else
				{
				/*	//todo error - not mounted.*/
					errnoSet(S_yaffs2Lib_VOLUME_NOT_AVAILABLE);
					retVal=ERROR;
				}
				semGive(pVolDesc->devSem);
				goto ioctlRet;
			}
		case FIOFLUSH:
			{
				semTake(pVolDesc->devSem,WAIT_FOREVER);
				retVal=yaffs_flush_file(pFd->obj,1,1,1);
				semGive(pVolDesc->devSem);
				goto ioctlRet;
			}
		case FIONREAD:
			/* unread */
			{
				buf64 = pFd->position;
				if(buf64 < pFd->obj->variant.file_variant.file_size>0)
					buf64 = pFd->obj->variant.file_variant.file_size-buf64;
				else
					buf64 = 0;
				*(unsigned long*)arg = buf64;
				retVal = OK;
				goto ioctlRet;
			}
		case FIONREAD64:
			{
				buf64 = pFd->position;
				if(buf64 < pFd->obj->variant.file_variant.file_size>0)
					buf64 = pFd->obj->variant.file_variant.file_size-buf64;
				else
					buf64 = 0;
				*(fsize_t *)arg = buf64;
				retVal = OK;
				goto ioctlRet;
			}
		case FIOUNMOUNT:
			{
				/*retVal = yaffsVolUnmount(pVolDesc);*/
				retVal = yaffs_unmount("/hd");
				goto ioctlRet;
			}
		case FIONFREE:
			{
				struct yaffs_dev *dev;
				if((void *)arg == NULL){
					errnoSet(S_yaffs2Lib_INVALID_PARAMETER);
					retVal=ERROR;
					goto ioctlRet;
				}
				dev=pFd->obj->my_dev;
				if(dev->is_mounted){
					*(unsigned long *)arg = yaffs_get_n_free_chunks(dev) * 4096;
					retVal=OK;
				}
				goto ioctlRet;
			}
		case FIONFREE64:
			{
				if((void *)arg == NULL){
					errnoSet(S_yaffs2Lib_INVALID_PARAMETER);
					retVal=ERROR;
					goto ioctlRet;
				}
				if(pFd->obj->my_dev->is_mounted){
					*(fsize_t*)arg = yaffs_freespace(pFd->pVolDesc->diskname);
					retVal=OK;
				}
				goto ioctlRet;
			}
		case FIOMKDIR:
			{
				unsigned char path[PATH_MAX+1];
				DEV_HDR *pDevHdr;
				YAFFS_FILE_DESC_ID pFdNew;
				retVal=ERROR;

				if(ioFullFileNameGet((char *)arg,&pDevHdr, path)!=OK){
					errnoSet(S_yaffs2Lib_ILLEGAL_PATH);
				}else if((void *)pDevHdr!=(void *)pVolDesc){
					errnoSet(S_yaffs2Lib_NOT_SAME_VOLUME);
				}else{
					pFdNew=yaffsOpen(pVolDesc,(char *)path,O_CREAT,FSTAT_DIR);
					if(pFdNew!=(void *)ERROR){
						yaffsClose(pFdNew);
						retVal=OK;
					}
				}
				goto ioctlRet;
			}
		case FIORMDIR:                          /*32 rmdir*/
			{
				retVal = yaffsDelete(pFd->pVolDesc,pFd->oripath);
				goto ioctlRet;
			}
			/*
		case FIOLABELGET:
		case FIOLABELSET:
		case FIOCONTIG:
		
struct yaffs_DIR * oldydir;
int dirflag=0;
		*/
		case FIOREADDIR:                                                                                      /*37 ls*/
		{
			struct yaffs_dirent *de;
			char str[1000];
			struct yaffs_stat s;
			int flag = 0;
			
			DIR *pDir = (DIR *)arg;

if(dirflag==0)
{
	oldydir = pFd->ydir;
	dirflag=1;
}

			if((void *)arg == NULL){
				/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
				ERRORTRACE("ERROR2-%s:INVALID_PARAMETER!\n",__FUNCTION__);
				retVal=ERROR;
dirflag=0;
				goto ioctlRet;
			}else if(pFd->ydir==NULL || !pFd->obj || pFd->obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY){
				/*errnoSet(S_yaffs2Lib_NOT_DIRECTORY);*/
				ERRORTRACE("ERROR-%s:NOT_DIRECTORY!\n",__FUNCTION__);
				retVal=ERROR;
dirflag=0;
				goto ioctlRet;
			}


			if((de = yaffs_readdir(oldydir)) != NULL)
			{
				flag = 1;
				memset(pDir->dd_dirent.d_name,0,256);
				strcpy(pDir->dd_dirent.d_name,de->d_name);
				sprintf(str,"%s/%s",pFd->path,de->d_name);
/*printf("dn:%s ,di:%s, st:%s\n",de->d_name,pDir->dd_dirent.d_name,str);*/
				/*pDir->dd_cookie = 1;*/
				retVal = OK;
				goto ioctlRet;
			}
			if(flag == 0){
				pDir->dd_cookie = (-1);
				 pDir->dd_eof = TRUE;
				/*retVal=ERROR;*/
dirflag=0;
					}
dirflag=0;			
			goto ioctlRet;
		}	/* case FIOREADDIR */
		case FIOREADDIRPLUS:                                                                                      /*72 */
		{
			struct yaffs_dirent *de;
			char str[1000];
			struct yaffs_stat s;
			int flag = 0;
			
			DIRPLUS *pDir = (DIRPLUS *)arg;
			
if(dirflag==0)
{
	oldydir = pFd->ydir;
	dirflag=1;
}
			if((void *)arg == NULL){
				/*errnoSet(S_yaffs2Lib_INVALID_PARAMETER);*/
				ERRORTRACE("ERROR2-%s:INVALID_PARAMETER!\n",__FUNCTION__);
				retVal=ERROR;
dirflag=0;
				goto ioctlRet;
			}else if(pFd->ydir==NULL || !pFd->obj || pFd->obj->variant_type != YAFFS_OBJECT_TYPE_DIRECTORY){
				/*errnoSet(S_yaffs2Lib_NOT_DIRECTORY);*/
				ERRORTRACE("ERROR-%s:NOT_DIRECTORY!\n",__FUNCTION__);
				retVal=ERROR;
dirflag=0;		
				goto ioctlRet;
			}
			
			if((de = yaffs_readdir(oldydir)) != NULL)
			{
				flag = 1;
				memset(pDir->dd_dirent.d_name,0,256);
				strcpy(pDir->dd_dirent.d_name,de->d_name);
				sprintf(str,"%s/%s",pFd->path,de->d_name);
/*printf("dn:%s ,di:%s, st:%s\n",de->d_name,pDir->dd_dirent.d_name,str);*/
				yaffsStatGet(pFd, &(pDir->dd_stat));
			/*	pDir->dd_cookie = 1;*/
				retVal = OK;
				goto ioctlRet;
			}
			if(flag == 0){
				pDir->dd_cookie = (-1);
				 pDir->dd_eof = TRUE;
				/*retVal=ERROR;*/
dirflag=0;	
					}
dirflag=0;
			goto ioctlRet;
		}	/* case FIOREADDIR */
		case FIOFSTATGET:                                                                    /*64   0x40open*/
			{
				retVal=yaffsStatGet(pFd, (struct stat *)arg);

				goto ioctlRet;
			}
		
		case FIOUNLINK:
	                {
	               	    retVal = yaffsDelete(pFd->pVolDesc,pFd->oripath);
	                   goto ioctlRet;
	                } /* FIOUNLINK */
		
		case FIOFSTATFSGET:                                                          
			{
				retVal=yaffsFsStatGet(pFd, (struct statfs*)arg);
				goto ioctlRet;
			}
		case FIOTIMESET:
			/* ignore time set */
			goto ioctlRet;
		case FIOTRUNC:
		case FIOTRUNC64:
		case FIOCHKDSK:
		default:
			printf("Ioctl unsupport ioctl function %d\n");
			retVal = ERROR;
			goto ioctlRet;
	}

ioctlRet:
	/*printf("retVal=%d\n",retVal);*/
	return retVal;
}

/*-----------------------------------------------------------------------------------------------*/

STATUS yaffsVolMount(YAFFS_VOLUME_DESC_ID pVolDesc,char* name)
{
	int result;
	semTake(pVolDesc->devSem, WAIT_FOREVER);
	if(!pVolDesc->pYaffsDev->is_mounted){				
		result = yaffs_mount(name);                                                /*yaffs_mount*/
		if(result != 0)
		{
			semGive(pVolDesc->devSem);
			printf("filesystem_mount error!%d %s\n",result,name);
			return -1;
		}
	}
	else
	{
		printf("disk mounted before!\n");
		semGive(pVolDesc->devSem);
		return -1;
	}
	semGive(pVolDesc->devSem);
	return OK;
}

STATUS yaffsDrvInit(void){
	if(yaffsDrvNum != ERROR) return OK;

	upmInit();/*初始化PowerPC UPM接口*/

	yaffsDrvNum = iosDrvInstall(
			(FUNCPTR) yaffsCreate,
			(FUNCPTR) yaffsDelete,
			(FUNCPTR) yaffsOpen,
			(FUNCPTR) yaffsClose,
			(FUNCPTR) yaffsRead,
			(FUNCPTR) yaffsWrite,
			(FUNCPTR) yaffsIoctl
			);
/*	YAFFSPRINT("vx drvnum %d\n",yaffsDrvNum);*/

	if(yaffsDrvNum==ERROR)
		return ERROR;
	else
		return OK;					
	/*return (yaffsDrvNum==ERROR?ERROR:OK);*/
}

STATUS yaffsDevInit(char *name)
{
	int k;
	YAFFS_VOLUME_DESC_ID pVolDesc = NULL;

	if(yaffsDrvNum == ERROR) 			/*&& yaffsLibInit(0) == ERROR)*/ 
		return ERROR;

	/* allocate device descriptor */
	pVolDesc = (YAFFS_VOLUME_DESC_ID) malloc(sizeof(*pVolDesc));
	if(pVolDesc == NULL) return ERROR;
	bzero((char *)pVolDesc, sizeof(*pVolDesc));
	

	/* init semaphores */
	pVolDesc->devSem = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
	
	if(NULL == pVolDesc->devSem)
	{
		free((char *)pVolDesc);
		return (ERROR);
	}
	
	pVolDesc->shortSem = semMCreate(SEM_Q_PRIORITY|SEM_DELETE_SAFE);
	if(NULL == pVolDesc->shortSem)
	{
		semDelete(pVolDesc->devSem);
		free((char *)pVolDesc);
		return (ERROR);
	}

	/* Initialise file descriptors maxfile 20*/
	pVolDesc->pFdList = (YAFFS_FILE_DESC_ID)malloc(20*sizeof(struct YAFFS_FILE_DESC));
	if(NULL!=pVolDesc->pFdList)
	{
		bzero((char*)pVolDesc->pFdList,(20*sizeof(*(pVolDesc->pFdList))));
		pVolDesc->maxFiles = 20;
	}
	else
	{
		semDelete(pVolDesc->devSem);
		semDelete(pVolDesc->shortSem);
		free((char *)pVolDesc);
		return (ERROR);
	}


	pVolDesc->magic = YAFFS_MAGIC;			/*0x5941FF53*/
	pVolDesc->pYaffsDev=yaffs_start_up(name);     /*对yaffs_dev 相关参数进行初始化，先不挂载*/
	if(pVolDesc->pYaffsDev==NULL)
		return ERROR;
	
	pVolDesc->diskname= (char *) malloc(sizeof(char)*20);
	if(pVolDesc->diskname == NULL) 
	{
			
		semDelete(pVolDesc->devSem);
		semDelete(pVolDesc->shortSem);
		free((char *)pVolDesc);
		free((char *)pVolDesc->pFdList);
		return (ERROR);
	}
	bzero((char *)pVolDesc->diskname, sizeof(char)*20);
	pVolDesc->diskname = name;

	if(iosDevAdd((void *)pVolDesc, name, yaffsDrvNum) == ERROR)/*iosDevAdd   vxWorks*/
	{
		pVolDesc->magic = 0;
		return ERROR;
	}
	
	/* auto mount */
/*	yaffs_trace(YAFFS_TRACE_VERIFY,"==>>yaffs: Mounting %s\n",name);*/
	if(yaffsVolMount(pVolDesc,name)==ERROR) return ERROR;
	
	return OK;
}

int yaffs2DiskInit()
{
	int r,m;
	r = yaffsDrvInit();				 /*iosDrvInstall*/
	m=yaffsDevInit("/hd");	
	if(r==-1||m==-1)
		return ERROR;
	return OK;
}
