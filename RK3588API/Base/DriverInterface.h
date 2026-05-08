#ifndef __DRIVER_IO_CTRL_DEFINE_H__
#define __DRIVER_IO_CTRL_DEFINE_H__


#include "../platform.h"
	
#ifdef _BUILD_FOR_LINUX_
		#include "SysDifferent.h"

#else
	#ifdef _WINDOWS_
		#include <initguid.h>
	
		// Define an Interface Guid so that app can find the device and talk to it.
		DEFINE_GUID(GUID_DEVINTERFACE_CavigeEB3400,
			0x00E87618, 0x7CC5, 0xAA54, 0x86, 0x57, 0x4D, 0x7B, 0xEB, 0xB5, 0x49, 0xCA);
		//{00E87618-7CC5-AA54-8657-4D7BEBB549CA}
	#else 
		#ifdef _BUILD_FOR_VXWORKS_
		#endif	//vxworks
	#endif
	
#endif

/*------------------------------------驱动支持的基本能力信息定义开始-----------------------------------*/

#define MAX_PROCESS_PER_DEV         8   /*一个设备最多允许多少个进程同时访问*/
#define MAX_DEVICE_MSI_INT_COUNT    8   /*本驱动支持的MSI中断对象最大数量*/
#define MAX_DMA_BLOCK_SIZE          2   /*本驱动可支持的DMA存储区分块的最大大小，单位MB*/
#define DMA_BLOCK_BASE_SIZE (1 * 1024 * 1024)   /*驱动DMA存储区分块的基本计算大小（约定为1MB），实际存储区分块的大小必须是此尺寸的整数倍*/

#define MAX_DEV_UP_DMA_CHL          16  /*硬件设备最大可支持的上行DMA通道数量*/
#define MAX_UP_DMA_BLOCK_NUM_PER_CHL 64	/*每个DMA上行通道最多可分配和管理多少个DMA存储区分块*/

#define MAX_DEV_DOWN_DMA_CHL    16  /*硬件设备最大可支持的下行DMA通道数量*/
#define MAX_DOWN_DMA_BLOCK_NUM_PER_CHL  32  /*每个DMA下行通道最多可分配和管理多少个DMA存储区分块*/

#define MAX_DEVICE_EVENT_NUM    16  /*驱动可提供的最大映射的信号量数量，映射的信号量可在用户层产生伪中断*/

#define MAX_USED_BAR_NUM        6   /*BAR0---BAR6*/

#define MAX_DEVICE_NAME_LEN     260 /*设备名称字符串的buf长度，用于 IO_CTRL_GET_LINKNAME操作*/

#define MAX_PACK_BYTE_PER_DMA   (DMA_PAGE_SIZE) /*最大DMA包长，该长度至少要保证8字节对齐，否则可能会导致下行DMA出现异常*/

/*---------------------------------------------------结束----------------------------------------------------*/



//定义连接名的前面部分，后面加自动累加编号的数字，第一个卡的设备全名为 \\\\.\\CAVIGE_EB3400_01，以此类推
#define DEV_LINK_NAME_FRONT_A	"CV_FC_"
#define DEV_LINK_NAME_FRONT_W	L"CV_FC_"


/*为64位和32位编译定义地址转换宏，一般驱动给过来的地址是使用unsigned long long存储*/
#ifdef _WIN64
#define CHANGE_INT64_TO_POINT(nAddr) ((void*)(nAddr))
#else
#define CHANGE_INT64_TO_POINT(nAddr) ((void*)nAddr)
#endif


#pragma pack (4)

/*--------------------------------------上行DMA通道相关数据结构和操作宏定义开始--------------------------------------*/
/*上行DMA数据索引结构*/
typedef struct __UP_DMA_INDEX_NODE_STRUCT__
{
    volatile unsigned int nOffset_Vflag;		/*当前数据相对于大块开头的偏移量，bit0到bit2都置1表示描述符有效标记，因为数据是按照8字节对齐的（长度计算和偏移计算按照字节计算的），所以低3个bit可以用来作为标记*/
    volatile unsigned int length_block_Eflag;	/*最高bit用于流控切换标记，bit16：23为第几个DMA block块，Bit0:15为数据包的长度，注意：上行最高bit为1时，该索引可以是存在有效记录的*/

} UpIndexNode_st, *pUpIndexNode_st;


#define INDEX_GET_VALID_FLAG(x) (x.nOffset_Vflag & 7)
#define INDEX_GET_UP_OFFSET(x) (x.nOffset_Vflag & 0xFFFFFFF8)

#define INDEX_IS_E_FLAG_SET(x) (x.length_block_Eflag >> 31)
#define INDEX_GET_BLOCK_NUM(x) ((x.length_block_Eflag >> 16) & 0xFF)
#define INDEX_GET_DATA_LENGTH(x) (x.length_block_Eflag & 0xFFFF)


#define INDEX_PTR_GET_VALID_FLAG(x) (x->nOffset_Vflag & 7)
#define INDEX_PTR_GET_UP_OFFSET(x) (x->nOffset_Vflag & 0xFFFFFFF8)

#define INDEX_PTR_IS_E_FLAG_SET(x) (x->length_block_Eflag >> 31)
#define INDEX_PTR_GET_BLOCK_NUM(x) ((x->length_block_Eflag >> 16) & 0xFF)
#define INDEX_PTR_GET_DATA_LENGTH(x) (x->length_block_Eflag & 0xFFFF)


/*驱动DMA索引读写位置记录是采用的复合结构，高8个Bit为循环使用的圈数，低24个Bit为指针位置信息，所以定义下如下操作宏*/

/*注意：因为多核CPU的原因，round和ptr分开2个32bit的话，极端情况下可能用户层对比位时驱动刚好只更新了一个值而导致出错，所以只能用一个32bit来复用存储2个值，
结构体中的所有上行描述符位置读写的值都是复用的，最高的8个bit表示ROUND，其余24个bit表示位置*/

#define MAX_INDEX_ROUND		(0xFF)		//DMA描述符（索引）圈数的最大值，超过此值就归零了
#define MAX_INDEX_PTRNUM	(0xFFFFFF)	//DMA描述符（索引）位置的最大值，不能超过此值

#define GET_ROUND_BY_MUTIPTR(x) (((unsigned int)x) >> 24)
#define GET_PTRNUM_BY_MUTIPTR(x) ((((unsigned int)x) & MAX_INDEX_PTRNUM))

#define MAKE_INDEX_MUTI_PTR(r, p) (((((unsigned int)r) << 24) | (((unsigned int)p) & MAX_INDEX_PTRNUM)))
/*---------------------------------------------------结束----------------------------------------------------*/




/*---------------------------------------上行DMA通道运行信息结构定义开始-------------------------------------*/
/*用户层和内核共用的上行DMA通道交互用的结构*/
typedef struct __UP_DMA_CHL_RUNINFO_STRUCT__
{
    volatile int flagOpen;		/*通道开启标记*/
    volatile unsigned int nReadMutiPtr;	/*读位置复合指针（见DMA索引位置计算宏定义）*/
  //  	atomic_t nReadMultiPtr;
    volatile unsigned int nWriteMutiPtr;	/*写位置复合指针（见DMA索引位置计算宏定义）*/
    volatile int nDropPack;		/*当前驱动丢包记录数*/
//	atomic_t  nDropPack;
} UpDmaRun_st, *pUpDmaRun_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*-------------------------------------下行DMA通道运行信息结构定义开始--------------------------------------*/
/*用户层和内核共用的下行行DMA通道交互用的结构*/
typedef struct __DOWND_DMA_CHL_RUNINFO_STRUCT__
{    
	unsigned int reserved;
    volatile unsigned int nReadPtr;	/*下行指针（不是复合指针）*/
    volatile unsigned int nWritePtr; /*下行指针（不是复合指针）*/
    volatile unsigned int nFifoNum;	/*下行寄存器FIFO深度，只有此值为0时才读取一次，减少读寄存器的次数，提高效率*/
} DownDmaRun_st, *pDownDmaRun_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*------------------------------------------下行DMA索引结构定义开始------------------------------------------*/
/*
下行DMA通道按照每个索引块可存储最大DMA包长来计算其对应的数据存储块的位置，由驱动计算，计算完毕后，通过此结构返回给应用层
应用层不允许更改记录值，但是可以根据nAddRessVi写入下行DMA包，下行DMA包长不能超过 MAX_PACK_BYTE_PER_DMA
*/
typedef struct __DOWN_DMA_INDEX_NODE_STRUCT__
{
    unsigned long long nAddRessVi;		/*当前索引块指向的数据存储区的映射到进程空间的内存地址*/
    unsigned long long nAddRessPh;		/*当前索引块指向的数据存储区的物理地址（与nAddRessVi对应）*/

} DownIndexNode_st, *pDownIndexNode_st;
/*---------------------------------------------------结束----------------------------------------------------*/





/*-------------------------------------------驱动能力参数结构定义开始----------------------------------------*/
/*DeviceIoControl 的 IO_CTRL_GET_BASE_INFO 操作相关结构*/
typedef struct __DEV_PARAM_STRUCT__ /*读取的设备参数存入此处，例如板卡支持多少个通道，各通道参数等*/
{
	unsigned int unDmaBlockSize;		/*驱动数据块内存（block块）的实际尺寸，单位：字节*/
	unsigned int flagUpDmaTransCtrl;	/*驱动是否启用了流控*/
	unsigned int flag_x64;				/*当前驱动是64位驱动还是32位驱动，0表示32位，非0表示64位*/
#if defined(_BUILD_FOR_LINUX_)
	unsigned int oskn_iommuen;			/*是否操作系统使能了IOMMU*/
#endif
	unsigned char validPosition; //位置信息有效标记，0表示无效，1表示有效
	unsigned char nBusNum; //总线号
	unsigned char nDevNum; //设备号
	unsigned char nFunNum; //功能号

	//从硬件读取，获知硬件支持多少个上行DMA通道和多少个下行DMA通道
	unsigned int unUpChlNum;	/*驱动可使用的上行通道个数（规定，上行通道的使用一定是从0开始往后连续使用）*/
	unsigned int unDownChlNum;	/*驱动可使用的下行通道个数（规定，下行通道的使用一定是从0开始往后连续使用）*/
	unsigned int unEventNum;	/*驱动可使用的事件数量（规定，都是从0开始往后连续使用）*/

} DeviceParam_st, *pDeviceParam_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*-----------------------------------获取BAR空间的IOCTRL使用的结构定义开始-----------------------------------*/
/*DeviceIoControl 的 IO_CTRL_GET_BASE_INFO 操作相关结构*/
typedef struct __BAR_RESOURCE_IOCTRL_STRUCT__
{
    unsigned int nBarSize[MAX_USED_BAR_NUM];		/*Bar0~Bar5各Bar空间的大小*/
    unsigned long long pBarAddr[MAX_USED_BAR_NUM];	/*Bar0~Bar5各Bar空间的起始地址为了兼容，用64位整形存储指针的值，用户层用的时候转成(unsighed char*)类型指针即可*/

} BarIoCtrl_st, *pBarIoCtrl_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*---------------------------------获取上行DMA通道的IOCTRL使用的结构定义开始---------------------------------*/
/*DeviceIoControl 的 IO_CTRL_GET_UPCHL 操作相关结构*/
typedef struct __UP_DMA_CHL_IOCTRL_STRUCT__
{
	unsigned int nIndexNum;		/*当前上行DMA通道使用的索引块数量*/
	unsigned int nBlockNum;		/*当前上行DMA通道使用的数据存储块数量*/

	unsigned long long nIndexAddr;	/*各上行DMA通道的描述符的起始地址，为了兼容，用64位整形存储指针的值，用户层用的时候转成(pDmaUpIndexNode)类型指针即可*/

	unsigned long long nIndexPh; /*DMA通道的描述符起始物理地址，给到上层方便与FPGA核对，实际运行时不使用*/
	unsigned long long pBlockAddr[MAX_UP_DMA_BLOCK_NUM_PER_CHL]; /*各上行DMA通道的存储块的起始地址，为了兼容，用64位整形存储指针的值，用户层用的时候转成(unsighed char*)类型指针即可*/
	unsigned long long pBlockPh[MAX_UP_DMA_BLOCK_NUM_PER_CHL]; /*各上行DMA通道的存储块的物理地址，给到上层方便与FPGA核对，实际运行时不使用*/

	unsigned long long nRunInfoAddr;	/*存储驱动map过来的UpDmaRun_st结构的地址，此地址的结构为进程和驱动共用，用于上行DMA的传输控制*/
#if defined(_BUILD_FOR_LINUX_)||defined(_BUILD_FOR_VXWORKS_)
	int	 nRunInfoOffset;
	unsigned int size_nIndexAddr;
	unsigned int size_pBlockPh[MAX_UP_DMA_BLOCK_NUM_PER_CHL];
#endif
} UpChlIoCtrl_st, *pUpChlIoCtrl_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*---------------------------------获取下行DMA通道的IOCTRL使用的结构定义开始---------------------------------*/
/*DeviceIoControl 的 IO_CTRL_GET_DOWNCHL 操作相关结构*/
typedef struct __DOWN_DMA_CHL_IOCTRL_STRUCT__
{
	unsigned int reserved;
	unsigned int nUsedIndexNum; /*供使用的索引块的数量（不一定就是分配的数量，是根据分配到的内存块重新计算出来的），为了省事直接放这里了，用户层不允许更改*/

	unsigned long long nIndexAddr;	/*各下行DMA通道的索引的起始地址，为了兼容，用64位整形存储指针的值，用户层用的时候转成(pDmaUpIndexNode)类型指针即可*/
	unsigned long long nRunInfoAddr;	/*存储驱动map过来的DownDmaRun_st结构的地址，此地址的结构为进程和驱动共用，用于上行DMA的传输控制*/
#if defined(_BUILD_FOR_LINUX_)||defined(_BUILD_FOR_VXWORKS_)
//	unsigned long long pBlockAddr[MAX_DOWN_DMA_BLOCK_NUM_PER_CHL]; /*各上行DMA通道的存储块的起始地址，为了兼容，用64位整形存储指针的值，用户层用的时候转成(unsighed char*)类型指针即可*/
	unsigned long long pBlockPh[MAX_DOWN_DMA_BLOCK_NUM_PER_CHL]; /*各上行DMA通道的存储块的物理地址，给到上层方便与FPGA核对，实际运行时不使用*/

	int	 nRunInfoOffset;
	unsigned int size_nIndexAddr;
	unsigned int size_pBlockPh[MAX_DOWN_DMA_BLOCK_NUM_PER_CHL];
#endif
} DownChlIoCtrl_st, *pDownChlIoCtrl_st;
/*---------------------------------------------------结束----------------------------------------------------*/




/*---------------------------------注册事件中断通知IOCTRL使用的结构定义开始---------------------------------*/
/*DeviceIoControl 的 IO_CTRL_SET_EVENT 操作相关结构*/
typedef struct __DERIVE_SET_EVENT_IOCTRL_STRUCT__
{
    unsigned int nIndex;  /*事件序号，范围：0到(MAX_DEVICE_EVENT_NUM - 1)*/
    OS_HANDLE hEvent;     /*由用户层创建的事件句柄*/

} EventSet_st, *pEventSet_st;
/*---------------------------------------------------结束----------------------------------------------------*/





/*-------------------------------获取当前调试记录信息IOCTRL使用的结构定义开始-------------------------------*/
/*DeviceIoControl 的 IO_CTRL_GET_DBG_REC 操作相关结构*/
typedef struct __DRIVER_DBG_CUR_RECORD_STRUCT_
{
    /*根据调试需要自行修改结构中的元素*/
    unsigned long long nCountA;
    unsigned long long nCountB;
    unsigned long long nCountC;
    unsigned long long nCountD;

    unsigned long long nCountW;
    unsigned long long nCountX;
    unsigned long long nCountY;
    unsigned long long nCountZ;

} CurDbg_st, *pCurDbg_st;
/*---------------------------------------------------结束----------------------------------------------------*/

#pragma pack ()



#if defined( _WINDOWS_)
/*获取驱动设备对象的连接名，系统的接口名称不直观，所以驱动创建了连接名，通过此操作获取连接名
无输入内容，输出内容为UNICODE编码的字符串，如果传入的buf长度不足，则返回失败并给出名称长度
OutType---WCHAR pBuf[MAX_DEVICE_NAME_LEN]*/
#define IO_CTRL_GET_LINKNAME CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x50, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从驱动获取硬件的基本能力信息，OutType---DeviceParam_st结构*/
#define IO_CTRL_GET_BASE_INFO CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x51, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*映射BAR空间，OutType---BarRes_st结构*/
#define IO_CTRL_MAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x52, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消BAR空间映射，无输入输出参数*/
#define IO_CTRL_UNMAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x53, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请上行通道的使用权限（独占），InType---unsigned int（chl），OutType---UpChlIoCtrl_st结构*/
#define IO_CTRL_GET_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x54, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放上行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x55, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请下行通道的使用权限（独占），InType---unsigned int（chl），OutType---DownChlIoCtrl_st结构*/
#define IO_CTRL_GET_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x56, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放下行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x57, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从用户层获取为某个通道创建的同步事件对象，用于通知上行的DMA通知通道有数据需要接收，
InType---unsigned int（index） + HANDLE（根据用户层程序的不同可能是32bit + 32bit或者是64bit，驱动会做合理性判断）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)
OutType---unsigned long long，返回值为该事件序号对应的计数器的内存地址，该计数器记录上次有信号到本次有信号之间，驱动实际给了多少次信号*/
#define IO_CTRL_SET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x58, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的某个上通道的事件对象，InType---unsigned int（index）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)*/
#define IO_CTRL_UNSET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x59, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的所有上通道的事件对象，无输入输出参数*/
#define IO_CTRL_UNSET_ALL_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5A, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)



/*获取驱动当前内核的相关调试信息，
OutType---CurDbg_st结构，该结构存储了内核中的一些调试相关计数等，每次获取将获得当前记录的值*/
#define IO_CTRL_GET_DBG_REC CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5B, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*复位所有上行DMA，无输入输出参数*/
#define IO_CTRL_RESET_ALL_UPCHL_DMA CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5C, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)
			
#elif defined(_BUILD_FOR_LINUX_)


struct 		CVG_IOCTL_MAX_ST{
	int				dmaChan;
	int				dmaWIndex;
	int				realLen;
	/*
	 * 在读取数据的时候，需要offset变量
	 * 并且约定dmaWIndex为nCurMutiPtr
	 * readOffset表示nCurIndexNum
	 * 且realLen表示返回的长度，最大4096
	 * 
	 * 并且从内核返回后，dmaWIndex是nCurReadRound
	 * readOffset是nNextReadPtr，以便更新API中的指针
	*/
	int				readOffset;
	unsigned char   dData[4096];
};
#define 	FILE_DEVICE_UNKNOWN	'X'
#define 	CTL_CODE(A,B,C,D)	_IOWR(A,B,struct CVG_IOCTL_MAX_ST)



#define IO_CTRL_GET_LINKNAME CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x50, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从驱动获取硬件的基本能力信息，OutType---DeviceParam_st结构*/
#define IO_CTRL_GET_BASE_INFO CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x51, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*映射BAR空间，OutType---BarRes_st结构*/
#define IO_CTRL_MAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x52, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消BAR空间映射，无输入输出参数*/
#define IO_CTRL_UNMAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x53, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请上行通道的使用权限（独占），InType---unsigned int（chl），OutType---UpChlIoCtrl_st结构*/
#define IO_CTRL_GET_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x54, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放上行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x55, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请下行通道的使用权限（独占），InType---unsigned int（chl），OutType---DownChlIoCtrl_st结构*/
#define IO_CTRL_GET_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x56, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放下行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x57, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从用户层获取为某个通道创建的同步事件对象，用于通知上行的DMA通知通道有数据需要接收，
InType---unsigned int（index） + HANDLE（根据用户层程序的不同可能是32bit + 32bit或者是64bit，驱动会做合理性判断）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)
OutType---unsigned long long，返回值为该事件序号对应的计数器的内存地址，该计数器记录上次有信号到本次有信号之间，驱动实际给了多少次信号*/
#define IO_CTRL_SET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x58, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的某个上通道的事件对象，InType---unsigned int（index）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)*/
#define IO_CTRL_UNSET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x59, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的所有上通道的事件对象，无输入输出参数*/
#define IO_CTRL_UNSET_ALL_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5A, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)



/*获取驱动当前内核的相关调试信息，
OutType---CurDbg_st结构，该结构存储了内核中的一些调试相关计数等，每次获取将获得当前记录的值*/
#define IO_CTRL_GET_DBG_REC CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5B, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*复位所有上行DMA，无输入输出参数*/
#define IO_CTRL_RESET_ALL_UPCHL_DMA CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5C, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)

#define IO_CTRL_DOWN_DMA_PKG	CTL_CODE(\
			FILE_DEVICE_UNKNOWN,\
			0X5E,\
			METHOD_BUFFERED,\
			FILE_ANY_ACCESS)

#define IO_CTRL_READ_DMA_PKG	CTL_CODE(\
			FILE_DEVICE_UNKNOWN,\
			0X5F,\
			METHOD_BUFFERED,\
			FILE_ANY_ACCESS)


#elif defined(_BUILD_FOR_VXWORKS_)

struct 		CVG_IOCTL_MAX_ST{
	unsigned char dummy[4096];
};
#define 	FILE_DEVICE_UNKNOWN	'X'
#define 	CTL_CODE(A,B,C,D)	(A << 8 | B)



#define IO_CTRL_GET_LINKNAME CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x50, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从驱动获取硬件的基本能力信息，OutType---DeviceParam_st结构*/
#define IO_CTRL_GET_BASE_INFO CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x51, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*映射BAR空间，OutType---BarRes_st结构*/
#define IO_CTRL_MAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x52, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消BAR空间映射，无输入输出参数*/
#define IO_CTRL_UNMAP_BAR CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x53, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请上行通道的使用权限（独占），InType---unsigned int（chl），OutType---UpChlIoCtrl_st结构*/
#define IO_CTRL_GET_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x54, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放上行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_UPCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x55, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*申请下行通道的使用权限（独占），InType---unsigned int（chl），OutType---DownChlIoCtrl_st结构*/
#define IO_CTRL_GET_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x56, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*释放下行通道的使用权限，InType---unsigned int（chl）*/
#define IO_CTRL_RELEASE_DOWNCHL CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x57, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*从用户层获取为某个通道创建的同步事件对象，用于通知上行的DMA通知通道有数据需要接收，
InType---unsigned int（index） + HANDLE（根据用户层程序的不同可能是32bit + 32bit或者是64bit，驱动会做合理性判断）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)
OutType---unsigned long long，返回值为该事件序号对应的计数器的内存地址，该计数器记录上次有信号到本次有信号之间，驱动实际给了多少次信号*/
#define IO_CTRL_SET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x58, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的某个上通道的事件对象，InType---unsigned int（index）, index 从0到 (MAX_DEVICE_EVENT_NUM - 1)*/
#define IO_CTRL_UNSET_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x59, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*取消驱动引用的所有上通道的事件对象，无输入输出参数*/
#define IO_CTRL_UNSET_ALL_EVENT CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5A, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)



/*获取驱动当前内核的相关调试信息，
OutType---CurDbg_st结构，该结构存储了内核中的一些调试相关计数等，每次获取将获得当前记录的值*/
#define IO_CTRL_GET_DBG_REC CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5B, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)


/*复位所有上行DMA，无输入输出参数*/
#define IO_CTRL_RESET_ALL_UPCHL_DMA CTL_CODE(\
			FILE_DEVICE_UNKNOWN, \
			0x5C, \
			METHOD_BUFFERED, \
			FILE_ANY_ACCESS)
#endif

#endif


