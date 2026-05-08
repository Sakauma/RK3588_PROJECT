#ifndef GLKCONFIG_H
#define GLKCONFIG_H
#include <stdio.h>

#include "platform.h"

/*
 GLK200 ASM下沉版本，需要在portable文件夹中定义相关的函数
*/
/*
 是否使用ASM传输视频数据，针对25S的需求,如果使用了，就需要保留出来2个128MB内存
*/
#define GLK_ASMTRANS_IMG

#define GLK_FC_ASM
//#define GLK_FC_AE_1553


#if defined(GLK_GLINK)
/*
 如果定义了GLINK，强制开启FC1553
*/
#define GLK_FC_AE_1553
#endif

#if defined(GLK_FC_ASM) && defined(GLK_FC_AE_1553)
#error "do not define ASM and 1553 at same time"
#endif
/*
 协议栈启动,设备句柄
*/
typedef void (*PROTOCAL_START)(OS_HANDLE pBHandle, int Chs);
/*
 协议栈停止,设备句柄
*/
typedef void (*PROTOCAL_STOP)(OS_HANDLE pBHandle, int Chs);
/*
 调度和消息配置,设备句柄
*/
typedef void (*PROTOCAL_SCHCFG)(OS_HANDLE pBHandle,int Chs, struct SCHE_TB* tb);

/*
 * 主要适用于透传的处理，在透传处理中，主要需要为DMA字段的SUP和WT两个字段进行合理的填充。
 * 其中SUPWT是在API中代入的
*/
typedef void (*PROTOCAL_SEND_HOOK)(unsigned char* pSendMem,int SUPWT);


/*
 * 填充DMA头，每种协议类型的DMA头不太一样，因此交给各协议模块自己填充，其中
 * DMA头大小是DMA_HAEDER_P0SIZE/PNSIZE）
 * 参数如下：
 *		@1：第一个是调度表的指针。在调度表中包含消息头信息，以及消息的结构体内容。
 *			根据各协议的定义，可以转换出来其细节。
 *		@2：把DMA头填充到的目的区域
 *		@3: 是否是最后一包,Last Package,1:表示是最后一包，0：不是最后一包
 *		@4：是否是第一包,First Package，1：是第一包，0：不是第一包
 *		@5：总长度，不是一个DMA的长度
*/
typedef void (*PROTOCAL_FILLDMAHEAD)(struct SCHE_TB * tb,void * memPtr,int LP,int FP, int PayloadLen);

/*
 * 是否在通道上使用二叉树管理消息数据缓存
 * 例如如果需要在通道上为每个消息缓存图像（假如图像的使用场景）
*/
#define		USE_DCACHE_DATA_FOR_DC

#define		DMA_HEADER_BIG_ENDIAN

#if defined(GLK_FC_ASM)
	#define 	MSGTB_SIZE	(sizeof(struct ASM_MSGDESC))
	#define 	DMA_HEAD_P0SIZE	(sizeof(struct ASM_T_DMA_HEADER_P0))
	#define 	DMA_HEAD_PNSIZE	(sizeof(struct ASM_T_DMA_HEADER_P1))
	#define		DMA_UP_FIXHEAD_SIZE	(sizeof(struct ASM_R_DMA_FIX_HEADER))
	/*
	 透传使用的DMA头大小
	*/
	//#define		DN_DIRECT_HEAD_SIZE	(sizeof(struct CVG_FC_HEADER) + sizeof(struct CVG_ASM_HEADER))

	#include "./include/glkasm.h"
#elif defined(GLK_FC_AE_1553)

	#define 	MSGTB_SIZE	(sizeof(struct FC_1553_MSG))
	#define 	DMA_HEAD_P0SIZE	(sizeof(struct FC1553_T_DMA_HEADER_P0))
	#define 	DMA_HEAD_PNSIZE	(sizeof(struct FC1553_T_DMA_HEADER_P1))
	#define		DMA_UP_FIXHEAD_SIZE	(sizeof(struct FC1553_R_DMA_FIX_HEADER))
	/*
	 透传使用的DMA头大小
	*/
	//#define		DN_DIRECT_HEAD_SIZE	(sizeof(struct CVG_FC_HEADER) )
	#if defined(GLK_GLINK)
	#include "./include/glkglink.h"
	#endif
	#include "./include/glkfc1553.h"
#endif

#ifdef GLK_ASMTRANS_IMG
	
	#define IMG_DMA_HEAD_SIZE	(sizeof(struct CVG_GLKIMG_DMAHD))
	/*
	 把切图地址固定放在最后地址空间
	*/
	#define RESERV_MEM_FIX_MODE	
	#define		RESERV_MEM_FIX_ADDR	(0X30000000)
#ifdef RESERV_MEM_FIX_MODE
	#define IMG_DMA_RESERVE_MEM_SIZE	(0)
#else
	/*
	 * 定义每个通道需要为图像切割保留的容量
	*/
	#define IMG_DMA_RESERVE_MEM_SIZE	(256*1024*1024)
#endif
	/*
	 * 定义需要保留几路切割图像的DMA通道
	*/
	#define IMG_DMA_CNT					(1)



	#include "./include/glkimg.h"

#else
	#define IMG_DMA_HEAD_SIZE				(0)
	#define IMG_DMA_RESERVE_MEM_SIZE	(0)
	#define IMG_DMA_CNT					(0)
#endif
	/*
	 * 在实时系统上，API要适当的控制发送DMA的频次。通道每发送一定的数量后，稍微释放一下CPU
	 * 根据不同的CPU架构和操作系统的性能，可以微调该值，以达到最优带宽
	 * 在华为920 处理器、麒麟系统上，光口速率4.25G时，50可以稳定控制在发送2.3Gbps[测试时长12小时]，100会导致下行DMA满的问题（未测试其他值）
	*/
#define OS_DMA_CNT_PER_SLEEP	(50)



#ifndef MSGTB_SIZE
#error "not define MSGTB_SIZE"
#endif
#ifdef DMA_HEADER_BIG_ENDIAN
#define __htonl__(X)	( (((X) & 0XFF000000) >>24) | (((X) & 0X00FF0000) >> 8) | (((X) & 0X0000FF00) << 8) | (((X) & 0X000000FF) << 24))
#define __hton3b__(X)	( (((X) & 0x000000FF) << 16) | (((X) & 0x0000FF00)) | (((X) & 0x00FF0000) >> 16))
#define __htons__(X)	( (((X & 0xFF00) >> 8) | ((X & 0x00FF) << 8) ))
#else
#define __htonl__(X)	(X)
#define __hton3b__(X)	(X)
#define __htons__(X)	(X)
#endif


#endif
