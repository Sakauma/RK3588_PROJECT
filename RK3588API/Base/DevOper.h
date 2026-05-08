
#ifndef DEVOPER_H
#define DEVOPER_H

#include "../platform.h"
#include "DevResourceGet.h"
#include "SysDifferent.h"
#ifdef __cplusplus
	extern "C" {
#endif


OS_UINT ScanLocalDevCnt(void);
DEVICE_HANDLE OpenDevByIndex(unsigned int nDevIndex);
DEVICE_HANDLE OpenDevByName(char* strDevName);
void CloseDev(DEVICE_HANDLE hDev);
int GetDevBaseInfo(DEVICE_HANDLE hDev, pDeviceParam_st pBaseInfo);
int MapBar(DEVICE_HANDLE hDev, pBarRes_st pBarInfo);
int UnmapBar(DEVICE_HANDLE hDev, pBarRes_st pBarInfo);
int RequestUpDmaChl(DEVICE_HANDLE hDev, pUpChlRes_st pUpChlInfo);
int ReleaseUpDmaChl(DEVICE_HANDLE hDev, pUpChlRes_st pUpChlInfo);
int RequestDownDmaChl(DEVICE_HANDLE hDev, pDownChlRes_st pDownChlInfo, unsigned int flag_x64);
int ReleaseDownDmaChl(DEVICE_HANDLE hDev, pDownChlRes_st pDownChlInfo);
int ResetAllDmaChl(DEVICE_HANDLE hDev);
	
	
#ifdef __cplusplus
	}
#endif
#endif


