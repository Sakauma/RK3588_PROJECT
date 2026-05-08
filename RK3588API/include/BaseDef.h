#ifndef BASE_DEF_H
#define BASE_DEF_H


/*
 * 在移位时，使用U32类型移位
 * */
#define 	L_SHIFT(V,B)	((unsigned int)(V) << (B))	
#define		R_SHIFT(V,B)	((unsigned int)(V) >> (B))

#define		LAST_PACK	(5)
#define		PTYPE_ASM(SUF,WT)		{SUF=1;WT=0;}
#define		PTYPE_FC1553(SUF,WT)	{SUF=1;WT=1;}
#define		PTYPE_ELS				{SUF=1;WT=2;}
#define		PTYPE_USEDEF(SUF,WT,DV)	{SUF=(DV>>2)&3;WT=DV&3;}
#define		MAKE_PTYE(SUF,WT)		(SUF << 1 | WT)

enum		GLKFC1553_SMODE{
	/*
	 * 设置FC1553协议栈的模式，1：NC，0：NT
	*/
	FC1553_PROTOCAL_MODE_NT = 0,
	FC1553_PROTOCAL_MODE_NC 
};

/*
 数据类型，Sample还是Quque
*/
enum	GLK_DATA_TPYE{
	MSG_TYPE_QUEUE = 0,
	MSG_TYPE_SAMPLE
};

/*
 优先级
*/
enum 	GLK_SCHE_PRIORITY{
	HIGH_PRIO_PERIOD = 2,/*周期高优先级*/
	LOW_PRIO_PERIOD = 3,/*周期低优先级*/
	HIGH_PRIO_APERIOD = 0,/*非周期高优先级*/
	LOW_PRIO_APERIOD = 1/*非周期低优先级*/
};

/*
 消息在周期发送的模式
*/
enum GLK_SCHE_SEND_MODE{
	GSSM_AT_LOWDELAY=0,	/*消息紧密发送，即不按照发送偏移执行*/
	GSSM_AT_OFFSET		/*消息按照发送偏移处理*/
};

/*
	*端口速率定义
*/
typedef enum
{
	PORT_SPEED_1G = 0,		//1.0625Gbps
	PORT_SPEED_2G,		//2.125Gbps
	PORT_SPEED_4G,		//4.25Gbps
	PORT_SPEED_8G,		//8.5Gbps

	//GLink
	PORT_SPEED_2p5G,		//2.5Gbps
	PORT_SPEED_5G,		    //5Gbps
	PORT_SPEED_10G,		    //10Gbps

	MAX_PORT_SPEED_NUM,		//端口速率类型数
}emFC_PORT_SPEED;


//板卡类型定义
typedef enum
{
	FC_HARDWARE_TYPE_IRAX80V14 = 0x0,			//IRAX 80卡V14 K7
	FC_HARDWARE_TYPE_V7 = 0x1,			//V7高速接口卡 V7
	FC_HARDWARE_TYPE_IMBED = 0x2,			//嵌入式节点卡
	FC_HARDWARE_TYPE_KU = 0x3,			//KU高速接口卡 KU
}emFC_HARDWARE_TYPE;

/*
	总线类型
*/
typedef enum
{
	FC_SYSTEM_SWITCH = 0,  //交换式
	FC_SYSTEM_BUS = 1,  //总线式
}emFCSYSTEMMODE;

/*
	PON网络下节点类型
*/
typedef enum
{
	PON_MODE_ONU = 0,
	PON_MODE_OLT
}emFC_PON_MODE;

/*
	GT数据加扰模式
*/
typedef enum
{
	GT_SCR_MODE_LB = 0,			//低字节优先
	GT_SCR_MODE_HB,				//高字节有效
}emGT_SCR_MODE;


/*
	复位类型
*/
typedef enum
{
	FC_COMM_RST_ALL = 0,
	FC_COMM_RST_GT = 1,
	FC_COMM_RST_MAC = 2,
	FC_COMM_RST_NM = 3,
	FC_COMM_RST_ASM = 4,
	FC_COMM_RST_1553 = 5,
	FC_COMM_RST_FI = 6,
	FC_COMM_RST_BRIDGE = 7,
	FC_COMM_RST_DDR = 8,
	FC_COMM_RST_DMA = 9,
}emFCCommRst;

//modify by lx 20240425 GLINK 支持24光口6bank8协议栈
#define	FC_PORT_MAX_NUM				(24)		//24端口，待完善，最终通过能力寄存器获取 @zhuyanbing,2021.12.03
#define FC_PORT_MAX_GROUP			(6)			//24端口，6个BANK
//#define FC_PORT_MAX_NUM			(8)			//8端口，待完善，最终通过能力寄存器获取 @zhuyanbing,2021.12.03
//#define FC_PORT_MAX_GROUP			(2)			//8端口，2个BANK

#define FC_1553_PORT_MAX_NUM		(2)
#define FC_MAX_CHL_NUM				(8)
#define FC_FI_PORT_MAX_NUM			(2)			//AT01板卡只支持2个通道（故障注入板卡）

//板卡FPGA信号定义
enum
{
	CARD_FPGA_TYPE_MASK = 0Xf,
};

//板卡类型定义
typedef enum
{
	FC_CARD_TYPE_RESERVE = 0x0,			//预留
	FC_CARD_TYPE_1553 = 0x1,			//FC-AE-1553节点卡
	FC_CARD_TYPE_ASM = 0x2,			//FC-AE-ASM节点卡 
	FC_CARD_TYPE_NM = 0x3,			//FC-NM采集卡
	FC_CARD_TYPE_FI = 0x4,			//FC-FI故障注入卡
	FC_CARD_TYPE_PRESSURE = 0x5,			//FC-压力测试卡
	FC_CARD_TYPE_818 = 0x6,			//FC-818卡 
	FC_CARD_TYPE_IPFC = 0x7,			//FC-IPFC
	FC_CARD_TYPE_1553B = 0x8,			//FC-1553B桥    
	FC_CARD_TYPE_TPB = 0x9,			//FC总线式拓扑桥 
	FC_CARD_TYPE_1553PON = 0xa,			//FC-AE-1553节点-PON  
	FC_CARD_TYPE_GLINKGW = 0xb,			//GLINK网关
	FC_CARD_TYPE_FCTM = 0xc,			//FC桥接匹配终端 
	FC_CARD_TYPE_GLINK = 0xd,			//GLINK节点卡 
	FC_CARD_TYPE_GLINK_SWITCH = 0xe,			//GLINK交换卡 

}emFC_CARD_TYPE;

//光口类型定义
typedef enum
{
	FC_PORT_TYPE_RESERVE = 0x0,			//预留
	FC_PORT_TYPE_1553 = 0x1,			//FC-AE-1553节点卡
	FC_PORT_TYPE_ASM = 0x2,			//FC-AE-ASM节点卡 
	FC_PORT_TYPE_NM = 0x3,			//FC-NM采集卡
	FC_PORT_TYPE_FI = 0x4,			//FC-FI故障注入卡
	FC_PORT_TYPE_PRESSURE = 0x5,			//FC-压力测试卡
	FC_PORT_TYPE_818 = 0x6,			//FC-818卡 
	FC_PORT_TYPE_IPFC = 0x7,			//FC-IPFC
	FC_PORT_TYPE_1553B = 0x8,			//FC-1553B桥    
	FC_PORT_TYPE_TPB = 0x9,			//FC总线式拓扑桥 
	FC_PORT_TYPE_1553PON = 0xa,			//FC-AE-1553节点-PON  
	FC_PORT_TYPE_GLINKGW = 0xb,			//GLINK网关
	FC_PORT_TYPE_FCTM = 0xc,			//FC桥接匹配终端 
}emFC_PORT_TYPE;

typedef struct _HardWareInfo_
{//硬件板卡信息
	FC_UINT64  FPGADate; //逻辑版本日期
	FC_DWORD   ddrState; //ddr状态
	FC_FLOAT   Temperature; //温度
	FC_FLOAT   CCINT;
	FC_FLOAT   CCAUX;
} FC_HardWareInfo_st, * pFC_HardWareInfo_st;

typedef struct _FrameInfo_
{
	//帧统计信息
	FC_DWORD fc2_rx_mux_fc3_cnt;			//mac接收报文统计
	FC_DWORD fc2_tx_mux_fc3_cnt;			//mac发送报文统计
	FC_DWORD fc2_rx_mux_crc_err_cnt;		//Fc2接收crc错误报文统计
} FC_FrameInfo_st, * pFC_FrameInfo_st;


/*
	端口链路恢复状态
*/
typedef enum
{
	PORT_STATE_INIT = 0,		//Initialization
	PORT_STATE_OFFLINE = 1,		//Offline
	PORT_STATE_LR,				//Link Recovery
	PORT_STATE_AC,				//Active State
	PORT_STATE_INVALID = 4,		//无效
}emDevPortState;

/*
	节点通道工作模式，适用于冗余功能的设备，如FC-AE-1553节点卡
*/
typedef enum
{
	FC_CHL_MODE_REDUNDANCE = 0,				//通道冗余模式，2通道冗余
	FC_CHL_MODE_INDEPENDENT,				//通道独立模式

}emFCChlMode;

/*
 设置周期同步模式
*/
enum CYCLE_SYNC_MODE {
	CSM_INTERNAL,
	CSM_EXTERNAL
};



#endif
