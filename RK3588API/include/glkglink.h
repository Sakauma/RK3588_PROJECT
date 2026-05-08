#ifndef GLKGLINK_H
#define GLKGLINK_H
#include "../platform.h"
#include "../glkconfig.h"
#include "glkfc1553.h"

#define GLINK_SMART_LONGMSG(Msg)	{\
	Msg.Smart_LongMsg = 1;\
}


#define	GLINK_ENHANCE_PRO_CMD_PAYLOAD_EN	(1 << 2)
#define	GLINK_ENHANCE_PRO_STA_PAYLOAD_EN	(1 << 1)
#define	GLINK_ENHANCE_PRO					(1 << 4)
/*
 * 参考寄存器手册
*/
#define GLINK_SET_ENHANCED_DEFAULT_FALUE	(0X001C)
#endif
