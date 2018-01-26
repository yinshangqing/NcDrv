/*************************************************************************
	> File Name: Nc_test.c
	> Author: yinshangqing
	> Mail: 841668821@qq.com 
	> Created Time: 2017年05月05日 星期五 15时23分09秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define CLR8259INT		0x20
#define CLR8259DATA		0x20
#define MASK8259		0x21
#define INT_NUMBER		0X20

unsigned char new_mask;
int fd;
int num = 0;

void signal_fun(int signum);

int main(int argc,char **argv)
{
	/* 开始测试 */
	printf("开始测试...\n");
	fd = open("/dev/NcDrv_dev",O_RDWR);
	if(fd < 0){
		printf("can't open!\n");
		exit(1);
	}
	int Oflags;
	/* 捕捉SIGIO信号(有驱动发送) */
	signal(SIGIO,signal_fun);
	fcntl(fd,F_SETOWN,getpid());
	Oflags = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,Oflags | FASYNC);
	while(1){
		sleep(10);
	}

	return 0;	
}

void signal_fun(int signum)
{
	unsigned char old_mask = inb(MASK8259);
	new_mask = old_mask & 0xf7;
	outb(CLR8259DATA,CLR8259INT);
	printf("接受到第%d次中断!!!\n",++ num);
	outb(new_mask,MASK8259);
}


