#ifndef D_CACHE_H
#define D_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "./rbtree.h"
/*
 * 函数说明
 * 向二叉树中添加一个消息节点，并为该消息节点，设置行列大小
 *
*/
void 	dCache_addcache(struct rb_root *root,int msgindex,int MatrixR,int MatrixC,int W,int H,int Byte_PerData);
/*
 * 函数说明
*/
void	dCache_realseAll(struct rb_root *root);

/*
 函数说明：
*/
int 	dCache_writeData(struct rb_root *root,int msgindex, int matrixid, unsigned char* pMem, int memLen);

/*
 函数说明
*/
int 	dCache_CheckFull(struct rb_root *root,int msgindex,int matrixid);

/*
 清除小矩阵的满标志
*/
int 	dCache_Clear(struct rb_root * root,int msgindex,int matrixid);

int 	dCache_CopyData(struct rb_root * root,int msgindex,int matrixid,unsigned char * pDst,int Size);
#ifdef __cplusplus
}
#endif

#endif