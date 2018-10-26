#include "yportenv.h"
#include "yaffs_trace.h"
#include "yaffs_guts.h"
#include "yaffs_packedtags2.h"

#define SIZE_IN_MB 2048

#define PAGE_DATA_SIZE (4096)
#define PAGE_SPARE_SIZE  (224)
#define PAGE_SIZE  (PAGE_DATA_SIZE + PAGE_SPARE_SIZE)
#define PAGES_PER_BLOCK (128)
#define BLOCK_DATA_SIZE (PAGE_DATA_SIZE * PAGES_PER_BLOCK)
#define BLOCK_SIZE (PAGES_PER_BLOCK * (PAGE_SIZE))
#define BLOCKS_PER_MB ((1024*1024)/BLOCK_DATA_SIZE)
#define SIZE_IN_BLOCKS (BLOCKS_PER_MB * SIZE_IN_MB)

/*调试*/
#define FLASH_DEBUGFLAG 0                 
#if FLASH_DEBUGFLAG
    #define FLASH_PRINT(str,args...) printf(str,##args)
#else
    #define FLASH_PRINT(str,args...) ((void)0)
#endif


struct nanddrv_transfer {
	unsigned char *buffer;
	int offset;
	int nbytes;
};

/*//oob 储存格式 0~1 坏块标记 2~39 oob， 40~63 数据ecc*/


/*********************************************************************//**
 * 		yaffs内核接口函数，写一页，带oob区域
 * 		写入的时候会计算ecc
 * 
 **********************************************************************/
static int yaffs_nand_drv_WriteChunk(struct yaffs_dev *dev, int nand_chunk,const u8 *data, int data_len,const u8 *oob, int oob_len)				   				   
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-P:%d\n",__FUNCTION__,__LINE__,nand_chunk);

	struct yaffs_dev nand;
	u8 buffer[PAGE_SPARE_SIZE];
	u8 writebuffer[PAGE_SIZE];
	unsigned int row_addr;
	u8 *e;
	int i;
	if(!data || !oob)
		return YAFFS_FAIL;
	nand=*dev;

	row_addr = nand_chunk<<12;

	/* Calc ECC and marshall the oob bytes into the buffer */
	memset(buffer, 0xff, PAGE_SPARE_SIZE); /*224*/
	memset(writebuffer,0xff,PAGE_SIZE);                                  /*4096+224*/
	
	for(i = 0, e = buffer + 40; i < nand.param.total_bytes_per_chunk; i+=256, e+=3)  /*使用了YAFFS2自带的ECC校验功能*/
		yaffs_ecc_calc(data + i, e);                                                                            

	memcpy(buffer + 2, oob, oob_len);
	/* Set up and execute transfer */
	memcpy(writebuffer,data,data_len);      /*data to be write*/
	memcpy(writebuffer+PAGE_DATA_SIZE,buffer,PAGE_SPARE_SIZE);/*spare data with ecc to be write*/

	if( upmNandProgram(row_addr,writebuffer,PAGE_SIZE) == 0)
		return YAFFS_OK;
	else
		return YAFFS_FAIL;
}

/*********************************************************************//**
 * 		yaffs内核接口函数，读一页，带oob区域
 * 		读出的时候会计算ecc
 * 
 **********************************************************************/
static int yaffs_nand_drv_ReadChunk(struct yaffs_dev *dev, int nand_chunk,u8 *data, int data_len,u8 *oob, int oob_len,enum yaffs_ecc_result *ecc_result_out)					   				  				   
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-P:%d\n",__FUNCTION__,__LINE__,nand_chunk);

	struct yaffs_dev nand;
	u8 buffer[PAGE_SPARE_SIZE];

	int n_tr = 0;
	int ret=0,ret1=0;
	enum yaffs_ecc_result ecc_result;
	int i;
	unsigned int row_addr;

	u8 *e;
	u8 read_ecc[3];
	
	nand=*dev;
	row_addr = nand_chunk<<12;

	if(data)
		ret = upmNandRead(row_addr,data, data_len);    /*read main data*/
	ret1 = upmNandReadExtra(row_addr,buffer,PAGE_SPARE_SIZE);/*read spare data*/  
	if(ret < 0 ||ret1<0)
	{
		if (ecc_result_out)
			*ecc_result_out = YAFFS_ECC_RESULT_UNKNOWN;
		return YAFFS_FAIL;
	}

	/* Do ECC and marshalling */
	if(oob)
		memcpy(oob, buffer + 2, oob_len);

	ecc_result = YAFFS_ECC_RESULT_NO_ERROR;

/*进行YAFFS2 ECC校验*/
	if(data)
	{
		for(i = 0, e = buffer + 40; i < nand.param.total_bytes_per_chunk; i+=256, e+=3) 
		{
			yaffs_ecc_calc(data + i, read_ecc);
			ret = yaffs_ecc_correct(data + i, e, read_ecc);
			if (ret < 0)
				ecc_result = YAFFS_ECC_RESULT_UNFIXED;
			else if( ret > 0 && ecc_result == YAFFS_ECC_RESULT_NO_ERROR)
				ecc_result = YAFFS_ECC_RESULT_FIXED;
		}
	}

	if (ecc_result_out)
		*ecc_result_out = ecc_result;

	return YAFFS_OK;
}
/*********************************************************************//**
 * 		yaffs内核接口函数，擦除一个块
 * 		
 * 
 **********************************************************************/
static int yaffs_nand_drv_EraseBlock(struct yaffs_dev *dev, int block_no)                                            
{		
    /*printf(">>Sam_nandflash2k-F:%s-B:%d\n",__FUNCTION__,block_no);*/
	unsigned int row_addr;
	row_addr = block_no<<19;/* 块个数转换为大小*/
	if( upmNandErase(row_addr) == 0)	
	return YAFFS_OK;
	else
		return YAFFS_FAIL;
}
/*********************************************************************//**
 * 		yaffs内核接口函数，标记一个坏块
 * 		oob区域第0和1字节不为ff则是坏块
 * 
 **********************************************************************/
static int yaffs_nand_drv_MarkBad(struct yaffs_dev *dev, int block_no)
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-B:%d\n",__FUNCTION__,__LINE__,block_no);

	int dummy = 1;
	char error=0;
	unsigned int row_addr;
	row_addr = (block_no<<19);/*+ 127*(4*1024);*/
	
	error |= upmNandProgramExtra(row_addr,&dummy,1);
	
	if(error == 0) 
		return YAFFS_OK;
	else		
		return YAFFS_FAIL;
}

/*********************************************************************//**
 * 		yaffs内核接口函数，检查一个块的好坏
 * 		
 * 
 **********************************************************************/
static int yaffs_nand_drv_CheckBad(struct yaffs_dev *dev, int block_no)
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-B:%d\n",__FUNCTION__,__LINE__,block_no);
	char bad;
	char dummy[2]={0};
	unsigned int row_addr;
	row_addr = (block_no<<19);/*+ 127*(4*1024);*/
	char allff[]={0xff,0xff};

	upmNandReadExtra(row_addr,dummy,2);
	bad=memcmp(dummy,allff,2);
	if(bad!=0)
	{
/*		printf("block %d is bad block!\n",block_no);*/
		return YAFFS_FAIL;
	}
	else 
		return YAFFS_OK;
}
/*********************************************************************//**
 * 		yaffs内核接口函数，nand硬件初始化
 * 		初始化已经在别处完成，这里就直接返回正常了
 * 
 **********************************************************************/
static int yaffs_nand_drv_Initialise(struct yaffs_dev *dev)
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d\n",__FUNCTION__,__LINE__);
	return YAFFS_OK;
}

/*********************************************************************//**
 * 		yaffs内核接口函数，同样是写一页，只是输入参数有区别，
 * 		
 * 
 **********************************************************************/
static int yaffs_nand_tag_WriteChunk(struct yaffs_dev *dev, int nand_chunk,const u8 *data,const struct yaffs_ext_tags *tags)
{
	FLASH_PRINT("==>>F:%s-L:%d  P:%d=========================================>\n",__FUNCTION__,__LINE__,nand_chunk);
	int retval = 0,i=0;
	unsigned int row_addr;
	struct yaffs_packed_tags2 pt;
	void *spare;
	unsigned spareSize = 0;
	unsigned char buffer[PAGE_SPARE_SIZE];
	unsigned char writebuffer[PAGE_SIZE];

	unsigned char *e;
	row_addr = nand_chunk<<12;
	memset(buffer, 0xff, PAGE_SPARE_SIZE);	/*//oob 储存格式 0~1 坏块， 2~39 tag， 40~63 数据ecc*/
	memset(writebuffer,0xff,PAGE_SIZE);

	yaffs_pack_tags2(dev,&pt, tags, !dev->param.no_tags_ecc);	
	memcpy(buffer + 2, &pt, sizeof(struct yaffs_packed_tags2));

	if(data!=NULL)
		{		
			/* Calc ECC and marshall the oob bytes into the buffer */
			/* 计算ECC 的值, 存在buffer的40~63　，这样spare包含了data的ECC校验信息*/
			for(i = 0, e = buffer + 40; i < dev->param.total_bytes_per_chunk; i+=256, e+=3)
				yaffs_ecc_calc(data + i, e);                                                                              
		}
	
	if(data)
		memcpy(writebuffer,data,PAGE_DATA_SIZE);
	memcpy(writebuffer+PAGE_DATA_SIZE,buffer,PAGE_SPARE_SIZE);

	if( upmNandProgram(row_addr,writebuffer,PAGE_SIZE) == 0)
			return YAFFS_OK;  						 /*1*/
		else
			return YAFFS_FAIL;
}

/*********************************************************************//**
 * 		yaffs内核接口函数，同样是读取一页，只是输入参数有区别，
 * 		
 * 
 **********************************************************************/
extern int readflag;
static int yaffs_nand_tag_ReadChunk(struct yaffs_dev *dev, int nand_chunk,u8 *data, struct yaffs_ext_tags *tags)
{
	FLASH_PRINT("==>>F:%s-L:%d-P:%d\n",__FUNCTION__,__LINE__,nand_chunk);
	struct yaffs_packed_tags2 pt;
	int localData = 0,i=0;
	void *spare = NULL;
	unsigned spareSize;
	int retval = 0;
	int eccStatus; /* 0 = ok, 1 = fixed, -1 = unfixed */
	unsigned int row_addr;
	unsigned char buffer[PAGE_SPARE_SIZE];
	unsigned char readbuffer[PAGE_SIZE];
	unsigned char *e;
	unsigned char read_ecc[3];

	memset(readbuffer,0xff,PAGE_SIZE);

	row_addr = nand_chunk<<12;
	
	if (!tags)
	{
		spare = NULL;
		spareSize = 0;
	} 
	else if (dev->param.inband_tags) 
	{
		if (!data) 
		{
			localData = 1;
			data = yaffs_get_temp_buffer(dev);
		}
		spare = NULL;
		spareSize = 0;
	} 
	else 
	{
		spare = &pt;
		spareSize = sizeof(struct yaffs_packed_tags2);
	}
	if(data)
		retval = upmNandRead(row_addr,data, PAGE_DATA_SIZE);    /*read main data*/
	retval = upmNandReadExtra(row_addr,buffer,PAGE_SPARE_SIZE);/*read spare data*/  
	/* Do ECC and marshalling */
	memcpy((unsigned char*)&pt, buffer + 2, sizeof(struct yaffs_packed_tags2));	
	if (tags)
		yaffs_unpack_tags2(dev,tags, &pt, !dev->param.no_tags_ecc);
	
	readflag = 0;
	eccStatus = 0;

	if(data) 
	{			
		for(i = 0, e = buffer + 40; i < dev->param.total_bytes_per_chunk; i+=256, e+=3) 
		{
			yaffs_ecc_calc(data + i, read_ecc);
			eccStatus = yaffs_ecc_correct(data + i, e, read_ecc);
			if (eccStatus < 0)
			{
				printf("eccbreak!! L:%d  eccstatus:%d\n",__LINE__,eccStatus);
				break;
			}
		}
	}	


	if (tags && tags->chunk_used) 
	{
		/*printf("\n\n%s-chunk_used\n\n",__FUNCTION__);*/
	
		if (eccStatus < 0 || tags->ecc_result == YAFFS_ECC_RESULT_UNFIXED)		    
			tags->ecc_result = YAFFS_ECC_RESULT_UNFIXED;
		else if (eccStatus > 0 || tags->ecc_result == YAFFS_ECC_RESULT_FIXED)			     
			tags->ecc_result = YAFFS_ECC_RESULT_FIXED;
		else
			tags->ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
	}

	if (localData)
		yaffs_release_temp_buffer(dev, data);

	return (retval+1);	
}

/*********************************************************************//**
 * 		yaffs内核接口函数，同样是标记坏块，只是输入参数有区别，
 * 		
 * 
 **********************************************************************/
static int yaffs_nand_tag_MarkBad(struct yaffs_dev *dev, int block_no)
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-B:%d\n",__FUNCTION__,__LINE__,block_no);

	return yaffs_nand_drv_MarkBad(dev,block_no);
}

/*********************************************************************//**
 * 		yaffs内核接口函数，同样是查询坏块，只是输入参数有区别，
 * 		
 * 
 **********************************************************************/
static int yaffs_nand_tag_QueryBad(struct yaffs_dev *dev, int block_no,enum yaffs_block_state *state,u32 *seq_number)
{
	FLASH_PRINT("==>>Sam_nandflash2k-F:%s-L:%d-B:%d\n",__FUNCTION__,__LINE__,block_no);

	unsigned chunkNo;
	struct yaffs_ext_tags tags;
	*seq_number = 0;
	chunkNo = block_no* dev->param.chunks_per_block;

	if (!yaffs_nand_drv_CheckBad(dev, block_no)) 
	{
		*state = YAFFS_BLOCK_STATE_DEAD;
	}
	else 
	{
		yaffs_nand_tag_ReadChunk(dev, chunkNo, NULL, &tags);
		if (!tags.chunk_used)
		{
			*state = YAFFS_BLOCK_STATE_EMPTY;
		} 
		else 
		{
			*state = YAFFS_BLOCK_STATE_NEEDS_SCAN;
			*seq_number = tags.seq_number;
		}
	}
	return YAFFS_OK;
}

/*********************************************************************//**
 * 		yaffs内核接口函数，挂载一个分区
 * 		
 * 
 **********************************************************************/
extern void yaffsfs_OSInitialisation();
extern void yaffs_add_device(struct yaffs_dev *);

struct yaffs_dev *devTest = NULL;

struct yaffs_dev * yaffs_start_up(char *name)
{
	static int start_up_called = 0;
	if(start_up_called)
		return 0;
	start_up_called = 1;
	
	struct yaffs_dev *dev = NULL;
	struct yaffs_param *param;
	struct yaffs_driver *drv;
	struct yaffs_tags_handler *tag;		
	dev = malloc(sizeof(*dev));	
	if(!dev)
		return NULL;
	
	memset(dev, 0, sizeof(*dev));	
	dev->param.name = strdup(name);
	if(!dev->param.name) 
	{
		free(dev);
		return NULL;
	}	
	/*fcmInit();*/
	/*Call the OS initialisation (eg. set up lock semaphore */
	yaffsfs_OSInitialisation();

	drv = &dev->drv;
	
	drv->drv_write_chunk_fn = yaffs_nand_drv_WriteChunk;
	drv->drv_read_chunk_fn = yaffs_nand_drv_ReadChunk;
	drv->drv_erase_fn = yaffs_nand_drv_EraseBlock;
	drv->drv_mark_bad_fn = yaffs_nand_drv_MarkBad;
	drv->drv_check_bad_fn = yaffs_nand_drv_CheckBad;
	drv->drv_initialise_fn = yaffs_nand_drv_Initialise;
	dev->driver_context = (void *) 1;
	
	param = &dev->param;
	
	param->total_bytes_per_chunk = 4096;
	param->chunks_per_block = 128;
	param->spare_bytes_per_chunk  = 224;
	
	param->start_block = 0;
	param->end_block = 4095;/*yflash2_GetNumberOfBlocks()-1;*/
	param->is_yaffs2 = 1;
	param->use_nand_ecc=0;
	param->always_check_erased=1;
	
	param->n_reserved_blocks = 5;
	param->wide_tnodes_disabled=0;
	param->refresh_period = 1000;
	param->n_caches = 0; /*10;*/ /*/ Use caches*/
	param->no_tags_ecc = 0;
	param->enable_xattr = 10;/*1;*//*Flag to enable the use of extended attributes (xattr)*/	
	tag = &dev->tagger;
	
	tag->write_chunk_tags_fn		=yaffs_nand_tag_WriteChunk;
	tag->read_chunk_tags_fn			=yaffs_nand_tag_ReadChunk;
	tag->query_block_fn			=yaffs_nand_tag_QueryBad;
	tag->mark_bad_fn				=yaffs_nand_tag_MarkBad;
	
	yaffs_add_device(dev);	
	devTest = dev;
	return dev;
}

void ybytecount(void)
{
	unsigned long space = yaffs_freespace("/hd");
	unsigned long totalspace = yaffs_totalspace("/hd");
	if(space>=0 && totalspace>=0)
		printf("/hd total Space: %13x Bytes        %d MB\n/hd Free Space:  %13x Bytes        %d MB\n",totalspace,totalspace/(1024*1024),space,space/(1024*1024));
	else 
		printf("get  space error!\n");
}



