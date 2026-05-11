/*************************************************
Copyright (C), 2026, yhk
File name:      ConsoleApplication1.cpp
Author:       yhk     Version:        2.0        Date: 2026-01-25
Description:    主程序文件，用于测试KU卡的回环功能
通过上行DMA通道发送数据，在下行DMA通道接收数据
并调用回调函数保存YUV文件
支持从配置文件读取配置
Others:         无
Function List:
1. main - 主函数，负责初始化设备、发送数据、接收数据
2. ChannelRecvDataCallBack - 接收数据回调函数，每个报文独立保存为YUV文件
3. ReadSimpleConfig - 读取简单格式配置文件
4. CreateSaveDirectory - 创建保存目录
5. SavePacketToYuvFile - 将单个报文保存为独立的YUV文件
History:
1. Date: 2026-01-25
Author: yhk
Modification: 增加YUV文件保存功能，每个报文独立保存
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include "../../RK3588API/include/ApiBase.h"
#include "../../RK3588API/include/ApiExt.h"
#include "../../RK3588API/glkconfig.h"
#include "../../RK3588API/third_party/dCache.h"
#include "../../RK3588API/third_party/rbtree.h"
#include "../../RK3588API/Base/DevRegisterDef.h"
#include "../../RK3588API/Base/BaseDevOper.h"
#include "../encoder/encoder_pipeline.h"
#include "shell/shellin.h"
#include "shellcmd.h"

/* 宏定义 */
#define UP_LOOP_DMA_CHANNUM    (2)     /* 自回环上行DMA */
#define DOWN_LOOP_DMA_CHANNUM  (2)     /* 自回环下行DMA */
#define CONFIG_FILE_NAME       "rk3588.ini"  /* 配置文件名称 */
#define MAX_CHANNEL_NUM        (31)    /* 最大通道号限制 */

/* YUV保存相关宏定义 */
#define DEFAULT_SAVE_PATH      "/tmp/pcie_yuv/"
#define MAX_FILE_PATH_LEN      256
#define PACKET_HEADER_SIZE     16      /* 报文头大小，16字节 */
#define SAVE_PKG     			1

/* 全局变量 */
static unsigned int g_uFileCounter = 0;
static char g_acSavePath[MAX_FILE_PATH_LEN] = DEFAULT_SAVE_PATH;
static bool g_bPrintMsg = 1;
static unsigned int g_uCorrectPackets = 0;   /* 正确包数量 */
static unsigned int g_uErrorPackets = 0;     /* 错误包数量 */
static EncoderPipeline g_stEncoderPipeline;
static EncoderPipelineConfig g_stEncoderConfig;
static bool g_bEncoderStarted = false;

/*************************************************
Function:       CreateSaveDirectory
Description:    创建保存目录
Input:
pcPath - 目录路径
Output:         无
Return:         0-成功，-1-失败
Others:         无
*************************************************/
static int CreateSaveDirectory(const char* pcPath)
{
	struct stat stStat;
	int nResult = 0;

	if (pcPath == NULL)
	{
		return -1;
	}

	nResult = stat(pcPath, &stStat);
	if (nResult == -1)
	{
		/* 目录不存在，创建目录 */
		nResult = mkdir(pcPath, 0755);
		if (nResult != 0)
		{
			os_printf("错误: 无法创建目录 %s\n", pcPath);
			return -1;
		}
		os_printf("目录创建成功: %s\n", pcPath);
	}

	return 0;
}

/*************************************************
Function:       SavePacketToYuvFile
Description:    将单个报文保存为独立的YUV文件
Input:
dmaChan   - DMA通道号
pData     - 数据指针
uDataLen  - 数据长度
Output:         无
Return:         0-成功，-1-失败
Others:         每个报文创建一个独立的文件
*************************************************/
static int SavePacketToYuvFile(int dmaChan, const volatile unsigned char* pData, unsigned int uDataLen)
{
	char acFilename[MAX_FILE_PATH_LEN];
	time_t tNow;
	struct tm* pTm;
	FILE* pFile = NULL;
	size_t stWritten = 0;
	const void* pWriteData = NULL;
	int nLen = 0;

	if (pData == NULL || uDataLen == 0)
	{
		return -1;
	}

	/* 转换volatile指针为普通指针 */
	pWriteData = (const void*)pData;

	/* 生成文件名 */
	tNow = time(NULL);
	pTm = localtime(&tNow);
	if (pTm == NULL)
	{
		return -1;
	}

	/* 文件名格式: 路径/通道号_序号_年月日_时分秒_毫秒.yuv */
	{
		struct timeval tv;
		unsigned long ulMilliseconds = 0;

		/* 获取毫秒级时间 */
		if (gettimeofday(&tv, NULL) == 0)
		{
			ulMilliseconds = tv.tv_usec / 1000;
		}

		nLen = snprintf(acFilename, sizeof(acFilename),
			"%sch%d_%06u_%04d%02d%02d_%02d%02d%02d_%03lu.yuv",
			g_acSavePath,
			dmaChan,
			g_uFileCounter,
			pTm->tm_year + 1900,
			pTm->tm_mon + 1,
			pTm->tm_mday,
			pTm->tm_hour,
			pTm->tm_min,
			pTm->tm_sec,
			ulMilliseconds);
	}

	if (nLen >= (int)sizeof(acFilename))
	{
		os_printf("错误: 文件名过长\n");
		return -1;
	}

	/* 打开文件 */
	pFile = fopen(acFilename, "wb");
	if (pFile == NULL)
	{
		os_printf("错误: 无法创建文件 %s\n", acFilename);
		return -1;
	}

	/* 写入数据到文件 */
	stWritten = fwrite(pWriteData, 1, uDataLen, pFile);
	if (stWritten != uDataLen)
	{
		os_printf("警告: 写入文件失败，期望 %u 字节，实际 %zu 字节\n",
			uDataLen, stWritten);
		fclose(pFile);
		return -1;
	}

	/* 强制刷新缓冲区，确保数据写入磁盘 */
	fflush(pFile);
	fclose(pFile);

	os_printf("成功创建文件: %s (大小: %u 字节)\n", acFilename, uDataLen);
	g_uFileCounter++;

	return 0;
}

/*************************************************
Function:       ReadSimpleConfig
Description:    从简单格式配置文件读取配置值
Input:
pcFilename      - 配置文件名
pcKey           - 要查找的键
nDefaultValue   - 默认值
Output:         无
Return:         读取到的整数值，如果读取失败则返回默认值
Others:         配置文件格式：key=value
*************************************************/
static int ReadSimpleConfig(const char* pcFilename, const char* pcKey, int nDefaultValue)
{
	FILE* pFile = NULL;
	char acLine[256];
	char* pcEqualPos = NULL;
	char acConfigKey[64];
	int nKeyLen = 0;
	int nValue = nDefaultValue;

	pFile = fopen(pcFilename, "r");
	if (pFile == NULL)
	{
		/* 文件不存在，返回默认值 */
		return nDefaultValue;
	}

	while (fgets(acLine, sizeof(acLine), pFile))
	{
		/* 跳过注释和空行 */
		if (acLine[0] == '#' || acLine[0] == '\n' || acLine[0] == '\r')
		{
			continue;
		}

		/* 查找等号 */
		pcEqualPos = strchr(acLine, '=');
		if (pcEqualPos == NULL)
		{
			continue;
		}

		/* 提取键名 */
		nKeyLen = pcEqualPos - acLine;
		if (nKeyLen >= (int)sizeof(acConfigKey))
		{
			nKeyLen = sizeof(acConfigKey) - 1;
		}

		strncpy(acConfigKey, acLine, nKeyLen);
		acConfigKey[nKeyLen] = '\0';

		/* 去除键名末尾空格 */
		while (nKeyLen > 0 && (acConfigKey[nKeyLen - 1] == ' ' || acConfigKey[nKeyLen - 1] == '\t'))
		{
			acConfigKey[--nKeyLen] = '\0';
		}

		/* 比较键名 */
		if (strcmp(acConfigKey, pcKey) == 0)
		{
			nValue = atoi(pcEqualPos + 1);
			break;
		}
	}

	fclose(pFile);
	return nValue;
}

/*************************************************
Function:       ReadConfigString
Description:    从配置文件读取字符串
Input:
pcFilename   - 配置文件名
pcKey        - 键名
pcDefault    - 默认值
pcBuffer     - 缓冲区
nBufferSize  - 缓冲区大小
Output:
pcBuffer     - 读取到的字符串
Return:         0-成功，-1-失败
Others:         配置文件格式：key=value
*************************************************/
static int ReadConfigString(const char* pcFilename, const char* pcKey,
	const char* pcDefault, char* pcBuffer, int nBufferSize)
{
	FILE* pFile = NULL;
	char acLine[256];
	char* pcEqualPos = NULL;
	char acConfigKey[64];
	int nKeyLen = 0;
	int nResult = -1;

	if (pcFilename == NULL || pcKey == NULL || pcBuffer == NULL || nBufferSize <= 0)
	{
		if (pcDefault != NULL)
		{
			strncpy(pcBuffer, pcDefault, nBufferSize - 1);
			pcBuffer[nBufferSize - 1] = '\0';
		}
		return -1;
	}

	pFile = fopen(pcFilename, "r");
	if (pFile == NULL)
	{
		/* 文件不存在，使用默认值 */
		if (pcDefault != NULL)
		{
			strncpy(pcBuffer, pcDefault, nBufferSize - 1);
			pcBuffer[nBufferSize - 1] = '\0';
		}
		return -1;
	}

	while (fgets(acLine, sizeof(acLine), pFile))
	{
		/* 跳过注释和空行 */
		if (acLine[0] == '#' || acLine[0] == '\n' || acLine[0] == '\r')
		{
			continue;
		}

		/* 查找等号 */
		pcEqualPos = strchr(acLine, '=');
		if (pcEqualPos == NULL)
		{
			continue;
		}

		/* 提取键名 */
		nKeyLen = pcEqualPos - acLine;
		if (nKeyLen >= (int)sizeof(acConfigKey))
		{
			nKeyLen = sizeof(acConfigKey) - 1;
		}

		strncpy(acConfigKey, acLine, nKeyLen);
		acConfigKey[nKeyLen] = '\0';

		/* 去除键名末尾空格 */
		while (nKeyLen > 0 &&
			(acConfigKey[nKeyLen - 1] == ' ' || acConfigKey[nKeyLen - 1] == '\t'))
		{
			acConfigKey[--nKeyLen] = '\0';
		}

		/* 比较键名 */
		if (strcmp(acConfigKey, pcKey) == 0)
		{
			char* pcValue = pcEqualPos + 1;
			/* 去除值的前后空格 */
			while (*pcValue == ' ' || *pcValue == '\t')
			{
				pcValue++;
			}

			/* 去除换行符和尾部空格 */
			{
				int nLen = strlen(pcValue);
				while (nLen > 0 &&
					(pcValue[nLen - 1] == '\n' || pcValue[nLen - 1] == '\r' ||
						pcValue[nLen - 1] == ' ' || pcValue[nLen - 1] == '\t'))
				{
					pcValue[--nLen] = '\0';
				}
			}

			strncpy(pcBuffer, pcValue, nBufferSize - 1);
			pcBuffer[nBufferSize - 1] = '\0';
			nResult = 0;
			break;
		}
	}

	fclose(pFile);

	/* 如果没找到，使用默认值 */
	if (nResult != 0 && pcDefault != NULL)
	{
		strncpy(pcBuffer, pcDefault, nBufferSize - 1);
		pcBuffer[nBufferSize - 1] = '\0';
	}

	return nResult;
}

static const char* InputPixelFormatName(InputPixelFormat eFormat)
{
	switch (eFormat)
	{
	case InputPixelFormat::Gray10Le16:
		return "gray10le16";
	case InputPixelFormat::Nv12:
		return "nv12";
	case InputPixelFormat::Raw16:
	default:
		return "raw16";
	}
}

static InputPixelFormat ParseInputPixelFormat(const char* pcFormat)
{
	if (pcFormat != NULL &&
		(strcmp(pcFormat, "gray10le16") == 0 || strcmp(pcFormat, "GRAY10LE16") == 0 ||
			strcmp(pcFormat, "gray10") == 0 || strcmp(pcFormat, "GRAY10") == 0))
	{
		return InputPixelFormat::Gray10Le16;
	}
	if (pcFormat != NULL &&
		(strcmp(pcFormat, "nv12") == 0 || strcmp(pcFormat, "NV12") == 0 ||
			strcmp(pcFormat, "yuv420sp") == 0 || strcmp(pcFormat, "YUV420SP") == 0))
	{
		return InputPixelFormat::Nv12;
	}
	return InputPixelFormat::Raw16;
}

static const char* EncoderOsdStatus(const EncoderPipelineConfig* pstConfig)
{
	if (pstConfig == NULL)
	{
		return "unknown";
	}
	if (!pstConfig->osd_enable || pstConfig->osd_mode == "burned-in")
	{
		return "fpga/burned-in";
	}
	return "software";
}

static void LoadEncoderConfig(EncoderPipelineConfig* pstConfig)
{
	char acCodec[32];
	char acOutput[MAX_FILE_PATH_LEN];
	char acOsdMode[32];
	char acRaw16MapMode[32];
	char acInputPixelFormat[32];

	if (pstConfig == NULL)
	{
		return;
	}

	pstConfig->width = ReadSimpleConfig(CONFIG_FILE_NAME, "FRAME_WIDTH", 1920);
	pstConfig->height = ReadSimpleConfig(CONFIG_FILE_NAME, "FRAME_HEIGHT", 1080);
	pstConfig->fps = ReadSimpleConfig(CONFIG_FILE_NAME, "FRAME_FPS", 30);
	pstConfig->bitrate = ReadSimpleConfig(CONFIG_FILE_NAME, "VIDEO_BITRATE", 8000000);

	ReadConfigString(CONFIG_FILE_NAME, "INPUT_PIXEL_FORMAT", "gray10le16", acInputPixelFormat, sizeof(acInputPixelFormat));
	pstConfig->input_pixel_format = ParseInputPixelFormat(acInputPixelFormat);

	const bool bRaw16Input = pstConfig->input_pixel_format == InputPixelFormat::Raw16;
	pstConfig->raw16_shift = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_SHIFT", bRaw16Input ? 8 : 2);
	pstConfig->raw16_black_level = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_BLACK_LEVEL", 0);
	pstConfig->raw16_white_level = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_WHITE_LEVEL", bRaw16Input ? 65535 : 1023);
	pstConfig->raw16_auto_low_clip_permille = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_AUTO_LOW_CLIP_PERMILLE", 5);
	pstConfig->raw16_auto_high_clip_permille = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_AUTO_HIGH_CLIP_PERMILLE", 5);
	pstConfig->queue_depth = ReadSimpleConfig(CONFIG_FILE_NAME, "ENCODER_QUEUE_DEPTH", 3);
	pstConfig->raw16_little_endian = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW16_LITTLE_ENDIAN", 1) != 0;
	pstConfig->prefer_main10 = ReadSimpleConfig(CONFIG_FILE_NAME, "PREFER_MAIN10", 0) != 0;
	pstConfig->input_has_img_dma_header = ReadSimpleConfig(CONFIG_FILE_NAME, "INPUT_HAS_IMG_DMA_HEADER", 0) != 0;
	pstConfig->osd_enable = ReadSimpleConfig(CONFIG_FILE_NAME, "OSD_ENABLE", 0) != 0;
	pstConfig->osd_test_enable = ReadSimpleConfig(CONFIG_FILE_NAME, "OSD_TEST_ENABLE", 0) != 0;
	g_bPrintMsg = ReadSimpleConfig(CONFIG_FILE_NAME, "PRINTF_UP_MAG", 0) != 0;

	ReadConfigString(CONFIG_FILE_NAME, "VIDEO_CODEC", "h265", acCodec, sizeof(acCodec));
	if (strcmp(acCodec, "h264") == 0 || strcmp(acCodec, "H264") == 0 ||
		strcmp(acCodec, "avc") == 0 || strcmp(acCodec, "AVC") == 0)
	{
		pstConfig->codec = VideoCodec::H264;
	}
	else
	{
		pstConfig->codec = VideoCodec::H265;
	}

	ReadConfigString(CONFIG_FILE_NAME, "RAW16_MAP_MODE", bRaw16Input ? "shift" : "window", acRaw16MapMode, sizeof(acRaw16MapMode));
	if (strcmp(acRaw16MapMode, "window") == 0 || strcmp(acRaw16MapMode, "WINDOW") == 0)
	{
		pstConfig->raw16_mapping_mode = Raw16MappingMode::Window;
	}
	else if (strcmp(acRaw16MapMode, "auto_window") == 0 || strcmp(acRaw16MapMode, "AUTO_WINDOW") == 0 ||
		strcmp(acRaw16MapMode, "auto") == 0 || strcmp(acRaw16MapMode, "AUTO") == 0)
	{
		pstConfig->raw16_mapping_mode = Raw16MappingMode::AutoWindow;
	}
	else
	{
		pstConfig->raw16_mapping_mode = Raw16MappingMode::Shift;
	}

	ReadConfigString(CONFIG_FILE_NAME, "VIDEO_OUTPUT_PATH",
		"/tmp/rk3588_capture.h265", acOutput, sizeof(acOutput));
	pstConfig->output_path = acOutput;

	ReadConfigString(CONFIG_FILE_NAME, "OSD_MODE", "burned-in", acOsdMode, sizeof(acOsdMode));
	pstConfig->osd_mode = acOsdMode;
}

/*************************************************
Function:       ChannelRecvDataCallBack
Description:    PCIE数据接收回调函数
接收数据并保存为YUV文件
Input:
pChHandle - 通道句柄
dmaChan   - DMA通道号
pPack     - 数据包指针
nPackLen  - 数据包长度
nDropPack - 丢弃的数据包数
pUserParam- 用户参数
Output:         无
Return:         无
Others:         每个报文独立保存为一个YUV文件
*************************************************/
static void ChannelRecvDataCallBack(OS_HANDLE pChHandle,
	int dmaChan,
	const volatile unsigned char* pPack,
	unsigned int nPackLen,
	int nDropPack,
	void* pUserParam)
{
#if 0
	const volatile unsigned char* pPayload = NULL;
	unsigned int uPayloadLen = 0;
	static int nDirInitialized = 0;
	static unsigned long long ullLastFrameCount = 0;  // 上一次的帧计数
	static int bFirstFrame = 1;  // 是否是第一帧

								 /* 防止未使用变量警告 */
	(void)pChHandle;
	(void)pUserParam;

	/* 从配置文件读取保存路径（只读一次） */
	if (nDirInitialized == 0)
	{
		ReadConfigString(CONFIG_FILE_NAME, "YUV_SAVE_PATH",
			DEFAULT_SAVE_PATH, g_acSavePath, sizeof(g_acSavePath));

		g_bPrintMsg = ReadSimpleConfig(CONFIG_FILE_NAME, "PRINTF_UP_MAG", 1);

		/* 确保路径以'/'结尾 */
		{
			int nLen = strlen(g_acSavePath);
			if (nLen > 0 && g_acSavePath[nLen - 1] != '/')
			{
				if (nLen < (int)sizeof(g_acSavePath) - 1)
				{
					g_acSavePath[nLen] = '/';
					g_acSavePath[nLen + 1] = '\0';
				}
			}
		}

		nDirInitialized = 1;
		os_printf("YUV保存路径: %s (注：已注释保存功能)\n", g_acSavePath);
		os_printf("打印控制: %s\n", g_bPrintMsg ? "启用" : "禁用");
	}
	if (g_bPrintMsg)
	{
		/* 打印基本信息 */
		os_printf("============================================================\n");
		os_printf("ChannelRecvDataCallBack 被调用:\n");
		os_printf("  - dmaChan: %d\n", dmaChan);
		os_printf("  - nPackLen: %u bytes\n", nPackLen);
		os_printf("  - nDropPack: %d\n", nDropPack);
	}


	/* 检查数据长度是否足够 */
	if (nPackLen < (PACKET_HEADER_SIZE))  // 报文头16字节 + 有效数据256字节
	{
		os_printf("错误: 报文长度 %u 不足，期望至少 %d 字节\n",
			nPackLen, PACKET_HEADER_SIZE);
		g_uErrorPackets++;
		if (g_bPrintMsg)
		{
			os_printf("============================================================\n");
		}
		return;
	}

	/* 跳过报文头（16字节） */
	pPayload = pPack + PACKET_HEADER_SIZE;
	uPayloadLen = nPackLen - PACKET_HEADER_SIZE;

	if (g_bPrintMsg)
	{
		/* 打印报文头信息（前16字节） */
		os_printf("  - 报文头（16字节）:\n");
		for (unsigned int i = 0; i < PACKET_HEADER_SIZE; i++)
		{
			os_printf("%02X ", pPack[i]);
			if ((i + 1) % 8 == 0)
			{
				os_printf(" ");
			}
			if ((i + 1) % 16 == 0)
			{
				os_printf("\n");
			}
		}
	}

	/* 提取帧计数（8字节，根据提供的报文格式，看起来是大端字节序） */
	unsigned long long ullFrameCount = 0;
	for (int i = 0; i < 8; i++)
	{
		ullFrameCount = (ullFrameCount << 8) | pPayload[i];
	}

	if (g_bPrintMsg)
	{
		os_printf("  - 帧计数（8字节）: 0x%016llX (%llu)\n", ullFrameCount, ullFrameCount);
	}

	/* 帧计数连续性检查 */
	if (bFirstFrame)
	{
		if (g_bPrintMsg)
		{
			os_printf("  - 帧计数检查: 第一帧，接受帧计数 %llu\n", ullFrameCount);
		}
		ullLastFrameCount = ullFrameCount;
		bFirstFrame = 0;
	}
	else
	{
		unsigned long long ullExpected = ullLastFrameCount + 1;
		if (ullFrameCount == ullExpected)
		{
			if (g_bPrintMsg)
			{
				os_printf("  - 帧计数检查: ✓ 正确（期望: %llu，实际: %llu）\n",
					ullExpected, ullFrameCount);
			}
		}
		else
		{
			os_printf("  - 帧计数检查: ✗ 错误（期望: %llu，实际: %llu）\n",
				ullExpected, ullFrameCount);
			os_printf("============================================================\n");
			g_uErrorPackets++;

			return;  // 帧计数错误，直接返回，不检查后续数据
		}
		ullLastFrameCount = ullFrameCount;
	}

	/* 提取随机数（1字节，紧接在帧计数后） */
	unsigned char ucRandom = pPayload[8];
	if (g_bPrintMsg)
	{
		os_printf("  - 随机数（1字节）: 0x%02X (%u)\n", ucRandom, ucRandom);
	}

	/* 验证后续数据是否为随机数依次+1（支持FF后从00重新开始） */
	int bDataValid = 1;
	unsigned char ucExpected = ucRandom + 1;  // 第一个数据字节应该是随机数+1
	unsigned int uDataLength = uPayloadLen - 9;  // 减去帧计数8字节和随机数1字节

	for (unsigned int i = 9; i < 9 + uDataLength; i++)
	{
		if (i >= uPayloadLen)
		{
			break;
		}

		if (pPayload[i] != ucExpected)
		{
			if (g_bPrintMsg)
			{
				os_printf("  - 数据验证: ✗ 错误，位置 %u（期望: 0x%02X，实际: 0x%02X）\n",
					i, ucExpected, pPayload[i]);
			}
			bDataValid = 0;
			break;
		}

		// 累加，支持FF后从00重新开始
		if (ucExpected == 0xFF)
		{
			ucExpected = 0x00;
		}
		else
		{
			ucExpected++;
		}
		if (g_bPrintMsg && bDataValid)
		{
			/* 每64字节打印一次进度（仅当数据验证正确时） */
			if ((i - 8) % 64 == 0)  // 减去帧计数8字节
			{
				os_printf("  - 数据验证: 前 %u 字节 ✓ 正确\n", i - 8);
			}
		}
	}

	if (bDataValid)
	{
		if (g_bPrintMsg)
		{
			os_printf("  - 数据验证: ✓ 全部 %u 字节正确\n", uDataLength);
		}
		g_uCorrectPackets++;
	}
	else
	{
		g_uErrorPackets++;
	}

	if (g_bPrintMsg)
	{
		/* 打印有效数据的前后部分（用于调试） */
		os_printf("  - 有效数据（从帧计数开始）:\n");
		for (unsigned int i = 0; i < 256 && i < uPayloadLen; i++)
		{
			os_printf("%02X ", pPayload[i]);
			if ((i + 1) % 8 == 0)
			{
				os_printf(" ");
			}
			if ((i + 1) % 16 == 0)
			{
				os_printf("\n");
			}
		}
		os_printf("\n");

		if (uPayloadLen >= 256)
		{
			os_printf("  - 有效数据后256字节:\n");
			unsigned int startPos = uPayloadLen - 256;

			for (unsigned int i = startPos; i < uPayloadLen; i++)
			{
				os_printf("%02X ", pPayload[i]);
				if ((i - startPos + 1) % 8 == 0)
				{
					os_printf(" ");
				}
				if ((i - startPos + 1) % 16 == 0)
				{
					os_printf("\n");
				}
			}
			os_printf("\n");
		}

		os_printf("  - 有效数据起始地址: %p\n", pPayload);
		os_printf("  - 有效数据长度: %u bytes\n", uPayloadLen);

		/* 打印统计信息 */
		os_printf("  - 统计: 正确包=%u, 错误包=%u, 总计=%u\n",
			g_uCorrectPackets, g_uErrorPackets, g_uCorrectPackets + g_uErrorPackets);

		os_printf("============================================================\n");
	}

	return;
#else




	const volatile unsigned char* pPayload = NULL;
	unsigned int uPayloadLen = 0;

	(void)pChHandle;
	(void)pUserParam;

	if (nDropPack > 0)
	{
		os_printf("警告: DMA通道%d 丢包数=%d\n", dmaChan, nDropPack);
	}

	if (nPackLen <= PACKET_HEADER_SIZE)
	{
		os_printf("警告: 报文长度 %u 不足，小于等于报文头大小(%d字节)，丢弃\n",
			nPackLen, PACKET_HEADER_SIZE);
		g_uErrorPackets++;
		return;
	}

	pPayload = pPack + PACKET_HEADER_SIZE;
	uPayloadLen = nPackLen - PACKET_HEADER_SIZE;

	if (!g_bEncoderStarted)
	{
		os_printf("错误: 编码管线未启动，丢弃DMA数据\n");
		g_uErrorPackets++;
		return;
	}

	if (!g_stEncoderPipeline.SubmitPacket(dmaChan, pPayload, uPayloadLen))
	{
		os_printf("错误: 提交编码管线失败\n");
		g_uErrorPackets++;
		return;
	}

	g_uCorrectPackets++;
	if (g_bPrintMsg)
	{
		EncoderPipelineStats stStats = g_stEncoderPipeline.GetStats();
		os_printf("DMA包已提交: chan=%d, payload=%u, packets=%llu, frames=%llu, encoded=%llu, drop=%llu, main10_fallbacks=%llu\n",
			dmaChan,
			uPayloadLen,
			(unsigned long long)stStats.packets,
			(unsigned long long)stStats.frames_in,
			(unsigned long long)stStats.frames_encoded,
			(unsigned long long)stStats.frames_dropped,
			(unsigned long long)stStats.main10_fallbacks);
	}
#endif
}

/*************************************************
Function:       main
Description:    主函数，负责初始化设备、发送数据、接收数据
支持从配置文件读取上行和下行通道号
Input:          无
Output:         无
Return:         0-成功，-1-失败
Others:         无
*************************************************/
int main(void)
{
	OS_HANDLE dev = NULL;
	OS_HANDLE ch = NULL;
	int cardCnt = 0;
	unsigned int lcversion = 0;
	unsigned int v = 0;
	unsigned char pMsg[1024];
	int nUpChannel = UP_LOOP_DMA_CHANNUM;    /* 上行通道号 */
	int nDownChannel = DOWN_LOOP_DMA_CHANNUM; /* 下行通道号 */
	int nSendLen = 128;
	int i = 0;
	int r = 0;
	char c = 0;

	printf("into main \n");

	/* 从配置文件读取通道号 */
	nUpChannel = ReadSimpleConfig(CONFIG_FILE_NAME, "UP_LOOP_DMA_CHANNUM", UP_LOOP_DMA_CHANNUM);
	nDownChannel = ReadSimpleConfig(CONFIG_FILE_NAME, "DOWN_LOOP_DMA_CHANNUM", DOWN_LOOP_DMA_CHANNUM);

	printf("从配置文件读取通道号: 上行=%d, 下行=%d\n", nUpChannel, nDownChannel);
	LoadEncoderConfig(&g_stEncoderConfig);
	printf("编码配置: %dx%d fps=%d bitrate=%d codec=%s input=%s main10=%s output=%s osd=%s\n",
		g_stEncoderConfig.width,
		g_stEncoderConfig.height,
		g_stEncoderConfig.fps,
		g_stEncoderConfig.bitrate,
		g_stEncoderConfig.codec == VideoCodec::H264 ? "h264" : "h265",
		InputPixelFormatName(g_stEncoderConfig.input_pixel_format),
		g_stEncoderConfig.prefer_main10 ? "prefer" : "off",
		g_stEncoderConfig.output_path.c_str(),
		EncoderOsdStatus(&g_stEncoderConfig));

	/* 验证通道号有效性 */
	if (nUpChannel < 0 || nUpChannel > MAX_DEV_UP_DMA_CHL)
	{
		printf("警告：上行通道号 %d 超出范围(0-%d)，使用默认值 %d\n",
			nUpChannel, MAX_DEV_UP_DMA_CHL, UP_LOOP_DMA_CHANNUM);
		nUpChannel = UP_LOOP_DMA_CHANNUM;
	}

	if (nDownChannel < 0 || nDownChannel > MAX_DEV_DOWN_DMA_CHL)
	{
		printf("警告：下行通道号 %d 超出范围(0-%d)，使用默认值 %d\n",
			nDownChannel, MAX_DEV_DOWN_DMA_CHL, DOWN_LOOP_DMA_CHANNUM);
		nDownChannel = DOWN_LOOP_DMA_CHANNUM;
	}

	cardCnt = cvg_scan_devcnt();
	if (cardCnt == 0)
	{
		printf("not find KU card\n");
		return -1;
	}

	printf("cardCnt: %d\n", cardCnt);

	/* 打开,不重置 */
	dev = cvg_card_open(0, 0);
	if (dev == NULL)
	{
		printf("open card failed \n");
		return -1;
	}

	/* 打印版本 */
	cvg_card_readreg(dev, 0x10000, &lcversion);
	os_printf("lcversion = %0x\n", lcversion);

	/* 打开板卡后进行基本功能设置 */
	ch = cvg_open_chan(dev, nUpChannel, nDownChannel);
	if (ch == NULL)
	{
		printf("open channel failed \n");
		cvg_card_close(dev);
		return -1;
	}

	if (!g_stEncoderPipeline.Start(g_stEncoderConfig))
	{
		printf("start encoder pipeline failed: %s\n", g_stEncoderPipeline.LastError().c_str());
		cvg_close_chan(ch);
		cvg_card_close(dev);
		return -1;
	}
	g_bEncoderStarted = true;
	{
		EncoderPipelineStats stStats = g_stEncoderPipeline.GetStats();
		if (stStats.main10_fallbacks > 0)
		{
			printf("警告: %s\n", g_stEncoderPipeline.LastError().c_str());
		}
	}

	cvg_pro_start(ch, 0);

	/* 注册回调函数 */
	cvg_pro_register_cb(ch, NULL, ChannelRecvDataCallBack);

	cvg_card_readreg(dev, 0x1008, &v);
	os_printf("%s %d %x\n", __func__, __LINE__, v);

	/* 初始化发送数据 */
	for (i = 0; i < 1024; i++)
	{
		pMsg[i] = (unsigned char)i;
	}

	/* 发送数据 */
	i = 0;
	while (1)
	{
		os_printf("press any to send again (q to quit) \n");
		c = getc(stdin);

		if (c == 'q')
		{
			os_printf("quit \n");
			break;
		}

		os_printf("start send msg %d\n", i++);

		v = 0;
		while (v < (unsigned int)nSendLen)
		{
			/* 每行打印16个字节 */
			if ((v % 16) == 0)
			{
				if (v != 0)
				{
					os_printf("\n");
				}
				os_printf("%08X: ", v);
			}

			/* 打印十六进制值 */
			os_printf("%02X ", pMsg[v]);

			/* 在第8个字节后添加一个空格以便阅读 */
			if ((v % 16) == 7)
			{
				os_printf(" ");
			}

			v++;
		}

		os_printf("\n");

		r = cvg_pro_write_data(ch, pMsg, nSendLen, 0);
		//os_printf("cvg_pro_write_data ret %d \n", r);

		/* 保留原有的寄存器读取 */
		cvg_card_readreg(dev, 0x1024, &v);
		os_printf("%s %d 0x1024 上行: %x\n", __func__, __LINE__, v);

		cvg_card_readreg(dev, 0x1028, &v);
		os_printf("%s %d 0x1028 上行: %x\n", __func__, __LINE__, v);

		cvg_card_readreg(dev, 0x200C, &v);
		os_printf("%s %d 0x200C: %x\n", __func__, __LINE__, v);

		os_printf("总共接收包数: %u (正确: %u, 错误: %u)\n",
			g_uCorrectPackets + g_uErrorPackets, g_uCorrectPackets, g_uErrorPackets);
		{
			EncoderPipelineStats stStats = g_stEncoderPipeline.GetStats();
			os_printf("编码统计: packets=%llu frames_in=%llu encoded=%llu dropped=%llu main10_fallbacks=%llu errors=%llu\n",
				(unsigned long long)stStats.packets,
				(unsigned long long)stStats.frames_in,
				(unsigned long long)stStats.frames_encoded,
				(unsigned long long)stStats.frames_dropped,
				(unsigned long long)stStats.main10_fallbacks,
				(unsigned long long)stStats.encode_errors);
		}

		os_sleepms(100);
	}

	cvg_pro_stop(ch);
	g_bEncoderStarted = false;
	g_stEncoderPipeline.Stop();
	cvg_close_chan(ch);
	cvg_card_close(dev);

	os_sleepms(100);
	{
		EncoderPipelineStats stStats = g_stEncoderPipeline.GetStats();
		os_printf("exit, encoded=%llu dropped=%llu main10_fallbacks=%llu errors=%llu output=%s\n",
			(unsigned long long)stStats.frames_encoded,
			(unsigned long long)stStats.frames_dropped,
			(unsigned long long)stStats.main10_fallbacks,
			(unsigned long long)stStats.encode_errors,
			g_stEncoderConfig.output_path.c_str());
	}

	return 0;
}
