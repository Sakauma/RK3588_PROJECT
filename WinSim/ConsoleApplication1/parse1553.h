#ifndef PARSE_FC1553_H
#define PARSE_FC1553_H

#include "../../GLKAPI/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

	void parse_fc1553_package(OS_HANDLE dc, int channel, void* fc1553Header, void* payload, int payploadlen);

#ifdef __cplusplus
}
#endif


#endif

