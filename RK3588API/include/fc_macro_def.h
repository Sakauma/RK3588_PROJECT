#ifndef FC_MACRO_DEF_H
#define FC_MACRO_DEF_H

#define DN_DMA_HEAD_SOF				(0xaaaa5555)
#define FC_HEAD_SOF					(0x5656b5bc)

#define FC_MAX_FRAME_NUM			(32)	//单次下发最大包数，按2k负载的话，最大支持64k数据传输，该宏定义用于替换消息、SA配置中的frameMaxNum

//定义现有逻辑针对下行数据包（消息）的模式
#define DN_MSG_MODE_DIRECT			(0)		//即时消息：根据优先级，立即发送
#define DN_MSG_MODE_PERIOD			(1)		//周期消息:NC消息
#define DN_MSG_MODE_RESPONSE		(2)		//应答消息：NT响应命令



#define CRC_CALC_MODE_AUTO			(0)		//CRC计算方式，自动计算
#define CRC_CALC_MODE_MAUNUL		(1)		//包内计算的CRC

#define MSG_SEQ_END_MSG_SHIFT		(0)
#define MSG_SEQ_START_MSG_SHIFT		(1)
#define MSG_SEQ_END_QUEUE_SHIFT		(2)
#define MSG_SEQ_START_QUEUE_SHIFT	(3)

/***********************************************原语相关定义*********************************************/
#define	FC2_IDLE 									(0xBC95B5B5)
#define	FC2_READY 									(0xBC954A4A)
#define	FC2_OLS										(0xBC358A55)
#define	FC2_NOS										(0xBC55BF45)
#define	FC2_LRR										(0xBC35BF49)
#define	FC2_LR 										(0xBC49BF49)
#define	SOFf 										(0xBCB55858)
#define	SOFn1										(0xBCB53737)
#define	SOFi1										(0xBCB55757)
#define	SOFc1										(0xBCB51717)
#define	SOFn2										(0xBCB53535)
#define	SOFi2										(0xBCB55555)
#define	SOFn3										(0xBCB53636)
#define	SOFi3 										(0xBCB55656)
#define	EOFn_n										(0xBC95D5D5)
#define	EOFn_p										(0xBCB5D5D5)
#define	EOFt_n										(0xBC957575)
#define	EOFt_p 										(0xBCB57575)
#define	EOFdt_n 									(0xBC959595)
#define	EOFdt_p 									(0xBCB59595)
#define	EOFrt_n										(0xBC959999)
#define	EOFrt_p 									(0xBCB59999)
#define	EOFni_n 									(0xBC8AD5D5)
#define	EOFni_p										(0xBCAAD5D5)
#define	EOFa_n 										(0xBC95F5F5)
#define	EOFa_p 										(0xBCB5F5F5)
#define	EOFdti_n									(0xBC8A9595)
#define	EOFdti_p 									(0xBCAA9595)
#define	EOFrti_n 									(0xBC8A9999)
#define	EOFrti_p 									(0xBCAA9999)
#define SYNx										(0xBC7F0000)
#define SYNy										(0xBCBF0000)
#define SYNz										(0xBCDF0000)
#define PON_HS										(0xBCB5AAAA)
#define ARBF										(0xBC94FFFF)

/*************************************FC帧头字段相关定义**************************************************/
//FC帧头 R_CTL类型定义
#define FC_HEAD_R_CTL_CMD_SEQ						(0x6)			//命令序列
#define FC_HEAD_R_CTL_DATA_SEQ						(0x4)			//数据序列
#define FC_HEAD_R_CTL_STATE_SEQ						(0x7)			//状态序列

#define FC_HEAD_R_CTL_ELS_REQ						(0x22)		//ELS 请求
#define FC_HEAD_R_CTL_ELS_RSP						(0x23)		//ELS应答

#define FC_HEAD_R_CTL_LS_NOP						(0x80)		//No Operation
#define FC_HEAD_R_CTL_LS_ABTS						(0x81)		//Abort Sequence
#define FC_HEAD_R_CTL_LS_BA_ACC						(0x84)		//Basic_Adcept
#define FC_HEAD_R_CTL_LS_BA_RJT						(0x85)		//Basic_Reject


//FC帧头 TYPE类型定义
#define FC_HEAD_TYPE_LS								(0x00)		//LS
#define FC_HEAD_TYPE_ELS							(0x01)		//ELS
#define FC_HEAD_TYPE_1553							(0x48)		//FC_AE_1553
#define FC_HEAD_TYPE_ASM							(0x49)		//FC_AE_ASM
#define FC_HEAD_TYPE_AV								(0x60)		//FC_AE_AV
#define FC_HEAD_TYPE_ARINC818						(0x61)		//ARINC818


//FC帧头 F_CTL关键bit位定义
#define F_CTL_EXCHANGE_CONTEXT_SHIFT				(23)
#define F_CTL_EXCHANGE_CONTEXT_MASK					(1 << 23)

#define F_CTL_SEQUENCE_CONTEXT_SHIFT				(22)
#define F_CTL_SEQUENCE_CONTEXT_MASK					(1 << 22)


#define F_CTL_FIRST_SEQ_SHIFT						(21)
#define F_CTL_FIRST_SEQ_MASK						(1 << 21)

#define F_CTL_LAST_SEQ_SHIFT						(20)
#define F_CTL_LAST_SEQ_MASK							(1 << 20)

#define F_CTL_END_SEQ_SHIFT							(19)
#define F_CTL_END_SEQ_MASK							(1 << 19)

//为1时，Word 1,Bit31-24:CS_CTL or Priority
#define F_CTL_CS_CTL_PRIO_EN_SHIFT					(17)
#define F_CTL_CS_CTL_PRIO_EN_MASK					(1 << 17)

#define F_CTL_SEQ_INITATIVE_SHIFT					(16)			//0	hold sequence initative ; 1 transfer sequence initiative
#define F_CTL_SEQ_INITATIVE_MASK					(1 << 16)

#define F_CTL_ACK_FORM_SHIFT						(12)
#define F_CTL_ACK_FORM_MASK							(0x3 << 12)
#define F_CTL_ACK_FORM_NO_PROVIDED_MASK				(0x0 << 12)
#define F_CTL_ACK_FORM_ACK_1_REQ_MASK				(0x1 << 12)
#define F_CTL_ACK_FORM_ACK_0_REQ_MASK				(0x3 << 12)

#define F_CTL_ABORT_SEQ_CONDITION_SHIFT				(4)
#define F_CTL_ABORT_SEQ_CONDITION_MASK				(0x3 << 4)
#define F_CTL_ABORT_ACK_CONTINUE_SEQ_MASK			(0x0)
#define F_CTL_ABORT_ACK_ABORT_SEQ_MASK				(0x1)
#define F_CTL_ABORT_ACK_STOP_SEQ_MASK				(0x2)
#define F_CTL_ABORT_DATA_DISCARD_MUTIL_FRM_MASK		(0x0)
#define F_CTL_ABORT_DATA_DISCARD_SIG_FRM_MASK		(0x1)
#define F_CTL_ABORT_DATA_PROC_POLICY_MASK			(0x2)

#define F_CTL_RELA_OFFSET_PRESENT_SHIFT				(3)
#define F_CTL_RELA_OFFSET_PRESENT_MASK				(0x1 << 3)
#define F_CTL_ROP_PARAM_USE_FOR_FRM					(0x0 << 3)
#define F_CTL_ROP_PARAM_FIELD						(0x1 << 3)

#define F_CTL_EXCHANGE_REASSEMBLY_SHIFT				(2)

#define F_CTL_FILL_BYTES_SHIFT						(0)
#define F_CTL_FILL_BYTES_MASK						(0x3)
#define F_CTL_FB_NO_FILL_MASK						(0)
#define F_CTL_FB_ONE_BYTE_FILL_MASK					(1)
#define F_CTL_FB_TWO_BYTE_FILL_MASK					(2)
#define F_CTL_FB_TRE_BYTE_FILL_MASK					(3)

/*******************************************************************************************************/


/********************************************ELS字段相关定义*********************************************/

//Command Code
#define ELS_CMD_CODE_LS_RJT							(0x01)		//LS_RJT
#define ELS_CMD_CODE_LS_ACC							(0x02)		//LS_ACC
#define ELS_CMD_CODE_PLOGI							(0x03)		//N_Port Login
#define ELS_CMD_CODE_FLOGI							(0x04)		//F_Port Login
#define ELS_CMD_CODE_LOGO							(0x05)		//Logout
#define ELS_CMD_CODE_RSI							(0x0A)		//Request Sequence Initiative
#define ELS_CMD_CODE_ESTS							(0x0B)		//Establish Streaming
#define ELS_CMD_CODE_ESTC							(0x0C)		//Estimate Credit
#define ELS_CMD_CODE_ADVC							(0x0D)		//Advise Credit
#define ELS_CMD_CODE_RTV							(0x0E)		//Read Timeout Value
#define ELS_CMD_CODE_RLS							(0x0F)		//Read Link Error Status Block
#define ELS_CMD_CODE_ECHO							(0x10)		//Echo
#define ELS_CMD_CODE_TEST							(0x11)		//Test
#define ELS_CMD_CODE_RRQ							(0x12)		//Reinstate Recover Qualifier
#define ELS_CMD_CODE_REC							(0x13)		//Read Exchange Concise
#define ELS_CMD_CODE_PRLI							(0x20)		//Process Login
#define ELS_CMD_CODE_PRLO							(0x21)		//Process Logout
#define ELS_CMD_CODE_TPLS							(0x23)		//Test Process Login State
#define ELS_CMD_CODE_CSR							(0x68)		//Clock Synchronization Request
#define ELS_CMD_CODE_CSU							(0x69)		//Clock Synchronization Update

//TODO:待完善定义，FC_LS_3 12-10v0.pdf P34		by zhuyb 20211030

#endif

