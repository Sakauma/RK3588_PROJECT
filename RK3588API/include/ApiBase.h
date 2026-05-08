#ifndef GLKAPI_H
#define GLKAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../platform.h"
#include "errcode.h"
#include "BaseDef.h"

/*************************************************************/

/*
 导出的API函数
*/

int		cvg_scan_devcnt();
/*
 * 函数说明：
 *		读取寄存器
 * 参数：
 *		@1：板卡设备句柄
 *		@2：寄存器
 *		@3：读取的返回值
*/
int 		cvg_card_readreg(OS_HANDLE pBHandle,int offset,unsigned int * Ack);


/*
 * 函数说明：
 *		写寄存器
 * 参数：
 *		@1：板卡设备句柄
 *		@2：寄存器
 *		@3：待写入的值
*/
int 		cvg_card_writereg(OS_HANDLE pBHandle,int offset,unsigned int Value);

/*
 * 函数说明：
 *		读取板卡硬件环境中DDR的容量
 * 参数：
 *		@1：板卡设备句柄
 *		@：返回的板载DDR的容量，单位B
*/
int 		cvg_card_mem_get(OS_HANDLE pBHandle,unsigned int * pMemSize);

/*
 *  函数说明：
 *  	使用文件名的方式打开设备，在linux架构平台上，建议使用byname方式，
 *  参数：
 *  	@1：设备文件名，例如"/dev/xxx"
 *  	@2: 打开设备时是否强制硬复位硬件。
 *  	@3：内存管理方式，0：自动管理，建议使用自动管理，通道之间平均分配板载1GB DDR， 	1：手动管理
*/
OS_HANDLE 	cvg_card_open_byname(char * pName,int reset);

/*
 * 函数说明：
 * 	打开设备，使用索引号的方式。建议在Windows系统上使用索引号的方式
 *  @3：当前版本无意义
 * */
OS_HANDLE 	cvg_card_open(unsigned int nDevIndex,int reset);

/*
 * 函数说明：
 * 	关闭板卡
 * 参数：
 * 	@1：设备句柄
 * */
int 		cvg_card_close(OS_HANDLE pBHandle);

/*
 * 函数说明：
 *		复位FPGA内部的逻辑
 * 参数：	
 *		@1：设备句柄
*/
int			cvg_card_reset(OS_HANDLE pBHandle);

/*
 * 函数说明：
 * 	打开通道。
 * 参数：
 * 	@1：设备句柄
 * 	@2：通道号
 * */
OS_HANDLE 	cvg_open_chan(OS_HANDLE pBHandle, int upChs, int downChs);



/*
 * 函数说明：
 * 	关闭通道
 * 参数：
 * 	@1：通道句柄
 * */
int 		cvg_close_chan(OS_HANDLE pChHandle);

/*
 * 函数说明：
 * 	协议栈启动
 * 参数：
 * 	@1：通道句柄
 * 	@2：触发模式，默认为0（当前版本只支持0）
 * */
int 		cvg_pro_start(OS_HANDLE pChHandle,int TrigMode);

/*
 * 函数说明：
 * 	协议栈停止
 * 参数：
 * 	@1：通道句柄
 * */
int		cvg_pro_stop(OS_HANDLE pChHandle);

/*
 * 函数说明：
 * 	注册协议栈接收到数据的回调函数
 * 参数：
 * 	@1：通道句柄
 * 	@2：回调函数的用户上下文
 * 	@3：回调函数指针
 * */
int 		cvg_pro_register_cb(OS_HANDLE pChHande,void * pUserContext,funUpDmaCallBack CB);

/*
 * 函数说明：
 * 	用于针对某个通道设置消息数量，只有在设置消息数量后，才可以执行后面的添加消息等步骤
 * 	在设置完后，在添加消息时，需要按照周期高、低优先级、非周期高、低优先级的顺序添加
 * 参数：
 * 	@1：通道句柄
 * 	@2：消息数量，消息数量是包含周期、非周期等全部的数量。而不是指特定某种类型的。
 * 
 * */
int 		cvg_pro_cfg_msgsize(OS_HANDLE pChHandle,int msgCnt);


/*
 * 函数说明
 * 	针对特定的消息索引号发送数据
 * 参数：
 * 	@1：通道句柄
 * 	@2：待发送的用户负载指针
 * 	@3：负载长度，单位字节
 * 	@4：消息索引号
 * */
int 		cvg_pro_write_data(OS_HANDLE pChHandle,unsigned char *pBuffer,int pBufLen,int msgIndex);


/*
 * 函数说明：
 * 	透传消息，不经过协议栈处理
 * 参数：
 * 	@1：通道句柄
 * 	@2：待发送的用户负载指针
 * 	@3：负载长度
 * 	@4：消息索引号
 * 	@5：直传标志位，用来填写DMA包头的SUF和WT两个字段
 * */
int 		cvg_pro_direct_write_data(OS_HANDLE pChHandle, unsigned char* pBuffer, int pBufLen, int msgIndex, int SUPWT);


/*
 * 函数说明：
 * 	临时获取消息的调度部分结构体指针
 * 参数：
 * 	@1：通道句柄
 * 	@2：消息索引号
 * */
struct SCHE_TB* cvg_sche_get(OS_HANDLE pChHandle,int msgindex);


/*
 * 函数说明：
 * 	添加消息，通用API，根据在glkconfig中定义编译条件，例如ASM还是FC1553,传入的pMsg是满足特性消息的指针
 * 参数：
 * 	@1：通道句柄
 * 	@2：消息结构体
*/
int 		cvg_addmsgtb(OS_HANDLE pChHandle,void *pMsg);

/*
 * 函数说明：
 * 	使能消息,函数可以在系统运行时刻，单独使能某个消息
 * 参数：
 * 	@1：通道句柄
 * 	@2：消息ID
 * 	@3: 使能:1,使能，0：禁用
*/
int		cvg_msg_enable(OS_HANDLE pChHande,int msgIndex,int en);


/*
 * 函数说明：
 * 	通道准备消息，通过该函数，让API把用户添加的消息配置到硬件中。硬件开始初始化协议栈的各种参数，并准备启动
 * 参数：
 * 	@1：通道句柄
 * */
int 		cvg_init_msgtb(OS_HANDLE pChHandle);

/* 
 * FC1553协议栈设置模式
 * 	参数：
 * 	@1：通道句柄；
 * 	@2：模式1：NC，0：NT
*/
int 		cvg_FC1553_setMode(OS_HANDLE pChHandle,int Mode);

/*
 * 函数说明：
 * 	FC1553设置交换交换错误使能位
 * 参数：
 * 	@1：通道句柄
 * 	@2：错误屏蔽码
*/
int 	cvg_FC1553_setErrMask(OS_HANDLE pChHandle,unsigned int EnMask);

/*
 * 函数说明：
 * 		FC1553设置交换超时： 32bit有效，从命令序列开始，到结束状态序列的超时时间，单位微秒。NC侧 一定要大于NT侧的此项设置
 * 参数：
 *		@1:通道句柄
 *		@2：超时值，单位us
*/
int	 	cvg_FC1553_setExchTOV(OS_HANDLE pChHandle,int tov);

/*
  * 函数说明：
 * 		FC1553设置交换超时： 32bit有效，从命令序列开始，到应答Burst Size状态序列的超时时间，单位微秒。NC侧一定要大于NT侧的此项设置
 * 参数：
 *		@1:通道句柄
 *		@2：超时值，单位us
*/
int	 	cvg_FC1553_setExchBurTOV(OS_HANDLE pChHandle,int tov);


/*
  * 函数说明：
 * 		FC1553设置交换超时： 32bit有效，命令序列至数据序列,数据序列至数据序列，状态序列至数据序列的超时时间，单位微秒
 * 参数：
 *		@1:通道句柄
 *		@2：超时值，单位us
*/
int	 	cvg_FC1553_setExchTrxTOV(OS_HANDLE pChHandle,int tov);


/*
  * 函数说明：
 * 		FC1553设置交换超时： 32bit有效，交换失败后，等待重传的超时时间，单位微秒
 * 参数：
 *		@1:通道句柄
 *		@2：超时值，单位us
*/
int	 	cvg_FC1553_setExchRetryTOV(OS_HANDLE pChHandle,int tov);


/*
 * 函数说明：
 * 		FC1553协议栈清空交换错误信息
 * 参数：
 * 		@1:通道句柄
*/
int cvg_FC1553_clearExchCnt(OS_HANDLE pChHandle);

/*
 * 函数说明
 *		FC-AE-1553与Glink兼容设置。
 *	Bit0：abts_en
 *		1：使能ABTS；
 *		0：不使能；
 *		FC协议中规定ABTS是可选项,如果使能ABTS，若NC检查到超时或者错误的状态/数据序列，则发送ABTS以结束交换。目前大部分厂家没有使用ABTS帧，默认0。
 *	bit1:seq_cnt_mode 
 *		1553 ：x
 *		Glink： 0
 *	bit2:cmd_payload_en 
 *		1553 ：x
 *		Glink ：1 
 *	bit3:sta_payload_en 
 *		1553 ：x
 *		Glink ：1 
 *	bit4 ：
 *		0：非GLink协议 
 *		1：GLink协议
 *	该寄存器常用配置：
 *	1553 ：5’h0C
 *	Glink ：5’h1C
*/
int cvg_EnhancePro_set(OS_HANDLE pChHandle, int Mask);

/*
 * 高优先级（ASM）/NC接收缓冲区自动读取容量门限
 * 	参数：
 *		@1:通道句柄
 *		@：门限值，单位8字节
*/
int cvg_pro_hprx_threshold_set(OS_HANDLE pChHandle,int threshold);
#define cvg_pro_nc_threshold_set cvg_pro_hprx_threshold_set

/*
 * 低优先级（ASM）/NT接收缓冲区自动读取容量门限
 * 	参数：
 *		@1:通道句柄
 *		@：门限值，单位8字节
*/
int cvg_pro_lprx_threshold_set(OS_HANDLE pChHandle,int threshold);
#define cvg_pro_nt_threshold_set cvg_pro_lprx_threshold_set


/*
 * 高优先级（ASM）/NC接收缓冲区自动读取容量门限
 * 	参数：
 *		@1:通道句柄
 *		@：门限值，单位1us
*/
int cvg_pro_hprx_tov_set(OS_HANDLE pChHandle,int tov);
#define cvg_pro_nc_tov_set cvg_pro_hprx_tov_set

/*
 * 低优先级（ASM）/NT接收缓冲区自动读取容量门限
 * 	参数：
 *		@1:通道句柄
 *		@：门限值，单位1us
*/
int cvg_pro_lprx_tov_set(OS_HANDLE pChHandle,int tov);
#define cvg_pro_nt_tov_set cvg_pro_lprx_tov_set

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
int cvg_pro_rx_drd_mode_set(OS_HANDLE pChHandle,int mode);

int cvg_pro_rx_drd_mode_get(OS_HANDLE pChHandle,int *mode);

/*
 * 函数说明：
 * 		对cvg_pro_rx_drd_mode_set函数的扩展，只更改低2位
*/
#define cvg_pro_rx_drd_rdmode_set(HD ,V) {\
	unsigned int ordm  = 0;\
	cvg_pro_rx_drd_mode_get(HD,&ordm);\
	ordm &= ~3;\
	ordm |= V & 3;\
	cvg_pro_rx_drd_mode_set(HD,ordm);\
}

/*
 * 函数说明：
 * 		对cvg_pro_rx_drd_mode_set函数的扩展，只更改bit2位
*/
#define cvg_pro_rx_drd_outmode_set(HD ,V) {\
	unsigned int ordm  = 0;\
	cvg_pro_rx_drd_mode_get(HD,&ordm);\
	ordm &= ~(1<<2);\
	ordm |= V & (1<<2);\
	cvg_pro_rx_drd_mode_set(HD,ordm);\
}

/*
 * 函数说明：
 * 		对cvg_pro_rx_drd_mode_set函数的扩展，只更改低bit3位
*/
#define cvg_pro_rx_drd_arbmode_set(HD ,V) {\
	unsigned int ordm  = 0;\
	cvg_pro_rx_drd_mode_get(HD,&ordm);\
	ordm &= ~(1<<3);\
	ordm |= V & (1<<3);\
	cvg_pro_rx_drd_mode_set(HD,ordm);\
}



/*
 函数说明：获取是1553还是Glink协议
 参数：
	@1：通道句柄；
	@2：返回的寄存器值，
	bit4 ：
	0：非GLink协议
	1：GLink协议
*/
int cvg_EnhancePro_get(OS_HANDLE pChHandle, int* Mask);

/*
 * 函数说明：
 *		辅助函数，通过通道句柄，返回通道号
 * 参数：
 *		@1：通道句柄
*/
int			aux_cvg_chan_tonum(OS_HANDLE pCHandle);

/* 
 * 函数说明：
 *		协议栈复位，每个通道运行一个独立的协议栈，因此协议栈复位关联通道
 * 参数：
 *		@1：通道句柄
*/
int			cvg_pro_reset(OS_HANDLE pChHandle);


/*
 * 设置协议栈周期同步模式，1：外部触发，0：内部同步
 *  参数：
 *  	@1：通道句柄
 *		@2：同步模式
*/
int			cvg_pro_cycle_syncmode(OS_HANDLE pChHandle,int SyncMode);

/*
 * 设置协议栈调度周期
 *  参数：
 *  	@1：通道句柄
 *		@2：周期值,单位1uS
*/
int			cvg_pro_cycle_size(OS_HANDLE pChHandle, int CycleSize);

/*
 * 设置协议栈调度周期保护时间
 *  参数：
 *  	@1：通道句柄
 *		@2：周期值,单位1uS
*/
int			cvg_pro_cycle_protecttm(OS_HANDLE pChHandle, int protecttm);


/*
 * 函数说明：
 * 		读取逻辑版本号。
 * 参数：	
 *		@1：板卡句柄
 *		@2：待返回的版本号
 * 
*/
int		aux_cvg_card_lc_version(OS_HANDLE pBHandle, unsigned int* Version);


/*
 * 函数说明：clk_setoutmode
 *	 切换时钟芯片切换输出时钟频率寄存器 0：106.25Mhz 适配FC的1.0625G、2.125G、4.25G、8.5G 1：125Mhz 适配GLINK的2.5G、5G
*/
int		aux_cvg_card_clk_som(OS_HANDLE pBHandle, int clkMode);

/*
 * 函数说明：
 * 		根据通道句柄设置通道的速率
 * 参数：
 *		@1：通道句柄
 *		@2：速率
*/
int 	cvg_port_setspeed(OS_HANDLE pChHandle,emFC_PORT_SPEED emSpeed);

/*
 * cache类相关函数
 * 函数说明：
 *		在特殊应用中，例如使用ASM消息传输视频使用时，逻辑把一幅大图像差分成多个小图像分别上传，需要API维护缓存成多个小图像，
 *		同时又保存一个大图像的完整格式。可以使用该类函数管理。
 * 参数：
 *		@1：通道句柄
 *		@2：消息ID
 *		@3：大图像分成多少行
 *		@4：大图像分成多少列
 *		@5：每个小图像的宽度
 *		@6：每个小图像的高度
 *		@7：图像每个像素占用多少个字节
 * 返回值：
 *		负数：失败；
 *		0：OK
*/
int		cvg_dCache_CreateMatrix(OS_HANDLE pChHandle, int msgIndex, int MatrixR, int MatirxC, int W, int H, int Byte_PerData);

/*
 * 函数说明：
 *		为消息缓存数据，一旦小区域写满了，就不能再写入，必须通过clear函数清楚才可以继续写。
 *		@1：通道句柄
 *		@2：消息ID
 *		@3：矩阵索引号
 *		@4：内存指针
 *		@5：内存大小
 * 返回值：
 *		-1：满了，见错误码
 *		0：写入OK
 *		其他：失败
 */
int 	cvg_dCache_writeData(OS_HANDLE pChHandle, int msgindex, int matrixid, unsigned char* pMem, int memLen);

/*
 * 函数说明
 *		判断小区域是否写满了
 *	参数：
 *		@1：通道句柄
 *		@2：消息索引
 *		@3：矩阵索引号
 * 返回值：
 *		负数：参数错误，见错误码
 *		1：满了
 *		0：不满
*/
int 	cvg_dCache_CheckFull(OS_HANDLE pChHandle, int msgindex, int matrixid);

/*
 * 函数说明
 *		清除满标志
 *	参数：
 *		@1：通道句柄
 *		@2：消息索引
 *		@3：矩阵索引号
 * 返回值：
 *		负数参数错误，见错误码
 *		0：ok
*/
int 	cvg_dCache_Clear(OS_HANDLE pChHandle, int msgindex, int matrixid);

/*
 * 函数说明：
 *		从指定的小区域拷贝完整的数据
 * 参数：
 *		@1：通道句柄
 *		@2：消息索引
 *		@3：矩阵索引号
 *		@4：拷贝的目的内存
 *		@5：目的内存大小
 * 返回值：
 *		负数：失败，见错误码
 *		正数：拷贝的数据量
*/
int 	cvg_dCache_CopyData(OS_HANDLE pChHandle, int msgindex, int matrixid, unsigned char* pDst, int Size);

/*
  函数说明：
		ASM，优先级使能控制
  参数：
		@1：通道句柄
		@2：使能控制，1：使能，0：禁用
 返回值：
	0：OK
	负数：失败。
*/
int		cvg_Asm_PrioEn(OS_HANDLE pChaHandle, int En);


/*
  函数说明：
		上行的字节序控制，
  参数：
		@1：通道句柄
		@2：字节序模式：
		2'b0: 以Byte为单位高低Byte倒序
		2'b1: 不做任何调整
		2'b2: 以word为单位高低word倒序，word内以Byte为单位高低Byte倒序
		2'b3: 以word为单位高低word倒序，word内不调整
 返回值：
	0：OK
	负数：失败。
*/
int cvg_UpDma_ByteEndianCfg(OS_HANDLE pChaHandle, int mode);


/*
 * 函数说明：
 *		获取端口的链接状态
 * 参数：
 *		@1：通道句柄
 * 返回值：
 *		1：链接
 *		0：未链接
 *		负数：见错误号码
*/
int cvg_GetPortLinkSt(OS_HANDLE pChaHandle);

/*
 * 2025-12-8：增加一个DMA转换的函数。
 *	函数说明：用来把ASM的DMA包头字节序转换一下
 *            DMA上来的ASM数据都是按照大端字节序的格式，因此需要首先进行转换成小端的格式
 *	参数：
 *			@1：通过DMA回调时的原始指针。只有针对ASM或1553消息才进行转换。图像切割不需要，dma头不一样
 *	返回值：无
*/
void	cvg_dma_header_endian_conv(unsigned char* pMem);
#ifdef __cplusplus
}
#endif

#endif
