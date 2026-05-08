#ifndef _ERROR_CODE_H
#define _ERROR_CODE_H


/*
 *系统错误码
*/
#define FC_SUCCESS								0			//成功
#define FC_OPEN_DEVICE_FAIL						1			//打开设备时标
#define FC_ERR_INVALID_HANDLE					2			//无效的句柄
#define FC_ERR_INVALID_PARAM					3			//无效的参数
#define FC_ERR_MEM_MALLOC_FAIL					4			//内存分配失败
#define FC_ERR_THREAD_CREATE_FAIL				5			//任务或线程创建失败
#define FC_ERR_MEMPOOL_MALLOC_FAIL				6			//API内部内存池申请失败
#define FC_ERR_SEND_DMA_PKT_FAIL				7			//下行DMA报文发送失败
#define FC_ERR_UP_DAM_START_FAIL				8			//上行DMA接收任务创建失败
#define FC_ERR_CREATE_SEMPHORE_FAIL				9			//信号量创建失败
#define FC_ERR_START_WORK_THREAD				10			//NC、NT、NM上行接收任务或线程创建失败
#define FC_ERR_DOWN_DMA_BUF_FULL				11			//下行DMA无可用描述符
#define FC_ERR_NOT_INIT_SA						13			//SA未被初始化，FC-AE-1553有效
#define FC_ERR_SA_USED							14			//SA已被占用，FC-AE-1553有效
#define FC_ERR_COMMU_HW_TMOUT					15			//与硬件交互超时，读写寄存器失败
#define FC_ERR_BUFFER_TOO_SMALL					16			//上行接收报文Buffer太小
#define FC_ERR_DATA_LENG_OUT_SETTING			17			//下发的数据超出了设定长度
#define FC_ERR_SA_HW_STORE_FULL					19			//SA设定的硬件缓存满
#define FC_ERR_RECV_BUF_TOO_SMALL				20			//接收数据的buf太小
#define FC_ERR_SA_NOT_ENABLE_SEND				21			//SA没有开启发送使能
#define FC_WARING_SA_NO_NEW_DATA				22			//警告消息，表明sample的数据无新内容
#define FC_ERR_CREATE_NOTIFY_THREAD				23			//接收通知线程、任务创建失败
#define FC_ERR_INVALID_CHL_NUM					24			//无效的FC通道号
#define FC_ERR_OPEN_CHL_FAIL					25			//打开FC通道失败
#define FC_ERR_OPEN_UP_DMA_FAIL					26			//上行DMA开始失败
#define FC_ERR_CHL_UNOPEN						27			//FC通道未打开
#define FC_ERR_DOWNDMA_FAILED					28			//下行DMA失败，致命错误
/*
	*NC错误码
*/
#define FC_ERR_NC_IS_NOT_SUPPORT				100			//NC功能不支持
#define FC_ERR_NC_IS_NOT_INIT					101			//NC功能未初始化
#define FC_ERR_NC_IS_OPEN						102			//NC功能已被打开
#define FC_ERR_NC_STATUS_STOP					105			//NC没有启动或停止了
#define FC_ERR_NC_MSG_TYPE_NO_MATCH				107			//NC填充数据消息类型不匹配，非NC->NT消息
#define FC_ERR_NC_EXE_UNIT_INITED				108			//NC调度单元未初始化
#define FC_ERR_NC_NOT_ENOUGH_HW_RESOURCE		109			//NC IA资源超出限制

#define FC_ERR_NC_TABLE_INITED					110			//NC当前使用调度表已被初始化
#define FC_ERR_NC_TABLE_NOIT_INIT				111			//NC当前使用调度表未被初始化
#define FC_ERR_NC_TABLE_RUNNING					112			//NC当前使用调度表正在运行中
#define FC_ERR_NC_INVALID_TABLE_INDEX			113			//无效的调度表索引
#define FC_ERR_NC_INVALID_EXE_UNIT_INDEX		114			//无效的调度单元索引
#define FC_ERR_NC_INVALID_MSG_INDEX				115			//无效的消息索引
#define FC_ERR_NC_MSG_NOT_INIT					116			//NC消息未初始化
#define FC_ERR_NC_MSG_MNG_IS_NOT_INIIT			117			//NC消息管理结构体未被初始化
#define FC_ERR_NC_INVALID_TABLE_TYPE			118			//无效的table类型
#define FC_ERR_SA_iPST_UNSUPPORT_OPER			119			//SA设定的的iPST不支持该操作
#define FC_ERR_NC_EXE_UNIT_NOT_INIT				120			//NC调度单元未被初始化
#define FC_ERR_AP_MSG_INDEX_INVALID				121			//NC非周期消息索引无效
/*
	*NT错误码
*/
#define FC_ERR_NT_IS_NOT_SUPPORT				200			//NT功能不支持
#define FC_ERR_NT_IS_NOT_OPEN					201			//NT功能未被打开
#define FC_ERR_NT_INIT_FAIL						202			//NT功能初始化失败
#define FC_ERR_NT_NOT_INIT						203			//NT功能未被初始化
#define FC_ERR_SA_IS_NOT_INIT					205			//NT下SA未被初始化
#define FC_ERR_SA_HAS_INIT						206			//NT下SA已被初始化
#define FC_ERR_SA_INIT_FAIL						207			//NT下SA初始化失败
#define FC_ERR_START_NOTIFY_FAIL				208			//NT上行通知任务启动失败
#define FC_ERR_SA_NO_RECV_DATA					209			//NT下SA未接收到数据
#define FC_ERR_RECV_BUFF_TOO_SMALL				210			//NT上行接收Buff大小小于实际接收数据施工难度
#define FC_ERR_NT_IS_INITED						211			//NT功能已被初始化
#define FC_ERR_INIT_ELS							212			//初始化ELS出现问题
#define FC_ERR_NT_IS_OPEN						213			//NT功能已被打开


/*
	*ELS错误码
*/
#define FC_ERR_BB_CREDIT_VALUE_OVER_RANGE		400			//BB信用值超出允许设置最大值（8）
#define FC_ERR_ESL_MANAGE_UNINIT				401			//ELS初始化失败
#define FC_ERR_ELS_ACC_TIMEOUT					402			//ELS ACC应答超时

#define OSHANDLE_ERROR_TYPE						403			//设备句柄和通道句柄使用混用错误
#define FC_ASM_MSGID_NOT_EXSIT					404			//ASM消息索引号错误，即超出初始化时定义的消息数量

/* RTC同步角色定义*/
#define SYNC_MODE_NO							0			/* 无角色，禁用该功能 */
#define SYNC_MODE_SERVER						1			/* 服务器- 发送广播消息 */
#define SYNC_MODE_CLIENT						2			/* 客户端- 接收广播消息 */

#endif
