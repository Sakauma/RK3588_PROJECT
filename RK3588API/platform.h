#ifndef RK3588API_PLATFORM_H
#define RK3588API_PLATFORM_H

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#ifndef _BUILD_FOR_LINUX_
#define _BUILD_FOR_LINUX_
#endif
#include <semaphore.h>
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* OS_HANDLE;
typedef void* DEVICE_HANDLE;
typedef unsigned int OS_UINT;
typedef int OS_BOOL;
typedef intptr_t OS_LONGLONG;

typedef int FC_RESULT;
typedef uint8_t FC_BYTE;
typedef uint16_t FC_WORD;
typedef uint32_t FC_DWORD;
typedef uint64_t FC_UINT64;
typedef int FC_BOOL;
typedef float FC_FLOAT;

typedef void (*funUpDmaCallBack)(OS_HANDLE pChHandle,
	int dmaChan,
	const volatile unsigned char* pPack,
	unsigned int nPackLen,
	int nDropPack,
	void* pUserParam);

#ifndef FC_TRUE
#define FC_TRUE 1
#endif

#ifndef FC_FALSE
#define FC_FALSE 0
#endif

#ifdef _WIN32
typedef HANDLE OS_SEMAPHORE;
typedef CRITICAL_SECTION OS_MUTEX;
static inline void os_sleepms(unsigned int ms)
{
	Sleep(ms);
}
#else
typedef sem_t* OS_SEMAPHORE;
typedef sem_t OS_MUTEX;
static inline void os_sleepms(unsigned int ms)
{
	usleep(ms * 1000U);
}
#endif

#ifndef os_printf
#define os_printf printf
#endif

#ifdef __cplusplus
}
#endif

#endif
