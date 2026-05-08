
#pragma once

//#include "CommonStruct.h"

#include "DriverInterface.h"



typedef struct __DEV_BAR_RESOURCE_STRUCT__
{
	unsigned int nBarSize[MAX_USED_BAR_NUM];
	unsigned char* pBarAddr[MAX_USED_BAR_NUM];

} BarRes_st, *pBarRes_st;


typedef struct __DEV_UP_CHL_CTRL_STRUCT_
{
	unsigned int flagValid;	
	unsigned int Reserved;
	unsigned int nChl;		

	int nIndexNum;		
	int nBlockNum;		

	pUpDmaRun_st pRunCtrl;	

	pUpIndexNode_st pIndex;	
	unsigned char* pBlocks[MAX_UP_DMA_BLOCK_NUM_PER_CHL];
#if defined(_BUILD_FOR_LINUX_)|| defined(_BUILD_FOR_VXWORKS_)
	// descript each block for pBlocks;
	unsigned int    size_pBlocks[MAX_UP_DMA_BLOCK_NUM_PER_CHL];
	// descript size of pIndex
	int		size_pIndex ;
	// descript base Address of pRunCtrl,pRunCtrl is an offset address  based baseRunCtrl
	void *		baseRunCtrl;
#endif
	
	//Dma_Map_Info axiUpChlRes;
} UpChlRes_st, *pUpChlRes_st;

typedef struct __DEV_DOWN_CHL_CTRL_STRUCT_
{
	unsigned int flagValid;	
	unsigned int nChl;		
	unsigned int flag_x64;	
	unsigned int reserved;
	unsigned int nIndexNum;

	pDownDmaRun_st pRunCtrl;
	pDownIndexNode_st pIndex;

	unsigned char* pBlocks[MAX_DOWN_DMA_BLOCK_NUM_PER_CHL];	
#if defined(_BUILD_FOR_LINUX_)|| defined(_BUILD_FOR_VXWORKS_)
	// descript each block for pBlocks;
	unsigned int    size_pBlocks[MAX_UP_DMA_BLOCK_NUM_PER_CHL];
	// descript size of pIndex
	int		size_pIndex ;
	// descript base Address of pRunCtrl,pRunCtrl is an offset address  based baseRunCtrl
	void *		baseRunCtrl;
#endif

	
	//Dma_Map_Info axiDnChlRes;
} DownChlRes_st, *pDownChlRes_st;
