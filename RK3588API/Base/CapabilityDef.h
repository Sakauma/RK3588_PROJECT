

#ifndef __DRIVER_HARDWARE_CAPABILITY_DEF_H__
#define __DRIVER_HARDWARE_CAPABILITY_DEF_H__

#define		MAX_PROCESS_PER_DEV					(8)		/*一个设备最多允许多少个进程同时访问*/

#define		MAX_UP_DMA_INDEX_MEM_SIZE 			(0xFFFF * 1024)	/*上行描述符最大内存尺寸限制，16个bit表示多少K的空间（硬件设计限制的，最初是11个BIT）*/

#define		MAX_DEVICE_MSI_INT_COUNT			(8)		/*本驱动支持的MSI中断对象最大数量*/
	
#define		DEFAULT_DMA_BLOCK_SIZE				(2)		/*默认的DMA存储区分块的大小，单位MB*/
#define		MAX_DMA_BLOCK_SIZE					(2)		/*可支持的DMA存储区分块的最大大小，单位MB*/

#define		DEFAULT_UP_DMA_BLOCK_NUM_PER_CHL 	(32)	/*每个上行DMA通道默认可分配和管理多少个DMA存储区分块，当读注册表参数无效时使用此参数*/
#define		DEFAULT_DOWN_DMA_BLOCK_NUM_PER_CHL	(32)	/*每个DMA下行通道默认分配和管理多少个DMA存储区分块*/

#define		MAX_DMA_BLOCK_PER_DEV				(256)	/*每个设备最大可分配多少个内存块*/
#define		MIN_DMA_BLOCK_PER_DEV				(64)	/*每个设备最少需要分配的DMA存储区块数量，少于此数量则认为资源不足*/
#define		DEFAULT_DMA_BLOCK_PER_DEV			(210)	/*每个设备初始默认分多少个内存块*/

#define		DEFAULT_INIT_UP_CHL_NUM_PER_DEV		(2)		/*每个设备按照2个上行通道来先申请索引相关内存*/
#define		DEFAULT_INIT_DOWN_CHL_NUM_PER_DEV	(2)		/*每个设备按照2个下行通道来先申请索引相关内存*/
#define		DEFAULT_INIT_PROCESS_NUM_PER_DEV	(2)		/*每个设备按照2个进程信息申请相关DMA内存*/

#define		AVERAGE_PACK_BYTE_CLCL_INDEX		(200)	/*按照 AVERAGE_PACK_BYTE_CLCL_INDEX 定义的平均包长来计算索引的长度*/


/*硬件需要物理地址的起始地址能整除8（按照8个字节对齐），所以分配内存的时候多分一个交换页大小，然后除以8取余数作为偏移，
也就是说，实际分到的内存会大一点，而实际使用的可能会在分到的内存首地址的基础上往后偏移一点*/
#define		ALIGN_PH_ADDR_BASE_LEN				(8)


#define		MAX_UP_DMA_EXT_INT_COUN				(0xFF)		/*上行DMA增强中断最大允许次数*/
#define		MIN_UP_DMA_EXT_INT_TM				(200)		/*上行DMA增强中断最小允许间隔（毫秒）*/
#define		MAX_UP_DMA_EXT_INT_TM				(520)		/*上行DMA增强中断最大允许间隔（毫秒）*/

#define		UP_DMA_CHL_INT_TMOUT				(500)		/*上行DMA数据通道，不足指定数量的包上送时，UP_DMA_CHL_INT_TMOUT 长时间就给中断，单位微秒*/
#define		UP_DMA_CHL_INT_NUM					(200)		/*上行DMA数据通道，达到 UP_DMA_CHL_INT_NUM 个包上送，就给中断*/
#define		UP_DMA_CHL_EXT_INT_COUNT			(3)			/*上行数据通道默认中断补刀中断次数*/
#define		UP_DMA_CHL_EXT_INT_TIME				(500)		/*上行数据通道默认中断补刀中断间隔*/


#define		MIN_INT_INTERVAL_TM					(200)		/*最小中断间隔时间，单位：us*/
#define		MAX_INT_INTERVAL_TM					(1000)		/*最大中断间隔时间，单位：us*/


#endif