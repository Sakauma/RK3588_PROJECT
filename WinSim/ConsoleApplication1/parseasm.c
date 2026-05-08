#include "../../GLKAPI/platform.h"
#include "../../GLKAPI/glkconfig.h"
#include "../../GLKAPI/include/ApiBase.h"
#include "../../GLKAPI/include/glkasm.h"
#include "shellcmd.h"


static void output_first10_dword(void *payload,int plsize)
{
	plsize /= 4;
	int index ;
	unsigned int * pu32 = (unsigned int*)payload;
	for(index = 0; index < 10 && index < plsize ; index++){
		os_printf("%08x ",pu32[index]);
	}
	os_printf("\r\n");
}

/*
 函数说明：解析ASM消息
*/
void parse_asm_package(OS_HANDLE dc, int channel, void* asmHeader, void* payload, int payploadlen)
{
	//struct DEVICE_CHANNEL* dc = (struct DEVICE_CHANNEL*)dc;
	struct ASM_R_DMA_FIX_HEADER* upDma = (struct ASM_R_DMA_FIX_HEADER*)asmHeader;
	struct ASM_MSGDESC* msg = (struct ASM_MSGDESC*)cvg_sche_get(dc, upDma->msgTbIndex);
	if(asm_rx_debug()){	
	//	os_printf("%s %d RX_B=%d sID=%x dID=%x msgIndex=%d msgID=%x\r\n", __func__,__LINE__,asm_rx_bytes(),upDma->sID, upDma->dID, upDma->msgTbIndex,msg->msgID);
		output_first10_dword(payload,payploadlen);
	}
}