
#pragma once



/*各硬件功能模块基本偏移*/
#define REG_BASE_OFFSET_DMA						(0x00000000)	/*PCIE DMA模块基址*/

#define REG_B0_DOWN_CHL_DPR_W(chl) 				(REG_BASE_OFFSET_DMA + 0x2000 + (chl * 0x0100) + 0x0000)	/*DMA下行通道描述符写寄存器*/
#define REG_B0_DOWN_CHL_DPR_S0_R(chl) 			(REG_BASE_OFFSET_DMA + 0x2000 + (chl * 0x0100) + 0x0004)	/*DMA下行通道描述符状态0*/
#define REG_B0_DOWN_CHL_DPR_S1_R(chl) 			(REG_BASE_OFFSET_DMA + 0x2000 + (chl * 0x0100) + 0x0008)	/*DMA下行通道描述符状态1*/

