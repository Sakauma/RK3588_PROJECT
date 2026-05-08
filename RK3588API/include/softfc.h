#ifndef DN_DIRECT_H
#define DN_DIRECT_H
#include <string.h>
#include "../platform.h"
#include "../glkconfig.h"
#include "fc_macro_def.h"

/*
 该文件定义，软件封装的FC报文，并通过DMA透传发送使用
*/
/*
 按照FC头大端格式定义
*/
struct CVG_FC_HEADER{
	/*DW0*/
	unsigned int	nSOF;			//0xbcb55656
	unsigned int	nR_CTL : 8;
	unsigned int	nDID : 24;
	/*DW1*/
	unsigned int	nCS_CTL : 8;
	unsigned int	nSID : 24;
	/*DW2*/
	unsigned int	nTYPE : 8;
	unsigned int	nF_CTL : 24;
	/*DW3*/
	unsigned int 	nSEQ_ID : 8;
	unsigned int	nDF_CTL : 8;
	unsigned int	nSEQ_CNT : 16;
	/*DW4*/
	unsigned int	nOX_ID : 16;
	unsigned int	nRX_ID : 16;
	/*DW5*/
	unsigned int	 nParam;
};


//通用fc帧头,不包含SOF
typedef struct fc_head_st
{
	unsigned int	nR_CTL : 8;
	unsigned int	nDID : 24;

	unsigned int	nCS_CTL : 8;
	unsigned int	nSID : 24;

	unsigned int	nTYPE : 8;
	unsigned int	nF_CTL : 24;

	unsigned int 	nSEQ_ID : 8;
	unsigned int	nDF_CTL : 8;
	unsigned int	nSEQ_CNT : 16;


	unsigned int	nOX_ID : 16;
	unsigned int	nRX_ID : 16;

	unsigned int	 nParam;

}COM_FC_Head_st_NOSOF, * pCOM_FC_Head_st_NOSOF;

/*
 * 透传时使用的ASM包头
*/
struct CVG_ASM_HEADER{
	unsigned int  nMsgID;
	unsigned int  Security;
	unsigned int  Reserved;
	unsigned int  nL : 1;
	unsigned int  nPrio : 7;
	unsigned int  nLen : 24;
};



//Class Service Parameters 定义
typedef struct
{
	unsigned int nServiceOptions : 16;
	unsigned int nInitiatorCtl : 16;

	unsigned int nRecipientCtl : 16;
	unsigned int nRecvDataFileSize : 16;

	unsigned int nTotalSequences : 16;
	unsigned int nNPort_EECredit : 16;

	unsigned int nOpenSeqPerExchange : 16;
	unsigned int nReserved : 16;

}LOGI_Class_Service_Param_st;

typedef struct
{
	unsigned int nFlags : 8;
	unsigned int nReserved1 : 24;

	unsigned int nReserved2;
	unsigned int nReserved3;

	unsigned int nReserved4 : 16;
	unsigned int nNPIV_CNT : 16;
}Auxiliary_Parameter_Data_st, * PAuxiliary_Parameter_Data_st;


//ELS FLOGI、PLOGI、FDISC、PLOGI LS_ACC、FLOGI LS_ACC、FDISC LS_ACC负载数据格式定义
typedef struct els_multiplex_payload_st
{
	unsigned int	CommandCode;		//ELS Command Code

	unsigned char	ServiceParam[16];	//服务参数，区分FLOGI、PLOGI、LS_ACC

	unsigned char 	PortName[8];
	unsigned char 	NodeName[8];

	unsigned int	Obsolete[4];

	LOGI_Class_Service_Param_st Class2Param;
	LOGI_Class_Service_Param_st Class3Param;

	Auxiliary_Parameter_Data_st AuxiliaryParam;
	unsigned int 				VendorVersion[4];

}ELS_Multiplex_Payload_st, * PELS_Multiplex_Payload_st;


typedef struct down_dma_head
{
	unsigned int nSOF;			//0xaaaa_5555	
	unsigned int nPortID : 12;
	unsigned int nMsgMode : 4;
	unsigned int nTxPhyPort : 4;
	unsigned int nPrio : 4;
	unsigned int nProtol : 8;
	unsigned int nDataLen_L : 32;
	unsigned int nDataLen_H : 24;
	unsigned int nMsgSeq : 7;
	unsigned int nCrcMode : 1;
	unsigned int reserved[5];//全0
}Down_Dma_Head_st, * pDown_Dma_Head_st;

#define BUILD_FC_HEADER(MEM,R_CTL,DID,CS_CTL,SID,TYPE,F_CTL,SEQ_ID,DF_CTL,SEQ_Cnt,OX_ID,RX_ID,param) {\
	if(MEM){\
		struct CVG_FC_HEADER * cfh = (struct CVG_FC_HEADER*)MEM;\
		cfh->nSOF = 0x5656b5bc;\
		cfh->nR_CTL = R_CTL;\
		cfh->nDID = __hton3b__(DID);\
		cfh->nCS_CTL = CS_CTL;\
		cfh->nSID = __hton3b__(SID);\
		cfh->nTYPE = TYPE;\
		cfh->nF_CTL = __hton3b__(F_CTL);\
		cfh->nSEQ_ID = SEQ_ID;\
		cfh->nDF_CTL = DF_CTL;\
		cfh->nSEQ_CNT = __htons__(SEQ_Cnt);\
		cfh->nOX_ID = __htons__(OX_ID);\
		cfh->nRX_ID = __htons__(RX_ID);\
		cfh->nParam = param;\
	}\
}

#define BUILD_ASM_HEADER(MEM,MSGID,SECURITY,MSGLEN,PRIORITY) {\
	if (MEM) {\
		struct CVG_ASM_HEADER* _asm_ = (struct CVG_ASM_HEADER*)((unsigned char*)MEM + sizeof(struct CVG_FC_HEADER));\
		_asm_->nMsgID = __htonl__(MSGID);\
		_asm_->Security = SECURITY;\
		_asm_->nPrio =  PRIORITY >> 1;\
		_asm_->nL = ((MSGLEN & 0x1000000) != 0);\
		_asm_->nLen = __hton3b__(MSGLEN & 0xFFFFFF);\
	}\
}



#define BUILD_ELS_LOGIN_FRAME(MEM,nPhyPort) {\
	memset(MEM, 0, SOFT_ELS_LOGIN_SIZE);\
	unsigned int nMsgSeq = (1 << MSG_SEQ_START_MSG_SHIFT) | (1 << MSG_SEQ_START_QUEUE_SHIFT) | (1 << MSG_SEQ_END_MSG_SHIFT) | (1 << MSG_SEQ_END_QUEUE_SHIFT);\
	pDown_Dma_Head_st  dmaHead = (pDown_Dma_Head_st)MEM;\
	dmaHead->nSOF = DN_DMA_HEAD_SOF;\
	dmaHead->nCrcMode = 0;\
	dmaHead->nMsgMode = DN_MSG_MODE_DIRECT;\
	dmaHead->nDataLen_H = 0;\
	dmaHead->nDataLen_L = SOFT_ELS_LOGIN_SIZE;\
	dmaHead->nPortID = nPhyPort;\
	dmaHead->nProtol = FC_HEAD_TYPE_ELS;\
	dmaHead->nMsgSeq = nMsgSeq;\
	pCOM_FC_Head_st_NOSOF fcHead = (pCOM_FC_Head_st_NOSOF)((unsigned char * )MEM + sizeof(Down_Dma_Head_st));\
	fcHead->nSID = 0x0;				\
	fcHead->nDID = 0xfffffe;\
	fcHead->nSEQ_CNT = 0;\
	fcHead->nR_CTL = FC_HEAD_R_CTL_ELS_REQ;\
	fcHead->nTYPE = FC_HEAD_TYPE_ELS;\
	fcHead->nF_CTL = 0x290008;	\
	PELS_Multiplex_Payload_st  els = (PELS_Multiplex_Payload_st)((unsigned char *)MEM + sizeof(Down_Dma_Head_st) + sizeof(COM_FC_Head_st_NOSOF));\
	els->CommandCode = ELS_CMD_CODE_FLOGI;\
}


#define SOFT_ASM_HEADER_SIZE	sizeof(struct CVG_ASM_HEADER)
#define SOFT_FC_HEADER_SIZE		sizeof(struct CVG_FC_HEADER)
#define SOFT_ELS_LOGIN_SIZE		(sizeof(ELS_Multiplex_Payload_st) + sizeof(COM_FC_Head_st_NOSOF) + sizeof(Down_Dma_Head_st))
#endif
