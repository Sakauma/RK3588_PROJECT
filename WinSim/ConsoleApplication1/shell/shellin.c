#include <stdio.h>
#include "shellin.h"
#include "shell.h"
static Shell shell;
char shellBuffer[512];

#ifdef WIN32

static void shellout(char c)
{
	fputc(c, stdout);
}

static  char shellin(char* pData)
{
	int c;
	//c = fscanf(stdin,"%s", pData );
	char * p = fgets(pData, 10, stdin);
	return strlen(p);
}

#else

static void shellout(char c)
{
	fputc(c,stdout);
}

static  char shellin(char * pData)
{
	int c;
	c = getc(stdin);
	*pData = (char)c;
	//printf("INPUT %x\n",c);
	return 0;
}
#endif
void usershellinit()
{
	int Status;
	
	shell.write = 	shellout;
	shell.read = 	shellin;
	shellInit(&shell, shellBuffer, 512);
}


void usershellsche()
{
	shellTask(&shell);
}
