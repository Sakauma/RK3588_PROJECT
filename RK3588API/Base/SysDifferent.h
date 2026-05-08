#ifndef SYSDIFF_H
#define SYSDIFF_H



#include "../platform.h"

#ifdef _WIN32	//Windows

	#include <process.h>
	#include <Windows.h>
	#include <winioctl.h>
	#define EXTFUN extern 

#elif defined(_VXWORKS)
	#include <vxworks.h>
	#include <vxAtomicLib.h>
	#include <semaphore.h>
	#include <stdlib.h>
	#include <string.h>
	#include <pthread.h>
	#include <unistd.h>
	#include <sys/times.h>

#elif defined(_BUILD_FOR_LINUX_)
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/time.h>
	#include <time.h>
	#include <stdlib.h>
	#include <string.h>
	#include <semaphore.h>
#endif



#ifdef __cplusplus
	extern "C" {
#endif




typedef void(*fun_SD_Thread)(void*);

inline static void SD_Atomic_Add(volatile int* pOper, int nAdd)
{
#ifdef _WIN32
	InterlockedAdd((volatile long*)pOper, (long)nAdd);
#elif defined(_BUILD_FOR_VXWORKS_)
	vxAtomic32Add((atomic32_t *)pOper,(atomic32Val_t)nAdd);
#else
	__sync_fetch_and_add(pOper, nAdd);

#endif
}


inline static void SD_Atomic_Set(volatile int* pOper, int nSet)
{
#ifdef _WIN32
	InterlockedExchange((volatile long*)pOper, (long)nSet);
#elif defined(_BUILD_FOR_VXWORKS_)

#elif defined(_BUILD_FOR_LINUX_)
	__sync_lock_test_and_set(pOper, nSet);
#endif
}


inline static void SD_Atomic_Increment(volatile int* pOper)
{
#ifdef _WIN32
	InterlockedIncrement((volatile long*)pOper);
#elif defined(_BUILD_FOR_VXWORKS_)
	vxAtomic32Inc((atomic32_t*)pOper);
#elif defined(_BUILD_FOR_LINUX_)
	__sync_fetch_and_add(pOper, 1);

#endif
}


inline static void SD_Atomic_Decrement(volatile int* pOper)
{
#ifdef _WIN32
	InterlockedDecrement((volatile long*)pOper);
#elif defined(_BUILD_FOR_VXWORKS_)
	vxAtomic32Dec((atomic32_t*)pOper);
#elif defined(_BUILD_FOR_LINUX_)
	__sync_fetch_and_sub(pOper, 1);
#endif
}





inline static OS_SEMAPHORE SD_CreateSemaphore(int flagSingled)
{
#ifdef _WIN32
	return CreateSemaphore(NULL, 0, flagSingled, NULL); 
#elif defined(_BUILD_FOR_VXWORKS_)
	return semBCreate(SEM_Q_FIFO,flagSingled?SEM_FULL:SEM_EMPTY);
#elif defined(_BUILD_FOR_LINUX_)
	OS_SEMAPHORE pEvent = (OS_SEMAPHORE)malloc(sizeof(sem_t));
	if (NULL == pEvent)
	{
		return NULL;
	}
	memset(pEvent, 0, sizeof(sem_t));
	sem_init(pEvent, 0, flagSingled ? 1 : 0);
	return pEvent;
#endif
}


inline static void SD_DestroySemaphore(OS_SEMAPHORE hEvt)
{
#ifdef _WIN32
	CloseHandle(hEvt);
#elif defined(_BUILD_FOR_VXWORKS_)
	semDelete(hEvt);
#elif defined(_BUILD_FOR_LINUX_)
	sem_destroy(hEvt);
	free(hEvt);
#endif
}


inline static void SD_WaitSemaphore(OS_SEMAPHORE hEvt)
{
#ifdef _WIN32
	WaitForSingleObject(hEvt, INFINITE);
#elif defined(_BUILD_FOR_VXWORKS_)
	semTake(hEvt,WAIT_FOREVER);
#elif defined(_BUILD_FOR_LINUX_)
	sem_wait(hEvt);

#endif
}


inline static int SD_WaitSemTimeOut(OS_SEMAPHORE hEvt, unsigned int nTmOut)
{
#ifdef _WIN32
	return (WAIT_OBJECT_0 == WaitForSingleObject(hEvt, nTmOut)) ? 1 : 0;
#elif defined(_BUILD_FOR_VXWORKS_)
	return ((OK == semTake(hEvt,nTmOut))? 1 : 0);
#endif

	return 0;		//zhuyb
}


inline static void SD_SetEvent(OS_SEMAPHORE hEvt)
{
#ifdef _WIN32
	ReleaseSemaphore(hEvt, 1, NULL);
#elif defined(_BUILD_FOR_VXWORKS_)
	semGive(hEvt);
#elif defined(_BUILD_FOR_LINUX_)
	int nVal = 0;
	sem_getvalue(hEvt, &nVal);
	if (0 == nVal)
	{
		sem_post(hEvt);
	}

#endif
}



inline static void SD_InitMutex(OS_MUTEX* pMutex)
{
#ifdef _WIN32
	InitializeCriticalSection(pMutex);
#elif defined(_BUILD_FOR_LINUX_)
	//if(pMutex->inited == 0){
	//        sem_init(&(pMutex->sem),0,1);
	//}
	//pMutex->inited = 1;
	sem_init(pMutex,0,1);
#elif defined(_BUILD_FOR_VXWORKS_)
	pMutex->sem = semBCreate(SEM_Q_PRIORITY ,SEM_FULL );
#endif
}


inline static void SD_UninitMutex(OS_MUTEX* pMutex)
{
#ifdef _WIN32
	DeleteCriticalSection(pMutex);
#elif defined(_BUILD_FOR_LINUX_)
//	if(pMutex->inited == 0){
//		return ;
//	}
  //  sem_destroy(&(pMutex->sem));
//	pMutex->inited = 0;
	sem_destroy(pMutex);
#elif defined(_BUILD_FOR_VXWORKS_)
#endif
}


inline static void SD_MutexLock(OS_MUTEX* pMutex)
{
#ifdef _WIN32
	EnterCriticalSection(pMutex);
#elif defined(_BUILD_FOR_LINUX_)
//	if(pMutex->inited == 0){
//		return ;
//	}
  //  sem_wait(&(pMutex->sem));
	sem_wait(pMutex);
#elif defined(_BUILD_FOR_VXWORKS_)
    semTake(pMutex->sem,-1);
#endif
}


inline static void SD_MutexUnlock(OS_MUTEX* pMutex)
{
#ifdef _WIN32
	LeaveCriticalSection(pMutex);
#elif defined(_BUILD_FOR_LINUX_)
//	if(pMutex->inited == 0){
//		return ;
//	}
//    sem_post(&(pMutex->sem));
	sem_post(pMutex);
#elif defined(_BUILD_FOR_VXWORKS_)
    semGive(pMutex->sem);
#endif
}


unsigned long long SD_GetSysTmMs();
void SD_Sleep_US(OS_UINT dwUs);
extern void SD_Sleep(unsigned int nMS);

OS_HANDLE SD_CreateThread(fun_SD_Thread pFun, void* pParam,int pri);
int SD_IsThreadRun(OS_HANDLE hThread);
void SD_TerminateThread(OS_HANDLE hThread);
int SD_ChangeThreadPriority(OS_HANDLE hThread, int nChangeLevel);


#ifdef __cplusplus
}
#endif



#endif