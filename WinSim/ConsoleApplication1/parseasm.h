#ifndef PARSE_ASM_H
#define PARSE_ASM_H
#include "../../GLKAPI/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

	void parse_asm_package(OS_HANDLE dc, int channel, void* asmHeader, void* payload, int payploadlen);

#ifdef __cplusplus
}
#endif


#endif
