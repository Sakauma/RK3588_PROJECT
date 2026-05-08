#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../GLKAPI/platform.h"
#ifndef WIN32
	#include <getopt.h>
	#include <sys/time.h>
#else
	#include "getopt.h"
#endif
#include "shellcmd.h"

/*
 该文件是用来在控制台中调试使用的，用于统计、打印控制之类的，不作为发布使用
*/
/*
 默认是禁用统计
*/
int 	statis_en = 0;
#define MAX_ST	(3)
struct IMG_STATIS_INFO	statis_info[MAX_ST] = {0};

/*
 ASM收发控制使能
*/
struct ASM_CTRL_INFO	asm_ctrl = {
	.asm_statis_rx_en = 0,
	.asm_tx_en = 1,
	.asm_tx_sleepms =0,
	.asm_recv_bytecnt = 0,
	.asm_msg_index = 0,
	.fill_type = 0
};

static int exitApp = 0;
void sysexit()
{
	exitApp = 1;
}


int sysexitflag()
{
	return exitApp;
}
/*
 切图保存处理吞吐量能力统计
*/
struct IMG_STATIS_INFO	img_statisifno[MAX_ST] = { 0 };

/*
 切图保存控制使能
*/
struct IMG_SAVE_ST_INFO	issi_saveinfo[MAX_ST] = {1};

/*
 获取ASM接收使能的状态。在parseasm.c中调用
*/
void	img_update_rx_info(int ch,int bytes)
{
#if 1//ndef WIN32
	img_statisifno[ch].byteCnt += bytes;
	struct timeval _tm_ = { 0 };
	gettimeofday(&_tm_, NULL);
	img_statisifno[ch].espTime = img_statisifno[ch].espTimeEnd- img_statisifno[ch].espTimeStart;
	img_statisifno[ch].espTimeEnd = _tm_.tv_sec * 1000 + _tm_.tv_usec / 1000;
	if (img_statisifno[ch].espTime == 0) {
		img_statisifno[ch].espTimeStart = img_statisifno[ch].espTimeEnd;
		return;
	}
	else if(img_statisifno[ch].espTimeEnd - img_statisifno[ch].espTimeStart >= 1000){
		img_statisifno[ch].espTimeStart = img_statisifno[ch].espTimeEnd;
		if (img_statisifno[ch].statis_printen) {
			printf("%d Bytes/second = %d\r\n", ch, img_statisifno[ch].byteCnt);
		}
	}
#endif
}


int 	asm_rx_debug()
{
	return asm_ctrl.asm_statis_rx_en;
}

int 	asm_rx_bytes()
{
	return asm_ctrl.asm_recv_bytecnt;
}

int 	asm_tx_en()
{
	return asm_ctrl.asm_tx_en;
}

void 	asm_update_rx_info(int bytes)
{
	asm_ctrl.asm_recv_bytecnt += bytes;
}


int 	asm_sleep()
{
	os_sleepms(asm_ctrl.asm_tx_sleepms);
	return 0;
}

int		asm_trig_msgindex()
{
	return asm_ctrl.asm_msg_index;
}

int asm_fill_type()
{
	return asm_ctrl.fill_type;
}


int 	img_save_en(int ch)
{
	if(ch < MAX_ST){
		return issi_saveinfo[ch].save_en;
	}
	return 0;
}


void 	img_cmd_input(int argc,char * argv[])
{
#if 1//ndef WIN32
	if(argc == 1){
		optind = 0;
		printf("please input parameters like :imgsavecfg -C 0/1 -E 0/1 -P 0/1\n");
		printf("-C channel select\n");
		printf("-S save control\n");
		printf("-P print statis en control\n");
		return ;
	}
	int opt, ch = 0, en = 0, pr = 0;;
	int		en_flag = 0, pr_flag = 0;
	while((opt = getopt(argc,argv,"C:E:P:")) != -1){
		switch(opt){
			case 'E':
				en_flag = 1;
				en = atoi(optarg);
			break;
			case 'C':
				ch = atoi(optarg);
			break;
			case 'P':
				pr_flag = 1;
				pr = atoi(optarg);
				break;
		}
	}
	if(ch< MAX_ST){
		if(en_flag)issi_saveinfo[ch].save_en = en;
		if(pr_flag)img_statisifno[ch].statis_printen = en;
	}
	optind = 0;
#endif
}

void 	asm_cmd_input(int argc,char * argv[])
{
#if 1//ndef WIN32
	if(argc == 1){
		optind = 0;
		printf("please input parameters like :asmcmd -R 0/1 -T 0/1 -S msecond -M msgindex -D -1/d\n");
		printf("-R 0/1 control print ASM msg first 10 DWS when recv asm msg\n");
		printf("-T 0/1 control auto send ASM msg \n");
		printf("-S control auto send ASM msg speed ,Sleep msecond\n");
		printf("-M witch asm msg be send out\n");
		printf("-D SendData Type -1:rand data,or filled will [d]\n");
		return ;
	}
	int opt;
	while((opt = getopt(argc,argv,"R:T:S:M:D:")) != -1){
		switch(opt){
			case 'R':
				asm_ctrl.asm_statis_rx_en = atoi(optarg);
			break;
			case 'T':
				asm_ctrl.asm_tx_en = atoi(optarg);
			break;
			case 'S':
				asm_ctrl.asm_tx_sleepms = atoi(optarg);
			break;
			case 'M':
				asm_ctrl.asm_msg_index = atoi(optarg);
			break;
			case 'D':
				asm_ctrl.fill_type = atoi(optarg);
			break;
			default:
			break;
		}
	}
	optind = 0;
	
#else
	printf("not support on windows \n");
#endif
}

void 	enable_statis(int argc,char * argv[])
{
#if 1//ndef WIN32
	if(argc == 1){
		optind = 0;
		printf("please input parameters like :sten -E 0/1\n");
		return ;
	}
	int opt;
	while((opt = getopt(argc,argv,"E:")) != -1){
		switch(opt){
			case 'E':
				statis_en = atoi(optarg);
			break;
			default:
			break;
		}
	}
	optind = 0;
	if(statis_en){
		memset(statis_info,0,sizeof(statis_info));
	}
	printf(" statis %s \n",statis_en == 0 ? "disabled" :"enabled");
#else
	printf("not support on windows \n");
#endif
}



void 	asm_statis_save(int imgNum,int bytes)
{
#if 1 //ndef WIN32
	if(imgNum >= MAX_ST)return ;
	statis_info[imgNum].byteCnt += bytes;
	statis_info[imgNum].packCnt ++;
	struct timeval _tm_;
	gettimeofday(&_tm_,NULL);
	
	if(statis_info[imgNum].packCnt == 1){
		statis_info[imgNum].espTimeStart = _tm_.tv_sec*1000 + _tm_.tv_usec / 1000;
		statis_info[imgNum].espTime = 0;
	}else{
		
		statis_info[imgNum].espTimeEnd = _tm_.tv_sec*1000 + _tm_.tv_usec / 1000;
		int diff = (statis_info[imgNum].espTimeEnd - statis_info[imgNum].espTimeStart);
		if(diff >=1000){
			statis_info[imgNum].espTime += diff;
			if(statis_en){
				printf("%d Bytes/second = %d\n",imgNum,statis_info[imgNum].byteCnt);
			}
			statis_info[imgNum].espTimeStart = statis_info[imgNum].espTimeEnd;
		}
	}
#endif	
}
