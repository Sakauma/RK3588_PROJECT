#include "../../GLKAPI/platform.h"
#include "../../GLKAPI/glkconfig.h"
#include "../../GLKAPI/include/glkfc1553.h"
#include "../../GLKAPI/include/ApiBase.h"

void parse_fc1553_package(OS_HANDLE  dc,int channel, void* fc1553Header, void* payload, int payploadlen)
{
	struct FC1553_R_DMA_FIX_HEADER* upDma = (struct FC1553_R_DMA_FIX_HEADER*)fc1553Header;
	struct FC_1553_MSG* msg = (struct FC_1553_MSG*)cvg_sche_get(dc, upDma->msgTbIndex);
	os_printf("%s %d %d %d %d \n", __func__, __LINE__, upDma->sID, upDma->dID, upDma->msgTbIndex);
}