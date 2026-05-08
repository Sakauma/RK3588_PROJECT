#ifndef UART_INIT_H
#define UART_INIT_H

#include "shell.h"
#ifdef __cplusplus
extern "C" {
#endif
	void 	usershellinit();
	void 	usershellsche();
	void 	switchShell(ShellCommand* cmdList, int size);
#ifdef __cplusplus
}
#endif
#endif
