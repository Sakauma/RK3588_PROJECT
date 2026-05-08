#ifndef GLK_ASM_H
#define GLK_ASM_H

#include "schetb.h"
#pragma pack(4)
/*
 所有消息都按照这个结构去描述
*/
struct ASM_MSGDESC{
	/*
	 调度信息
	*/
	struct SCHE_TB		stb;
	/*
	 消息头信息
	*/
	struct MSG_HEADER msghead;
	/*
	 ASM消息ID
	*/
	unsigned int msgID;
	unsigned int sID;
	unsigned int dID;
	unsigned int Priority;
};

/*
 * 所有DMA头按照大端格式填写，参考寄存器文档
 * 首先写按照表格的左侧数据，然后写右侧。
 * 在单个字节内部，首先写右侧，然后写左侧。
 */


/*
 第一包ASM DMA头结构
*/
struct ASM_T_DMA_HEADER_P0{
	/*DW0*/
	unsigned int LP : 4;
	unsigned int WT : 2;
	unsigned int SUF : 2;
	
	
	unsigned int Reserve0 : 8;
	unsigned int msgTbIndex : 16;
	
	/*DW1*/
	unsigned int Reserve1	;
	/*DW2*/
	unsigned int R_CTL : 8;
	unsigned int DID		: 24;
	
	/*DW3*/
	unsigned int CS_CTL : 8;
	unsigned int SID		: 24;
	/*DW4*/
	unsigned int AsmMsgID;	/*消息ID*/
	/*DW5*/
	unsigned int Security;
	/*DW6*/
	unsigned int Reserve4;
	/*DW7*/
	unsigned int Priority : 7;
	unsigned int L : 1;	/*当PLen大小超过0XFFFFFF时，置1*/
	unsigned int PLen		: 24;

};

struct ASM_T_DMA_HEADER_P1{
	/*DW0*/
	unsigned int LP : 4;
	unsigned int WT : 2;
	unsigned int SUF : 2;
	unsigned int Reserve0 : 8;
	unsigned int msgTbIndex : 16;
	/*DW1*/
	unsigned int Reserve1	;
};

/*
 ASM 上行DMA头
*/
struct ASM_R_DMA_FIX_HEADER {
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
	unsigned int	msgID;
	/*DW3*/
	unsigned int	Security;
	/*DW4*/
	unsigned int	Reserved;
	/*DW5*/
	unsigned int	L : 1;
	unsigned int	Priority : 7;
	unsigned int	PLen : 24;
	
};
#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif
	void glkasm_pro_start(OS_HANDLE pBHandle, int Chs);
	void glkasm_pro_stop(OS_HANDLE pBHandle, int Chs);
	void glkasm_pro_msgconfig(OS_HANDLE pBHandle, int Chs, struct SCHE_TB* tb);
	void glkasm_pro_filldmaheader(struct SCHE_TB* tb, void * memPtr,int LP,int FP,int PayloadLen);
	void glkasm_pro_hook(unsigned char* pBuffer, int SUPWT);

	int  glkasm_msgid_to_msgindex(OS_HANDLE pChHandle, unsigned int msgID, unsigned int* msgindex);
#ifdef __cplusplus
}
#endif

#endif
