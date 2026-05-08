
#include "../platform.h"
#include "../glkconfig.h"
#include "DownDmaRegister.h"
#include "DevResourceGet.h"
//#include "../PublicHead/FC_L0_API.h"
#include "DevOper.h"
#include "../third_party/rbtree.h"

#pragma pack (4)

 
/*
 上行DMA通道数据管理
*/
#if 0
typedef struct __DEV_UP_DMA_MANAGER_STRUCT_
{
	unsigned int unDmaBlockSize;
	unsigned int flagUpDmaTransCtrl;
	UpChlRes_st pChls ;//[MAX_DEV_UP_DMA_CHL];
	unsigned int nChlNum;
	volatile int nRunFlag;
	OS_HANDLE	hThread;
	funUpDmaCallBack pFunCallBack;
	void* pParamFun;	
	
} UpChlMng_st, *pUpChlMng_st;
#endif


/*
 * 与协议栈相关的函数	 
*/
struct GLK_PRO_OPS{
	PROTOCAL_START		protocal_start;
	PROTOCAL_STOP		protocal_stop;
	PROTOCAL_SCHCFG		protocal_cfgmsg;
	PROTOCAL_FILLDMAHEAD	protocal_fillDmaHeader;
	PROTOCAL_SEND_HOOK  protocal_sendhook;
};

/* 
 DMA通道管理的结构，包含上下行两个
*/
struct 	DEVICE_CHANNEL{
	/*
	 0X12344321
	*/
	unsigned int	validCode;
	/*
	 通道号
	*/
	int				channel;
	/*
	 通道上行线程锁信息
	*/
	OS_HANDLE	hThread;
	/*
	 上行回调函数
	*/
	funUpDmaCallBack pFunCallBack;
	/*
	 回调函数的用户上下文
	*/
	void* pParamFun;	
	volatile int nRunFlag;
	unsigned int unDmaBlockSize;
	unsigned int flagUpDmaTransCtrl;
	/*
	 * 在对当前通道上的所有消息进行管理时，会自动计算发送时的DDR偏移。使用ddrMemOffset管理
	 * 自动根据创建的消息数量和每个消息使用的板载内存进行管理
	*/
	unsigned int 	dn_ddrMemOffset;
	/*
	 *  在对当前通道上的所有消息进行管理时，当前通道占用DDR偏移的起始地址
	 */
	unsigned int	dn_ddrBaseOffset;
	/*
	 反向指针，指向DevObjMng_st
	*/
	void 	* 		devObjMng;
	/*
	 上行DMA通道管理
	*/
	UpChlRes_st		upChan;
	/*
	 下行DMA管理
	*/
	DownChlRes_st	DownChan;
	/*
	 协议栈相关的函数
	*/
	struct GLK_PRO_OPS proops;
	/*
	 消息管理
	*/
	int				scheTBSize;
	/*
	 *使用数组还是二叉树???
	*/
	struct SCHE_TB	*scheTb;
	/*
	 透传使用的缓冲
	*/
	unsigned char* DN_TxBuf;
#ifdef		USE_DCACHE_DATA_FOR_DC
	struct rb_root 	root;
#endif
	/*
	 * 根据OS_DMA_CNT_PER_SLEEP的配置
	 * 当前通道发送一定量的数据包后，简单释放CPU
	*/
	int		os_sleepCnt; 
	/*
	 针对DMA数据传输通道借用物理通道的场景使用
	*/
	OS_HANDLE	parent_dchandle;
};
/*
 内部使用的通道到设备DevObjMng的转换结构
*/
#define CHAN_2_DEV(X)	(((struct DEVICE_CHANNEL*)X)->devObjMng)



typedef void(*funEventCallBack)(unsigned int nEventIndex, int nTrigCount, void* context);

#ifdef WIN32
typedef struct __DEV_EVENT_CTRL_STRUCT_
{
	unsigned int nEventIndex;	
	volatile int nRunFlag;	

	volatile int* pCounter;

	HANDLE hEvent;		
    HANDLE hThread;		

	funEventCallBack pFunCallBack;	
	void* pParamFun;				

} EventCtrl_st, *pEventCtrl_st;

typedef struct _DEV_MEM_MAP_RECORD_WINDOWS_STRUCT_
{
	int NotUse;
	int NotUse1;

} MapRec_st, *pMapRec_st;		
#endif

#ifdef _BUILD_FOR_LINUX_
/*
 * 在某些linux系统上，看起来sem_t这个锁大小或者必须是按照特定的对齐方式进行内存排列，否则会崩溃，不同的内核版本可能有不同的要求，
 * 这儿统一处理
 * */
#define CVG_SEM_PAD	(sizeof(sem_t) - sizeof(unsigned int))
#endif

/*
 * 设备句柄
*/
typedef struct __DEV_OBJECT_MANAGER_STRUCT_	
{
	/*
	 0X43214321
	*/
	unsigned int	validCode;
#ifdef _BUILD_FOR_LINUX_
	unsigned char   pad[CVG_SEM_PAD];
#endif
	OS_MUTEX hMutexObj;
	DEVICE_HANDLE hDev;	
	DeviceParam_st devParam;
	BarRes_st barResource;		
	/*
	 * 板载内存的总大小。OnBoardMemSize
	*/
	int			OBMS;
	/*
	 管理申请内存时的当前指针
	*/
	int			MemMallocPtr;

#if 0	
	/*
	 内存管理模式
	*/
	int			MemManMode;

	/*
	 把DMA通道信息与设备剥离
	*/
	UpChlMng_st upChlMng;		
	DownChlRes_st pDownChls[MAX_DEV_DOWN_DMA_CHL];	
	OS_MUTEX pLockDownChls[MAX_DEV_DOWN_DMA_CHL];	
	int flagMutexInited[MAX_DEV_DOWN_DMA_CHL];	
#endif
} DevObjMng_st, *pDevObjMng_st;

#pragma pack ()

#define		DEVICE_VALID_CODE		(0X43214321)
#define		CHAN_VALID_CODE			(0X12341234)
#define		CHILD_CHAN_VALID_CODE	(0X1234FFFF)
/*
 判断设备句柄是否合法
*/
#define		DEVERROR(X)	(!((X) &&  (*(unsigned int*)X)== DEVICE_VALID_CODE))
/*
判断通道句柄是否合法
*/
#define		CHSERROR(X)	(!((X) &&  (*(unsigned int*)X)== CHAN_VALID_CODE))

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 原先的注释都出现的编码错误，临时删除
*/
OS_UINT Dev_ScanLocalDevCnt();
DEVICE_HANDLE Dev_OpenDevByIndex(unsigned int nDevIndex);
DEVICE_HANDLE Dev_OpenDevByName(char* strDevName);
void Dev_CloseDev(DEVICE_HANDLE hDev);
int Dev_GetBaseParam(DEVICE_HANDLE hDev, pDeviceParam_st pDevParam);
int Dev_MapBarResource(DEVICE_HANDLE hDev, pBarRes_st pBarRes);
void Dev_UnmapBarResource(DEVICE_HANDLE hDev, pBarRes_st pBarRes);
int Dev_WriteBarRegister32(pBarRes_st pBarRes, unsigned int nBar, unsigned int nOffset, unsigned int nWrite);
int Dev_ReadBarRegister32(pBarRes_st pBarRes, unsigned int nBar, unsigned int nOffset, unsigned int* pRead);
int Dev_GetUpDmaChl(DEVICE_HANDLE hDev, pUpChlRes_st pChlInfo);
int Dev_ReleaseUpDmaChl(DEVICE_HANDLE hDev, pUpChlRes_st pChlInfo);
int Dev_ResetAllDmaChl(OS_HANDLE hDev);
/*
 函数说明：启动DMA上行管理线程
	参数：
		@1：通道句柄
		@2：设备管理参数
		@3：回调函数
		@3：回调函数用户上下文
*/	
int Dev_StartUpDmaThread(struct DEVICE_CHANNEL * pMng, pDeviceParam_st pDevParam, funUpDmaCallBack pFunCallBack, void* pParamUser);
/*
 函数说明：停止上行DMA
*/
void Dev_StopUpDmaThread(struct DEVICE_CHANNEL * pMng);
void Dev_OpenCloseUpDmaChl(pUpChlRes_st pChlInfo, int nFlagOpen);
int Dev_GetDownDmaChl(DEVICE_HANDLE hDev, pDeviceParam_st pDevParam, pDownChlRes_st pChlInfo);
int Dev_ReleaseDownDmaChl(DEVICE_HANDLE hDev, pDownChlRes_st pChlInfo);
unsigned int Dev_GetDownDmaChlFree(pDownChlRes_st pChlInfo, pBarRes_st pBarRes, int flagReadReg);
int Dev_DownDmaPack(OS_HANDLE dev, pDownChlRes_st pChlInfo, pBarRes_st pBarRes, unsigned char* pPack, unsigned int nPackLen);
int Dev_DownDmaPack_WithRetry(OS_HANDLE dev, pDownChlRes_st pChlInfo, pBarRes_st pBarRes, unsigned char* pPack, unsigned int nPackLen, int RetryCnt, int RetrySp);

#if defined(_WIN32)
int Dev_OpenEvent(DEVICE_HANDLE hDev, unsigned int nEventIndex, funEventCallBack pFunCallBack, void* pParamUser, pEventCtrl_st pCtrl);
int Dev_CloseEvent(DEVICE_HANDLE hDev, pEventCtrl_st pCtrl);
#endif

inline void CopyMemoryFastCache1(void* pDest, void* pSour, unsigned int nLen)
{
	//unsigned char pTestMem[4096];
//        memcpy(pDest,pSour,nLen);
}


//inline void CopyMemoryFastCache(void* pDest, void* pSour, unsigned int nLen)
#define CopyMemoryFastCache(pDest, pSour, nLen) { \
	volatile unsigned char* pVolDest = (unsigned char*)pDest;\
	volatile unsigned char* pVolSour = (unsigned char*)pSour;\
	volatile unsigned long long  * pCopy = NULL;		\
	unsigned int nCopyDword = 0;\
	unsigned int nLeft = 0;	\
	unsigned int i = 0;\
	nCopyDword = nLen / (sizeof(unsigned long long) * 8);\
	nLeft = nLen % (sizeof(long long) * 8);\
	pCopy = (unsigned long long *)pVolDest;\
	for(i = 0; i < nCopyDword ; i ++){\
		pCopy[i]   = ((volatile unsigned long long *)pVolSour)[i];\
		pCopy[i+1] = ((volatile unsigned long long *)pVolSour)[i+1];\
		pCopy[i+2] = ((volatile unsigned long long *)pVolSour)[i+2];\
		pCopy[i+3] = ((volatile unsigned long long *)pVolSour)[i+3];\
		pCopy[i+4] = ((volatile unsigned long long *)pVolSour)[i+4];\
		pCopy[i+5] = ((volatile unsigned long long *)pVolSour)[i+5];\
		pCopy[i+6] = ((volatile unsigned long long *)pVolSour)[i+6];\
		pCopy[i+7] = ((volatile unsigned long long *)pVolSour)[i+7];\
		i += 8;\
	}\
	pVolDest += nCopyDword * sizeof(unsigned long long) * 8 ;\
	pVolSour += nCopyDword * sizeof(unsigned long long) * 8;\
	for (i = 0; i < nLeft; i++)\
	{\
		pVolDest[i] = pVolSour[i];\
	}\
}

#define __os_memcpy(A,B,C) memcpy(A,B,C)//CopyMemoryFastCache(A,B,C)

#if	(CAVIGE_CPU_ARCH == FTD2000)
inline void asm_memcpy(volatile void * dest,volatile void *src,int count)
{
	if (count & 63) {
		count = (count & -64) + 64;
	}
	asm volatile (
			"NEONCopyPLD: \n"
			"sub %[src], %[src], #32 \n"
			"sub %[dest], %[dest], #32 \n"
			"1: \n"
			"ldp q0, q1, [%[src], #32] \n"
			"ldp q2, q3, [%[src], #64]! \n"
			"subs %[count], %[count], #64 \n"
			"stp q0, q1, [%[dest], #32] \n"
			"stp q2, q3, [%[dest], #64]! \n"
			"b.gt 1b \n"
			: [dest]"+r"(dest), [src]"+r"(src), [count]"+r"(count) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}
#endif	//(CAVIGE_CPU_ARCH == FTD2000)
#ifdef _BUILD_FOR_LINUX_
	#if	(CAVIGE_CPU_ARCH == FTD2000)
		#define __map_memcpy(A,B,C) asm_memcpy(A,B,C)
	#elif (CAVIGE_CPU_ARCH == LOONSON3A5000)
		#define __map_memcpy(A,B,C) memcpy(A,B,C)
	#endif
#elif defined(_BUILD_FOR_VXWORKS_)
	#define __map_memcpy(A,B,C) CopyMemoryFastCache(A,B,C)
#elif defined(_WINDOWS_)
	#define __map_memcpy(A,B,C) memcpy(A,B,C)
#endif
#ifdef __cplusplus
}
#endif

