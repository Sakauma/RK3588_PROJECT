#pragma once
#include "../glkconfig.h"

#define REG_READ_ACK_ERROR	(0XFFFFFFFF)
/*************************************************************************/
/*			FC板卡类寄存器基地址定义										 */
/*************************************************************************/
//以下为GLink中用的的基地址 modify by lx 20240429
#define REG_BASE_OFFSET_ADDR					(0x00000)		//DMA寄存器
#define REG_COMM_BASE_ADDR						(0x10000)		//COMMON寄存器
#define REG_FLASH_BASE_ADDR						(0x20000)		//Flash更新
#define REG_GT_BASE_ADDR						(0x30000)		//GT寄存器
#define REG_MAC_BASE_ADDR						(0x9000)		//FC-AE-MAC寄存器 MAC基地址变更 从0x40000变为协议栈基地址+A口0x9000/B口0xA000 20250506 lixin modify
#define REG_TPB_BASE_ADDR						(0x50000)		//xadc
#define REG_BASE1_ADDR							(0x60000)		//预留
#define REG_BASE2_ADDR							(0x70000)		//预留
#define REG_1553_BASE_ADDR						(0x80000)		//FC-AE-1553寄存器

#define REG_B0_RESET_DMA_RW (REG_BASE_OFFSET_ADDR + 0x0030)	/*DMA复位寄存器，bit31写1进行复位，bit0读出来是1的话表示复位成功，bit0写1清成功标记*/

//新增FC_L1-L4层寄存器定义 20250514 lx
#define REG_1553_L4_BASE_ADDR					(0x0000)        // 0000-7FFFF FC_L4层寄存器，12bit,32KB
#define REG_1553_L3_BASE_ADDR					(0x8000)		// 8000-8FFFF FC_L3层寄存器，12bit,4KB
#define REG_1553_L2_A_BASE_ADDR					(0x9000)		// 9000-9FFFF FC_L2层A通道寄存器，12bit,4KB
#define REG_1553_L2_B_BASE_ADDR					(0xA000)		// A000-AFFFF FC_L2层B通道寄存器，12bit,4KB
#define REG_1553_L1_A_BASE_ADDR					(0xB000)		// B000-BFFFF FC_L1层A通道寄存器，12bit,4KB
#define REG_1553_L1_B_BASE_ADDR					(0xC000)		// C000-CFFFF FC_L1层B通道寄存器，12bit,4KB

#define REG_1553_L3_GLINK_MODE					(REG_1553_L3_BASE_ADDR + 0x30)			// 1bit 0：FC协议 1：GLINK协议
#define REG_1553_L3_PRO_TYPE					(REG_1553_L3_BASE_ADDR + 0x34)			// 1bit有效 1：上行透传接收 0：上行不透传
#define REG_1553_L3_CFG_DID						(REG_1553_L3_BASE_ADDR + 0x38)			// 32bit有效
																//[23:0]：匹配DID，用于总线式时过滤不属于该节点的消息
																//[24]：0 交换式，不匹配DID 1 总线式，匹配DID
#define REG_1553_L3_RX_ENABLE					(REG_1553_L3_BASE_ADDR + 0x3C)			//1bit有效，1：接收使能打开 0：接收使能关闭
#define REG_1553_L3_REDUN_TIMEOUT				(REG_1553_L3_BASE_ADDR + 0x40)			//32bit有效，冗余模块超时时间，单位u秒
#define REG_1553_L3_REDUN_MODE					(REG_1553_L3_BASE_ADDR + 0x44)			//1bit有效 1：冗余模式 0：独立模式
#define REG_1553_L3_DMATEST_MODE				(REG_1553_L3_BASE_ADDR + 0x48)			//1bit有效 1：DMA回环测试 0：DMA正常模式
#define REG_1553_L3_ELS_STATE_CNT				(REG_1553_L3_BASE_ADDR + 0x110)			//32bit有效
																//[15:0]：1553数据帧计数
																//[23:16]：上行els帧计数
																//[31:24]：DMA上行帧计数
#define REG_1553_L3_STATE						(REG_1553_L3_BASE_ADDR + 0x114)			//32bit有效
																//[7:0]:冗余上行通道0计数
																//[15:8]: 冗余上行通道1计数
																//[23:16] : 冗余上行通道仲裁4后上行计数
																//[31:24] : 冗余上行状态机
#define REG_1553_L3_MAC_CNT						(REG_1553_L3_BASE_ADDR + 0x118)			//32bit有效 协议栈到mac帧数统计
#define REG_1553_L3_CH_CNT						(REG_1553_L3_BASE_ADDR + 0x11C)			//32bit有效 mac到协议栈到帧数统计
#define REG_1553_L3_DN_CNT						(REG_1553_L3_BASE_ADDR + 0x120)			//32bit有效 协议栈到dma下行计数
#define REG_1553_L3_UP_CNT						(REG_1553_L3_BASE_ADDR + 0x124)			//32bit有效 dma到协议栈上行计数

#define REG_1553_L1_OLT_EN(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x30)			//1bit有效 1: OLT 0 : ONU default: ONU
#define REG_1553_L1_LOOP_EN(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x34)			//1bit有效 1: 开启回环 0 : 关闭回环
#define REG_1553_L1_GT_RATE(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x38)			//GT速率，32bit有效 0：1.0625 1：2.125 2：4.25 default：8.5
#define REG_1553_L1_WAIT_TIME(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x3C)			//PON模式超时时间，使用默认
#define REG_1553_L1_GAP_TIME(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x40)			//使用默认
#define REG_1553_L1_GT_EN(mode)					(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x44)			//光使能 1：关闭光使能 0：打开光使能
#define REG_1553_L1_SCR_BYPASS(mode)			(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x48)			//1bit有效 1：不加扰 0：加扰 8.5G加扰打开
#define REG_1553_L1_PON_MODE(mode)				(REG_1553_L1_A_BASE_ADDR + mode*0x1000+ 0x4C)			//1bit有效 0:交换式 1：总线式

/************************************************************************************************************************************************/
/*															COMMON域寄存器																		*/
/************************************************************************************************************************************************/
#define REG_PCIE_IP_VERSION						(REG_COMM_BASE_ADDR + 0x0000)		//IPCore 逻辑年月日
#define REG_PCIE_REDUN_MODE						(REG_COMM_BASE_ADDR + 0x0014)		//冗余模块设置，Bit0: 0--->独立模式下 	1--->冗余模式
																					//			   Bit1: 1	冗余模式下ELS报文通过PORT_A发送
																					//			   Bit2: 1	冗余模式下ELS报文通过PORT_B发送	
#define	REG_PCIE_FC_CARD_CAP					(REG_COMM_BASE_ADDR + 0x1010)		//板卡能力：[31:24]:支持的工作模式：0b'00：独立模式，0b'1：冗余模式
																					//			[19:16]:参考时钟：0x0：106.25Mhz；0x1：125Mhz
																					//			[15:8]:硬件型号：
																					//[7:0]:板卡类型：00 预留
																					//01:FC-AE-1553节点卡 02：FC-AE-ASM节点卡 03：FC-NM节点卡
																					//04:FC-FI故障注入卡 05：FC-压力测试卡 06：FC-818卡
																					//07:FC-IPFC 08：FC-1553B桥 09：FC总线式拓扑桥
																					//0a:FC-AE-1553节点-PON 0b：GLINK网关 0c：FC桥接匹配终端
																					//0d:FC-Glink节点卡 0e：GLink交换卡
		
#define	REG_PCIE_FC_CH_CAP						(REG_COMM_BASE_ADDR + 0x1014)			//通道能力寄存器
																					//			 [ 3: 0]--类型：1：仿真卡；2：仿真+监控卡；3：监控卡；4：故障注入卡；5：故障注入卡+监控卡
#define	REG_PCIE_FC_CARD_SPEED					(REG_COMM_BASE_ADDR + 0x1018)			//通道速率：1：1.0625Gbps；2：2.125Gbps；4：4.25Gbps；8：8.25Gbps；
#define	REG_PCIE_BD_MEMSIZE						(REG_COMM_BASE_ADDR + 0x101C)	
#define	REG_DMA_NUM_MASK						(REG_COMM_BASE_ADDR + 0x1020)			//DMA使能掩码
																//[7:0]当前板卡下行DMA通道使能，                 如8‘b0000_0011表示使能DMA通道1和2
																//[15:8]上行DMA通道使能，用法同下行
#define	REG_DMA_DL_FUNC							(REG_COMM_BASE_ADDR + 0x1024)			//下行DMA通道功能：0x00：NC,  0x01：NT,0x02：NM,0x03：ASM,0x04：FI,0x05：ARINC818,0x06：IPFC,0x07：ETH,0x08：压力测试
															// [ 3: 0]-下行DMA通道1功能,[ 7:4]-下行DMA通道2功能... ...
#define	REG_DMA_UL_FUNC							(REG_COMM_BASE_ADDR + 0x1028)			//上行DMA通道功能：0x00：NC,  0x01：NT,0x02：NM,0x03：ASM,0x04：FI,0x05：ARINC818,0x06：IPFC,0x07：ETH,0x08：压力测试
															// [ 3: 0]-上行DMA通道1功能,[ 7:4]-上行DMA通道2功能... ...
				
//yanfei@add PON相关寄存器
#define REG_COMM_PON_OLT_CTL						(REG_COMM_BASE_ADDR + 0x38)			//总线式1553节点角色控制，bit0:1--->OLT设备，bit0:0--->ONU设备
#define REG_COMM_PON_OLT_LOOP						(REG_COMM_BASE_ADDR + 0x3C)			//总线式1553 OLT 节点回环ONU数据控制，bit[1:0]：2通道回环控制，1回环，0不回环		
#define REG_COMM_GT_RATE							(REG_COMM_BASE_ADDR + 0x40)			//0：1.0625  1：2.125  2：4.25  default：8.5
#define REG_COMM_WAIT_TIME							(REG_COMM_BASE_ADDR + 0x44)			//使用默认值 ,逻辑配    
#define REG_COMM_GAP_TIME							(REG_COMM_BASE_ADDR + 0x48)			//使用默认值 ,逻辑配     
#define REG_COMM_SPF_CTRL							(REG_COMM_BASE_ADDR + 0x4C)			//0:打开接口   1：关闭接口，暂时不用     
#define REG_COMM_PON_MODE							(REG_COMM_BASE_ADDR + 0x50)			//0:交换式   1：总线式     
#define REG_COMM_SCR_BYPASS							(REG_COMM_BASE_ADDR + 0x54)			//1：不加扰  0：加扰  ,8.5G加扰打开
#define REG_COMM_RESET								(REG_COMM_BASE_ADDR + 0x104)			//复位寄存器

//yanfei@add 2023-12-12 板卡状态寄存器
#define REG_BORD_DDR						        (REG_COMM_BASE_ADDR + 0x100)//1~OK
#define REG_BORD_TEMPERATURE						(REG_TPB_BASE_ADDR + 0x200) 
#define REG_BORD_CCINT						        (REG_TPB_BASE_ADDR + 0x204)
#define REG_BORD_CCAUX						        (REG_TPB_BASE_ADDR + 0x208)
#define REG_BORD_SET_TESTFREQ_CHL				    (REG_COMM_BASE_ADDR + 0x60) //切换测频通道
#define REG_BORD_READ_FREQ				            (REG_COMM_BASE_ADDR + 0x11C)//读取测频值，在设置测频通道后延时1.5s以上才可读

//lixin 加扰
#define REG_MAC_SCR_MODE(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x24)	//高字节、低字节优先加扰选择，bit[7:0]有效，1：高字节优先，0低字节优先，默认0xff
#define REG_MAC_TIMESYN_TRIG_MODE					(REG_COMM_BASE_ADDR + 0x6C) //触发信号模式，32bit有效 bit[31:16]表示触发周期大小
																				//bit[15:4]表示触发周期误差大小，单位微秒
																				//bit[3:0]表示触发模式，0表示不使用外部触发，1表示使用外部触发加补偿模式，2表示直接使用外部触发信号，3表示内部测试信号
#define REG_MAC_TRIG_FRE							(REG_COMM_BASE_ADDR + 0x114) //28bit有效，表示两次触发信号的计数，除以125表示时间，单位微秒

//时钟同步																				
#define REG_TIME_RECV_MODE						    (REG_COMM_BASE_ADDR + 0x38)//对时接口选择 
																			   //[3:0]：模式选择 0：CPU对时 1：原语对时 2：ELS对时 3：CPU+PPS对时 4：CPU+原语对时 5：CPU+ELS对时
																			   //[27:4]：通道选择 每个bit对应一个通道，最大支持24个通道 为1时表示打开本通道，为0时关闭，同一时间只能有一个通道打开
#define REG_TIME_SEND_MODE						    (REG_COMM_BASE_ADDR + 0x3C)//授时接口选择
																			   //[3:0]：模式选择 0：保留 1：原语授时使能 2：ELS授时使能 3：PPS授时使能
																			   //[27:4]：通道选择 每个bit对应一个通道，最大支持24个通道 为1时表示打开本通道，为0时关闭，同一时间只能有一个通道打开
#define REG_TIME_CONFIG					            (REG_COMM_BASE_ADDR + 0x30)//模块配置
																			   //[0]：寄存器调整时间补偿使能 0：关闭   1：打开
																			   //[1]：时间补偿使能 0：关闭   1：打开
																			   //[2]：CPU时间同步输入更新 有需要时写1会更新CPU的时间信息，当然CPU时间信息也是软件给的
																			   //[3]：脉冲信号 有需要时每秒写一次1，表示1s的脉冲，对应上面的PPS功能
																			   //[4]：CPU输出更新信号 写1时会上传一次最新的CPU输出的时间（CPU模式下使用的）

#define REG_TIME_ADJUST					            (REG_COMM_BASE_ADDR + 0x34)//寄存器调整时间值
																			   //[31:0]：和上面的寄存器调整时间补偿使能配合使用，打开使能的时候，需要把这个值传下去用于补偿

#define REG_TIME_CPU_LOW					        (REG_COMM_BASE_ADDR + 0x40)//表示CPU输入值的[31:0]
#define REG_TIME_CPU_MID					        (REG_COMM_BASE_ADDR + 0x44)//表示CPU输入值的[63:32]
#define REG_TIME_CPU_HIGHT					        (REG_COMM_BASE_ADDR + 0x48)//表示CPU输入值的[79:64]
#define REG_TIME_LOST_FLAG					        (REG_COMM_BASE_ADDR + 0x118)//时钟源丢失指示 
																				//bit[1:0] 第[0]位等于1时表示授时时钟源丢失了30s 第[1]位等于1时表示授时时钟源丢失了15min      

/************************************************************************************************************************************************/
/*															MAC时钟域寄存器																		*/
/*
	TODO:FC-AE-1553节点下MAC寄存器定义和FC-AE-ASM节点下定义不同，差异如下：
		1553节点卡设计时每个MAC连接2路GTX数据（即2个通道或Port），因此在获取Port或Chl状态时，需区分MAC地址偏移
		ASM节点卡设计时每个MAC连接1路GTX数据（即2个通道或Port）
*/
/************************************************************************************************************************************************/

#define REG_MAC_SOFT_RST(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x4)					//Bit0-Bit7对应通道1MAC-通道7MAC，可按通道进行MAC模块复位，写1为复位。
#define REG_MAC_SYS_CTL(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x8)					//系统控制，Bit0：软件复位，复位后100us，不能使用FC模块，Bit1:FC使能寄存器
#define REG_MAC_BBC_VALUE(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x38)					//设置BB信用值
#define REG_MAC_EN_LS_RECOVER(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x20)					//bit[27:24] = port[3:0]，链路恢复协议是能，bit[7:0]，链路恢复协议连接次数
#define REG_MAC_LS_E_D_TOV(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x24)					//同步丢失时间
#define REG_MAC_LS_R_A_TOV(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x28)					//初始化时间
#define REG_MAC_LINK_STATE(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x2C)					//链路link状态
#define REG_MAC_LINK_OFFLINE_CNT(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x58)		            //链路link异常次数
#define REG_MAC_BBC_ENABLE(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x34)                   //信用值关闭使能,bit0:1信用值不使能 bit1:1Releady不使能
#define REG_MAC_PORT_STATE(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x50)					//链路恢复端口状bit[11:0]：12’h1:链路初始化状态 12’h2 - 12’h100:链路恢复 12’h200 : 链路激活状态  12’h400 : 链路离线状态
#define REG_MAC_ARB_EN(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xBC)					//控制链路在空闲时发送的原语内容:1-arbff， 0-idle


//时钟同步
 #define REG_MAC_CPU_TIME_LOCK(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x90)					//上位机读操作，时间戳生效使能
 #define REG_MAC_CPU_TIME_VALUE_L(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x94)					//上位机时间戳[31:0]值读取
 #define REG_MAC_CPU_TIME_VALUE_M(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x98)					//上位机时间戳[63:32]值读取
 #define REG_MAC_CPU_TIME_VALUE_H(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x9C)					//上位机时间戳[79:64]值读取	
 #define REG_MAC_SYNC_SRC_SEL(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xa0)					/*对时接口：
																												0:CPU对时；
																														1:原语对时
																														2:ELS对时
																														3:CPU+PPS对时，分别对应整数部分和小数部分
																														4:CPU+原语对时，分别对用整数部分和小数部分
																														5：CPU+ELS对时，分别对应整数部分和小数部分
																											*/
 
 #define REG_MAC_SYNC_DST_SEL(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xa4)					/*授时接口
																										bit0:保留
																										bit1:原语授时使能；
																										bit2：ELS授时使能；
																										bit3:PPS授时使能
																									*/
 #define REG_MAC_TIME_COMP_EN(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xa8)					//时间值补偿开关，默认为0，不补偿
 #define REG_MAC_TIME_SRC_LOSS(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xac)					//时间同步丢失
		
 #define REG_MAC_TIMESTAMP_L(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xc)					//系统时标低32位，单位10ns
#define REG_MAC_TIMESTAMP_M(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x10)					//系统时标高32位，单位10ns
 #define REG_MAC_TIMESTAMP_H(chl,mode)				(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x84)					//上位机配置授时时间戳

//yanfei@add 8.5G加扰
#define REG_MAC_DISP_BYPASS(chl,mode)					(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x24)
#define REG_MAC_ARBFF_EN(chl,mode)						(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x30)

//lixin add 20240417 GLink仿真 帧数统计信息
#define REG_MAC_RECEIVE_FRAME_FC2_2_FC3(chl,mode)		(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x68)					//mac接收报文统计
#define REG_MAC_SEND_FRAME_FC3_2_FC2(chl,mode)			(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0xa4)					//mac发送报文统计
#define REG_MAC_RECEIVE_FRAME_CRC_ERROER(chl,mode)		(REG_1553_BASE_ADDR + (0x10000 * chl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x70)					//Fc2接收crc错误报文统计


/**********************************************************************************************************************************************/
/*																GT时钟域寄存器																 */
/**********************************************************************************************************************************************/

/*直接模式DRP配置端口速率，适用于KU系列FPGA芯片GTH的硬件板卡@zhuyb,20220902*/																					//0：正常模式；1：Near-end PCS Loop; 2:Near-end PMA Loop
#define REG_GTH_DRP_SEL								(REG_GT_BASE_ADDR + 0x0010)	    //DRP配置通道选择
#define REG_GTH_DRP_ADDR							(REG_GT_BASE_ADDR + 0x0014)	    //DRP配置地址
#define REG_GT_DRP_DI								(REG_GT_BASE_ADDR + 0x0018)	    //DRP配置数据
#define REG_GT_DRP_EN								(REG_GT_BASE_ADDR + 0x001c)	    //上升沿有效，置0，置1，再置0
#define REG_GT_DRP_WE								(REG_GT_BASE_ADDR + 0x0020)	    //读写选择寄存器 0是读操作 1是写操作
#define REG_GT_DRP_DO								(REG_GT_BASE_ADDR + 0x0024)	    //读DRP配置数据


//GTH DRP配置IPCore内部寄存器地址，采用二级寻址
#define GTH_TXOUT_DIV_ADDR							(0x0063)		
#define GTH_RXOUT_DIV_ADDR							(0x007c)		
#define GTH_RXCDR_CFG2_ADDR							(0x0010)      
#define GTH_RX_WIDEMODE_CDR_ADDR					(0x0066)       
#define GTH_PLL_SEL_MODE_GEN_ADDR					(0x00AD)       

#define REG_GT_CLK_MODE								(REG_GT_BASE_ADDR + 0x21C)	// 时钟芯片切换输出时钟频率寄存器 0：106.25Mhz 适配FC的1.0625G、2.125G、4.25G、8.5G 1：125Mhz 适配GLINK的2.5G、5G

//端口速率配置间接DRP配置模式通过寄存器配置，适用于GTX FPGA芯片硬件 @zhuyb,20220902

#define REG_GT_RESET								(REG_GT_BASE_ADDR + 0x30)	//bit0~bit7，对应CH0~CH7 GT复位

#define REG_GT_RATE									(REG_GT_BASE_ADDR + 0x60)	//从bit0开始，每4个bit对应一个通道的速率，速率取值GT_PORT_SPEED_1G、GT_PORT_SPEED_2G、GT_PORT_SPEED_4G

#define GT_PORT_SPEED_MASK(p)						(0xf << (p*4))		
#define GT_PORT_SPEED_SHIFT(p)						(p * 4)						

#define GT_PORT_SPEED_1G							(3)
#define GT_PORT_SPEED_2G							(2)
#define GT_PORT_SPEED_4G							(1)

//新版本时钟配置相关寄存器 @yanfei add 2023-11-03
#define REG_GT_BANK_NUM								(REG_GT_BASE_ADDR + 0x0218)	//bank数量，每1bit表示一个bank，等于1表示该bank有效。
#define REG_GT_BANK_SEL								(REG_GT_BASE_ADDR + 0x0004)	//需要配置的bank，低16位有效，每1位对应1个BANK。
#define REG_GT_CHAN_SEL								(REG_GT_BASE_ADDR + 0x0008)	//低4位有效，每1位对应BANK内的1个通道。
#define REG_GT_RATE0(nBank)							(REG_GT_BASE_ADDR + 0x0040+(nBank/2)*4)	//速率配置
//K7:100 : 1.0625、011 : 2.125、010 : 4.25；V7 :100 : 1.0625、011 : 2.125、010 : 4.25、001 : 8.5
#define REG_GT_CFG_APPLY							(REG_GT_BASE_ADDR + 0x0000)	//写入1触发配置保存，再写入0取消触发。
#define REG_GT_QPLL_REFCLKSEL						(REG_GT_BASE_ADDR + 0x0018) //V7
#define REG_GT_CPLL_REFCLKSEL0						(REG_GT_BASE_ADDR + 0x0028) //K7
//向0x14（QPLL） / 0x20（CPLL）写入1，触发PLL复位，再写入0解除复位。
#define REG_GT_QPLL_RESET							(REG_GT_BASE_ADDR + 0x0014) 
#define REG_GT_CPLL_RESET0							(REG_GT_BASE_ADDR + 0x0020) 
//向0x30（TX）和0x38（RX）写入1，触发软复位，再写入0解除复位。
#define REG_GT_SOFT_TX_RST0							(REG_GT_BASE_ADDR + 0x0030) 
#define REG_GT_SOFT_RX_RST0							(REG_GT_BASE_ADDR + 0x0038) 

/*******************************************************************************************************************/
/*										      ULP:FC-AE-1553 1553寄存器											   */
/*Modify:
		@zhuyb,20220806:添加chl识别，适用于通道独立模式下，不同通道的寻址，每通道偏移0x1_0000;参见《FC-AE-1553 IPCore使用手册》
*/
/*******************************************************************************************************************/
#define FC_1553_MODE_NC								(1)
#define FC_1553_NC_NT_ONLINE						(1)
#define FC_1553_NC_NT_OFFLINE						(0)

#define REG_FC1553_NC_ONLINE(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x0004)		//NC、NT在线
#define REG_FC1553_NC_NT_MODE(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6000)		//NC、NT功能切换，1：NC； 0：NT
#define REG_FC1553_MX_C2S_TOV(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x600C)		//NC_CS_TOV/NT_CS_TOV:	从命令序列开始到final状态序列的超时时间，NC侧一定要大于NT侧的此项设置
#define REG_FC1553_MX_C2S_BUR_TOV(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6010)		//NC_C-D/S_BURST_TOV/NT_C-D/S_BURST_TOV:	从命令序列开始，到应答Burst Size状态序列的超时时间。NC侧一定要大于NT侧的此项设置
#define REG_FC1553_CS2D_RX_TOV(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6014)		//C-S/D_RX_TOV:			命令序列至数据序列,数据序列至数据序列，状态序列至数据序列的超时时间 
#define REG_FC1553_RETRY_TOV(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6018)		//交换失败后，等待重传的超时时间

#define REG_FC1553_NC_EXCH_TOTOL_NUM(chl)			(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6020)		
#define REG_FC1553_NC_EXCH_ERR_NUM(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6024)		
#define REG_FC1553_NT_EXCH_TOTOL_NUM(chl)			(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6028)		
#define REG_FC1553_NT_EXCH_ERR_NUM(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x602C)		
#define REG_FC1553_NC_NT_CLEAN_EXCH_NUM(chl)		(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x601C)	

#define REG_FC1553_RX_ENABLE(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_RX_ENABLE)	 //使能接收

#define REG_FC1553_APINDEX(chl)						(REG_1553_BASE_ADDR + (chl * 0x10000)+ 0x2000)		//非周期消息调度起始IA，高16位有效
#define REG_FC1553_STREAM_CONTROL(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000)+ 0x2004)		//每次写的时候需要读此寄存器，如果小于10则不能写；初始值0x1FF

#define REG_FC1553_SET_PROTOCOL(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6004)	    //设置协议类型
																										//bit0:abts_en 目前大部分厂家没有使用ABTS帧，默认为0; 
																										//bit1:seq_cnt_mode 1553 x;Glink 0 
																										//bit2:cmd_payload_en 1553 x;Glink 1 
																										//bit3:sta_payload_en 1553 x;Glink 1 
																										//bit4 0为非GLinl协议 1为GLink协议
#define REG_FC1553_SET_GLINK_MODE(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_GLINK_MODE)	    //1bit 0：FC协议 1：GLINK协议

#define REG_FC1553_MTT_ERROR_FILTER(chl)			(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x6008)	    //屏蔽mtt消息异常 交换错误检查屏蔽位设置
#define REG_FC1553_SET_GTPORT(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x5008)	    //0-7为第一个通道 8-15为第二个通道
#define REG_FC1553_ROUND_TESTMODE(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_DMATEST_MODE)	    //DMA回环测试寄存器 1为开启回环测试

#define REG_FC1553_DOWN_DMA_NUM(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_DN_CNT)	    //dma下行帧计数
#define REG_FC1553_UP_DMA_NUM(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_UP_CNT)	    //dma上行帧计数
#define REG_FC1553_PROTOCOL_MAC_NUM(chl)			(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_MAC_CNT)	    //协议栈到mac帧数统计
#define REG_FC1553_MAC_PROTOCOL_NUM(chl)			(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_CH_CNT)	    //mac到协议栈帧数统计
#define REG_FC1553_RESET(chl)						(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x200)	    //协议栈复位

#define REG_FC1553_ARBF_EN(nChl,mode)				(REG_1553_BASE_ADDR + (0x10000 * nChl)+REG_MAC_BASE_ADDR + mode * 0x1000 + 0x30)		//arbf使能
#define REG_FC1553_SET_TIMEMODE(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + 0x500c)	    //协议栈模式寄存器 1bit有效，0表示普通周期模式，1表示外部触发模式

//预留寄存器 add by lx 20250516 
#define REG_FC1553_REDUN_TIMEOUT(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_REDUN_TIMEOUT)	    //32bit有效，冗余模块超时时间，单位u秒
#define REG_FC1553_REDUN_MODE(chl)					(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_REDUN_MODE)	    //1bit有效 1：冗余模式 0：独立模式
#define REG_FC1553_ELS_STATE_CNT(chl)				(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_ELS_STATE_CNT)	    //32bit有效
																															//[15:0]：1553数据帧计数
																															//[23:16]：上行els帧计数
																															//[31:24]：DMA上行帧计数
#define REG_FC1553_STATE(chl)						(REG_1553_BASE_ADDR + (chl * 0x10000) + REG_1553_L3_STATE)	    //32bit有效
																													//[7:0]:冗余上行通道0计数
																													//[15:8]: 冗余上行通道1计数
																													//[23:16] : 冗余上行通道仲裁4后上行计数
																													//[31:24] : 冗余上行状态机
#define REG_FC1553_NM_BASE_ADDR(nChl,mode)			(REG_1553_BASE_ADDR + (0x10000 * nChl) +REG_1553_L2_A_BASE_ADDR+ mode * 0x1000 + 0x800)	//GLink协议栈中NM功能站点基地址

#define REG_FC1553_OLT_EN(chl,mode)					(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_OLT_EN(mode))			//1bit有效 1: OLT 0 : ONU default: ONU
#define REG_FC1553_LOOP_EN(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_LOOP_EN(mode))			//1bit有效 1: 开启回环 0 : 关闭回环
#define REG_FC1553_GT_RATE(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_GT_RATE(mode))			//GT速率，32bit有效 0：1.0625 1：2.125 2：4.25 default：8.5
#define REG_FC1553_WAIT_TIME(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_WAIT_TIME(mode))			//PON模式超时时间，使用默认
#define REG_FC1553_GAP_TIME(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_GAP_TIME(mode))			//使用默认
#define REG_FC1553_GT_EN(chl,mode)					(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_GT_EN(mode))			//光使能 1：关闭光使能 0：打开光使能
#define REG_FC1553_SCR_BYPASS(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_SCR_BYPASS(mode))			//1bit有效 1：不加扰 0：加扰 8.5G加扰打开
#define REG_FC1553_PON_MODE(chl,mode)				(REG_1553_BASE_ADDR + (chl * 0x10000)+REG_1553_L1_PON_MODE(mode))			//1bit有效 0:交换式 1：总线式



/*****************************************************************************************************************/
/*
 *	GLK200版本。L4层发生变化
*/
/*****************************************************************************************************************/

//由FPGA来定义
//#define REG_DMA_NOTIFY_RESET	(0x804)  //复位通知
#define REG_DMA_FPGA_RESET		(0x800)    //FPGA复位寄存器



/*
 * 定义多个站点的基地址，根据寄存器手册，目前定义了8个站点
 * ST:Station 
 * PORTAB:PORTA or PORTB
*/
#define 	REG_FIBER_STATION_BASE(ST)	(0X00080000 + ST * 0X00010000)

/*
 * L3层基地址
*/
#define		REG_FIBER_L3_BASE(ST)			(REG_FIBER_STATION_BASE(ST) + 0X8000)

/*
 * L2层基地址
*/
#define		REG_FIBER_L2_BASE(ST,PORTAB)		(REG_FIBER_STATION_BASE(ST) + 0X9000 + PORTAB * 0X1000)

/*
 * L1层基地址
*/
#define		REG_FIBER_L1_BASE(ST,PORTAB)		(REG_FIBER_STATION_BASE(ST) + 0XB000 + PORTAB * 0X1000)


/*
 GLK200 系统定义的寄存器
*/
/*
 协议栈基地址，都是基于站点（通道定义）
*/
#define		REG_PROTOCAL_L4_BASE(X)		REG_FIBER_STATION_BASE(X)


/*
 每个通道的板载DDR，为上行DMA空间保留的容量，单位B
*/
#define CVG_OBDN_DEFAULT_MEMSIZE	(512*1024*1024)

/*
 板载内存大小寄存器,单位bit
*/
#define	REG_OBMS_OFFSET				(REG_PROTOCAL_L4_BASE(0) + 0X0010)


/*
 协议栈复位寄存器
*/
#define REG_PRO_RESET(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0030)

/*
 FC1553 NC、NT模式寄存器
*/
#define REG_FC1553_NX_MODE_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X0034)

/*
 * Bit0：abts_en
1：使能ABTS；
0：不使能；
FC协议中规定ABTS是可选项,如果使能ABTS，若NC检查到超时或者错误的状态/数据序列，则发送ABTS以结束交换。目前大部分厂家没有使用ABTS帧，默认0。
bit1:seq_cnt_mode 
1553 ：x
Glink： 0
bit2:cmd_payload_en 
1553 ：x
Glink ：1 
bit3:sta_payload_en 
1553 ：x
Glink ：1 
bit4 ：
0：非GLink协议 
1：GLink协议
该寄存器常用配置：
1553 ：5’h0C
Glink ：5’h1C
*/
#define	REG_PROTOCAL_SET_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X0038)



/*
 * 32bit有效，设置交换错误检查的屏蔽位
*/
#define REG_FC1553_EXCH_ERRMSK_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X403c)

/*
 * 32bit有效，从命令序列开始，到结束状态序列的超时时间，单位微秒。NC侧 一定要大于NT侧的此项设置
*/
#define REG_FC1553_EXCH_TOV_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X0040)

/*
 * 32bit有效，从命令序列开始，到应答Burst Size状态序列的超时时间，单位微秒。NC侧一定要大于NT侧的此项设置
*/
#define REG_FC1553_EXCH_BUR_TOV_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X0044)

/*
 * 32bit有效，命令序列至数据序列,数据序列至数据序列，状态序列至数据序列的超时时间，单位微秒
*/
#define REG_FC1553_EXCH_TRX_TOV_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X0048)

/*
 * 32bit有效，交换失败后，等待重传的超时时间，单位微秒
*/
#define REG_FC1553_EXCH_RETRY_TOV_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X004c)

/*
bit[3:0]有效，计数器清零
Bit0：清除NC总交换计数
Bit1：清除NC错误交换计数
Bit2：清除NT总交换计数
Bit3：清除NT错误交换计数
*/
#define REG_FC1553_CLEAR_COUNTER_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X0050)

/*
 * 配置板载DDR的上行存储基地址空间(NC)。
 */
#define	REG_UPMEM_NC_OB_BASE_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0054)

 /*
  * 配置板载DDR的上行存储基地址空间(ASM高优先级)。
 */
#define REG_UPMEM_ASM_HP_OB_BASE_OFF(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X4034)


 /*
  * 配置板载DDR的上行存储基地址空间(NT)。
  */
#define	REG_UPMEM_NT_OB_BASE_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0058)

  /*
   * 配置板载DDR的上行存储基地址空间(ASM低优先级)。
  */
#define REG_UPMEM_ASM_LP_OB_BASE_OFF(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X4038)

#if defined(GLK_FC_ASM)
	#define REG_UPMEM_HP_BASE_OFF(ST)			REG_UPMEM_ASM_HP_OB_BASE_OFF(ST)
	#define REG_UPMEM_LO_BASE_OFF(ST)			REG_UPMEM_ASM_LP_OB_BASE_OFF(ST)
/*
 ASM上，使用同一个配置，参考寄存器文档
*/
	#define	REG_UPLOAD_TOV_OFF(ST)				(REG_PROTOCAL_L4_BASE(ST) + 0X4040)
	/*
	 * 低优先级接收缓冲区自动读取容量门限
	 */
	#define	REG_UPMEM_ASM_HP_AUTOREAD_TOV_OFF(ST)			REG_UPLOAD_TOV_OFF(ST)
	/*
	 高优先级接收缓冲区超时门限
	 */
	#define	REG_UPMEM_ASM_HP_AUTOREAD_THRESHOLD_OFF(ST)			REG_UPLOAD_TOV_OFF(ST)

	/*
	* 低优先级接收缓冲区超时门限
	*/
	#define	REG_UPMEM_ASM_LP_AUTOREAD_THRESHOLD_OFF(ST)			REG_UPLOAD_TOV_OFF(ST)

	#define REG_GROUP_RX_CNT_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X4044)	
	#define REG_CHECK_RX_CNT_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X4048)	
#elif defined(GLK_FC_AE_1553)	
	#define REG_UPMEM_HP_BASE_OFF(ST)			REG_UPMEM_NC_OB_BASE_OFF(ST)
	#define REG_UPMEM_LO_BASE_OFF(ST)			REG_UPMEM_NT_OB_BASE_OFF(ST)
	#define	REG_UPMEM_NC_AUTOREAD_THRESHOLD_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X005C)
	#define	REG_UPLOAD_TOV_OFF(ST)				REG_UPMEM_NC_AUTOREAD_THRESHOLD_OFF(ST)
	  /*
	   * 低优先级接收缓冲区自动读取容量门限
	  */
	#define	REG_UPMEM_NT_AUTOREAD_THRESHOLD_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0060)
	  /*
	   * 高优先级接收缓冲区超时门限
	  */
	#define	REG_UPMEM_NC_AUTOREAD_TOV_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0064)

	  /*
	   * 低优先级接收缓冲区超时门限
	  */
	#define	REG_UPMEM_NT_AUTOREAD_TOV_OFF(ST)			(REG_PROTOCAL_L4_BASE(ST) + 0X0068)

#endif

/*
 * 高优先级接收缓冲区自动读取容量门限
*/


/*
 *	rx_drd的模式设置
 *	i_rx_drd_mode定义
 *	bit[1:0],读数模式
 *		2'b00: 模式0，有消息则立即读取
 *		2'b01: 模式1，消息缓存量超过门限值，或者超时时间到后开始读取
 *		2'b10: 模式2，通过寄存器或者IO触发信号强制读取，上升沿触发一次
 *	bit[2:2],ASM消息的输出时序：
 *		1'b0 : 时序0，适用于输出给软件解析，也适用于IP核跟控制器之间的通信包长度小于ASM消息长度的（如DMA）场合，ASM消息将被拆成若干个4KB以内的包传输
 *	 	1'b1 : 时序1，适用于FPGA直接处理场合，ASM消息无拆包处理，支持超长包传输。
 *	bit[3:3],ASM消息的请求模式：
 *		1'b0 : H 和L 公平仲裁，1553使用模式
 *		1'b1 : H 和L 优先级仲裁，ASM使用模式
*/
#define REG_UPMEM_RX_DRD_MODE(ST)					(REG_PROTOCAL_L4_BASE(ST) + 0X006C)

/*
 ASM优先级使能寄存器
*/
#define REG_ASM_PRIORITY_EN_REG(ST)						(REG_PROTOCAL_L4_BASE(ST) + 0X0074)


/*
 * FC-AE 1553上行字节序控制寄存器
 * BIT1-0有效。
 * 2'b0: 以Byte为单位高低Byte倒序
 * 2'b1: 不做任何调整
 * 2'b2: 以word为单位高低word倒序，word内以Byte为单位高低Byte倒序
 * 2'b3: 以word为单位高低word倒序，word内不调整
*/
#define REG_DMAUP_FC1553_BYTE_ENDIAN_REG(ST)					(REG_PROTOCAL_L4_BASE(ST) + 0X0078)
/*
 和FCAE-1553一样，只是偏移不同
*/
#define REG_DMAUP_ASM_BYTE_ENDIAN_REG(ST)					(REG_PROTOCAL_L4_BASE(ST) + 0X4030)



/*
 调度写使能，以便逻辑读取其中的配置
*/
#define 	REG_SCH_WEN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X1000)

/*
 调度读取使能
*/
#define 	REG_SCH_REN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X1004)

/*
 调度ram索引,写的时候写入索引号
*/
#define 	REG_SCH_RAMADDR_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X1008)

/*
 *  	sch_ram_wdata0[31:4]:RSV，保留
 *  	sch_ram_wdata0[3:2],:TYPE,
 *  	0：非周期高（非法值）
 *  	1：非周期低（非法值）
 *	2: 周期高
 *	3: 周期低
 *	sch_ram_wdata0[1]:ACT, 当前调度信息有效
 *	sch_ram_wdata0[0]:END, 1表示当前为最后一条有效的索引。
*/
#define 	REG_SCH_RAMD0_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X100C)

/*
 T_OFFSET ，
	T_OFFSET[31]：1表示T_OFFSET[30:0]的值有效，0表示该消息在周期内紧密发送；
	T_OFFSET[30:0]：该条消息的发送时间偏移，单位为us，适用于周期消息
*/
#define 	REG_SCH_RAMD1_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1010)

/*
表示周期的大小，单位微秒
*/
#define 	REG_SCH_CYCLE_SIZE_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1018)

/*
周期消息保护预留时间设置，单位微秒，推荐默认值为10
*/
#define 	REG_SCH_CYCLE_PROTM_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X101C)

#if 0
/*
 *	 cycle_set[0]: cycle_trig周期开始触发标识,周期调度开始需要先写1，再写0
 *	cycle_set[1]:cycle_run,周期调度在线标识，0标识周期不运行，1表示周期运行，当想结束周期调度消息可以将该寄存器写0
 *	cycle_set[2]: cycle_sync，周期消息的同步模式，0：内部触发，1：外部触发
*/
#define		CYCLE_START_BIT	(0)
#define		CYCLE_RUN_BIT	(1)
#define		CYCLE_SYNC_BIT	(2)
#define		REG_CYCLE_SET_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1020)
#else
/*
 * 设置同步模式，
 * 0：内部，1：外部
*/
#define		REG_CYCLE_MODE_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1020)

/*
 周期开始调度寄存器，写1到0跳变
*/
#define		REG_CYCLE_TRIG_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1024)
/*
 周期在线表示，可控制停止
*/
#define		REG_CYCLE_RUN_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1028)
#endif
/*
 * ASM消息配置
 * */
 /*
  ASM MsgTab RAM0的写使能，所有data写完后，写1写0触发
 */
#define 	REG_ASM_MSGTB_WEN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2000)

 /*
  ASM MsgTab RAM0的读取使能，所有data写完后，写1写0触发
 */
#define 	REG_ASM_MSGTB_REN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2004)

 /*
  存放消息信息的RAM的索引，每一条消息对应一个索引，最大支持512条消息，该地址与SchTab的地址对应
  TB索引号
 */
#define 	REG_ASM_MSGTB_TBI_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2008)

 /*
  msg ID，ASM消息的Message ID
 */
#define 	REG_ASM_MSGTB_D0_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X200C)

 /*
  asm_msgtab_ram0_data1[0]：1表示该消息已经配置生效，0表示无效消息。
 asm_msgtab_ram0_data1[1]：1表示当前为最后一条有效的索引。
 asm_msgtab_ram0_data1[3:2]：0：非周期高
 1：非周期低
 2: 周期高
 3: 周期低
 asm_msgtab_ram0_data1[31:4]：保留

 */
#define 	REG_ASM_MSGTB_D1_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2010)


 /*
  FC MsgTab RAM0的写使能，写1写0触发
 */
#define		REG_FC1553_MGSTB_WEN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2014)

 /*
 FC MsgTab RAM0的读使能，写1写0触发
 */
#define		REG_FC1553_MGSTB_REN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2018)

 /*
  存放消息信息的RAM的地址，每一条消息对应一个地址，最大支持512条消息，该地址与SchTab的地址对应。
 */
#define		REG_FC1553_MGSTB_TBI_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X201C)

 /*
 fc_msgtab_ram0_data0[7:0]: 对应1553B的相关的命令位，与执行原指令字中Xflags相同。
 fc_msgtab_ram0_data0[31：8]: SID
 */
#define		REG_FC1553_MGSTB_RAM0_D0_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2020)

 /*
 fc_msgtab_ram0_data1[0]: 1表示该消息已经配置生效，0表示无效消息。
 fc_msgtab_ram0_data1[1]: 1表示当前为最后一条有效的索引。
 fc_msgtab_ram0_data1[3:2]: 0：非周期高
 1：非周期低
 2: 周期高
 3: 周期低
 fc_msgtab_ram0_data1[4]: Priority Enable，F_CLT中的优先级使能位
 fc_msgtab_ram0_data1[5]: GLink模式下，表示Smart长消息类型时为1，其它情况下为0
 fc_msgtab_ram0_data1[7:6]: 重传次数
 fc_msgtab_ram0_data1[31:8]: DID
 */
#define		REG_FC1553_MGSTB_RAM0_D1_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2024)


 /*
 fc_msgtab_ram0_data2[31:0]: SA
 */
#define		REG_FC1553_MGSTB_RAM0_D2_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2028)


 /*
 fc_msgtab_ram0_data3[31:0]: DC/MC，数据长度或者模式码
 */
#define		REG_FC1553_MGSTB_RAM0_D3_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X202C)


 /*
 fc_msgtab_ram0_data4[31:0]: MA/OPID ，Multicast Addr/Other Port ID, NT->NT传输时使用，表示多播地址或者接收NT地址
 */
#define		REG_FC1553_MGSTB_RAM0_D4_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2030)


 /*
 fc_msgtab_ram0_data5[31:0]: Other-SA，NT->NT接收NT子地址
 */
#define		REG_FC1553_MGSTB_RAM0_D5_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2034)

 /*
 预留
 */
#define		REG_FC1553_MGSTB_RAM0_D6_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2038)


 /*
 MsgTab RAM1的写使能，所有data写完后，写1写0触发
 */
#define 	REG_PRO_MSGTB_RAM1_WEN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X203c)

 /*
 MsgTab RAM1的读使能，所有data写完后，写1写0触发
 */
#define 	REG_PRO_MSGTB_RAM1_REN_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2040)

 /*
 放消息信息的RAM的索引，每一条消息对应一个索引，最大支持512条消息，该地址与SchTab的地址对应。
 */
#define 	REG_PRO_MSGTB_RAM1_TBI_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2044)

 /*
  软件通过这个寄存器获取逻辑读取的位置，可以通过这个寄存器计算逻辑有多少剩余空间
 */
#define		REG_PRO_MSGTB_RAM1_RPTR_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X204C)
#define		REG_PRO_MSGTB_RAM1_WPTR_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2048)
 /*
 msgtab_ram1_data2[18:0]: ST_ADDR，每条消息的DDR存储首地址，软件维护，根据bms大小从地址0开始为每条消息分配DDR地址空间，DDR大小逻辑提供。
 msgtab_ram1_data2[22:19]:BMS, 该消息的缓存空间大小，计算方式为：(2^BMS) * 4KB
 msgtab_ram1_data2[23]: SAP, 1表示Sample类型，即该消息的缓存中仅存储一条消息，可重复发送。
 0表示Queue类型，即该消息的缓存中可存储多条消息，不能重复发送。

 */
#define 	REG_PRO_MSGTB_RAM1_D2_OFFSET(ST)	(REG_PROTOCAL_L4_BASE(ST) + 0X2050)

 /*
 ch_ram_wdata2[31:16]: IPI，第一次执行相对于总线第一个基本周期的位置。
 sch_ram_wdata2[15:0]: ICI，消息执行的间隔，间隔以基本周期为单位。
 */
#define 	REG_SCH_RAMD2_OFFSET(ST)		(REG_PROTOCAL_L4_BASE(ST) + 0X1014)


/*
 * 站点L1层控制寄存器0
*/
#define		REG_FC_L1_CTL0_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X30)


/*
 * 站点L1层控制寄存器1
*/
#define		REG_FC_L1_CTL1_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X34)


/*
 * 站点L1层控制寄存器2
*/
#define		REG_FC_L1_CTL2_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X38)


/*
 * 站点L1层控制寄存器3
*/
#define		REG_FC_L1_CTL3_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X3C)

/*
 * 站点L1层控制寄存器3
*/
#define		REG_FC_L1_CTL3_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X3C)

/*
 * 站点L1层控制寄存器3
*/
#define		REG_FC_L1_CTL3_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X3C)

/*
 * 站点L1层控制寄存器4
*/
#define		REG_FC_L1_CTL4_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X40)

/*
 * 站点L1层控制寄存器5
*/
#define		REG_FC_L1_CTL5_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X44)

/*
 * 站点L1层控制寄存器6
*/
#define		REG_FC_L1_CTL6_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X48)


/*
 * 站点L1层控制寄存器7
*/
#define		REG_FC_L1_CTL7_OFFSET(ST,PORTAB)	(REG_FIBER_L1_BASE(ST,PORTAB) + 0X4C)

/*
 * 站点L2层控制寄存器0
*/
#define		REG_FC_L2_CTL7_OFFSET(ST,PORTAB)	(REG_FIBER_L2_BASE(ST,PORTAB) + 0X4C)

/*
 * 站点FC/GLINK协议选择
*/
#define		REG_FC_L3_CTL0_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X30)

/*
 *站点上行透传模式
*/
#define		REG_FC_L3_CTL1_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X34)

/*
 *站点L3层 DID匹配设置
*/
#define		REG_FC_L3_CTL2_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X38)

/*
 *站点L3层 接收使能匹配设置
*/
#define		REG_FC_L3_CTL3_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X3C)

/*
 *站点L3层 冗余单位超时设置
*/
#define		REG_FC_L3_CTL4_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X40)

/*
 *站点L3层 冗余模式设置
*/
#define		REG_FC_L3_CTL5_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X44)

/*
 *站点L3层 DMA测试模式
*/
#define		REG_FC_L3_CTL6_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X48)


/*
 *站点L3层 统计0
*/
#define		REG_FC_L3_STATE0_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X110)

/*
 *站点L3层 统计1
*/
#define		REG_FC_L3_STATE1_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X114)


/*
 *站点L3层 统计2
*/
#define		REG_FC_L3_STATE2_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X118)

/*
 *站点L3层 统计3
*/
#define		REG_FC_L3_STATE3_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X11C)

/*
 *站点L3层 统计4
*/
#define		REG_FC_L3_STATE4_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X120)

/*
 *站点L3层 统计5
*/
#define		REG_FC_L3_STATE5_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X124)

/*
 *站点L3层 统计6
*/
#define		REG_FC_L3_STATE6_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X128)

/*
 *站点L3层 统计7
*/
#define		REG_FC_L3_STATE7_OFFSET(ST)	(REG_FIBER_L3_BASE(ST) + 0X12C)

/**************************************************/
/*图像切割的寄存器定义*/
#define		IMG_BASE_OFFSET(ST,X)			(REG_PROTOCAL_L4_BASE(0) + 0X5000 + (X)*0X80)

#define		REG_IMG_BASE_FIG_CTRL(ST,X)			(IMG_BASE_OFFSET(ST,X) + 0)
#define		REG_IMG_BASE_FIG_FULLCNT(ST,X)		(IMG_BASE_OFFSET(ST,X) + 4)
#define		REG_IMG_BASE_FIG_MSG_INDEX(ST,X)	(IMG_BASE_OFFSET(ST,X) + 8)
#define		REG_IMG_BASE_FIG_BASE_ADDR(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0XC)
#define		REG_IMG_BASE_FIG_MEM_SIZE(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X10)
#define		REG_IMG_BASE_FIG_SML_SIZE(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X14)
#define		REG_IMG_BASE_FIG_BIG_RES_M(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X18)
#define		REG_IMG_BASE_FIG_BIG_RES_N(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X1C)
#define		REG_IMG_BASE_FIG_SML_RES_X(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X20)
#define		REG_IMG_BASE_FIG_SML_RES_Y(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X24)
#define		REG_IMG_BASE_FIG_OL_I(ST,X)		(IMG_BASE_OFFSET(ST,X) + 0X28)
#define		REG_IMG_BASE_FIG_OL_J(ST,X)		(IMG_BASE_OFFSET(ST,X) + 0X2C)
#define		REG_IMG_BASE_FIG_SML_CUR_M(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X30)
#define		REG_IMG_BASE_FIG_SML_CUR_N(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X34)
#define		REG_IMG_BASE_FIG_DARK_ALPHA(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X38)
#define		REG_IMG_BASE_FIG_DARK_BETA(ST,X)	(IMG_BASE_OFFSET(ST,X) + 0X3C)
#define		REG_IMG_BASE_FIG_RESET(ST,X)		(IMG_BASE_OFFSET(ST,X) + 0x48)
/**************************************************/
