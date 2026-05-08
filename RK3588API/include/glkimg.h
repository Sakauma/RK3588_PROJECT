#ifndef GLKIMG_H
#define GLKIMG_H



#pragma pack(4)
struct CVG_GLKIMG_DMAHD{
#if 0
	unsigned short usImgCols; 		//图像列数
	unsigned short usImgRows; 		//图像行数
	unsigned short usPayloadLen; 	//负载长度
	unsigned char  ucStEnd;     	//包数据起始标志位
	unsigned char  ucPktType;   	//包类型

	int nPktCount;            		//包计数
	int nFrameCount;          		//图像帧计数
	unsigned short					spImgCol;	//字节1-0，切图列号
	unsigned short					spImgRow;	//字节3-2，切图行号
	unsigned int					orgImgSeqNum;//原始图像序列号
	int nResv2[2];            		//保留
#else
	
	
	unsigned char  ucPktType;   	//包类型
	unsigned char  ucStEnd;     	//包数据起始标志位
	unsigned short usPayloadLen; 	//负载长度
	unsigned short usImgRows; 		//图像行数
	unsigned short usImgCols; 		//图像列数

	int nFrameCount;          		//图像帧计数
	int nPktCount;            		//包计数
	unsigned int					orgImgSeqNum;//原始图像序列号
	unsigned short					spImgRow;	//字节3-2，切图行号
	unsigned short					spImgCol;	//字节1-0，切图列号
	int nResv2[2];            		//保留

#endif
};

/*
 * 切分图像时需要使用的参数内容
*/
struct CVG_IMG_SPLIT {
	int orgImgW;	//原始图像的宽，单位字节；
	int orgImgH;	 //原始图像的高，单位字节
	int spImgW;		 //切分后小图像的宽，单位字节
	int spImgH;		 //切分后小图像的高，单位字节
	int overlapX;	 //切分后小图像的X方向期望的率，百分比值，例如10表示10%
	int overlapY;	 //切分后小图像的Y方向期望的率，百分比值，例如10表示10%
	int matrix_m;	 //返回切分后的列数
	int matrix_n;	 //返回切分后的行数
	int board_a;	 //返回切分后左右两侧边框填充像素个数
	int board_b;	 //返回切分后上下两侧边框填充像素个数
	int overlap_XB;	 //返回图像左右两侧的填充像素个数
	int overlap_YB;  //返回图像上下两侧的填充像素个数
};
#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 函数说明：用于输入原始图像的宽、高和待切分的图像宽、高以及重叠率部分参数后，验证参数是否合理
 * 返回值：
 * 	0:合理
 * 	-1：不合理
*/
int 	cvg_calc_split_imginfo_verify(struct CVG_IMG_SPLIT * cis);


/*
*	 函数说明：为原始图像创建切割图像缓冲区，可以调用该函数多次，实现缓冲多个切割图像的缓冲区。
*	参数说明：
*		@1: 返回图像缓冲区句柄；
*		@2：图像验证后的参数信息；
*		@3：图像传输的通道DMA号
*		@4：待缓存的切割图像后的行号；
*		@5：待缓存的切割图像后的列号；
*		@6：图像的每个像素占用的字节数，根据实际情况填写。
*	返回值：
*		-1：失败；
*		0：创建的图像缓冲区句柄；
*/
int		cvg_create_split_imagehandle(void ** cacheHandle,struct CVG_IMG_SPLIT* cis, int imgID, int spImg_r, int spImage_c, int bpp);

/*
 * 函数说明：删除切图的缓存信息
 *	参数：
 *		@1：图像缓存缓冲区句柄
 *		@2：切图行号
 *		@3：切图列号
 * 返回值：无
*/
void cvg_delete_item(void* cacheHandle, int spImg_r, int spImg_c);

/*
 *	 函数说明：当从DMA返回数据后，使用此函数把逻辑切割后的小图像数据由API进行组成一副完整的切割后的图像。
 *	参数：
 *		@1：创建的图像缓冲区指针;
 *		@2: DMA通道ID值；
 *		@3: 上行的DMA头结构
 *		@4：切割后的图像数据指针；
 *		@5：图像大小；
 *		@6: 是否是大图的最后一个DMA帧。如果是最后一针，根据切图协议，需要返回给5型机一个状态，以便可以继续发送下一个大图
 *	返回值：
 *		-1：参数1句柄错误；
 *		-2：参数2DMA通道号和创建时设定的通道号不一致，也就是说传入的@1句柄中的混淆；
 *		-3：传入的行、列号错误，即未通过cvg_create_split_imagehandle函数创建；
 *		-4：上一个切图未满，新的大图序号发生变化错误
 *		-5：功能性错误，在DMA头中描述的切图负载长度比DMA上行数据包还长；
 *		-6：在写入第一行小图的数据时，上行DMA包的描述不是第一包，这种情况及有可能是因为上位机处理速度不够，没有及时取走数据，DMA中的第一包没有写入到小图缓冲区中
 *		0：缓冲区满了，即表示一帧小图像接收完毕，可以处理图像；
 *	正数：写入缓存的大小
 */
int 	cvg_cache_data(void* cacheHandle, int chanid, void * imgDmaHead, void* spImageBuf, int bufSize
	,int* lastPack
);


/*
 *	  函数说明：当缓存数据后的返回值为0时，表示数据缓存正好达到一帧切割后的图像大小。使用此函数获取小图像的首指针。
 *	参数说明：
 *		@1：创建的图像缓冲区指针；
 *		@2：DMA头结构；
 *		@3：返回原始图像的序列号；
 *		@4：返回切割后的图像行号；
 *		@5：返回切割后的图像列号；
 *		@6：返回给应用当前切割图像的实际缓存数据量大小；
 *		@7：是否复位，在缓冲区满了后，再写入数据将会被抛弃。通过该变量控制返回给应用后，数据可以再次写入。
 *	返回值：
 *		NULL：错误；
 *		非空：切割图像缓冲区的首地址
*/
void*	cvg_cache_get_bufptr(void* cacheHandle, void * dmaHead,int * orgImgSeq,int *spImgr, int *spImgc,  int* AckBufSize, int reset);

/*
 * 函数说明
 *		仅仅复位切图写入指针的动作，在调用cvg_cache_get_bufptr时，如果reset参数位0，函数返回时已经把指针清零了，外部如果调用拷贝数据不及时，再次写入的动作就可能会覆盖原始数据，
 *		虽然概率很小，但不是没有可能。因此可以调用cvg_cache_get_bufptr时，reset设置成0，然后单独调用这个函数清指针位置
 * 参数说明：
 *		@1：创建的图像缓冲区指针；
 *		@2：DMA头结构；
 * 返回值：无
*/
void	cvg_cache_reset_bufptr(void* cacheHandle, void* dmaHead);

/*
	*  函数说明：在应用退出时，需调用此函数，释放内存
	*  参数说明：
	*  	@1：创建的图像缓冲区指针；
	*  返回值：无
	*
 */
void	cvg_cache_free_handle(void* cacheHandle);



/*
*	函数说明：在使用切图参数验证算法校验合法后，通过该函数把切图参数配置给逻辑，由逻辑实现图像切割算法处理。
*	参数：
*	@1：DMA通道句柄；
*	@2：创建的图像缓冲区指针；
*	返回值：
*	0：OK；
*	负数：非法；
*/
int cvg_proimg_hwcfg(OS_HANDLE pChHandle, void* cacheHandle,int asmMsgIndex);

/*
 * 函数说明：
 *	在图像切割系统中，上传图像时，使用了独立的DMA通道，但和物理节点的通道不一样。
 * 在通过图像的DMA通道发送数据时，需要使用物理通道，使用这个函数转换
 * 参数：
 *		@1：上行图像通道句柄
 * 返回值：
 *		NULL：child非法；
 *		非空：返回物理通道
*/
OS_HANDLE cvg_parent_dc(OS_HANDLE child);
#ifdef __cplusplus
}
#endif

#endif
