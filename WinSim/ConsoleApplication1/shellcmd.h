#ifndef SHELL_CMD_H
#define SHELL_CMD_H

/*
 每秒统计使能。
*/
struct IMG_STATIS_INFO{
	unsigned int packCnt;
	unsigned int byteCnt;
	int espTimeStart;//ms
	int espTimeEnd;	//ms
	int espTime;
	int			statis_printen;
};


/*
 图像切图后的保存使能控制
*/
struct IMG_SAVE_ST_INFO{
	int		save_en;
};

struct ASM_CTRL_INFO{
	/*
	 ASM消息接收统计使能
	*/
	int 	asm_statis_rx_en;
	/*
	 接收了多少数据，字节数
	*/
	int 	asm_recv_bytecnt;
	/*
	 ASM消息发送使能，通过发送使能用来控制在主应用中是否启动发送
	*/
	int 	asm_tx_en;
	/*
	 在使能发送的时候，发送间隔
	*/
	int 	asm_tx_sleepms;
	/*
	 手动触发时的消息号
	*/
	int 	asm_msg_index;
	/*
	 填充数据时的规则，-1：表示随机数，否则使用输入的数据填充
	*/
	int 	fill_type;

};
#ifdef __cplusplus
extern "C" {
#endif
	void 	sysexit();
	int 	sysexitflag();
	void 	asm_statis_save(int imgNum, int bytes);
	int 	asm_rx_debug();
	void 	asm_update_rx_info(int bytes);
	int 	asm_rx_bytes();
	int 	asm_sleep();
	int		asm_trig_msgindex();
	int 	asm_fill_type();
	int 	asm_tx_en();
	
	int 	img_save_en(int ch);
	void	img_update_rx_info(int ch, int bytes);
#ifdef __cplusplus
}
#endif

#endif
