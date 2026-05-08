#ifndef PHYSICAL_LAYER_H
#define PHYSICAL_LAYER_H

#include "../platform.h"
#include "../glkconfig.h"
#include "ApiBase.h"


/*
 * 文件说明：该文件是原先的FC_L1_API.h中的移植代码，主要与physical设备相关
 * 在GLK版本上，设备句柄重新定义，并简化了原先的设备描述数据结构
 * 可以直接包含该头文件使用
*/

/*
 *功能说明：当前打开设备，是否支持NC、NT功能
 *参数说明：
		   hDev		已打开的通道句柄

 *返回值：
		   FC_TRUE		支持
		   FC_FALSE	不支持
*/
//OS_BOOL FC_Dev_Chl_IsSupportNC(OS_HANDLE pChHandle);
//OS_BOOL FC_Dev_Chl_IsSupportNT(OS_HANDLE pChHandle);

/*
  *功能说明：设置、获取端口速率 yanfei@add 2023-11-03
  *参数说明：
			hDev		设备句柄
			nBank      bank序号，每个bank上4个通道
			nPort		物理端口号
			emSpeed	端口速率
  *返回值：
		   FC_SUCCESS		成功
		   other			失败，参见错误码定义
*/
//FC_RESULT FC_Dev_GetBankNum(OS_HANDLE hDev, FC_DWORD* pValue);
FC_RESULT FC_Dev_SetPortSpeed_Bank(OS_HANDLE hDev, FC_WORD nBank, FC_WORD nCh, FC_WORD nPort, emFC_PORT_SPEED emSpeed);
FC_RESULT FC_Dev_GetPortSpeed_Bank(OS_HANDLE hDev, FC_WORD nBank, FC_WORD nCh, FC_WORD nPort, emFC_PORT_SPEED* pSpeed);
FC_RESULT FC_Dev_SetAllPortSpeed_Bank(OS_HANDLE hDev, FC_WORD nCh, emFC_PORT_SPEED emSpeed);
//KU add by lx GLink KU系列板卡
FC_RESULT FC_Dev_SetPortSpeed_KU(OS_HANDLE hDev, FC_WORD nCh, emFC_PORT_SPEED emSpeed);
FC_RESULT FC_Dev_SetAllPortSpeed_KU(OS_HANDLE hDev, emFC_PORT_SPEED emSpeed);
FC_RESULT FC_Dev_GetAllPortSpeed_KU(OS_HANDLE hDev, emFC_PORT_SPEED* pSpeed);
FC_RESULT FC_Dev_SetProtocol(OS_HANDLE hDev, FC_WORD nProtocol);


/*
*功能说明：总线切换 yanfei@add 2023-11-29
*参数说明：
		  hDev			设备句柄
		  systemMode	总线类型还是交换类型
*返回值：
		 FC_SUCCESS		成功
		 other			失败，参见错误码定义
*/
FC_RESULT FC_Dev_Set_SystemMode(OS_HANDLE hDev, emFCSYSTEMMODE systemMode);
FC_RESULT FC_Dev_Get_SystemMode(OS_HANDLE hDev, emFCSYSTEMMODE* systemMode);


/*
   8.5Gbps速率下，ARBF原语使能
*/
FC_RESULT FC_Dev_En_PortARBF(OS_HANDLE hDev, FC_WORD nPort, FC_BOOL bEn);
FC_RESULT FC_Dev_IsEn_PortARBF(OS_HANDLE hDev, FC_WORD nPort, FC_BOOL* pEn);

/*
   8.5Gbps速率下，加扰使能
*/
FC_RESULT FC_Dev_En_Scrambling(OS_HANDLE hDev, FC_BOOL bEn);
FC_RESULT FC_Dev_IsEn_Scrambling(OS_HANDLE hDev, FC_BOOL* pEn);

FC_RESULT FC_Dev_Set_ScramblingMode(OS_HANDLE hDev, FC_WORD nPort, emGT_SCR_MODE bMode);
FC_RESULT FC_Dev_Get_ScramblingMode(OS_HANDLE hDev, FC_WORD nPort, emGT_SCR_MODE* pMode);

FC_RESULT FC_Dev_Set_PONMode(OS_HANDLE hDev, emFC_PON_MODE emMode);
FC_RESULT FC_Dev_Get_PONMode(OS_HANDLE hDev, emFC_PON_MODE* pMode);

FC_RESULT FC_Dev_En_PON_OLTLoop(OS_HANDLE hDev, FC_WORD nPort, FC_BOOL bEn);
FC_RESULT FC_Dev_IsEn_PON_OLTLoop(OS_HANDLE hDev, FC_WORD nPort, FC_BOOL* pEn);

FC_RESULT FC_Dev_SetPONSpeed(OS_HANDLE hDev, emFC_PORT_SPEED emSpeed);

FC_RESULT FC_Dev_SetPortScramble(OS_HANDLE hDev, FC_WORD nBank, FC_WORD nCh, FC_WORD nPort, FC_BOOL bEn);//通道加扰


 /*
  *功能说明：复位逻辑协议固件
  *参数说明：
			hDev		已打开的设备句柄

  *返回值：
			FC_ERR_INVALID_HANDLE	无效的句柄
			FC_SUCCESS					成功
 */
FC_RESULT FC_Dev_SystemReset(OS_HANDLE hDev);

/*
 *功能说明：复位端口，重新发起链路恢复协议
 *参数说明：
		   hDev		已打开的设备句柄
		   nPort		复位Port

 *返回值：
		   FC_ERR_INVALID_HANDLE	无效的句柄
		   FC_SUCCESS				成功
*/
FC_RESULT FC_Dev_Reset_Port(OS_HANDLE hDev, FC_WORD nPort);
#define 	FC_Dev_GTReset_Port		FC_Dev_Reset_Port

/*
*功能说明：MAC复位
*参数说明：
		  hDev		已打开的设备句柄
		  stationindex		站点ID

*返回值：
		  FC_ERR_INVALID_HANDLE	无效的句柄
		  FC_SUCCESS				成功
*/
FC_RESULT FC_Dev_MAC_Reset_Port(OS_HANDLE hDev, FC_WORD stationindex);

/*
  *功能说明：复位所有端口
  *参数说明：
			hDev		已打开的设备句柄

  *返回值：
		   FC_ERR_INVALID_HANDLE	无效的句柄
		   FC_SUCCESS				成功
 */
FC_RESULT FC_Dev_Reset_AllPort(OS_HANDLE hDev);
#define 	cvg_card_GTReset_All	FC_Dev_Reset_AllPort

/*yanfei@add2023-12-13
*功能说明：复位DMA,调用这个接口之前先停止上位机操作，调用之后需要重新调用初始化接口
*参数说明：
		  hDev		已打开的设备句柄

*返回值：
		 FC_ERR_INVALID_HANDLE	无效的句柄
		 FC_SUCCESS				成功
*/
FC_RESULT FC_Dev_Reset_DMA(OS_HANDLE hDev);
#define 	cvg_card_Dma_ResetAll	FC_Dev_Reset_DMA

/*yanfei@add2023-12-13
*功能说明：获取板卡状态信息
*参数说明：
		  hDev		已打开的设备句柄
		  hardInfo  硬件状态信息
*返回值：
		 FC_ERR_INVALID_HANDLE	无效的句柄
		 FC_SUCCESS				成功
*/
FC_RESULT FC_Dev_Get_HardWareInfo(OS_HANDLE hDev, pFC_HardWareInfo_st hardInfo);

//yanfei@add2023-12-13 测频接口
FC_RESULT FC_Dev_StartTestFreq(OS_HANDLE hDev, FC_DWORD nPort);//设置测频通道
//调用FC_Dev_StartTestFreq后，等待2s再调用该接口，方可获取到实际的端口速率
FC_RESULT FC_Dev_GetClockSpeed(OS_HANDLE hDev, FC_WORD nPort, float* pSpeed);


/*
  *功能说明：复位所有端口
  *参数说明：
		hDev		已打开的设备句柄
		nPort		Port索引
		pState		端口状态信息:

  *返回值：
		  FC_ERR_INVALID_HANDLE	无效的句柄
		  FC_SUCCESS				成功
 */
FC_RESULT FC_Dev_Get_PortState(OS_HANDLE hDev, const FC_DWORD stationindex, FC_WORD nPort, emDevPortState* pState);

FC_RESULT FC_Dev_Get_PortLinkState(OS_HANDLE hDev, const FC_DWORD stationindex, FC_WORD nPot, FC_DWORD* pLinkState);

FC_RESULT Dev_Get_AllPortLinkState(OS_HANDLE hDev, FC_DWORD* pLinkState);

//端口链接异常计数 yanfei@add 2023-12-14
FC_RESULT Dev_Get_PortLinkOfflineCnt(OS_HANDLE hDev, const FC_DWORD stationindex, FC_WORD nPort, FC_DWORD* noffCnt);



/*
 *功能说明：设置板卡工作模式，针对FC-AE-1553节点卡有效
 *参数说明：
			hDev	已打开的设备句柄
			emMode	模式配置，独立模式或2通道冗余模式
*/
FC_RESULT FC_Dev_Set_RedunMode(OS_HANDLE hDev, emFCChlMode emMode);

/*
 *功能说明：获取板卡工作模式，针对FC-AE-1553节点卡有效
 *参数说明：
			hDev	已打开的设备句柄
			pMode	模式配置，独立模式或2通道冗余模式
*/
FC_RESULT FC_Dev_Get_RedunMode(OS_HANDLE hDev, emFCChlMode* pMode);


/*
 *功能说明：获取板卡的通道个数，针对监控软件有效
 *参数说明：
			hDev	已打开的设备句柄
			pDMAValue  寄存器返回的DMA通道个数
			pGTValue   寄存器返回的GT光口个数
			pBankNumValue 寄存器返回的Bank个数
			pPortNumValue  寄存器返回的DMA通道的端口数，冗余为2，非冗余为1
*/
FC_RESULT FC_Dev_GetChannelNum(OS_HANDLE hDev, FC_DWORD* pDMAValue, FC_DWORD* pGTValue, FC_DWORD* pBankNumValue, FC_DWORD* pPortNumValue);

//add by lx 20240417 GLINK仿真相关函数
FC_RESULT FC_Dev_Set_Ch_GTPort(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD gtport);
FC_RESULT FC_Dev_Get_Ch_GTPort(OS_HANDLE hDev, const FC_DWORD stationindex, FC_DWORD* gtport);
FC_RESULT FC_Dev_Set_Ch_Protocol_GTPort(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD protocol, const FC_DWORD gtport);
FC_RESULT FC_Dev_Get_Ch_Protocol_GTPort(OS_HANDLE hDev, const FC_DWORD stationindex, FC_DWORD* protocol, FC_DWORD* gtport);
FC_RESULT FC_Dev_Get_Ch_ExChangeInfo(OS_HANDLE hDev, const FC_DWORD stationindex,
	FC_DWORD* downdmanum, FC_DWORD* updmanum, FC_DWORD* macreceivenum, FC_DWORD* macsendnum);
FC_RESULT FC_Dev_Get_Port_FrameInfo(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD port, FC_FrameInfo_st* framinfo);
FC_RESULT FC_Dev_Set_DMA_TestMode(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD testmode);
FC_RESULT FC_Dev_Ch_Reset(OS_HANDLE hDev, const FC_DWORD stationindex);
FC_RESULT FC_Dev_Bank_Reset(OS_HANDLE hDev, const FC_DWORD bank);
FC_RESULT FC_Dev_Set_BBEnable(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD port, const FC_DWORD enable);
FC_RESULT FC_Dev_Get_BBEnable(OS_HANDLE hDev, const FC_DWORD stationindex, const FC_DWORD port, FC_DWORD* enable);
FC_RESULT FC_Dev_Set_TimeSyn(OS_HANDLE hDev, const FC_DWORD timesynvalue, const FC_DWORD synerrorvalue, const FC_DWORD triggermode);
FC_RESULT FC_Dev_Get_TimeSyn_Fre(OS_HANDLE hDev, FC_DWORD* timesynfre);
/*
 参数是通道句柄，不是设备句柄
*/
FC_RESULT FC_Dev_Ch_Set_TimeMode(OS_HANDLE hChl, const FC_DWORD timemode);

/*
 *  已废弃函数，使用
 * cvg_EnhancePro_set
 * cvg_EnhancePro_get
 */
//FC_RESULT  FC_Chl_SetProtocol(OS_HANDLE hChl, FC_WORD nProtocol);
//FC_RESULT  FC_Chl_GetProtocol(OS_HANDLE hChl, FC_WORD* nProtocol);

#endif

