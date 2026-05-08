#ifndef SCHE_TB_H
#define SCHE_TB_H

/*
 传入一个消息结构体，配置ICI参数
*/
#define CVG_SCHETB_SET_ICI(msg,ici)	{\
	struct SCHE_TB *st = (struct SCHE_TB*)(&msg);\
	st->ICI = ici; \
}

/*
 传入一个消息结构体，配置IPI参数
*/
#define CVG_SCHETB_SET_IPI(msg,ipi)	{\
	struct SCHE_TB *st = (struct SCHE_TB*)(&msg);\
	st->IPI = ipi; \
}

/*
 传入一个消息结构体，配置发送偏移参数
*/
#define CVG_SCHETB_SET_OFFSET(msg,mode,tOff)	{\
	struct SCHE_TB *st = (struct SCHE_TB*)(&msg);\
	st->U_TOFF.S_OFF.offset = tOff; \
	st->U_TOFF.S_OFF.sMode =  mode; \
}

#pragma pack(4)
struct	SCHE_TB{
	
	/*Cycle Initial Value，消息执行的间隔，间隔以基本周期为单位。*/
	unsigned int ICI		:16;
	/*Phase Initial Value，第一次执行相对于总线第一个基本周期的位置。*/
	unsigned int IPI		: 16;
	/*0：非周期高优先级
	1：非周期低优先级
	2: 周期高优先级
	3: 周期低优先级
	*/
	unsigned int Type : 2;
	unsigned int reserve	:4;
	unsigned int Act : 1;
	/*
		最后一个消息时置1，对于非周期消息，可以不处理
	*/
	unsigned int End : 1;
	unsigned int	Reserved : 24;
	/*
	 调度模式，1：表示T_Offset有效，否则表示紧密发送
	*/
	union{
		unsigned int schMode	;
		struct{
			unsigned int offset:30;
			unsigned int sMode:1;	/*0表示紧密发送，1表示消息在offset的偏移上发送*/
		}S_OFF;
	}U_TOFF;
	/*
		当前调度表的序号，也可以理解为消息的序号
	*/
	unsigned 	int	schindex;
};

/*
 通用消息头结构
*/
struct MSG_HEADER{
	/*
	 * 消息占用的板载DDR空间大小，该消息的缓存空间大小，计算方式为：(2^BMS) * 4KB,范围0-14
	 * 在配置消息时，如果是Sample类型，会被强制设置成4K（自动管理时）
	*/
	int 		 BMS;
	/*
	 逻辑需要的BMS值。
	*/
	int			 LCBMS;
	/*
	 1表示Sample类型，即该消息的缓存中仅存储一条消息，可重复发送。
	 0表示Queue类型，即该消息的缓存中可存储多条消息，不能重复发送。
	*/
	int			 sap;
	/*
	该值是用来在初始化的时候计算每个消息在DDR中的存储地址。
	*/
	unsigned int msgDataPtr;
	/*
	该字段是和逻辑提供的RD_PTR以及BMS做运算比较，计算出来的消息对应的板载DDR剩余空间大小，只有当leftMemSize小于DMA包长时计算一次（4096字节。
	*/
	int leftMemSize;
	/*写位置，这个值的定义和逻辑的RT_PTR一样，即BIT23表示循环标志0/1变化，BIT22-0表示当前的写位置，单位8字节*/
	union{
		unsigned int WritePtr;
		struct {
			unsigned int WritePtr	:23;	/*写的位置*/
			unsigned int EO_Flag	:1;	/*even odd flag*/
			unsigned int reserved 	:8; /*保留*/
		}SUWP;
	}UWP;
};
#pragma pack()


#endif
