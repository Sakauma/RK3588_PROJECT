#ifndef GLKFC1553_H
#define GLKFC1553_H
#include "../platform.h"
#include "../glkconfig.h"

/*
 FC-AE-1553增强协议设置
*/
#define	FC1553_ENHANCE_PRO_ABTS_EN	(1 << 0)
/*
 * 参考寄存器手册
*/
#define FC1553_SET_ENHANCED_DEFAULT_FALUE	(0X000C)


/*
 需要为FC1553 配置消息时填入的CmdFlag字段
*/
#define 	FC1553_FCMD_CMD_WITH_DATA		(1 << 7)
#define 	FC1553_FCMD_NT_BURST_SIZE_REQ	(1 << 6)
#define 	FC1553_FCMD_DT_BURST_SIZE_REQ	(1 << 5)
#define 	FC1553_FCMD_SUPPRESS_STATUS		(1 << 4)
#define 	FC1553_FCMD_NT_NT_TRANS			(1 << 3)
#define 	FC1553_FCMD_TR_TRANS			(1 << 2)
#define 	FC1553_FCMD_NC_MON_FORNTNT		(1 << 1)
#define 	FC1553_FCMD_MUITICAST			(1 << 0)

/*
 * T/R:当该位为0时，RT 执行发送数据操作
 * T/R:当该位为1时，RT 执行接收数据操作
*/
#define 	FC1553_CMD_NCNT(Msg)	{\
	Msg.CmdFlag &= ~(FC1553_FCMD_TR_TRANS | FC1553_FCMD_NT_NT_TRANS);\
	Msg.CmdFlag |= FC1553_FCMD_TR_TRANS;\
}

#define 	FC1553_CMD_NTNC(Msg) {\
	Msg.CmdFlag &= ~(FC1553_FCMD_TR_TRANS | FC1553_FCMD_NT_NT_TRANS);\
}

#define 	FC1553_CMD_NTNT(Msg) {\
	Msg.CmdFlag &= ~(FC1553_FCMD_TR_TRANS | FC1553_FCMD_NT_NT_TRANS);\
	Msg.CmdFlag |= FC1553_FCMD_NT_NT_TRANS;\
}


#define		FC1553_CMD_WITHDATA(Msg)	{\
	Msg.CmdFlag |= FC1553_FCMD_CMD_WITH_DATA;\
}
	
/*
 FC1553错误屏蔽码
*/
/*
 NC错误屏蔽码
*/
#define 	FC1553_NC_ERRMASK_B0	(1 << 0)		//Bit[0]：定时器0超时
#define 	FC1553_NC_ERRMASK_B1	(1 << 1)		//Bit[1]：定时器1超时
#define 	FC1553_NC_ERRMASK_B2	(1 << 2)		//Bit[2]：定时器2超时
#define 	FC1553_NC_ERRMASK_B3	(1 << 3)		//Bit[3]：交换过程中，收到错误的包(CRC错误，极性错误，长度错误等)
#define 	FC1553_NC_ERRMASK_B4	(1 << 4)		//Bit[4]：期望接收状态序列时出现意外
#define 	FC1553_NC_ERRMASK_B5	(1 << 5)		//Bit[5]：期望接收数据序列时出现意外
#define 	FC1553_NC_ERRMASK_B6	(1 << 6)		//Bit[6]：期望接收数据或者状态序列时出现意外(NT-NT的传输)
#define 	FC1553_NC_ERRMASK_B7	(1 << 7)		//Bit[7]：期望接收命令序列时出现意外
#define 	FC1553_NC_ERRMASK_B8	(1 << 8)		//Bit[8]：NC-NT的传输过程中，用户层无负载数据
#define 	FC1553_NC_ERRMASK_B9	(1 << 9)		//Bit[9]：NT或者发送NT的状态序列中，消息错误为1
#define 	FC1553_NC_ERRMASK_B10	(1 << 10)		//Bit[10]：NT的状态序列中，接收NT的消息错误为1，仅在NT-NT的传输中有效
#define 	FC1553_NC_ERRMASK_B11	(1 << 11)		//Bit[11]：NT-NT的传输中,发送NT的命令序列中，nt2nt和nc_mon相互矛盾
#define 	FC1553_NC_ERRMASK_B12	(1 << 12)		//Bit[12]：NT-NT的传输中,发送NT的命令序列中，NT Burst Size Request和Delayed NT Burst Size Request同时有效
#define 	FC1553_NC_ERRMASK_ALL	(FC1553_NC_ERRMASK_B0 | FC1553_NC_ERRMASK_B1 | FC1553_NC_ERRMASK_B2 | FC1553_NC_ERRMASK_B4 |\
									FC1553_NC_ERRMASK_B5 | FC1553_NC_ERRMASK_B6 | FC1553_NC_ERRMASK_B7|\
									FC1553_NC_ERRMASK_B8 | FC1553_NC_ERRMASK_B9 | FC1553_NC_ERRMASK_B10|\
									FC1553_NC_ERRMASK_B11 | FC1553_NC_ERRMASK_B12)
/*
 NT错误屏蔽码
*/
#define 	FC1553_NT_ERRMASK_B0     (1 << 0)			//Bit[0]：定时器0超时
#define 	FC1553_NT_ERRMASK_B1     (1 << 1)			//Bit[1]：定时器1超时
#define 	FC1553_NT_ERRMASK_B2     (1 << 2)			//Bit[2]：定时器2超时
#define 	FC1553_NT_ERRMASK_B3     (1 << 3)			//Bit[3]：定时器3超时    
#define 	FC1553_NT_ERRMASK_B4     (1 << 4)			//Bit[4]：D和SA匹配失败
#define 	FC1553_NT_ERRMASK_B5     (1 << 5)			//Bit[5]：交换过程中，收到错误的包(CRC错误，极性错误，长度错误等)
#define 	FC1553_NT_ERRMASK_B6     (1 << 6)			//Bit[6]：期望接收状态序列时出现意外
#define 	FC1553_NT_ERRMASK_B7     (1 << 7)			//Bit[7]：期望接收数据序列时出现意外
#define 	FC1553_NT_ERRMASK_B8     (1 << 8)			//Bit[8]：NT-NC的交换，NT无可发送的负载数据
#define 	FC1553_NT_ERRMASK_B9     (1 << 9)			//Bit[9]：NT-NT的交换，发送NT无可发送的负载数据
#define 	FC1553_NT_ERRMASK_B10    (1 << 10)			//Bit[10]：接收NT的状态序列中，消息错误为1，仅在NT-NT的传输中有效
#define 	FC1553_NT_ERRMASK_B11    (1 << 11)			//Bit[11]：接收的命令序列中，cmd_nt2nt和cmd_nc_mon相互矛盾
#define 	FC1553_NT_ERRMASK_B12    (1 << 12)			//Bit[12]：接收的命令序列中，NT Burst Size Request和Delayed NT Burst Size Request同时有效
#define 	FC1553_NT_ERRMASK_ALL	(FC1553_NT_ERRMASK_B0 | FC1553_NT_ERRMASK_B1 | FC1553_NT_ERRMASK_B2 | FC1553_NT_ERRMASK_B4 |\
									FC1553_NT_ERRMASK_B5 | FC1553_NT_ERRMASK_B6 | FC1553_NT_ERRMASK_B7|\
									FC1553_NT_ERRMASK_B8 | FC1553_NT_ERRMASK_B9 | FC1553_NT_ERRMASK_B10|\
									FC1553_NT_ERRMASK_B11 | FC1553_NT_ERRMASK_B12)

#pragma pack(4)	

struct FC_1553_MSG
{
	/*
	 调度信息
	*/
	struct		SCHE_TB		stb;
	/*
	 消息头信息
	*/
	struct MSG_HEADER	msghead;
	/*
	bit7	Cmd With Data	1表示命令帧允许携带负载，0表示不允许
	bit6	NT Burst Size Request	映射命令帧DW6的bit8
	bit5	Deleay NT Burst Size Request	映射命令帧DW6的bit7
	bit4	Suppress Status	映射命令帧DW6的bit4
	bit3	NT to NT Transter	映射命令帧DW6的bit3
	bit2	T/R*	映射命令帧DW6的bit2
	bit1	NC Monitor for NT to NT Transter	映射命令帧DW6的bit1
	bit0	Muiticast	映射命令帧DW6的bit0
	*/
	unsigned int 	CmdFlag;
	/*
	 NC消息的SID，只要交换机识别，卡可以仿真多个NC节点
	*/
	unsigned int 	sID	:	24;
	/*
	 F_CTL中的优先级使能位
	*/
	unsigned int 	PriorityEn:1;
	unsigned int 	SL:	1	;
	/*
	 重传控制
	*/
	unsigned int 	Retry:2	;
	/*
	 保留
	*/
	unsigned int 	Res0:	4;
	/*
	 目标ID
	*/
	unsigned int 	dID : 24;
	/*
	 保留
	*/
	unsigned int 	Res1:	8;
	/*
	 目标SA
	*/
	unsigned int 	SA : 32;
	/*
	 数据长度或模式码
	*/
	unsigned int 	DataCnt_MC;
	/*只有在RT->RT中使用*/
	unsigned int 	OtherDID;
	unsigned int 	OtherSA;
	/*
	只有Glink才有用
	*/
	unsigned int	Smart_LongMsg;
};

/*
 * 所有DMA头按照大端格式填写，参考寄存器文档
 * 首先写按照表格的左侧数据，然后写右侧。
 * 在单个字节内部，首先写右侧，然后写左侧。
 */
/*
 * 下行第一包DMA头结构
*/
struct FC1553_T_DMA_HEADER_P0 {
	//union {
	unsigned int LP : 4;
	unsigned int WT : 2;
	unsigned int SUF : 2;
	
	unsigned int Reserve0 : 8;
	unsigned int msgTbIndex : 16;

	unsigned int reserve1;
	unsigned int ELEN;
	unsigned int SMLEN;
};

/*
 * 下行DMA包格式，除第一包之外
*/
struct FC1553_T_DMA_HEADER_P1 {
	//union {
	unsigned int LP : 4;
	unsigned int WT : 2;
	unsigned int SUF : 2;
	
	
	unsigned int Reserve0 : 8;
	unsigned int msgTbIndex : 16;
	
//	unsigned int LP : 4;
	unsigned int reserve0;
};

/*
 fc1553 DMA头
*/
struct FC1553_R_DMA_FIX_HEADER {
	/*DW0*/
	unsigned int LP : 4;
	unsigned int WT : 2;
	unsigned int SUF : 2;
	
	unsigned int sID : 24;
	/*DW1*/
	unsigned int	dID : 16;
	unsigned int	STA : 5;
	unsigned int	msgTbIndex : 11;

	/*DW2*/
	unsigned int SA;
	/*DW3*/
	unsigned int ELEN;
	/*DW4*/
	unsigned int SMLEN;
	/*DW5*/
	unsigned int Reserve;
};

#pragma pack()
#ifdef __cplusplus
extern "C" {
#endif
	void glkfc1553_pro_start(OS_HANDLE pBHandle,int Chs);
	void glkfc1553_pro_stop(OS_HANDLE pBHandle,int Chs);
	void glkfc1553_pro_msgconfig(OS_HANDLE pBHandle, int Chs,struct SCHE_TB * tb);
	void glkfc1553_pro_filldmaheader(struct SCHE_TB* tb, void * memPtr,int LP,int FP, int PayloadLen);
	void glkfc1553_pro_hook(unsigned char* pBuffer, int SUPWT);
#ifdef __cplusplus
}
#endif
#endif
