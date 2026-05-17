/*************************************************
Copyright (C), 2026, yhk
File name:      ConsoleApplication1.cpp
Author:       yhk     Version:        2.2        Date: 2026-05-17
Description:    主程序文件，用于RK3588接收V7图像数据
通过PCIe/SRIO回调接收SWRITE图像包，按V7地址映射拼接2048x2048灰度帧
doorbell作为帧结束通知，后台线程负责JPEG编码和HTTP MJPEG实时预览
支持从rk3588.ini读取DMA通道、推流端口、队列和诊断开关
保留SWRITE测试发送、PNG本地诊断和原始非SWRITE包留样能力
Others:         无
Function List:
1. main - 主函数，负责初始化设备、启动接收和打印运行统计
2. ChannelRecvDataCallBack - 接收V7 SWRITE/doorbell报文并完成帧缓冲入队
3. ReadSimpleConfig - 读取简单格式配置文件
4. CreateSaveDirectory - 创建保存目录
5. FrameOutputWorkerMain - 后台线程，负责raw16转8bit、JPEG编码和MJPEG更新
6. MjpegServerMain - HTTP MJPEG服务线程，负责浏览器连接和重复发送最后一帧
7. SWRITE相关结构体及发送函数
History:
1. Date: 2026-01-25
Author: yhk
Modification: 增加YUV文件保存功能，每个报文独立保存
2. Date: 2026-01-25
Author: yhk
Modification: 增加SRIO SWRITE发送功能及计数
3. Date: 2026-05-17
Author: yhk
Modification: 增加V7图像拼帧、HTTP MJPEG实时预览和spool缓存
*************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jpeglib.h>
#include "../../RK3588API/include/ApiBase.h"
#include "../../RK3588API/include/ApiExt.h"
#include "../../RK3588API/glkconfig.h"
#include "../../RK3588API/third_party/dCache.h"
#include "../../RK3588API/third_party/rbtree.h"
#include "../../RK3588API/Base/DevRegisterDef.h"
#include "../../RK3588API/Base/BaseDevOper.h"
#include "shell/shellin.h"
#include "shellcmd.h"

/* 宏定义 */
#define UP_LOOP_DMA_CHANNUM    (5)     /* 自回环上行DMA */
#define DOWN_LOOP_DMA_CHANNUM  (4)     /* 自回环下行DMA */
#define CONFIG_FILE_NAME       "rk3588.ini"  /* 配置文件名称 */
#define MAX_CHANNEL_NUM        (31)    /* 最大通道号限制 */

/* YUV保存相关宏定义 */
#define DEFAULT_SAVE_PATH      "/tmp/pcie_yuv/"
#define MAX_FILE_PATH_LEN      256
#define PACKET_HEADER_SIZE     16      /* 报文头大小，16字节 */

/* 全局变量 */
static unsigned int g_uFileCounter = 0;
static char g_acSavePath[MAX_FILE_PATH_LEN] = DEFAULT_SAVE_PATH;
static bool g_bPrintMsg = 1;
static unsigned int g_uCorrectPackets = 0;   /* 正确包数量 */
static unsigned int g_uErrorPackets = 0;     /* 错误包数量 */

/* ==================== SRIO 相关定义 (从项目2移植) ==================== */
#define LOCAL_SRIO_ID   0x71                /* 本端SRIO源ID，按要求改为0x71 */
#define SRIO_MAX_PAYLOAD 256                /* 单包最大负载 */

/* SRIO SWRITE 报文全局发送计数和填充值 */
static unsigned int g_uSrioSendCount = 0;   /* SRIO 成功发送的次数 */
static unsigned char g_ucSrioFillValue = 0x00; /* 当前填充值，每发送一次递增 */
/* SRIO 接收计数 */
static unsigned int g_uSrioRecvCount = 0;   /* 收到的SRIO报文数量 */

/* ==================== V7 图像接收与HTTP MJPEG推流配置 ==================== */
#undef UP_LOOP_DMA_CHANNUM
#undef DOWN_LOOP_DMA_CHANNUM
#define UP_LOOP_DMA_CHANNUM        (0)
#define DOWN_LOOP_DMA_CHANNUM      (0)

#define V7_IMG_WIDTH           2048u
#define V7_IMG_HEIGHT          2048u
#define V7_TOTAL_ROWS          2049u
#define V7_BYTES_PER_PIXEL     2u
#define V7_ROW_BYTES           (V7_IMG_WIDTH * V7_BYTES_PER_PIXEL)
#define V7_SWRITE_PAYLOAD      256u
#define V7_PACKETS_PER_ROW     (V7_ROW_BYTES / V7_SWRITE_PAYLOAD)
#define V7_IMAGE_PACKETS       (V7_IMG_HEIGHT * V7_PACKETS_PER_ROW)
#define V7_TOTAL_PACKETS       (V7_TOTAL_ROWS * V7_PACKETS_PER_ROW)
#define V7_IMAGE_BYTES         (V7_IMG_HEIGHT * V7_ROW_BYTES)
#define V7_FRAME_BYTES         (V7_TOTAL_ROWS * V7_ROW_BYTES)
#define V7_BANK_NUM            2u
#define V7_BANK0_BASE          0x80000000u
#define V7_BANK1_BASE          0x81000000u
#define V7_SWRITE_FTYPE        0x6u
#define V7_DOORBELL_FTYPE      0xAu
#define V7_PACKET_STAT_SLOTS   16u
#define V7_DEFAULT_BANK0_OFFSET 0x00000000u
#define V7_DEFAULT_BANK1_OFFSET 0x00000000u

#define DEFAULT_PNG_SAVE_PATH      "/tmp/pcie_yuv/"
#define DEFAULT_RAW_CAPTURE_PATH   "/tmp/pcie_yuv/raw/"
#define DEFAULT_RAW_CAPTURE_FILES  256u
#define DEFAULT_RAW_CAPTURE_BYTES  512u
#define DEFAULT_STREAM_SPOOL_PATH  "/tmp/pcie_yuv/spool/"
#define DEFAULT_STREAM_PORT        7766
#define DEFAULT_STREAM_JPEG_QUALITY 85
#define DEFAULT_STREAM_MAX_MEM_FRAMES 8u
#define DEFAULT_STREAM_MAX_SPOOL_GB 8u
#define DEFAULT_STREAM_MIN_IMAGE_PACKETS 28000u
#define DEFAULT_STREAM_REPEAT_FPS 10
#define DEFAULT_STREAM_MAX_CLIENTS 4u
#define DEFAULT_STREAM_SEND_TIMEOUT_MS 800
#define STREAM_MAX_CLIENTS_LIMIT 16u
#define MAX_TRANSFER_FIELD_LEN     256
#define MAX_SYSTEM_CMD_LEN         1024
#define V7_SPOOL_MAGIC         "V7SRIO01"
#define V7_SPOOL_VERSION       1u
#define MJPEG_BOUNDARY             "frame"

static char g_acPngSavePath[MAX_FILE_PATH_LEN] = DEFAULT_PNG_SAVE_PATH;
static char g_acPendingPath[MAX_FILE_PATH_LEN] = { 0 };
static char g_acSentPath[MAX_FILE_PATH_LEN] = { 0 };
static char g_acFailedPath[MAX_FILE_PATH_LEN] = { 0 };
static char g_acRawCapturePath[MAX_FILE_PATH_LEN] = DEFAULT_RAW_CAPTURE_PATH;
static char g_acStreamSpoolPath[MAX_FILE_PATH_LEN] = DEFAULT_STREAM_SPOOL_PATH;
static char g_acTransferUser[MAX_TRANSFER_FIELD_LEN] = { 0 };
static char g_acTransferHost[MAX_TRANSFER_FIELD_LEN] = { 0 };
static char g_acTransferDir[MAX_TRANSFER_FIELD_LEN] = { 0 };
static char g_acPngScaleMode[32] = "minmax";
static char g_acStreamScaleMode[32] = "minmax";
static char g_acStreamMode[32] = "lossless";
static int g_nTransferEnable = 1;
static int g_nScpTimeoutSec = 30;
static int g_nRawCaptureEnable = 1;
static int g_nSwriteTraceEnable = 0;
static int g_nStreamEnable = 1;
static int g_nStreamPartialEnable = 1;
static int g_nStreamPort = DEFAULT_STREAM_PORT;
static int g_nStreamJpegQuality = DEFAULT_STREAM_JPEG_QUALITY;
static int g_nStreamRepeatFps = DEFAULT_STREAM_REPEAT_FPS;
static int g_nStreamSendTimeoutMs = DEFAULT_STREAM_SEND_TIMEOUT_MS;
static unsigned int g_uRawCaptureMaxFiles = DEFAULT_RAW_CAPTURE_FILES;
static unsigned int g_uRawCaptureMaxBytes = DEFAULT_RAW_CAPTURE_BYTES;
static unsigned int g_uStreamMaxMemFrames = DEFAULT_STREAM_MAX_MEM_FRAMES;
static unsigned int g_uStreamMaxSpoolGb = DEFAULT_STREAM_MAX_SPOOL_GB;
static unsigned int g_uStreamMinImagePackets = DEFAULT_STREAM_MIN_IMAGE_PACKETS;
static unsigned int g_uStreamMaxClients = DEFAULT_STREAM_MAX_CLIENTS;
static unsigned long long g_ullStreamMaxSpoolBytes = 0;
static OS_HANDLE g_hV7DevHandle = NULL;
static int g_nDoorbellReadEnable = 0;       /* 现场主流程不再通过doorbell访问BAR/DDR */
static int g_nMappedHasParamRow = 0;
static int g_nV7MemBar = -1;
static char g_acV7DevNode[MAX_FILE_PATH_LEN] = "/dev/cvgdev00";
static DEVICE_HANDLE g_hV7MapDev = (DEVICE_HANDLE)-1;
static BarRes_st g_stV7BarResource = { 0 };
static int g_bV7BarMapped = 0;
static unsigned int g_uV7BankOffset[V7_BANK_NUM] = {
    V7_DEFAULT_BANK0_OFFSET,
    V7_DEFAULT_BANK1_OFFSET
};
static unsigned int g_uFrameCompleteCount = 0;
static unsigned int g_uFrameQueuedCount = 0;
static unsigned int g_uFrameSendOkCount = 0;
static unsigned int g_uFrameSendFailCount = 0;
static unsigned int g_uDoorbellCount = 0;
static unsigned int g_uDoorbellFrameCount = 0;
static unsigned int g_uDoorbellReadFailCount = 0;
static unsigned int g_uIncompleteFrameCount = 0;
static unsigned int g_uPartialFrameQueuedCount = 0;
static unsigned int g_uNonSwriteCount = 0;
static unsigned int g_uRawCaptureCount = 0;
static unsigned int g_uSdkDropPackets = 0;
static unsigned int g_uFrameStreamOkCount = 0;
static unsigned int g_uFrameSpoolWriteCount = 0;
static unsigned int g_uFrameSpoolReadCount = 0;
static unsigned int g_uFrameStreamFailCount = 0;
static unsigned int g_uMjpegClientConnectCount = 0;
static unsigned int g_uMjpegClientDropCount = 0;
static unsigned int g_uMjpegClientActive = 0;
static unsigned int g_uMjpegRepeatCount = 0;
static unsigned int g_uMjpegSendFailCount = 0;
static unsigned int g_uJpegEncodeFailCount = 0;
static unsigned int g_uFrameOutputQueueDepth = 0;
static unsigned int g_uFrameMemoryQueueDepth = 0;
static unsigned int g_uFrameSpoolQueueDepth = 0;
static unsigned long long g_ullStreamSpoolBytes = 0;
static int g_nMjpegListenFd = -1;
static unsigned char* g_pLastJpegFrame = NULL;
static unsigned long g_ulLastJpegFrameBytes = 0;
static unsigned int g_uLastJpegSeq = 0;

typedef struct _V7_MjpegClient_
{
    int nFd;
    char acPeer[64];
    unsigned int uFramesSent;
} V7_MjpegClient;

static V7_MjpegClient g_stMjpegClients[STREAM_MAX_CLIENTS_LIMIT];

#pragma pack(4)
/* SRIO SWRITE 报文结构 */
typedef struct srio_swrite_pkt {
    unsigned char   nRes1;
    unsigned char   nRes2 : 4;
    unsigned char   nFTYPE : 4;
    unsigned char   nRes3 : 4;
    unsigned char   nCRF : 1;
    unsigned char   nPrio : 2;
    unsigned char   nRes4 : 1;
    unsigned char   nAddrMSB : 2;
    unsigned char   nRes5 : 6;
    unsigned int    nAddrLSB;
} SRIO_SWRITE_PKT;

/* SRIO 数据传输头 */
typedef struct pkt_SRIO_head_st {
    unsigned short  nSrcID;      /* 源ID */
    unsigned short  nDstID;      /* 目标ID */
    unsigned short  nRole;       /* 保留0 */
    unsigned short  nSendLen;    /* 以8字节为单位的长度 */
} SRIO_DownData_Head_st;
#pragma pack()

/* ==================== 字节序转换 (与项目2一致) ==================== */
static unsigned short TransByteWord(unsigned short nData) {
    return ((nData & 0x00FF) << 8) | ((nData & 0xFF00) >> 8);
}

static unsigned int TransByteDword(unsigned int nData) {
    return ((nData & 0x000000FF) << 24) |
        ((nData & 0x0000FF00) << 8)  |
        ((nData & 0x00FF0000) >> 8)  |
        ((nData & 0xFF000000) >> 24);
}

/* ==================== SRIO SWRITE 单包发送 (内部函数) ==================== */
static int Send_SWrite_Single(OS_HANDLE ch, unsigned short nDstID,
                            unsigned int nAddr, const unsigned char *pBuf,
                            unsigned short nSize, unsigned char nPrio, unsigned char nCRF)
{
    if (pBuf == NULL || nSize == 0 || nSize > SRIO_MAX_PAYLOAD || (nSize % 8) != 0)
        return -1;

    unsigned char pData[sizeof(SRIO_DownData_Head_st) + sizeof(SRIO_SWRITE_PKT) + SRIO_MAX_PAYLOAD];
    SRIO_DownData_Head_st head;
    SRIO_SWRITE_PKT swrite;

    memset(&head, 0, sizeof(head));
    memset(&swrite, 0, sizeof(swrite));

    unsigned int nLen = sizeof(head) + sizeof(swrite) + nSize;

    /* 填充SRIO传输头 */
    head.nDstID = TransByteWord(nDstID);
    head.nSrcID = TransByteWord(LOCAL_SRIO_ID);  /* 源ID=0x71 */
    head.nRole = 0;
    head.nSendLen = TransByteWord((unsigned short)((nLen - sizeof(head)) / 8)); /* 以8字节为单位 */

    /* 填充SWRITE报文头 */
    swrite.nFTYPE = 0x6;                    /* SWRITE */
    swrite.nPrio = nPrio;
    swrite.nCRF = nCRF;
    swrite.nAddrLSB = TransByteDword(nAddr);
    swrite.nAddrMSB = 0;
    /* 其余保留字段已初始化为0 */

    /* 拼包 */
    memcpy(pData, &head, sizeof(head));
    memcpy(pData + sizeof(head), &swrite, sizeof(swrite));
    memcpy(pData + sizeof(head) + sizeof(swrite), pBuf, nSize);

    return cvg_pro_write_data(ch, pData, nLen, 0);
}

/* ==================== SRIO SWRITE 分片发送接口 ==================== */
static int Send_SRIO_SWrite_Frag(OS_HANDLE ch, unsigned short nDstID,
                                unsigned int nAddr, const unsigned char *pBuf,
                                unsigned short nSize, unsigned char nPrio, unsigned char nCRF)
{
    if (pBuf == NULL || nSize == 0) return -1;

    unsigned short remaining = nSize;
    unsigned short offset = 0;
    unsigned int currentAddr = nAddr;
    int ret = 0;

    while (remaining > 0) {
        unsigned short chunkRaw = (remaining > SRIO_MAX_PAYLOAD) ? SRIO_MAX_PAYLOAD : remaining;
        unsigned short chunkPadded = (chunkRaw + 7) & ~7;   /* 8字节对齐 */
        unsigned char tempBuf[SRIO_MAX_PAYLOAD];
        memset(tempBuf, 0, chunkPadded);
        memcpy(tempBuf, pBuf + offset, chunkRaw);

        ret = Send_SWrite_Single(ch, nDstID, currentAddr, tempBuf, chunkPadded, nPrio, nCRF);
        if (ret < 0) {
            os_printf("[错误] SWRITE 分片发送失败，偏移=%u\n", offset);
            return ret;
        }

        remaining    -= chunkRaw;
        offset       += chunkRaw;
        currentAddr  += chunkRaw;      /* 地址按实际数据长度递增 */
    }
    return 0;
}

/* ==================== 构建并发送一次 SRIO SWRITE ==================== */
static int SendOneSrioSwrite(OS_HANDLE ch)
{
    #define SWRITE_PAYLOAD_LEN  446
    #define SWRITE_BASE_ADDR    0x00000000   /* 可以修改 */

    unsigned char payload[SWRITE_PAYLOAD_LEN];
    /* 首字节0xAA, 末字节0xFF, 中间全部填充当前递增填充值 */
    payload[0] = 0xAA;
    for (unsigned int i = 1; i < SWRITE_PAYLOAD_LEN - 1; i++) {
        payload[i] = g_ucSrioFillValue;
    }
    payload[SWRITE_PAYLOAD_LEN - 1] = 0xFF;

    int ret = Send_SRIO_SWrite_Frag(ch, 0x80, SWRITE_BASE_ADDR,
                                    payload, SWRITE_PAYLOAD_LEN, 0, 0);
    if (ret == 0) {
        g_uSrioSendCount++;
        os_printf("[SRIO发送] 成功发送第 %u 个SWRITE报文, 填充值=0x%02X, 数据长度=%u字节\n",
                g_uSrioSendCount, g_ucSrioFillValue, SWRITE_PAYLOAD_LEN);
        /* 填充值递增 (0~255循环) */
        g_ucSrioFillValue++;
    } else {
        os_printf("[SRIO发送] 发送失败，错误码=%d\n", ret);
    }
    return ret;
}

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

static unsigned int ReadConfigUInt(const char* pcFilename, const char* pcKey, unsigned int uDefaultValue)
{
    char acValue[64];
    char* pcEnd = NULL;
    unsigned long ulValue = 0;

    if (ReadConfigString(pcFilename, pcKey, "", acValue, sizeof(acValue)) != 0 ||
        acValue[0] == '\0')
    {
        return uDefaultValue;
    }

    ulValue = strtoul(acValue, &pcEnd, 0);
    if (pcEnd == acValue)
    {
        return uDefaultValue;
    }

    return (unsigned int)ulValue;
}

typedef struct _V7_FrameBuffer_
{
    unsigned char* pImageRaw16;
    unsigned char* pPacketBitmap;
    unsigned int uBankIndex;
    unsigned int uPacketCount;
    unsigned int uImagePacketCount;
    unsigned int uDuplicateCount;
    unsigned int uFrameCounter;
    unsigned int uMinRow;
    unsigned int uMaxRow;
    unsigned int uMinAddr;
    unsigned int uMaxAddr;
    unsigned int uLastRow;
    unsigned int uLastBlock;
    unsigned int uLastAddr;
    int bHasFrameCounter;
} V7_FrameBuffer;

typedef struct _V7_FrameJob_
{
    unsigned char* pImageRaw16;
    char acSpoolPath[MAX_FILE_PATH_LEN * 2];
    unsigned int uBankIndex;
    unsigned int uFrameCounter;
    int bHasFrameCounter;
    int bOnDisk;
    unsigned int uSeq;
    struct _V7_FrameJob_* pNext;
} V7_FrameJob;

typedef struct _V7_SpoolHeader_
{
    char acMagic[8];
    unsigned int uVersion;
    unsigned int uImageBytes;
    unsigned int uBankIndex;
    unsigned int uFrameCounter;
    unsigned int uHasFrameCounter;
    unsigned int uSeq;
} V7_SpoolHeader;

typedef struct _V7_PacketStat_
{
    unsigned int uFtype;
    unsigned int uSrcId;
    unsigned int uDstId;
    unsigned int uLen;
    unsigned int uCount;
} V7_PacketStat;

/*
 * V7接收分为两个轻量级阶段：
 * 1. SDK回调线程只解析SWRITE地址、写入bank帧缓冲、维护bitmap和统计；
 * 2. 后台输出线程再完成raw16灰度拉伸、JPEG编码和MJPEG推流。
 *
 * 这样做可以避免JPEG编码、浏览器网络慢或者磁盘spool阻塞SDK回调，
 * 也方便用g_uSrioRecvCount判断是否仍有上游报文进入RK3588。
 */
static V7_FrameBuffer g_stFrameBuf[V7_BANK_NUM] = { 0 };
static V7_PacketStat g_stNonSwriteStats[V7_PACKET_STAT_SLOTS] = { 0 };
static pthread_mutex_t g_stFrameLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_stDiagLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_stJobLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_stJobCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_stMjpegLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_stMjpegCond = PTHREAD_COND_INITIALIZER;
static pthread_t g_stFrameWorker;
static pthread_t g_stMjpegWorker;
static int g_bFrameWorkerStarted = 0;
static int g_bFrameWorkerStop = 0;
static int g_bMjpegWorkerStarted = 0;
static int g_bMjpegWorkerStop = 0;
static V7_FrameJob* g_pJobHead = NULL;
static V7_FrameJob* g_pJobTail = NULL;

static unsigned short ReadBe16(const volatile unsigned char* pData)
{
    return (unsigned short)(((unsigned short)pData[0] << 8) | pData[1]);
}

static unsigned int ReadBe32(const volatile unsigned char* pData)
{
    return ((unsigned int)pData[0] << 24) |
           ((unsigned int)pData[1] << 16) |
           ((unsigned int)pData[2] << 8) |
           (unsigned int)pData[3];
}

static void AppendSlash(char* pcPath, int nSize)
{
    int nLen = 0;

    if (pcPath == NULL || nSize <= 0)
    {
        return;
    }

    nLen = strlen(pcPath);
    if (nLen > 0 && pcPath[nLen - 1] != '/')
    {
        if (nLen < nSize - 1)
        {
            pcPath[nLen] = '/';
            pcPath[nLen + 1] = '\0';
        }
    }
}

static int EnsureDirectory(const char* pcPath)
{
    struct stat stStat;

    if (pcPath == NULL || pcPath[0] == '\0')
    {
        return -1;
    }

    if (stat(pcPath, &stStat) == 0)
    {
        if (S_ISDIR(stStat.st_mode))
        {
            return 0;
        }
        os_printf("错误: %s 已存在但不是目录\n", pcPath);
        return -1;
    }

    if (mkdir(pcPath, 0755) == 0 || errno == EEXIST)
    {
        return 0;
    }

    os_printf("错误: 无法创建目录 %s, errno=%d\n", pcPath, errno);
    return -1;
}

static int BuildSubdir(char* pcOut, int nOutSize, const char* pcBase, const char* pcSubdir)
{
    int nLen = snprintf(pcOut, nOutSize, "%s%s/", pcBase, pcSubdir);
    if (nLen < 0 || nLen >= nOutSize)
    {
        return -1;
    }
    return 0;
}

static void CaptureRawPacketSample(const volatile unsigned char* pPack,
                                   unsigned int nPackLen,
                                   unsigned int uFtype,
                                   unsigned int uSrcId,
                                   unsigned int uDstId,
                                   unsigned int uCaptureSeq)
{
    FILE* fp = NULL;
    char acPath[MAX_FILE_PATH_LEN + 128];
    unsigned int uCopyLen = nPackLen;
    unsigned long ulNow = (unsigned long)time(NULL);

    if (pPack == NULL || !g_nRawCaptureEnable || uCaptureSeq == 0)
    {
        return;
    }

    if (g_uRawCaptureMaxBytes > 0 && uCopyLen > g_uRawCaptureMaxBytes)
    {
        uCopyLen = g_uRawCaptureMaxBytes;
    }

    if (snprintf(acPath, sizeof(acPath),
        "%sraw_%010lu_%06u_ftype%X_src%X_dst%X_len%u.bin",
        g_acRawCapturePath, ulNow, uCaptureSeq, uFtype, uSrcId, uDstId, nPackLen) >=
        (int)sizeof(acPath))
    {
        return;
    }

    fp = fopen(acPath, "wb");
    if (fp == NULL)
    {
        return;
    }

    for (unsigned int i = 0; i < uCopyLen; i++)
    {
        fputc(pPack[i], fp);
    }
    fclose(fp);
}

static void RecordNonSwritePacket(const volatile unsigned char* pPack,
                                  unsigned int nPackLen,
                                  unsigned int uFtype,
                                  unsigned int uSrcId,
                                  unsigned int uDstId)
{
    unsigned int uKeyCount = 0;
    unsigned int uCaptureSeq = 0;
    unsigned int uSlot = V7_PACKET_STAT_SLOTS;

    pthread_mutex_lock(&g_stDiagLock);
    g_uNonSwriteCount++;

    for (unsigned int i = 0; i < V7_PACKET_STAT_SLOTS; i++)
    {
        if (g_stNonSwriteStats[i].uCount != 0 &&
            g_stNonSwriteStats[i].uFtype == uFtype &&
            g_stNonSwriteStats[i].uSrcId == uSrcId &&
            g_stNonSwriteStats[i].uDstId == uDstId &&
            g_stNonSwriteStats[i].uLen == nPackLen)
        {
            uSlot = i;
            break;
        }
        if (uSlot == V7_PACKET_STAT_SLOTS &&
            g_stNonSwriteStats[i].uCount == 0)
        {
            uSlot = i;
        }
    }

    if (uSlot < V7_PACKET_STAT_SLOTS)
    {
        g_stNonSwriteStats[uSlot].uFtype = uFtype;
        g_stNonSwriteStats[uSlot].uSrcId = uSrcId;
        g_stNonSwriteStats[uSlot].uDstId = uDstId;
        g_stNonSwriteStats[uSlot].uLen = nPackLen;
        g_stNonSwriteStats[uSlot].uCount++;
        uKeyCount = g_stNonSwriteStats[uSlot].uCount;
    }
    else
    {
        uKeyCount = g_uNonSwriteCount;
    }

    if (g_nRawCaptureEnable &&
        g_uRawCaptureCount < g_uRawCaptureMaxFiles)
    {
        g_uRawCaptureCount++;
        uCaptureSeq = g_uRawCaptureCount;
    }
    pthread_mutex_unlock(&g_stDiagLock);

    if (g_bPrintMsg && (uKeyCount <= 8 || (uKeyCount % 1000) == 0))
    {
        os_printf("V7 non-SWRITE captured: ftype=0x%X, src=0x%X, dst=0x%X, len=%u, count=%u\n",
            uFtype, uSrcId, uDstId, nPackLen, uKeyCount);
    }

    CaptureRawPacketSample(pPack, nPackLen, uFtype, uSrcId, uDstId, uCaptureSeq);
}

static int ShellQuote(const char* pcIn, char* pcOut, int nOutSize)
{
    int nPos = 0;

    if (pcIn == NULL || pcOut == NULL || nOutSize < 3)
    {
        return -1;
    }

    pcOut[nPos++] = '\'';
    for (int i = 0; pcIn[i] != '\0'; i++)
    {
        if (pcIn[i] == '\'')
        {
            if (nPos + 4 >= nOutSize)
            {
                return -1;
            }
            pcOut[nPos++] = '\'';
            pcOut[nPos++] = '\\';
            pcOut[nPos++] = '\'';
            pcOut[nPos++] = '\'';
        }
        else
        {
            if (nPos + 1 >= nOutSize)
            {
                return -1;
            }
            pcOut[nPos++] = pcIn[i];
        }
    }

    if (nPos + 1 >= nOutSize)
    {
        return -1;
    }
    pcOut[nPos++] = '\'';
    pcOut[nPos] = '\0';
    return 0;
}

static unsigned int PngCrcTable[256];
static int PngCrcTableReady = 0;

static void PngInitCrcTable(void)
{
    for (unsigned int n = 0; n < 256; n++)
    {
        unsigned int c = n;
        for (int k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = 0xEDB88320u ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        PngCrcTable[n] = c;
    }
    PngCrcTableReady = 1;
}

static unsigned int PngUpdateCrc(unsigned int uCrc, const unsigned char* pData, unsigned int uLen)
{
    unsigned int c = uCrc;

    if (!PngCrcTableReady)
    {
        PngInitCrcTable();
    }

    for (unsigned int i = 0; i < uLen; i++)
    {
        c = PngCrcTable[(c ^ pData[i]) & 0xFFu] ^ (c >> 8);
    }
    return c;
}

static unsigned int PngAdler32(const unsigned char* pData, unsigned int uLen)
{
    unsigned int a = 1;
    unsigned int b = 0;

    for (unsigned int i = 0; i < uLen; i++)
    {
        a = (a + pData[i]) % 65521u;
        b = (b + a) % 65521u;
    }

    return (b << 16) | a;
}

static void PutBe32(unsigned char* pOut, unsigned int uValue)
{
    pOut[0] = (unsigned char)((uValue >> 24) & 0xFFu);
    pOut[1] = (unsigned char)((uValue >> 16) & 0xFFu);
    pOut[2] = (unsigned char)((uValue >> 8) & 0xFFu);
    pOut[3] = (unsigned char)(uValue & 0xFFu);
}

static int WriteBe32(FILE* pFile, unsigned int uValue)
{
    unsigned char buf[4];
    PutBe32(buf, uValue);
    return fwrite(buf, 1, 4, pFile) == 4 ? 0 : -1;
}

static int WritePngChunk(FILE* pFile, const char* pcType, const unsigned char* pData, unsigned int uLen)
{
    unsigned int uCrc = 0xFFFFFFFFu;

    if (WriteBe32(pFile, uLen) != 0)
    {
        return -1;
    }
    if (fwrite(pcType, 1, 4, pFile) != 4)
    {
        return -1;
    }
    if (uLen > 0 && fwrite(pData, 1, uLen, pFile) != uLen)
    {
        return -1;
    }

    uCrc = PngUpdateCrc(uCrc, (const unsigned char*)pcType, 4);
    if (uLen > 0)
    {
        uCrc = PngUpdateCrc(uCrc, pData, uLen);
    }
    uCrc ^= 0xFFFFFFFFu;

    return WriteBe32(pFile, uCrc);
}

static int WriteGray8Png(const char* pcPath, const unsigned char* pGray, unsigned int uWidth, unsigned int uHeight)
{
    static const unsigned char pngSig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    FILE* pFile = NULL;
    unsigned char ihdr[13];
    unsigned char* pScan = NULL;
    unsigned char* pIdat = NULL;
    unsigned char* pCursor = NULL;
    unsigned int uRowBytes = uWidth + 1;
    unsigned int uRawSize = uRowBytes * uHeight;
    unsigned int uBlockCount = (uRawSize + 65534u) / 65535u;
    unsigned int uIdatSize = 2u + uRawSize + (uBlockCount * 5u) + 4u;
    unsigned int uAdler = 0;
    unsigned int uRemaining = uRawSize;
    unsigned int uOffset = 0;
    int nRet = -1;

    pScan = (unsigned char*)malloc(uRawSize);
    pIdat = (unsigned char*)malloc(uIdatSize);
    if (pScan == NULL || pIdat == NULL)
    {
        os_printf("错误: PNG缓冲区分配失败\n");
        goto EXIT;
    }

    for (unsigned int y = 0; y < uHeight; y++)
    {
        unsigned int uDst = y * uRowBytes;
        pScan[uDst] = 0; /* filter type: None */
        memcpy(pScan + uDst + 1, pGray + y * uWidth, uWidth);
    }

    pCursor = pIdat;
    *pCursor++ = 0x78;
    *pCursor++ = 0x01;

    while (uRemaining > 0)
    {
        unsigned int uBlockLen = (uRemaining > 65535u) ? 65535u : uRemaining;
        int bFinal = (uRemaining == uBlockLen);
        unsigned short uLen = (unsigned short)uBlockLen;
        unsigned short uNLen = (unsigned short)~uLen;

        *pCursor++ = (unsigned char)(bFinal ? 1 : 0);
        *pCursor++ = (unsigned char)(uLen & 0xFFu);
        *pCursor++ = (unsigned char)((uLen >> 8) & 0xFFu);
        *pCursor++ = (unsigned char)(uNLen & 0xFFu);
        *pCursor++ = (unsigned char)((uNLen >> 8) & 0xFFu);
        memcpy(pCursor, pScan + uOffset, uBlockLen);
        pCursor += uBlockLen;
        uOffset += uBlockLen;
        uRemaining -= uBlockLen;
    }

    uAdler = PngAdler32(pScan, uRawSize);
    PutBe32(pCursor, uAdler);
    pCursor += 4;

    if ((unsigned int)(pCursor - pIdat) != uIdatSize)
    {
        os_printf("错误: PNG IDAT长度计算异常\n");
        goto EXIT;
    }

    pFile = fopen(pcPath, "wb");
    if (pFile == NULL)
    {
        os_printf("错误: 无法创建PNG文件 %s\n", pcPath);
        goto EXIT;
    }

    PutBe32(ihdr, uWidth);
    PutBe32(ihdr + 4, uHeight);
    ihdr[8] = 8;  /* bit depth */
    ihdr[9] = 0;  /* grayscale */
    ihdr[10] = 0; /* deflate */
    ihdr[11] = 0; /* adaptive filtering */
    ihdr[12] = 0; /* no interlace */

    if (fwrite(pngSig, 1, sizeof(pngSig), pFile) != sizeof(pngSig) ||
        WritePngChunk(pFile, "IHDR", ihdr, sizeof(ihdr)) != 0 ||
        WritePngChunk(pFile, "IDAT", pIdat, uIdatSize) != 0 ||
        WritePngChunk(pFile, "IEND", NULL, 0) != 0)
    {
        os_printf("错误: PNG文件写入失败 %s\n", pcPath);
        goto EXIT;
    }

    nRet = 0;

EXIT:
    if (pFile != NULL)
    {
        fclose(pFile);
    }
    if (pScan != NULL)
    {
        free(pScan);
    }
    if (pIdat != NULL)
    {
        free(pIdat);
    }
    return nRet;
}

static int ConvertRaw16ToGray8(const unsigned char* pRaw16, unsigned char* pGray8)
{
    unsigned int uPixelCount = V7_IMG_WIDTH * V7_IMG_HEIGHT;
    unsigned int uMin = 0xFFFFu;
    unsigned int uMax = 0u;

    if (pRaw16 == NULL || pGray8 == NULL)
    {
        return -1;
    }

    for (unsigned int i = 0; i < uPixelCount; i++)
    {
        unsigned int uValue = ((unsigned int)pRaw16[i * 2] << 8) | pRaw16[i * 2 + 1];
        if (uValue < uMin)
        {
            uMin = uValue;
        }
        if (uValue > uMax)
        {
            uMax = uValue;
        }
    }

    if (uMax == uMin)
    {
        memset(pGray8, 0, uPixelCount);
        return 0;
    }

    for (unsigned int i = 0; i < uPixelCount; i++)
    {
        unsigned int uValue = ((unsigned int)pRaw16[i * 2] << 8) | pRaw16[i * 2 + 1];
        pGray8[i] = (unsigned char)(((uValue - uMin) * 255u) / (uMax - uMin));
    }

    return 0;
}

static void ResetFrameBufferState(V7_FrameBuffer* pFrame)
{
    if (pFrame == NULL)
    {
        return;
    }

    if (pFrame->pPacketBitmap != NULL)
    {
        memset(pFrame->pPacketBitmap, 0, V7_TOTAL_PACKETS);
    }
    pFrame->uPacketCount = 0;
    pFrame->uImagePacketCount = 0;
    pFrame->uDuplicateCount = 0;
    pFrame->uFrameCounter = 0;
    pFrame->uMinRow = V7_TOTAL_ROWS;
    pFrame->uMaxRow = 0;
    pFrame->uMinAddr = 0xFFFFFFFFu;
    pFrame->uMaxAddr = 0;
    pFrame->uLastRow = 0;
    pFrame->uLastBlock = 0;
    pFrame->uLastAddr = 0;
    pFrame->bHasFrameCounter = 0;
}

static int InitFrameBuffers(void)
{
    for (unsigned int i = 0; i < V7_BANK_NUM; i++)
    {
        g_stFrameBuf[i].uBankIndex = i;
        if (g_stFrameBuf[i].pImageRaw16 == NULL)
        {
            g_stFrameBuf[i].pImageRaw16 = (unsigned char*)malloc(V7_IMAGE_BYTES);
        }
        if (g_stFrameBuf[i].pPacketBitmap == NULL)
        {
            g_stFrameBuf[i].pPacketBitmap = (unsigned char*)malloc(V7_TOTAL_PACKETS);
        }
        if (g_stFrameBuf[i].pImageRaw16 == NULL || g_stFrameBuf[i].pPacketBitmap == NULL)
        {
            os_printf("错误: V7帧缓冲区分配失败\n");
            return -1;
        }
        ResetFrameBufferState(&g_stFrameBuf[i]);
    }

    return 0;
}

static void FreeFrameBuffers(void)
{
    for (unsigned int i = 0; i < V7_BANK_NUM; i++)
    {
        if (g_stFrameBuf[i].pImageRaw16 != NULL)
        {
            free(g_stFrameBuf[i].pImageRaw16);
            g_stFrameBuf[i].pImageRaw16 = NULL;
        }
        if (g_stFrameBuf[i].pPacketBitmap != NULL)
        {
            free(g_stFrameBuf[i].pPacketBitmap);
            g_stFrameBuf[i].pPacketBitmap = NULL;
        }
    }
}

static unsigned long long GetFileSizeBytes(const char* pcPath)
{
    struct stat st;
    if (pcPath == NULL || stat(pcPath, &st) != 0)
    {
        return 0;
    }
    if (!S_ISREG(st.st_mode))
    {
        return 0;
    }
    return (unsigned long long)st.st_size;
}

static unsigned long long ScanSpoolDirectoryBytes(const char* pcPath)
{
    DIR* pDir = NULL;
    struct dirent* pEntry = NULL;
    unsigned long long ullBytes = 0;
    char acPath[MAX_FILE_PATH_LEN * 2];

    pDir = opendir(pcPath);
    if (pDir == NULL)
    {
        return 0;
    }

    while ((pEntry = readdir(pDir)) != NULL)
    {
        if (strstr(pEntry->d_name, ".raw16") == NULL)
        {
            continue;
        }
        if (snprintf(acPath, sizeof(acPath), "%s%s", pcPath, pEntry->d_name) >=
            (int)sizeof(acPath))
        {
            continue;
        }
        ullBytes += GetFileSizeBytes(acPath);
    }
    closedir(pDir);
    return ullBytes;
}

static int WaitForSpoolSpaceLocked(unsigned long long ullNeedBytes)
{
    int bWarned = 0;

    if (g_ullStreamMaxSpoolBytes == 0)
    {
        return 0;
    }
    if (ullNeedBytes > g_ullStreamMaxSpoolBytes)
    {
        os_printf("ERROR: one V7 spool frame is larger than STREAM_MAX_SPOOL_GB limit\n");
        return -1;
    }

    while (!g_bFrameWorkerStop &&
        g_ullStreamSpoolBytes + ullNeedBytes > g_ullStreamMaxSpoolBytes)
    {
        if (!bWarned)
        {
            os_printf("WARN: stream spool full, wait: used=%llu need=%llu limit=%llu\n",
                g_ullStreamSpoolBytes, ullNeedBytes, g_ullStreamMaxSpoolBytes);
            bWarned = 1;
        }
        pthread_cond_wait(&g_stJobCond, &g_stJobLock);
    }

    return g_bFrameWorkerStop ? -1 : 0;
}

static int WriteFrameJobToSpool(V7_FrameJob* pJob,
                                const unsigned char* pImageRaw16,
                                unsigned long long* pWrittenBytes)
{
    V7_SpoolHeader stHeader;
    char acTmpPath[MAX_FILE_PATH_LEN * 2];
    char acFinalPath[MAX_FILE_PATH_LEN * 2];
    FILE* pFile = NULL;
    size_t stWritten = 0;
    int nLen = 0;

    if (pJob == NULL || pImageRaw16 == NULL || pWrittenBytes == NULL)
    {
        return -1;
    }

    nLen = snprintf(acFinalPath, sizeof(acFinalPath),
        "%sv7_seq%06u_bank%u_frame%08u.raw16",
        g_acStreamSpoolPath,
        pJob->uSeq,
        pJob->uBankIndex,
        pJob->bHasFrameCounter ? pJob->uFrameCounter : 0);
    if (nLen < 0 || nLen >= (int)sizeof(acFinalPath))
    {
        os_printf("ERROR: stream spool path too long\n");
        return -1;
    }
    nLen = snprintf(acTmpPath, sizeof(acTmpPath), "%s.tmp", acFinalPath);
    if (nLen < 0 || nLen >= (int)sizeof(acTmpPath))
    {
        os_printf("ERROR: stream spool tmp path too long\n");
        return -1;
    }

    memset(&stHeader, 0, sizeof(stHeader));
    memcpy(stHeader.acMagic, V7_SPOOL_MAGIC, sizeof(stHeader.acMagic));
    stHeader.uVersion = V7_SPOOL_VERSION;
    stHeader.uImageBytes = V7_IMAGE_BYTES;
    stHeader.uBankIndex = pJob->uBankIndex;
    stHeader.uFrameCounter = pJob->uFrameCounter;
    stHeader.uHasFrameCounter = pJob->bHasFrameCounter ? 1u : 0u;
    stHeader.uSeq = pJob->uSeq;

    pFile = fopen(acTmpPath, "wb");
    if (pFile == NULL)
    {
        os_printf("ERROR: failed to create stream spool file %s\n", acTmpPath);
        return -1;
    }
    stWritten = fwrite(&stHeader, 1, sizeof(stHeader), pFile);
    stWritten += fwrite(pImageRaw16, 1, V7_IMAGE_BYTES, pFile);
    if (stWritten != sizeof(stHeader) + V7_IMAGE_BYTES)
    {
        os_printf("ERROR: failed to write stream spool file %s\n", acTmpPath);
        fclose(pFile);
        unlink(acTmpPath);
        return -1;
    }
    fflush(pFile);
    fclose(pFile);

    if (rename(acTmpPath, acFinalPath) != 0)
    {
        os_printf("ERROR: failed to rename stream spool file %s -> %s\n",
            acTmpPath, acFinalPath);
        unlink(acTmpPath);
        return -1;
    }

    strncpy(pJob->acSpoolPath, acFinalPath, sizeof(pJob->acSpoolPath) - 1);
    pJob->acSpoolPath[sizeof(pJob->acSpoolPath) - 1] = '\0';
    *pWrittenBytes = sizeof(stHeader) + V7_IMAGE_BYTES;
    return 0;
}

static unsigned char* ReadFrameJobFromSpool(V7_FrameJob* pJob)
{
    V7_SpoolHeader stHeader;
    unsigned char* pRaw16 = NULL;
    FILE* pFile = NULL;
    size_t stRead = 0;

    if (pJob == NULL || pJob->acSpoolPath[0] == '\0')
    {
        return NULL;
    }

    pFile = fopen(pJob->acSpoolPath, "rb");
    if (pFile == NULL)
    {
        os_printf("ERROR: failed to open stream spool file %s\n", pJob->acSpoolPath);
        return NULL;
    }

    stRead = fread(&stHeader, 1, sizeof(stHeader), pFile);
    if (stRead != sizeof(stHeader) ||
        memcmp(stHeader.acMagic, V7_SPOOL_MAGIC, sizeof(stHeader.acMagic)) != 0 ||
        stHeader.uVersion != V7_SPOOL_VERSION ||
        stHeader.uImageBytes != V7_IMAGE_BYTES)
    {
        os_printf("ERROR: invalid stream spool file %s\n", pJob->acSpoolPath);
        fclose(pFile);
        return NULL;
    }

    pRaw16 = (unsigned char*)malloc(V7_IMAGE_BYTES);
    if (pRaw16 == NULL)
    {
        os_printf("ERROR: failed to allocate stream spool read buffer\n");
        fclose(pFile);
        return NULL;
    }

    stRead = fread(pRaw16, 1, V7_IMAGE_BYTES, pFile);
    fclose(pFile);
    if (stRead != V7_IMAGE_BYTES)
    {
        os_printf("ERROR: short read from stream spool file %s\n", pJob->acSpoolPath);
        free(pRaw16);
        return NULL;
    }

    return pRaw16;
}

static void RemoveSpooledFrame(V7_FrameJob* pJob)
{
    unsigned long long ullSize = 0;

    if (pJob == NULL || pJob->acSpoolPath[0] == '\0')
    {
        return;
    }

    ullSize = GetFileSizeBytes(pJob->acSpoolPath);
    if (unlink(pJob->acSpoolPath) != 0)
    {
        os_printf("WARN: failed to remove stream spool file %s\n", pJob->acSpoolPath);
        return;
    }

    pthread_mutex_lock(&g_stJobLock);
    if (g_ullStreamSpoolBytes >= ullSize)
    {
        g_ullStreamSpoolBytes -= ullSize;
    }
    else
    {
        g_ullStreamSpoolBytes = 0;
    }
    if (g_uFrameSpoolQueueDepth > 0)
    {
        g_uFrameSpoolQueueDepth--;
    }
    g_uFrameSpoolReadCount++;
    pthread_cond_broadcast(&g_stJobCond);
    pthread_mutex_unlock(&g_stJobLock);
}

/*************************************************
Function:       EnqueueFrameJob
Description:    将已完成或可用的V7图像帧加入后台输出队列
Input:
pImageRaw16     - 去掉第0行参数行后的2048x2048 raw16图像缓冲
uBankIndex      - 乒乓bank编号
uFrameCounter   - 帧计数
bHasFrameCounter- 帧计数是否有效
Output:         无
Return:         0-成功，-1-失败
Others:         内存队列满时写入spool文件，后台线程按FIFO顺序读回
*************************************************/
static int EnqueueFrameJob(unsigned char* pImageRaw16, unsigned int uBankIndex,
                          unsigned int uFrameCounter, int bHasFrameCounter)
{
    V7_FrameJob* pJob = (V7_FrameJob*)malloc(sizeof(V7_FrameJob));
    int bUseSpool = 0;
    unsigned long long ullWrittenBytes = 0;
    if (pJob == NULL)
    {
        os_printf("错误: 帧输出任务分配失败\n");
        return -1;
    }

    memset(pJob, 0, sizeof(*pJob));
    pJob->uBankIndex = uBankIndex;
    pJob->uFrameCounter = uFrameCounter;
    pJob->bHasFrameCounter = bHasFrameCounter;

    pthread_mutex_lock(&g_stJobLock);
    pJob->uSeq = ++g_uFrameQueuedCount;
    if (g_uFrameMemoryQueueDepth < g_uStreamMaxMemFrames)
    {
        pJob->pImageRaw16 = pImageRaw16;
        g_uFrameMemoryQueueDepth++;
    }
    else
    {
        bUseSpool = 1;
        if (WaitForSpoolSpaceLocked(sizeof(V7_SpoolHeader) + V7_IMAGE_BYTES) != 0)
        {
            pthread_mutex_unlock(&g_stJobLock);
            free(pJob);
            return -1;
        }
    }
    pthread_mutex_unlock(&g_stJobLock);

    if (bUseSpool)
    {
        if (WriteFrameJobToSpool(pJob, pImageRaw16, &ullWrittenBytes) != 0)
        {
            free(pJob);
            return -1;
        }
        pJob->bOnDisk = 1;
        free(pImageRaw16);
        pImageRaw16 = NULL;

        pthread_mutex_lock(&g_stJobLock);
        g_ullStreamSpoolBytes += ullWrittenBytes;
        g_uFrameSpoolQueueDepth++;
        g_uFrameSpoolWriteCount++;
        pthread_mutex_unlock(&g_stJobLock);
    }

    pthread_mutex_lock(&g_stJobLock);
    if (g_pJobTail == NULL)
    {
        g_pJobHead = pJob;
        g_pJobTail = pJob;
    }
    else
    {
        g_pJobTail->pNext = pJob;
        g_pJobTail = pJob;
    }
    g_uFrameOutputQueueDepth++;
    pthread_cond_signal(&g_stJobCond);
    pthread_mutex_unlock(&g_stJobLock);

    return 0;
}

static int MoveFileToDir(const char* pcSrcPath, const char* pcDstDir, const char* pcFilename)
{
    char acDstPath[MAX_FILE_PATH_LEN * 2];
    int nLen = snprintf(acDstPath, sizeof(acDstPath), "%s%s", pcDstDir, pcFilename);
    if (nLen < 0 || nLen >= (int)sizeof(acDstPath))
    {
        return -1;
    }
    return rename(pcSrcPath, acDstPath);
}

static int SendPngByScp(const char* pcPngPath)
{
    char acLocalQuoted[MAX_FILE_PATH_LEN * 3];
    char acRemoteSpec[MAX_TRANSFER_FIELD_LEN * 3];
    char acRemoteQuoted[MAX_TRANSFER_FIELD_LEN * 4];
    char acCmd[MAX_SYSTEM_CMD_LEN];
    int nLen = 0;

    if (!g_nTransferEnable)
    {
        return 1;
    }
    if (g_acTransferUser[0] == '\0' || g_acTransferHost[0] == '\0' || g_acTransferDir[0] == '\0')
    {
        os_printf("警告: TRANSFER_ENABLE=1 但TRANSFER_USER/HOST/DIR未配置，跳过SCP\n");
        return 1;
    }

    nLen = snprintf(acRemoteSpec, sizeof(acRemoteSpec), "%s@%s:%s/",
        g_acTransferUser, g_acTransferHost, g_acTransferDir);
    if (nLen < 0 || nLen >= (int)sizeof(acRemoteSpec))
    {
        os_printf("错误: SCP远端路径过长\n");
        return -1;
    }

    if (ShellQuote(pcPngPath, acLocalQuoted, sizeof(acLocalQuoted)) != 0 ||
        ShellQuote(acRemoteSpec, acRemoteQuoted, sizeof(acRemoteQuoted)) != 0)
    {
        os_printf("错误: SCP路径引用失败\n");
        return -1;
    }

    nLen = snprintf(acCmd, sizeof(acCmd),
        "scp -q -o StrictHostKeyChecking=no -o ConnectTimeout=%d %s %s",
        g_nScpTimeoutSec, acLocalQuoted, acRemoteQuoted);
    if (nLen < 0 || nLen >= (int)sizeof(acCmd))
    {
        os_printf("错误: SCP命令过长\n");
        return -1;
    }

    os_printf("开始SCP发送: %s -> %s\n", pcPngPath, acRemoteSpec);
    if (system(acCmd) == 0)
    {
        return 0;
    }

    return -1;
}

/*************************************************
Function:       ProcessDiagnosticPngJob
Description:    保留的本地PNG/SCP诊断流程
Input:
pJob - 帧输出任务
Output:         无
Return:         0-成功，-1-失败
Others:         当前主流程使用HTTP MJPEG推流；该函数仅作为现场诊断备用
*************************************************/
static int ProcessDiagnosticPngJob(V7_FrameJob* pJob)
{
    unsigned char* pGray8 = NULL;
    char acFilename[128];
    char acTmpPath[MAX_FILE_PATH_LEN * 2];
    char acPngPath[MAX_FILE_PATH_LEN * 2];
    struct timeval tv;
    struct tm* pTm = NULL;
    time_t tNow;
    int nSendRet = 0;
    int nLen = 0;

    if (pJob == NULL || pJob->pImageRaw16 == NULL)
    {
        return -1;
    }

    pGray8 = (unsigned char*)malloc(V7_IMG_WIDTH * V7_IMG_HEIGHT);
    if (pGray8 == NULL)
    {
        os_printf("错误: 8bit灰度缓冲分配失败\n");
        return -1;
    }

    if (ConvertRaw16ToGray8(pJob->pImageRaw16, pGray8) != 0)
    {
        free(pGray8);
        return -1;
    }

    gettimeofday(&tv, NULL);
    tNow = tv.tv_sec;
    pTm = localtime(&tNow);
    if (pTm == NULL)
    {
        free(pGray8);
        return -1;
    }

    nLen = snprintf(acFilename, sizeof(acFilename),
        "v7_bank%u_frame%08u_seq%06u_%04d%02d%02d_%02d%02d%02d_%03ld.png",
        pJob->uBankIndex,
        pJob->bHasFrameCounter ? pJob->uFrameCounter : 0,
        pJob->uSeq,
        pTm->tm_year + 1900,
        pTm->tm_mon + 1,
        pTm->tm_mday,
        pTm->tm_hour,
        pTm->tm_min,
        pTm->tm_sec,
        tv.tv_usec / 1000);
    if (nLen < 0 || nLen >= (int)sizeof(acFilename))
    {
        os_printf("错误: PNG文件名过长\n");
        free(pGray8);
        return -1;
    }

    nLen = snprintf(acTmpPath, sizeof(acTmpPath), "%s%s.tmp", g_acPendingPath, acFilename);
    if (nLen < 0 || nLen >= (int)sizeof(acTmpPath))
    {
        os_printf("错误: PNG临时路径过长\n");
        free(pGray8);
        return -1;
    }
    nLen = snprintf(acPngPath, sizeof(acPngPath), "%s%s", g_acPendingPath, acFilename);
    if (nLen < 0 || nLen >= (int)sizeof(acPngPath))
    {
        os_printf("错误: PNG路径过长\n");
        free(pGray8);
        return -1;
    }

    if (WriteGray8Png(acTmpPath, pGray8, V7_IMG_WIDTH, V7_IMG_HEIGHT) != 0)
    {
        os_printf("错误: PNG编码失败\n");
        free(pGray8);
        return -1;
    }
    free(pGray8);

    if (rename(acTmpPath, acPngPath) != 0)
    {
        os_printf("错误: PNG临时文件改名失败 %s -> %s\n", acTmpPath, acPngPath);
        return -1;
    }

    os_printf("PNG保存完成: %s\n", acPngPath);

    nSendRet = SendPngByScp(acPngPath);
    if (nSendRet == 0)
    {
        g_uFrameSendOkCount++;
        if (MoveFileToDir(acPngPath, g_acSentPath, acFilename) != 0)
        {
            os_printf("警告: SCP成功但移动到sent目录失败: %s\n", acPngPath);
        }
        else
        {
            os_printf("SCP发送成功，文件已移动到sent目录: %s\n", acFilename);
        }
    }
    else if (nSendRet < 0)
    {
        g_uFrameSendFailCount++;
        if (MoveFileToDir(acPngPath, g_acFailedPath, acFilename) != 0)
        {
            os_printf("警告: SCP失败且移动到failed目录失败: %s\n", acPngPath);
        }
        else
        {
            os_printf("SCP发送失败，文件已移动到failed目录: %s\n", acFilename);
        }
    }
    else
    {
        os_printf("SCP未启用或未配置，文件保留在pending目录: %s\n", acPngPath);
    }

    return 0;
}

static unsigned long long GetTimeMs(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return 0;
    }
    return (unsigned long long)tv.tv_sec * 1000ull +
        (unsigned long long)tv.tv_usec / 1000ull;
}

static void InitMjpegClients(void)
{
    for (unsigned int i = 0; i < STREAM_MAX_CLIENTS_LIMIT; i++)
    {
        g_stMjpegClients[i].nFd = -1;
        g_stMjpegClients[i].acPeer[0] = '\0';
        g_stMjpegClients[i].uFramesSent = 0;
    }
    g_uMjpegClientActive = 0;
}

static int SetSocketNonBlocking(int nFd)
{
    int nFlags = fcntl(nFd, F_GETFL, 0);
    if (nFlags < 0)
    {
        return -1;
    }
    return fcntl(nFd, F_SETFL, nFlags | O_NONBLOCK);
}

static void SetClientSendTimeout(int nFd)
{
    struct timeval tv;
    tv.tv_sec = g_nStreamSendTimeoutMs / 1000;
    tv.tv_usec = (g_nStreamSendTimeoutMs % 1000) * 1000;
    setsockopt(nFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static int OpenMjpegListenSocket(void)
{
    int nFd = -1;
    int nReuse = 1;
    struct sockaddr_in stAddr;

    nFd = socket(AF_INET, SOCK_STREAM, 0);
    if (nFd < 0)
    {
        os_printf("ERROR: MJPEG socket() failed: %s\n", strerror(errno));
        return -1;
    }

    setsockopt(nFd, SOL_SOCKET, SO_REUSEADDR, &nReuse, sizeof(nReuse));

    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stAddr.sin_port = htons((unsigned short)g_nStreamPort);
    if (bind(nFd, (struct sockaddr*)&stAddr, sizeof(stAddr)) != 0)
    {
        os_printf("ERROR: MJPEG bind port %d failed: %s\n",
            g_nStreamPort, strerror(errno));
        close(nFd);
        return -1;
    }

    if (listen(nFd, (int)g_uStreamMaxClients) != 0)
    {
        os_printf("ERROR: MJPEG listen port %d failed: %s\n",
            g_nStreamPort, strerror(errno));
        close(nFd);
        return -1;
    }

    if (SetSocketNonBlocking(nFd) != 0)
    {
        os_printf("WARN: failed to set MJPEG listen socket nonblocking\n");
    }

    os_printf("V7 MJPEG server ready: http://0.0.0.0:%d (open http://192.168.1.100:%d)\n",
        g_nStreamPort, g_nStreamPort);
    return nFd;
}

static int SendAll(int nFd, const void* pData, size_t stLen)
{
    const unsigned char* pBuf = (const unsigned char*)pData;
    size_t stDone = 0;

    while (stDone < stLen)
    {
        ssize_t nRet = send(nFd, pBuf + stDone, stLen - stDone, MSG_NOSIGNAL);
        if (nRet > 0)
        {
            stDone += (size_t)nRet;
            continue;
        }
        if (nRet < 0 && errno == EINTR)
        {
            continue;
        }
        return -1;
    }
    return 0;
}

static void DropMjpegClient(unsigned int uIndex, const char* pcReason)
{
    if (uIndex >= STREAM_MAX_CLIENTS_LIMIT ||
        g_stMjpegClients[uIndex].nFd < 0)
    {
        return;
    }

    close(g_stMjpegClients[uIndex].nFd);
    os_printf("V7 MJPEG client closed: %s, reason=%s, frames=%u\n",
        g_stMjpegClients[uIndex].acPeer,
        pcReason != NULL ? pcReason : "unknown",
        g_stMjpegClients[uIndex].uFramesSent);
    g_stMjpegClients[uIndex].nFd = -1;
    g_stMjpegClients[uIndex].acPeer[0] = '\0';
    g_stMjpegClients[uIndex].uFramesSent = 0;
    if (g_uMjpegClientActive > 0)
    {
        g_uMjpegClientActive--;
    }
    g_uMjpegClientDropCount++;
}

static int SendMjpegHttpHeader(int nFd)
{
    const char* pcHeader =
        "HTTP/1.0 200 OK\r\n"
        "Server: RK3588-V7\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=" MJPEG_BOUNDARY "\r\n"
        "\r\n";
    return SendAll(nFd, pcHeader, strlen(pcHeader));
}

static void AcceptMjpegClients(void)
{
    while (1)
    {
        struct sockaddr_in stPeer;
        socklen_t stPeerLen = sizeof(stPeer);
        int nClient = accept(g_nMjpegListenFd, (struct sockaddr*)&stPeer, &stPeerLen);
        char acPeer[64];
        int nSlot = -1;

        if (nClient < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                return;
            }
            os_printf("WARN: MJPEG accept failed: %s\n", strerror(errno));
            return;
        }

        snprintf(acPeer, sizeof(acPeer), "%s:%u",
            inet_ntoa(stPeer.sin_addr), ntohs(stPeer.sin_port));

        SetClientSendTimeout(nClient);

        for (unsigned int i = 0; i < g_uStreamMaxClients; i++)
        {
            if (g_stMjpegClients[i].nFd < 0)
            {
                nSlot = (int)i;
                break;
            }
        }

        if (nSlot < 0)
        {
            os_printf("WARN: MJPEG client rejected, max clients reached: %s\n", acPeer);
            close(nClient);
            g_uMjpegClientDropCount++;
            continue;
        }

        if (SendMjpegHttpHeader(nClient) != 0)
        {
            os_printf("WARN: MJPEG client header send failed: %s\n", acPeer);
            close(nClient);
            g_uMjpegClientDropCount++;
            continue;
        }

        g_stMjpegClients[nSlot].nFd = nClient;
        strncpy(g_stMjpegClients[nSlot].acPeer, acPeer,
            sizeof(g_stMjpegClients[nSlot].acPeer) - 1);
        g_stMjpegClients[nSlot].acPeer[sizeof(g_stMjpegClients[nSlot].acPeer) - 1] = '\0';
        g_stMjpegClients[nSlot].uFramesSent = 0;
        g_uMjpegClientActive++;
        g_uMjpegClientConnectCount++;
        os_printf("V7 MJPEG client accepted: %s, slot=%d\n", acPeer, nSlot);
    }
}

static unsigned char* CopyLastJpegFrame(unsigned long* pBytes, unsigned int* pSeq)
{
    unsigned char* pCopy = NULL;

    if (pBytes == NULL || pSeq == NULL)
    {
        return NULL;
    }

    *pBytes = 0;
    *pSeq = 0;

    pthread_mutex_lock(&g_stMjpegLock);
    if (g_pLastJpegFrame != NULL && g_ulLastJpegFrameBytes > 0)
    {
        pCopy = (unsigned char*)malloc(g_ulLastJpegFrameBytes);
        if (pCopy != NULL)
        {
            memcpy(pCopy, g_pLastJpegFrame, g_ulLastJpegFrameBytes);
            *pBytes = g_ulLastJpegFrameBytes;
            *pSeq = g_uLastJpegSeq;
        }
    }
    pthread_mutex_unlock(&g_stMjpegLock);

    return pCopy;
}

static void BroadcastMjpegFrame(const unsigned char* pJpeg,
                                unsigned long ulBytes,
                                unsigned int uSeq,
                                int bRepeat)
{
    char acPartHeader[160];
    int nHeaderLen = 0;

    if (pJpeg == NULL || ulBytes == 0 || g_uMjpegClientActive == 0)
    {
        return;
    }

    nHeaderLen = snprintf(acPartHeader, sizeof(acPartHeader),
        "--" MJPEG_BOUNDARY "\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %lu\r\n"
        "X-V7-Seq: %u\r\n"
        "\r\n",
        ulBytes, uSeq);
    if (nHeaderLen <= 0 || nHeaderLen >= (int)sizeof(acPartHeader))
    {
        return;
    }

    for (unsigned int i = 0; i < g_uStreamMaxClients; i++)
    {
        if (g_stMjpegClients[i].nFd < 0)
        {
            continue;
        }

        if (SendAll(g_stMjpegClients[i].nFd, acPartHeader, (size_t)nHeaderLen) != 0 ||
            SendAll(g_stMjpegClients[i].nFd, pJpeg, ulBytes) != 0 ||
            SendAll(g_stMjpegClients[i].nFd, "\r\n", 2) != 0)
        {
            g_uMjpegSendFailCount++;
            DropMjpegClient(i, "send-failed");
            continue;
        }

        g_stMjpegClients[i].uFramesSent++;
    }

    if (bRepeat)
    {
        g_uMjpegRepeatCount++;
    }
}

static void PollMjpegClientReadEvents(fd_set* pReadFds)
{
    char acBuf[512];

    if (pReadFds == NULL)
    {
        return;
    }

    for (unsigned int i = 0; i < g_uStreamMaxClients; i++)
    {
        int nFd = g_stMjpegClients[i].nFd;
        if (nFd < 0 || !FD_ISSET(nFd, pReadFds))
        {
            continue;
        }

        while (1)
        {
            ssize_t nRead = recv(nFd, acBuf, sizeof(acBuf), MSG_DONTWAIT);
            if (nRead > 0)
            {
                continue;
            }
            if (nRead == 0)
            {
                DropMjpegClient(i, "peer-closed");
                break;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                break;
            }
            DropMjpegClient(i, "recv-error");
            break;
        }
    }
}

static void* MjpegServerMain(void* pArg)
{
    unsigned long long ullLastSendMs = 0;
    unsigned int uLastSeqSent = 0;

    (void)pArg;

    while (!g_bMjpegWorkerStop)
    {
        fd_set stReadFds;
        struct timeval tv;
        int nMaxFd = g_nMjpegListenFd;
        unsigned long long ullNow = GetTimeMs();
        unsigned int uIntervalMs = 1000u / (unsigned int)g_nStreamRepeatFps;

        if (uIntervalMs == 0)
        {
            uIntervalMs = 1;
        }

        FD_ZERO(&stReadFds);
        if (g_nMjpegListenFd >= 0)
        {
            FD_SET(g_nMjpegListenFd, &stReadFds);
        }
        for (unsigned int i = 0; i < g_uStreamMaxClients; i++)
        {
            int nFd = g_stMjpegClients[i].nFd;
            if (nFd >= 0)
            {
                FD_SET(nFd, &stReadFds);
                if (nFd > nMaxFd)
                {
                    nMaxFd = nFd;
                }
            }
        }
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if (g_nMjpegListenFd >= 0)
        {
            int nRet = select(nMaxFd + 1, &stReadFds, NULL, NULL, &tv);
            if (nRet > 0 && FD_ISSET(g_nMjpegListenFd, &stReadFds))
            {
                AcceptMjpegClients();
            }
            if (nRet > 0)
            {
                PollMjpegClientReadEvents(&stReadFds);
            }
        }
        else
        {
            os_sleepms(100);
        }

        ullNow = GetTimeMs();
        if (g_uMjpegClientActive > 0 &&
            (ullLastSendMs == 0 || ullNow - ullLastSendMs >= uIntervalMs))
        {
            unsigned long ulBytes = 0;
            unsigned int uSeq = 0;
            unsigned char* pJpeg = CopyLastJpegFrame(&ulBytes, &uSeq);
            if (pJpeg != NULL)
            {
                BroadcastMjpegFrame(pJpeg, ulBytes, uSeq, uSeq == uLastSeqSent);
                uLastSeqSent = uSeq;
                ullLastSendMs = ullNow;
                free(pJpeg);
            }
        }
    }

    for (unsigned int i = 0; i < STREAM_MAX_CLIENTS_LIMIT; i++)
    {
        if (g_stMjpegClients[i].nFd >= 0)
        {
            DropMjpegClient(i, "server-stop");
        }
    }

    if (g_nMjpegListenFd >= 0)
    {
        close(g_nMjpegListenFd);
        g_nMjpegListenFd = -1;
    }

    return NULL;
}

static int StartMjpegServer(void)
{
    if (!g_nStreamEnable)
    {
        return 0;
    }
    if (g_bMjpegWorkerStarted)
    {
        return 0;
    }

    signal(SIGPIPE, SIG_IGN);
    InitMjpegClients();
    g_nMjpegListenFd = OpenMjpegListenSocket();
    if (g_nMjpegListenFd < 0)
    {
        return -1;
    }

    g_bMjpegWorkerStop = 0;
    if (pthread_create(&g_stMjpegWorker, NULL, MjpegServerMain, NULL) != 0)
    {
        os_printf("ERROR: failed to start MJPEG server thread\n");
        close(g_nMjpegListenFd);
        g_nMjpegListenFd = -1;
        return -1;
    }

    g_bMjpegWorkerStarted = 1;
    return 0;
}

static void StopMjpegServer(void)
{
    if (!g_bMjpegWorkerStarted)
    {
        return;
    }

    g_bMjpegWorkerStop = 1;
    pthread_cond_broadcast(&g_stMjpegCond);
    pthread_join(g_stMjpegWorker, NULL);
    g_bMjpegWorkerStarted = 0;

    pthread_mutex_lock(&g_stMjpegLock);
    if (g_pLastJpegFrame != NULL)
    {
        free(g_pLastJpegFrame);
        g_pLastJpegFrame = NULL;
    }
    g_ulLastJpegFrameBytes = 0;
    g_uLastJpegSeq = 0;
    pthread_mutex_unlock(&g_stMjpegLock);
}

typedef struct _V7_JpegErrorMgr_
{
    struct jpeg_error_mgr stPub;
    jmp_buf stJmpBuf;
    char acMsg[JMSG_LENGTH_MAX];
} V7_JpegErrorMgr;

static void JpegErrorExit(j_common_ptr pCinfo)
{
    V7_JpegErrorMgr* pErr = (V7_JpegErrorMgr*)pCinfo->err;
    (*pCinfo->err->format_message)(pCinfo, pErr->acMsg);
    longjmp(pErr->stJmpBuf, 1);
}

static int EncodeGray8ToJpeg(const unsigned char* pGray8,
                             unsigned char** ppJpeg,
                             unsigned long* pJpegBytes)
{
    struct jpeg_compress_struct stCinfo;
    V7_JpegErrorMgr stErr;
    JSAMPROW rowPointer[1];

    if (pGray8 == NULL || ppJpeg == NULL || pJpegBytes == NULL)
    {
        return -1;
    }

    *ppJpeg = NULL;
    *pJpegBytes = 0;
    memset(&stCinfo, 0, sizeof(stCinfo));
    memset(&stErr, 0, sizeof(stErr));
    stCinfo.err = jpeg_std_error(&stErr.stPub);
    stErr.stPub.error_exit = JpegErrorExit;

    if (setjmp(stErr.stJmpBuf))
    {
        os_printf("ERROR: JPEG encode failed: %s\n", stErr.acMsg);
        jpeg_destroy_compress(&stCinfo);
        if (*ppJpeg != NULL)
        {
            free(*ppJpeg);
            *ppJpeg = NULL;
        }
        *pJpegBytes = 0;
        return -1;
    }

    jpeg_create_compress(&stCinfo);
    jpeg_mem_dest(&stCinfo, ppJpeg, pJpegBytes);
    stCinfo.image_width = V7_IMG_WIDTH;
    stCinfo.image_height = V7_IMG_HEIGHT;
    stCinfo.input_components = 1;
    stCinfo.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&stCinfo);
    jpeg_set_quality(&stCinfo, g_nStreamJpegQuality, TRUE);
    jpeg_start_compress(&stCinfo, TRUE);

    while (stCinfo.next_scanline < stCinfo.image_height)
    {
        rowPointer[0] = (JSAMPROW)&pGray8[stCinfo.next_scanline * V7_IMG_WIDTH];
        jpeg_write_scanlines(&stCinfo, rowPointer, 1);
    }

    jpeg_finish_compress(&stCinfo);
    jpeg_destroy_compress(&stCinfo);
    return 0;
}

static int UpdateLastJpegFrame(const unsigned char* pJpeg, unsigned long ulJpegBytes)
{
    unsigned char* pCopy = NULL;

    if (pJpeg == NULL || ulJpegBytes == 0)
    {
        return -1;
    }

    pCopy = (unsigned char*)malloc(ulJpegBytes);
    if (pCopy == NULL)
    {
        os_printf("ERROR: failed to allocate last JPEG frame buffer\n");
        return -1;
    }
    memcpy(pCopy, pJpeg, ulJpegBytes);

    pthread_mutex_lock(&g_stMjpegLock);
    if (g_pLastJpegFrame != NULL)
    {
        free(g_pLastJpegFrame);
    }
    g_pLastJpegFrame = pCopy;
    g_ulLastJpegFrameBytes = ulJpegBytes;
    g_uLastJpegSeq++;
    pthread_cond_broadcast(&g_stMjpegCond);
    pthread_mutex_unlock(&g_stMjpegLock);

    return 0;
}

/*************************************************
Function:       StreamRaw16Frame
Description:    将raw16灰度图像转换为浏览器可显示的JPEG帧
Input:
pRaw16 - 2048x2048 16bit灰度图像缓冲
Output:         无
Return:         0-成功，-1-失败
Others:         当前使用min-max线性拉伸为8bit，再用libjpeg编码灰度JPEG
*************************************************/
static int StreamRaw16Frame(const unsigned char* pRaw16)
{
    unsigned char* pGray8 = NULL;
    unsigned char* pJpeg = NULL;
    unsigned long ulJpegBytes = 0;
    int nRet = -1;

    if (!g_nStreamEnable)
    {
        return 0;
    }
    if (pRaw16 == NULL)
    {
        return -1;
    }

    pGray8 = (unsigned char*)malloc(V7_IMG_WIDTH * V7_IMG_HEIGHT);
    if (pGray8 == NULL)
    {
        os_printf("ERROR: failed to allocate stream gray buffer\n");
        return -1;
    }

    if (ConvertRaw16ToGray8(pRaw16, pGray8) != 0)
    {
        free(pGray8);
        return -1;
    }

    if (EncodeGray8ToJpeg(pGray8, &pJpeg, &ulJpegBytes) != 0)
    {
        g_uJpegEncodeFailCount++;
        free(pGray8);
        return -1;
    }

    nRet = UpdateLastJpegFrame(pJpeg, ulJpegBytes);
    free(pJpeg);
    free(pGray8);
    return nRet;
}

/*************************************************
Function:       ProcessFrameOutputJob
Description:    处理后台输出队列中的一帧
Input:
pJob - 帧输出任务，可能在内存中，也可能已经写入spool文件
Output:         无
Return:         0-成功，-1-失败
Others:         spool帧编码成功后立即删除本地spool文件并更新统计
*************************************************/
static int ProcessFrameOutputJob(V7_FrameJob* pJob)
{
    unsigned char* pRaw16 = NULL;
    int bLoadedFromDisk = 0;

    if (pJob == NULL)
    {
        return -1;
    }

    if (pJob->bOnDisk)
    {
        pRaw16 = ReadFrameJobFromSpool(pJob);
        bLoadedFromDisk = 1;
    }
    else
    {
        pRaw16 = pJob->pImageRaw16;
    }

    if (pRaw16 == NULL)
    {
        return -1;
    }

    if (StreamRaw16Frame(pRaw16) != 0)
    {
        if (bLoadedFromDisk)
        {
            free(pRaw16);
        }
        g_uFrameStreamFailCount++;
        return -1;
    }

    g_uFrameStreamOkCount++;
    if (pJob->bOnDisk)
    {
        RemoveSpooledFrame(pJob);
    }
    if (bLoadedFromDisk)
    {
        free(pRaw16);
    }

    return 0;
}

/*************************************************
Function:       FrameOutputWorkerMain
Description:    V7帧输出后台线程
Input:
pArg - 线程参数，未使用
Output:         无
Return:         NULL
Others:         编码失败时保留当前任务并重试，避免程序主动丢弃已收到帧
*************************************************/
static void* FrameOutputWorkerMain(void* pArg)
{
    (void)pArg;

    while (1)
    {
        V7_FrameJob* pJob = NULL;

        pthread_mutex_lock(&g_stJobLock);
        while (!g_bFrameWorkerStop && g_pJobHead == NULL)
        {
            pthread_cond_wait(&g_stJobCond, &g_stJobLock);
        }
        if (g_bFrameWorkerStop && g_pJobHead == NULL)
        {
            pthread_mutex_unlock(&g_stJobLock);
            break;
        }

        pJob = g_pJobHead;
        g_pJobHead = pJob->pNext;
        if (g_pJobHead == NULL)
        {
            g_pJobTail = NULL;
        }
        if (g_uFrameOutputQueueDepth > 0)
        {
            g_uFrameOutputQueueDepth--;
        }
        if (!pJob->bOnDisk && g_uFrameMemoryQueueDepth > 0)
        {
            g_uFrameMemoryQueueDepth--;
        }
        pthread_cond_broadcast(&g_stJobCond);
        pthread_mutex_unlock(&g_stJobLock);

        while (!g_bFrameWorkerStop && ProcessFrameOutputJob(pJob) != 0)
        {
            os_printf("WARN: stream output failed, retry frame seq=%u\n", pJob->uSeq);
            os_sleepms(1000);
        }
        if (pJob->pImageRaw16 != NULL)
        {
            free(pJob->pImageRaw16);
        }
        free(pJob);
    }

    return NULL;
}

/*************************************************
Function:       StartFrameOutputWorker
Description:    启动MJPEG服务线程和帧输出后台线程
Input:          无
Output:         无
Return:         0-成功，-1-失败
Others:         MJPEG服务负责浏览器连接，帧输出线程只更新最新JPEG帧
*************************************************/
static int StartFrameOutputWorker(void)
{
    if (g_bFrameWorkerStarted)
    {
        return 0;
    }

    g_bFrameWorkerStop = 0;
    if (StartMjpegServer() != 0)
    {
        return -1;
    }

    if (pthread_create(&g_stFrameWorker, NULL, FrameOutputWorkerMain, NULL) != 0)
    {
        os_printf("错误: 帧输出后台线程启动失败\n");
        StopMjpegServer();
        return -1;
    }
    g_bFrameWorkerStarted = 1;
    return 0;
}

/*************************************************
Function:       StopFrameOutputWorker
Description:    停止帧输出后台线程并关闭MJPEG服务
Input:          无
Output:         无
Return:         无
Others:         程序退出时调用，确保线程和socket正常释放
*************************************************/
static void StopFrameOutputWorker(void)
{
    if (!g_bFrameWorkerStarted)
    {
        return;
    }

    pthread_mutex_lock(&g_stJobLock);
    g_bFrameWorkerStop = 1;
    pthread_cond_signal(&g_stJobCond);
    pthread_mutex_unlock(&g_stJobLock);

    pthread_join(g_stFrameWorker, NULL);
    g_bFrameWorkerStarted = 0;
    StopMjpegServer();
}

static int InitV7Receiver(void)
{
    static int bInitialized = 0;

    if (bInitialized)
    {
        return 0;
    }

    ReadConfigString(CONFIG_FILE_NAME, "PNG_SAVE_PATH",
        DEFAULT_PNG_SAVE_PATH, g_acPngSavePath, sizeof(g_acPngSavePath));
    AppendSlash(g_acPngSavePath, sizeof(g_acPngSavePath));

    g_bPrintMsg = ReadSimpleConfig(CONFIG_FILE_NAME, "PRINTF_UP_MAG", 1);
    g_nTransferEnable = ReadSimpleConfig(CONFIG_FILE_NAME, "TRANSFER_ENABLE", 0);
    g_nScpTimeoutSec = ReadSimpleConfig(CONFIG_FILE_NAME, "SCP_TIMEOUT_SEC", 30);
    g_nRawCaptureEnable = ReadSimpleConfig(CONFIG_FILE_NAME, "RAW_CAPTURE_ENABLE", 1);
    g_nSwriteTraceEnable = ReadSimpleConfig(CONFIG_FILE_NAME, "SWRITE_TRACE_ENABLE", 0);
    g_nStreamEnable = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_ENABLE", 1);
    g_nStreamPartialEnable = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_PARTIAL_ENABLE", 1);
    g_nStreamPort = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_PORT", DEFAULT_STREAM_PORT);
    g_nStreamJpegQuality = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_JPEG_QUALITY",
        DEFAULT_STREAM_JPEG_QUALITY);
    g_nStreamRepeatFps = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_REPEAT_FPS",
        DEFAULT_STREAM_REPEAT_FPS);
    g_nStreamSendTimeoutMs = ReadSimpleConfig(CONFIG_FILE_NAME, "STREAM_SEND_TIMEOUT_MS",
        DEFAULT_STREAM_SEND_TIMEOUT_MS);
    g_uStreamMaxMemFrames = ReadConfigUInt(CONFIG_FILE_NAME, "STREAM_MAX_MEM_FRAMES",
        DEFAULT_STREAM_MAX_MEM_FRAMES);
    g_uStreamMaxSpoolGb = ReadConfigUInt(CONFIG_FILE_NAME, "STREAM_MAX_SPOOL_GB",
        DEFAULT_STREAM_MAX_SPOOL_GB);
    g_uStreamMinImagePackets = ReadConfigUInt(CONFIG_FILE_NAME, "STREAM_MIN_IMAGE_PACKETS",
        DEFAULT_STREAM_MIN_IMAGE_PACKETS);
    g_uStreamMaxClients = ReadConfigUInt(CONFIG_FILE_NAME, "STREAM_MAX_CLIENTS",
        DEFAULT_STREAM_MAX_CLIENTS);
    g_uRawCaptureMaxFiles = ReadConfigUInt(CONFIG_FILE_NAME, "RAW_CAPTURE_MAX_FILES",
        DEFAULT_RAW_CAPTURE_FILES);
    g_uRawCaptureMaxBytes = ReadConfigUInt(CONFIG_FILE_NAME, "RAW_CAPTURE_MAX_BYTES_PER_PACKET",
        DEFAULT_RAW_CAPTURE_BYTES);
    g_nDoorbellReadEnable = 0;
    if (g_nScpTimeoutSec <= 0)
    {
        g_nScpTimeoutSec = 30;
    }

    ReadConfigString(CONFIG_FILE_NAME, "TRANSFER_USER", "", g_acTransferUser, sizeof(g_acTransferUser));
    ReadConfigString(CONFIG_FILE_NAME, "TRANSFER_HOST", "", g_acTransferHost, sizeof(g_acTransferHost));
    ReadConfigString(CONFIG_FILE_NAME, "TRANSFER_DIR", "", g_acTransferDir, sizeof(g_acTransferDir));
    ReadConfigString(CONFIG_FILE_NAME, "PNG_SCALE_MODE", "minmax", g_acPngScaleMode, sizeof(g_acPngScaleMode));
    ReadConfigString(CONFIG_FILE_NAME, "STREAM_SCALE_MODE", "minmax",
        g_acStreamScaleMode, sizeof(g_acStreamScaleMode));
    ReadConfigString(CONFIG_FILE_NAME, "STREAM_MODE", "lossless",
        g_acStreamMode, sizeof(g_acStreamMode));
    ReadConfigString(CONFIG_FILE_NAME, "RAW_CAPTURE_PATH",
        DEFAULT_RAW_CAPTURE_PATH, g_acRawCapturePath, sizeof(g_acRawCapturePath));
    ReadConfigString(CONFIG_FILE_NAME, "STREAM_SPOOL_PATH",
        DEFAULT_STREAM_SPOOL_PATH, g_acStreamSpoolPath, sizeof(g_acStreamSpoolPath));
    AppendSlash(g_acRawCapturePath, sizeof(g_acRawCapturePath));
    AppendSlash(g_acStreamSpoolPath, sizeof(g_acStreamSpoolPath));
    if (strcmp(g_acPngScaleMode, "minmax") != 0)
    {
        os_printf("警告: 当前仅支持 PNG_SCALE_MODE=minmax，实际配置=%s，将按minmax处理\n",
            g_acPngScaleMode);
    }

    if (strcmp(g_acStreamScaleMode, "minmax") != 0)
    {
        os_printf("WARN: only STREAM_SCALE_MODE=minmax is supported, got %s\n",
            g_acStreamScaleMode);
    }
    if (g_nStreamPort <= 0)
    {
        g_nStreamPort = DEFAULT_STREAM_PORT;
    }
    if (g_nStreamJpegQuality <= 0 || g_nStreamJpegQuality > 100)
    {
        g_nStreamJpegQuality = DEFAULT_STREAM_JPEG_QUALITY;
    }
    if (g_nStreamRepeatFps <= 0 || g_nStreamRepeatFps > 60)
    {
        g_nStreamRepeatFps = DEFAULT_STREAM_REPEAT_FPS;
    }
    if (g_nStreamSendTimeoutMs < 100 || g_nStreamSendTimeoutMs > 5000)
    {
        g_nStreamSendTimeoutMs = DEFAULT_STREAM_SEND_TIMEOUT_MS;
    }
    if (g_uStreamMaxMemFrames == 0)
    {
        g_uStreamMaxMemFrames = 1;
    }
    if (g_uStreamMaxClients == 0)
    {
        g_uStreamMaxClients = DEFAULT_STREAM_MAX_CLIENTS;
    }
    if (g_uStreamMaxClients > STREAM_MAX_CLIENTS_LIMIT)
    {
        g_uStreamMaxClients = STREAM_MAX_CLIENTS_LIMIT;
    }
    if (g_uStreamMinImagePackets > V7_IMAGE_PACKETS)
    {
        g_uStreamMinImagePackets = V7_IMAGE_PACKETS;
    }
    g_ullStreamMaxSpoolBytes =
        (unsigned long long)g_uStreamMaxSpoolGb * 1024ull * 1024ull * 1024ull;

    if (BuildSubdir(g_acPendingPath, sizeof(g_acPendingPath), g_acPngSavePath, "pending") != 0 ||
        BuildSubdir(g_acSentPath, sizeof(g_acSentPath), g_acPngSavePath, "sent") != 0 ||
        BuildSubdir(g_acFailedPath, sizeof(g_acFailedPath), g_acPngSavePath, "failed") != 0)
    {
        os_printf("错误: PNG目录路径过长\n");
        return -1;
    }

    if (EnsureDirectory(g_acPngSavePath) != 0 ||
        EnsureDirectory(g_acPendingPath) != 0 ||
        EnsureDirectory(g_acSentPath) != 0 ||
        EnsureDirectory(g_acFailedPath) != 0 ||
        EnsureDirectory(g_acStreamSpoolPath) != 0)
    {
        return -1;
    }
    if (g_nRawCaptureEnable && EnsureDirectory(g_acRawCapturePath) != 0)
    {
        return -1;
    }
    g_ullStreamSpoolBytes = ScanSpoolDirectoryBytes(g_acStreamSpoolPath);

    if (InitFrameBuffers() != 0)
    {
        return -1;
    }

    if (StartFrameOutputWorker() != 0)
    {
        return -1;
    }

    bInitialized = 1;
    os_printf("V7图像接收已初始化: PNG路径=%s, SCP=%s, 目标=%s@%s:%s\n",
        g_acPngSavePath,
        g_nTransferEnable ? "启用" : "禁用",
        g_acTransferUser,
        g_acTransferHost,
        g_acTransferDir);
    os_printf("V7 live stream: %s, port=%d, mode=%s, mem_frames=%u, spool=%s, spool_used=%llu, spool_limit=%llu\n",
        g_nStreamEnable ? "enable" : "disable",
        g_nStreamPort,
        g_acStreamMode,
        g_uStreamMaxMemFrames,
        g_acStreamSpoolPath,
        g_ullStreamSpoolBytes,
        g_ullStreamMaxSpoolBytes);
    os_printf("V7 MJPEG stream: jpeg_quality=%d, repeat_fps=%d, max_clients=%u, send_timeout_ms=%d\n",
        g_nStreamJpegQuality,
        g_nStreamRepeatFps,
        g_uStreamMaxClients,
        g_nStreamSendTimeoutMs);
    os_printf("V7 live stream partial: %s, min_image_packets=%u/%u\n",
        g_nStreamPartialEnable ? "enable" : "disable",
        g_uStreamMinImagePackets,
        V7_IMAGE_PACKETS);
    os_printf("V7 doorbell DDR read: %s, BAR=%d(auto=-1), has_param_row=%d, bank0_off=0x%08X, bank1_off=0x%08X\n",
        "disable", -1, 0, 0, 0);
    os_printf("V7 raw capture: %s, path=%s, max_files=%u, max_bytes=%u\n",
        g_nRawCaptureEnable ? "enable" : "disable",
        g_acRawCapturePath,
        g_uRawCaptureMaxFiles,
        g_uRawCaptureMaxBytes);
    os_printf("V7 swrite trace: %s\n",
        g_nSwriteTraceEnable ? "enable" : "disable");
    return 0;
}

static unsigned int ExtractFrameCounter(const volatile unsigned char* pPayload)
{
    unsigned int uLow16 = ReadBe16(pPayload + 2);
    unsigned int uHigh16 = ReadBe16(pPayload + 4);
    return (uHigh16 << 16) | uLow16;
}

static int EnsureV7BarMapped(void)
{
    if (g_bV7BarMapped)
    {
        return 0;
    }

    memset(&g_stV7BarResource, 0, sizeof(g_stV7BarResource));
    g_hV7MapDev = Dev_OpenDevByName(g_acV7DevNode);
    if (g_hV7MapDev == (DEVICE_HANDLE)-1)
    {
        os_printf("ERROR: Dev_OpenDevByName(%s) failed\n", g_acV7DevNode);
        return -1;
    }

    if (Dev_MapBarResource(g_hV7MapDev, &g_stV7BarResource) == 0)
    {
        os_printf("ERROR: Dev_MapBarResource(%s) failed\n", g_acV7DevNode);
        Dev_CloseDev(g_hV7MapDev);
        g_hV7MapDev = (DEVICE_HANDLE)-1;
        return -1;
    }

    g_bV7BarMapped = 1;
    os_printf("V7 mapped BAR via %s\n", g_acV7DevNode);
    for (int i = 0; i < MAX_USED_BAR_NUM; i++)
    {
        os_printf("V7 BAR%d: addr=%p, size=0x%08X (%u)\n",
            i,
            g_stV7BarResource.pBarAddr[i],
            g_stV7BarResource.nBarSize[i],
            g_stV7BarResource.nBarSize[i]);
    }

    return 0;
}

static void ReleaseV7BarMapped(void)
{
    if (!g_bV7BarMapped)
    {
        return;
    }

    Dev_UnmapBarResource(g_hV7MapDev, &g_stV7BarResource);
    Dev_CloseDev(g_hV7MapDev);
    memset(&g_stV7BarResource, 0, sizeof(g_stV7BarResource));
    g_hV7MapDev = (DEVICE_HANDLE)-1;
    g_bV7BarMapped = 0;
}

static void PrintV7MemInfoOnce(void)
{
    static int bPrinted = 0;
    unsigned int uMemSize = 0;

    if (bPrinted)
    {
        return;
    }

    bPrinted = 1;
    if (g_hV7DevHandle != NULL &&
        cvg_card_mem_get(g_hV7DevHandle, &uMemSize) == 0)
    {
        os_printf("V7 onboard DDR size: %u bytes\n", uMemSize);
    }
}

static unsigned int ExtractDoorbellInfo(const volatile unsigned char* pPack, unsigned int nPackLen)
{
    unsigned int uInfo12 = 0xFFFFFFFFu;
    unsigned int uInfo14 = 0xFFFFFFFFu;

    if (pPack == NULL || nPackLen < PACKET_HEADER_SIZE)
    {
        return 0xFFFFFFFFu;
    }

    uInfo12 = ReadBe16(pPack + 12);
    uInfo14 = ReadBe16(pPack + 14);

    if (uInfo14 == 0 || uInfo14 == 1)
    {
        return uInfo14;
    }
    if (uInfo12 == 0 || uInfo12 == 1)
    {
        return uInfo12;
    }

    return uInfo14;
}

static unsigned int DoorbellInfoToBank(unsigned int uDoorbellInfo)
{
    if (uDoorbellInfo == 0xFFFFFFFFu)
    {
        return 0;
    }

    return uDoorbellInfo & 1u;
}

static int SelectV7MemoryBar(pBarRes_st pBarRes,
                                 unsigned int uBankIndex,
                                 unsigned int* pEffectiveOffset,
                                 unsigned int uReadBytes)
{
    unsigned long long ullNeedSize = 0;
    unsigned int uOffset = 0;

    if (pBarRes == NULL || uBankIndex >= V7_BANK_NUM || pEffectiveOffset == NULL)
    {
        return -1;
    }

    uOffset = g_uV7BankOffset[uBankIndex];
    ullNeedSize = (unsigned long long)uOffset + (unsigned long long)uReadBytes;

    if (g_nV7MemBar >= 0 && g_nV7MemBar < MAX_USED_BAR_NUM)
    {
        int nBar = g_nV7MemBar;
        if (pBarRes->pBarAddr[nBar] != NULL &&
            (unsigned long long)pBarRes->nBarSize[nBar] >= ullNeedSize)
        {
            *pEffectiveOffset = uOffset;
            return nBar;
        }

        if (uOffset != 0 &&
            pBarRes->pBarAddr[nBar] != NULL &&
            (unsigned long long)pBarRes->nBarSize[nBar] >= (unsigned long long)uReadBytes)
        {
            os_printf("WARN: V7 bank%u offset 0x%08X is outside BAR%d, fallback to BAR window offset 0\n",
                uBankIndex, uOffset, nBar);
            *pEffectiveOffset = 0;
            return nBar;
        }

        os_printf("ERROR: V7_MEM_BAR=%d is too small or not mapped. size=0x%08X need=0x%llX\n",
            nBar, pBarRes->nBarSize[nBar], ullNeedSize);
        return -1;
    }

    for (int i = 0; i < MAX_USED_BAR_NUM; i++)
    {
        if (pBarRes->pBarAddr[i] != NULL &&
            (unsigned long long)pBarRes->nBarSize[i] >= ullNeedSize)
        {
            *pEffectiveOffset = uOffset;
            return i;
        }
    }

    if (uOffset != 0)
    {
        for (int i = 0; i < MAX_USED_BAR_NUM; i++)
        {
            if (pBarRes->pBarAddr[i] != NULL &&
                (unsigned long long)pBarRes->nBarSize[i] >= (unsigned long long)uReadBytes)
            {
                os_printf("WARN: V7 bank%u offset 0x%08X is outside mapped BARs, fallback to BAR%d window offset 0\n",
                    uBankIndex, uOffset, i);
                *pEffectiveOffset = 0;
                return i;
            }
        }
    }

    os_printf("ERROR: no mapped BAR can cover V7 bank%u offset=0x%08X size=0x%X\n",
        uBankIndex, g_uV7BankOffset[uBankIndex], uReadBytes);
    return -1;
}

static void CopyFromMappedMemory(unsigned char* pDst, const volatile unsigned char* pSrc, unsigned int uLen)
{
    for (unsigned int i = 0; i < uLen; i++)
    {
        pDst[i] = pSrc[i];
    }
}

/*************************************************
Function:       QueueFrameFromMappedMemory
Description:    旧版doorbell后读取BAR/DDR的诊断路径
Input:
uBankIndex     - bank编号
uDoorbellInfo  - doorbell信息
Output:         无
Return:         0-成功，-1-失败
Others:         当前现场配置固定关闭该路径，主流程只使用SWRITE包拼帧，避免BAR映射异常导致崩溃
*************************************************/
static int QueueFrameFromMappedMemory(unsigned int uBankIndex, unsigned int uDoorbellInfo)
{
    int nBar = -1;
    volatile unsigned char* pFrameBase = NULL;
    volatile unsigned char* pImageBase = NULL;
    unsigned char* pImageRaw16 = NULL;
    unsigned int uFrameCounter = 0;
    unsigned int uReadBytes = 0;
    unsigned int uEffectiveOffset = 0;

    if (uBankIndex >= V7_BANK_NUM)
    {
        os_printf("ERROR: invalid V7 bank from doorbell info=0x%X\n", uDoorbellInfo);
        return -1;
    }

    PrintV7MemInfoOnce();
    if (EnsureV7BarMapped() != 0)
    {
        return -1;
    }

    uReadBytes = g_nMappedHasParamRow ? V7_FRAME_BYTES : V7_IMAGE_BYTES;
    nBar = SelectV7MemoryBar(&g_stV7BarResource, uBankIndex, &uEffectiveOffset, uReadBytes);
    if (nBar < 0)
    {
        return -1;
    }

    pFrameBase = (volatile unsigned char*)g_stV7BarResource.pBarAddr[nBar] +
        uEffectiveOffset;
    if (pFrameBase == NULL)
    {
        return -1;
    }

    pImageRaw16 = (unsigned char*)malloc(V7_IMAGE_BYTES);
    if (pImageRaw16 == NULL)
    {
        os_printf("ERROR: failed to allocate V7 mapped frame buffer\n");
        return -1;
    }

    __sync_synchronize();
    if (g_nMappedHasParamRow)
    {
        uFrameCounter = ExtractFrameCounter(pFrameBase);
        pImageBase = pFrameBase + V7_ROW_BYTES;
    }
    else
    {
        uFrameCounter = g_uDoorbellCount;
        pImageBase = pFrameBase;
    }
    CopyFromMappedMemory(pImageRaw16, pImageBase, V7_IMAGE_BYTES);
    __sync_synchronize();

    if (EnqueueFrameJob(pImageRaw16, uBankIndex, uFrameCounter, 1) != 0)
    {
        free(pImageRaw16);
        return -1;
    }

    g_uDoorbellFrameCount++;
    g_uFrameCompleteCount++;
    os_printf("V7 doorbell frame queued: bank=%u, info=0x%X, frame=%u, BAR%d+0x%08X, bytes=0x%X, completed=%u\n",
        uBankIndex,
        uDoorbellInfo,
        uFrameCounter,
        nBar,
        uEffectiveOffset,
        V7_IMAGE_BYTES,
        g_uFrameCompleteCount);

    return 0;
}

static void CompleteFrameLocked(V7_FrameBuffer* pFrame, int bPartial)
{
    unsigned char* pCompleted = NULL;
    unsigned char* pNewBuffer = NULL;

    if (pFrame == NULL || pFrame->pImageRaw16 == NULL)
    {
        return;
    }

    pNewBuffer = (unsigned char*)malloc(V7_IMAGE_BYTES);
    if (pNewBuffer == NULL)
    {
        os_printf("错误: 新帧缓冲区分配失败，丢弃已完成帧\n");
        ResetFrameBufferState(pFrame);
        return;
    }

    pCompleted = pFrame->pImageRaw16;
    pFrame->pImageRaw16 = pNewBuffer;

    g_uFrameCompleteCount++;
    if (bPartial)
    {
        g_uPartialFrameQueuedCount++;
    }
    os_printf("V7帧接收完成: bank=%u, frame=%u, 包=%u, 图像包=%u, 重复=%u, 完成帧=%u\n",
        pFrame->uBankIndex,
        pFrame->bHasFrameCounter ? pFrame->uFrameCounter : 0,
        pFrame->uPacketCount,
        pFrame->uImagePacketCount,
        pFrame->uDuplicateCount,
        g_uFrameCompleteCount);
    if (bPartial)
    {
        os_printf("V7 partial frame accepted for live stream: bank=%u, image=%u/%u, min_image=%u, partial_queued=%u\n",
            pFrame->uBankIndex,
            pFrame->uImagePacketCount,
            V7_IMAGE_PACKETS,
            g_uStreamMinImagePackets,
            g_uPartialFrameQueuedCount);
    }

    if (EnqueueFrameJob(pCompleted, pFrame->uBankIndex,
        pFrame->uFrameCounter, pFrame->bHasFrameCounter) != 0)
    {
        free(pCompleted);
    }

    ResetFrameBufferState(pFrame);
}

static int ShouldQueuePartialFrameLocked(const V7_FrameBuffer* pFrame)
{
    if (!g_nStreamPartialEnable || pFrame == NULL)
    {
        return 0;
    }

    if (pFrame->uImagePacketCount < g_uStreamMinImagePackets)
    {
        return 0;
    }

    if (pFrame->uMaxRow < V7_IMG_HEIGHT)
    {
        return 0;
    }

    return 1;
}

static void LogFrameMissingLocked(V7_FrameBuffer* pFrame, unsigned int uDoorbellInfo)
{
    unsigned int uMissing = 0;
    unsigned int uPrinted = 0;

    if (pFrame == NULL || pFrame->pPacketBitmap == NULL)
    {
        return;
    }

    if (pFrame->uPacketCount < V7_TOTAL_PACKETS)
    {
        uMissing = V7_TOTAL_PACKETS - pFrame->uPacketCount;
    }

    os_printf("V7 incomplete frame on doorbell: bank=%u, info=0x%X, frame=%u, packets=%u/%u, image=%u/%u, duplicate=%u, missing=%u\n",
        pFrame->uBankIndex,
        uDoorbellInfo,
        pFrame->bHasFrameCounter ? pFrame->uFrameCounter : 0,
        pFrame->uPacketCount,
        V7_TOTAL_PACKETS,
        pFrame->uImagePacketCount,
        V7_IMAGE_PACKETS,
        pFrame->uDuplicateCount,
        uMissing);
    if (pFrame->uPacketCount > 0)
    {
        os_printf("  swrite range: addr=0x%08X..0x%08X, row=%u..%u, last=addr0x%08X row%u.block%u\n",
            pFrame->uMinAddr,
            pFrame->uMaxAddr,
            pFrame->uMinRow,
            pFrame->uMaxRow,
            pFrame->uLastAddr,
            pFrame->uLastRow,
            pFrame->uLastBlock);
    }

    os_printf("  first missing:");
    for (unsigned int i = 0; i < V7_TOTAL_PACKETS && uPrinted < 8; i++)
    {
        if (pFrame->pPacketBitmap[i] == 0)
        {
            unsigned int uRow = i / V7_PACKETS_PER_ROW;
            unsigned int uBlock = i % V7_PACKETS_PER_ROW;
            os_printf(" row%u.block%u", uRow, uBlock);
            uPrinted++;
        }
    }
    if (uPrinted == 0)
    {
        os_printf(" none");
    }
    os_printf("\n");
}

static int SelectDoorbellFrameCandidateLocked(unsigned int uDoorbellBank)
{
    unsigned int uBestPackets = 0;
    int nBestBank = -1;

    for (unsigned int i = 0; i < V7_BANK_NUM; i++)
    {
        if (g_stFrameBuf[i].uPacketCount > uBestPackets)
        {
            uBestPackets = g_stFrameBuf[i].uPacketCount;
            nBestBank = (int)i;
        }
    }

    if (nBestBank >= 0)
    {
        return nBestBank;
    }

    if (uDoorbellBank < V7_BANK_NUM)
    {
        return (int)uDoorbellBank;
    }
    return -1;
}

static void FinalizeFrameOnDoorbell(unsigned int uDoorbellBank,
                                    unsigned int uDoorbellInfo,
                                    int bAllowEmpty)
{
    V7_FrameBuffer* pFrame = NULL;
    int nCandidateBank = -1;

    if (uDoorbellBank >= V7_BANK_NUM)
    {
        os_printf("V7 doorbell ignored: invalid bank=%u, info=0x%X\n",
            uDoorbellBank, uDoorbellInfo);
        g_uErrorPackets++;
        return;
    }

    pthread_mutex_lock(&g_stFrameLock);
    nCandidateBank = SelectDoorbellFrameCandidateLocked(uDoorbellBank);
    if (nCandidateBank < 0)
    {
        pthread_mutex_unlock(&g_stFrameLock);
        return;
    }

    pFrame = &g_stFrameBuf[nCandidateBank];
    if (pFrame->uPacketCount == 0 && !bAllowEmpty)
    {
        pthread_mutex_unlock(&g_stFrameLock);
        return;
    }

    if (pFrame->uPacketCount == V7_TOTAL_PACKETS &&
        pFrame->uImagePacketCount == V7_IMAGE_PACKETS)
    {
        g_uDoorbellFrameCount++;
        CompleteFrameLocked(pFrame, 0);
    }
    else
    {
        g_uIncompleteFrameCount++;
        if (g_bPrintMsg &&
            (g_uIncompleteFrameCount <= 8 || (g_uIncompleteFrameCount % 100) == 0))
        {
            LogFrameMissingLocked(pFrame, uDoorbellInfo);
        }
        if (ShouldQueuePartialFrameLocked(pFrame))
        {
            g_uDoorbellFrameCount++;
            CompleteFrameLocked(pFrame, 1);
        }
        else
        {
            ResetFrameBufferState(pFrame);
        }
    }
    pthread_mutex_unlock(&g_stFrameLock);
}

/*************************************************
Function:       ChannelRecvDataCallBack
Description:    PCIE数据接收回调函数
接收V7 SRIO SWRITE图像包，按地址写入乒乓帧缓冲
doorbell到来时判断完整帧或可用partial帧，并交给后台线程编码推流
Input:
pChHandle - 通道句柄
dmaChan   - DMA通道号
pPack     - 数据包指针
nPackLen  - 数据包长度
nDropPack - 丢弃的数据包数
pUserParam- 用户参数
Output:         无
Return:         无
Others:         回调入口先递增g_uSrioRecvCount，用于判断上游是否仍在发包
*************************************************/
static void ChannelRecvDataCallBack(OS_HANDLE pChHandle,
    int dmaChan,
    const volatile unsigned char* pPack,
    unsigned int nPackLen,
    int nDropPack,
    void* pUserParam)
{
#if 1
    unsigned int uPayloadLen = 0;
    unsigned int uSrcId = 0;
    unsigned int uDstId = 0;
    unsigned int uFtype = 0;
    unsigned int uSwriteAddr = 0;
    unsigned int uBankBase = 0;
    unsigned int uOffset = 0;
    unsigned int uRow = 0;
    unsigned int uBlock = 0;
    unsigned int uPacketIndex = 0;
    int nBankIndex = -1;
    V7_FrameBuffer* pFrame = NULL;
    const volatile unsigned char* pPayload = NULL;

    (void)pChHandle;
    (void)pUserParam;

    if (InitV7Receiver() != 0)
    {
        g_uErrorPackets++;
        return;
    }

    if (pPack == NULL)
    {
        g_uErrorPackets++;
        return;
    }

    g_uSrioRecvCount++;
    if (nDropPack > 0)
    {
        g_uSdkDropPackets += (unsigned int)nDropPack;
        if (g_bPrintMsg && (g_uSdkDropPackets <= 8 || (g_uSdkDropPackets % 1000) == 0))
        {
            os_printf("WARN: SDK reported dropped packets: nDropPack=%d, total=%u\n",
                nDropPack, g_uSdkDropPackets);
        }
    }

    if (nPackLen < PACKET_HEADER_SIZE)
    {
        if (g_bPrintMsg)
        {
            os_printf("忽略短包: nPackLen=%u\n", nPackLen);
        }
        return;
    }

    uSrcId = ReadBe16(pPack);
    uDstId = ReadBe16(pPack + 2);
    uFtype = (pPack[9] >> 4) & 0x0Fu;

    if (uFtype == V7_DOORBELL_FTYPE)
    {
        unsigned int uDoorbellInfo = ExtractDoorbellInfo(pPack, nPackLen);
        unsigned int uDoorbellMarker = ReadBe16(pPack + 12);
        unsigned int uBankIndex = DoorbellInfoToBank(uDoorbellInfo);

        g_uDoorbellCount++;
        if (g_bPrintMsg && (g_uDoorbellCount <= 8 || (g_uDoorbellCount % 100) == 0))
        {
            os_printf("V7 doorbell: src=0x%X, dst=0x%X, len=%u, info=0x%X, bank=%u, count=%u\n",
                uSrcId, uDstId, nPackLen, uDoorbellInfo, uBankIndex, g_uDoorbellCount);
            os_printf("  raw16:");
            for (unsigned int i = 0; i < nPackLen && i < PACKET_HEADER_SIZE; i++)
            {
                os_printf(" %02X", pPack[i]);
            }
            os_printf("\n");
        }

        if (uDoorbellMarker == 0xFFFFu)
        {
            if (g_bPrintMsg && (g_uDoorbellCount <= 8 || (g_uDoorbellCount % 100) == 0))
            {
                os_printf("V7 start/status doorbell: marker=0x%04X, info=0x%X, bank=%u\n",
                    uDoorbellMarker, uDoorbellInfo, uBankIndex);
            }
            FinalizeFrameOnDoorbell(uBankIndex, uDoorbellInfo, 0);
        }
        else
        {
            FinalizeFrameOnDoorbell(uBankIndex, uDoorbellInfo, 0);
        }
        return;
    }

    if (uFtype != V7_SWRITE_FTYPE)
    {
        RecordNonSwritePacket(pPack, nPackLen, uFtype, uSrcId, uDstId);
        return;
    }

    uPayloadLen = nPackLen - PACKET_HEADER_SIZE;
    if (uPayloadLen != V7_SWRITE_PAYLOAD)
    {
        os_printf("错误: V7 SWRITE payload长度异常: %u, 期望=%u, src=0x%X, dst=0x%X\n",
            uPayloadLen, V7_SWRITE_PAYLOAD, uSrcId, uDstId);
        g_uErrorPackets++;
        return;
    }

    uSwriteAddr = ReadBe32(pPack + 12);
    pPayload = pPack + PACKET_HEADER_SIZE;

    if (uSwriteAddr >= V7_BANK0_BASE &&
        uSwriteAddr < V7_BANK0_BASE + V7_FRAME_BYTES)
    {
        nBankIndex = 0;
        uBankBase = V7_BANK0_BASE;
    }
    else if (uSwriteAddr >= V7_BANK1_BASE &&
        uSwriteAddr < V7_BANK1_BASE + V7_FRAME_BYTES)
    {
        nBankIndex = 1;
        uBankBase = V7_BANK1_BASE;
    }
    else
    {
        os_printf("错误: V7 SWRITE地址越界: addr=0x%08X, src=0x%X, dst=0x%X\n",
            uSwriteAddr, uSrcId, uDstId);
        g_uErrorPackets++;
        return;
    }

    uOffset = uSwriteAddr - uBankBase;
    if ((uOffset % V7_SWRITE_PAYLOAD) != 0)
    {
        os_printf("错误: V7 SWRITE地址未按256字节对齐: addr=0x%08X\n", uSwriteAddr);
        g_uErrorPackets++;
        return;
    }

    uRow = uOffset / V7_ROW_BYTES;
    uBlock = (uOffset % V7_ROW_BYTES) / V7_SWRITE_PAYLOAD;
    if (uRow >= V7_TOTAL_ROWS || uBlock >= V7_PACKETS_PER_ROW)
    {
        os_printf("错误: V7 SWRITE行列越界: addr=0x%08X, row=%u, block=%u\n",
            uSwriteAddr, uRow, uBlock);
        g_uErrorPackets++;
        return;
    }
    uPacketIndex = uRow * V7_PACKETS_PER_ROW + uBlock;

    pFrame = &g_stFrameBuf[nBankIndex];

    if (uRow == 0 && uBlock == 0)
    {
        unsigned int uFrameCounter = ExtractFrameCounter(pPayload);
        if (pFrame->uPacketCount > 0 &&
            pFrame->pPacketBitmap[uPacketIndex] == 0)
        {
            os_printf("警告: bank=%d 收到新帧起始包，上一帧未完成，丢弃 packets=%u\n",
                nBankIndex, pFrame->uPacketCount);
            ResetFrameBufferState(pFrame);
        }

        if (pFrame->pPacketBitmap != NULL &&
            pFrame->pPacketBitmap[uPacketIndex] != 0 &&
            pFrame->bHasFrameCounter &&
            pFrame->uFrameCounter != uFrameCounter)
        {
            os_printf("警告: bank=%d 新帧开始但上一帧未完成，丢弃未完成帧 old=%u new=%u packets=%u\n",
                nBankIndex, pFrame->uFrameCounter, uFrameCounter, pFrame->uPacketCount);
            ResetFrameBufferState(pFrame);
        }

        if (pFrame->uPacketCount > 0 &&
            pFrame->bHasFrameCounter &&
            pFrame->uFrameCounter != uFrameCounter &&
            pFrame->pPacketBitmap[uPacketIndex] == 0)
        {
            os_printf("警告: bank=%d 收到新帧参数包，上一帧未完成，丢弃 old=%u new=%u packets=%u\n",
                nBankIndex, pFrame->uFrameCounter, uFrameCounter, pFrame->uPacketCount);
            ResetFrameBufferState(pFrame);
        }

        pFrame->uFrameCounter = uFrameCounter;
        pFrame->bHasFrameCounter = 1;
    }

    if (pFrame->pPacketBitmap[uPacketIndex] != 0)
    {
        pFrame->uDuplicateCount++;
        return;
    }

    pFrame->pPacketBitmap[uPacketIndex] = 1;
    pFrame->uPacketCount++;
    if (uRow < pFrame->uMinRow)
    {
        pFrame->uMinRow = uRow;
    }
    if (uRow > pFrame->uMaxRow)
    {
        pFrame->uMaxRow = uRow;
    }
    if (uSwriteAddr < pFrame->uMinAddr)
    {
        pFrame->uMinAddr = uSwriteAddr;
    }
    if (uSwriteAddr > pFrame->uMaxAddr)
    {
        pFrame->uMaxAddr = uSwriteAddr;
    }
    pFrame->uLastRow = uRow;
    pFrame->uLastBlock = uBlock;
    pFrame->uLastAddr = uSwriteAddr;

    if (uRow > 0)
    {
        unsigned int uImageOffset = (uRow - 1) * V7_ROW_BYTES +
                                    uBlock * V7_SWRITE_PAYLOAD;
        memcpy(pFrame->pImageRaw16 + uImageOffset,
            (const void*)pPayload,
            V7_SWRITE_PAYLOAD);
        pFrame->uImagePacketCount++;
    }

    g_uCorrectPackets++;
    if (g_nSwriteTraceEnable && g_bPrintMsg &&
        (g_uCorrectPackets <= 8 || (g_uCorrectPackets % 4096) == 0))
    {
        os_printf("V7 SWRITE accepted: bank=%d, addr=0x%08X, row=%u, block=%u, packets=%u\n",
            nBankIndex, uSwriteAddr, uRow, uBlock, pFrame->uPacketCount);
    }

    return;
#elif 0
    const volatile unsigned char* pPayload = NULL;
    unsigned int uPayloadLen = 0;
    static int nDirInitialized = 0;
    static unsigned long long ullLastFrameCount = 0;  // 上一次的帧计数
    static int bFirstFrame = 1;  // 是否是第一帧

                                /* 防止未使用变量警告 */
    (void)pChHandle;
    (void)pUserParam;

    /* SRIO 接收计数：简单统计，只要长度足够就认为是一个SRIO包 */
    g_uSrioRecvCount++;

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
    if (nPackLen < (PACKET_HEADER_SIZE + 256))  // 报文头16字节 + 有效数据256字节
    {
        os_printf("错误: 报文长度 %u 不足，期望至少 %d 字节\n",
            nPackLen, PACKET_HEADER_SIZE + 256);
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
#elif 0




    const volatile unsigned char* pPayload = NULL;
    unsigned int uPayloadLen = 0;
    static int nDirInitialized = 0;

    /* 防止未使用变量警告 */
    (void)pChHandle;
    (void)pUserParam;

    /* 从配置文件读取保存路径（只读一次） */
    if (nDirInitialized == 0)
    {
        ReadConfigString(CONFIG_FILE_NAME, "YUV_SAVE_PATH",
            DEFAULT_SAVE_PATH, g_acSavePath, sizeof(g_acSavePath));

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

        //if (CreateSaveDirectory(g_acSavePath) == 0)
        //{
        //	nDirInitialized = 1;
        //	os_printf("YUV保存路径: %s\n", g_acSavePath);
        //}
        //else
        //{
        //	os_printf("错误: 目录初始化失败\n");
        //	return;
        //}
    }

    /* 打印基本信息 */
    os_printf("============================================================\n");
    os_printf("ChannelRecvDataCallBack 被调用:\n");
    os_printf("  - dmaChan: %d\n", dmaChan);
    os_printf("  - nPackLen: %u bytes\n", nPackLen);
    os_printf("  - nDropPack: %d\n", nDropPack);

    /* 检查数据长度是否足够 */
    if (nPackLen <= PACKET_HEADER_SIZE)
    {
        os_printf("警告: 报文长度 %u 不足，小于等于报文头大小(%d字节)，丢弃\n",
            nPackLen, PACKET_HEADER_SIZE);
        os_printf("============================================================\n");
        return;
    }

    /* 跳过报文头（16字节） */
    pPayload = pPack + PACKET_HEADER_SIZE;
    uPayloadLen = nPackLen - PACKET_HEADER_SIZE;

    os_printf("  - 有效数据起始地址: %p\n", pPayload);
    os_printf("  - 有效数据长度: %u bytes\n", uPayloadLen);

    /* 打印报文头信息（用于调试） */
    if (nPackLen >= PACKET_HEADER_SIZE)
    {
        unsigned int i = 0;
        os_printf("  - 报文内容:\n");
        for (i = 0; i < nPackLen; i++)
        {
            if ((i % 16) == 0)
            {
                if (i != 0)
                {
                    os_printf("\n");
                }
            }
            os_printf("%02X ", pPack[i]);
            if ((i + 1) % 8 == 0)
            {
                os_printf(" ");
            }
        }
        os_printf("\n");
    }


    ///* 将每个报文独立保存为一个YUV文件 */
    //if (SavePacketToYuvFile(dmaChan, pPayload, uPayloadLen) != 0)
    //{
    //	os_printf("错误: 保存YUV文件失败\n");
    //}

    os_printf("============================================================\n");
#endif
}

/*************************************************
Function:       main
Description:    主函数，负责初始化设备、发送数据、接收数据
支持从配置文件读取上行和下行通道号
增加SRIO SWRITE发送功能
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
    int c = 0;

    printf("into main \n");

    /* 从配置文件读取通道号 */
    nUpChannel = ReadSimpleConfig(CONFIG_FILE_NAME, "UP_LOOP_DMA_CHANNUM", UP_LOOP_DMA_CHANNUM);
    nDownChannel = ReadSimpleConfig(CONFIG_FILE_NAME, "DOWN_LOOP_DMA_CHANNUM", DOWN_LOOP_DMA_CHANNUM);

    printf("从配置文件读取通道号: 上行=%d, 下行=%d\n", nUpChannel, nDownChannel);

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
    g_hV7DevHandle = dev;

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

    /* 注册回调函数后再启动协议栈，避免启动瞬间漏包 */
    cvg_pro_register_cb(ch, NULL, ChannelRecvDataCallBack);

    /* RX mode 0: read immediately; low thresholds reduce burst overrun risk. */
    cvg_pro_rx_drd_mode_set(ch, 0);
    cvg_pro_hprx_threshold_set(ch, 1);
    cvg_pro_lprx_threshold_set(ch, 1);
    cvg_pro_hprx_tov_set(ch, 1);
    cvg_pro_lprx_tov_set(ch, 1);

    cvg_pro_start(ch, 0);

    if (InitV7Receiver() != 0)
    {
        os_printf("ERROR: V7 receiver initialization failed\n");
        cvg_pro_stop(ch);
        cvg_close_chan(ch);
        cvg_card_close(dev);
        return -1;
    }

    cvg_card_readreg(dev, 0x1008, &v);
    os_printf("%s %d %x\n", __func__, __LINE__, v);

    /* 初始化发送数据 */
    for (i = 0; i < 1024; i++)
    {
        pMsg[i] = (unsigned char)i;
    }

    /* 发送数据与菜单循环 */
    i = 0;
    printf("\n========== 操作说明 ==========\n");
    printf("按 's' 发送一次 SRIO SWRITE (dest=0x80, src=0x71, 446字节)\n");
    printf("按 'q' 退出程序\n");
    printf("按 Enter 或其他键仅刷新统计，不发送测试数据\n");
    printf("===============================\n");
    printf("Current mode: listen-only. Only 's' sends an explicit SWRITE test; other keys do not send data.\n");

    while (1)
    {
        os_printf("请输入选择 (s=SWRITE测试, q=退出, Enter/其他=仅刷新统计): ");
        c = getc(stdin);

        if (c == 'q')
        {
            os_printf("quit \n");
            break;
        }
        else if (c == 's' || c == 'S')
        {
            /* 发送一次 SRIO SWRITE */
            SendOneSrioSwrite(ch);
        }
        else
        {
            if (c == EOF)
            {
                os_sleepms(1000);
                continue;
            }
            if (c != '\n' && c != '\r')
            {
                os_printf("input ignored; receiver stays in listen-only mode\n");
            }
            if (0)
            {
            /* 发送原有的测试数据 */
            os_printf("start send test msg %d\n", i++);

            v = 0;
            while (v < (unsigned int)nSendLen)
            {
                if ((v % 16) == 0)
                {
                    if (v != 0)
                        os_printf("\n");
                    os_printf("%08X: ", v);
                }
                os_printf("%02X ", pMsg[v]);
                if ((v % 16) == 7)
                    os_printf(" ");
                v++;
            }
            os_printf("\n");

            r = cvg_pro_write_data(ch, pMsg, nSendLen, 0);
            os_printf("send %d buff \n", r);
            }
        }

        /* 保留原有的寄存器读取 */
        cvg_card_readreg(dev, 0x2004, &v);
        os_printf("%s %d 0x2004: %x\n", __func__, __LINE__, v);
        cvg_card_readreg(dev, 0x2008, &v);
        os_printf("%s %d 0x2008: %x\n", __func__, __LINE__, v);
        cvg_card_readreg(dev, 0x200C, &v);
        os_printf("%s %d 0x200C: %x\n", __func__, __LINE__, v);
        cvg_card_readreg(dev, 0x2010, &v);
        os_printf("%s %d 0x2010: %x\n", __func__, __LINE__, v);
        cvg_card_readreg(dev, 0x1024, &v);
        os_printf("%s %d 0x1024: %x\n", __func__, __LINE__, v);
        cvg_card_readreg(dev, 0x1028, &v);
        os_printf("%s %d 0x1028: %x\n", __func__, __LINE__, v);

        /* 打印所有统计信息 */
        os_printf("===== 统计信息 =====\n");
        os_printf("V7接收包: 已接收=%u, 正确=%u, 错误=%u, 计数=%u\n",
            g_uSrioRecvCount,
            g_uCorrectPackets, g_uErrorPackets, g_uCorrectPackets + g_uErrorPackets);
        os_printf("SRIO 发送次数: %u (当前填充值=0x%02X)\n", g_uSrioSendCount, g_ucSrioFillValue);
        os_printf("V7帧: 完成=%u, 队列=%u, SCP成功=%u, SCP失败=%u\n",
            g_uFrameCompleteCount, g_uFrameQueuedCount, g_uFrameSendOkCount, g_uFrameSendFailCount);
        os_printf("V7 doorbell: recv=%u, queued=%u, incomplete=%u\n",
            g_uDoorbellCount, g_uDoorbellFrameCount, g_uIncompleteFrameCount);
        os_printf("V7 frame output: queued=%u, partial=%u, jobs=%u\n",
            g_uFrameCompleteCount,
            g_uPartialFrameQueuedCount,
            g_uFrameQueuedCount);
        os_printf("V7 non-SWRITE: recv=%u, raw_saved=%u\n",
            g_uNonSwriteCount, g_uRawCaptureCount);
        os_printf("V7 stream: encoded=%u, fail=%u, jpeg_fail=%u, q=%u, mem_q=%u, spool_q=%u, spool_bytes=%llu, spool_write=%u, spool_read=%u\n",
            g_uFrameStreamOkCount,
            g_uFrameStreamFailCount,
            g_uJpegEncodeFailCount,
            g_uFrameOutputQueueDepth,
            g_uFrameMemoryQueueDepth,
            g_uFrameSpoolQueueDepth,
            g_ullStreamSpoolBytes,
            g_uFrameSpoolWriteCount,
            g_uFrameSpoolReadCount);
        os_printf("V7 MJPEG: clients=%u, connect=%u, drop=%u, repeat=%u, send_fail=%u, last_jpeg=%lu, seq=%u\n",
            g_uMjpegClientActive,
            g_uMjpegClientConnectCount,
            g_uMjpegClientDropCount,
            g_uMjpegRepeatCount,
            g_uMjpegSendFailCount,
            g_ulLastJpegFrameBytes,
            g_uLastJpegSeq);
        os_printf("====================\n");

        os_sleepms(100);
    }

    cvg_pro_stop(ch);
    StopFrameOutputWorker();
    FreeFrameBuffers();
    ReleaseV7BarMapped();
    cvg_close_chan(ch);
    cvg_card_close(dev);
    g_hV7DevHandle = NULL;

    os_sleepms(100);
    os_printf("exit, 完成V7帧=%u, SCP成功=%u, SCP失败=%u, 旧YUV保存计数=%u\n",
        g_uFrameCompleteCount, g_uFrameSendOkCount, g_uFrameSendFailCount, g_uFileCounter);

    return 0;
}
