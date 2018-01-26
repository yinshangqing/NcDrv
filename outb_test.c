/*************************************************************************
	> File Name: outb_test.c
	> Author: yinshangqing
	> Mail: 841668821@qq.com 
	> Created Time: 2017年05月06日 星期六 14时43分25秒
 ************************************************************************/

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main()
{
	printf("************************************\n");	
	outb(0x20,0x20);

	printf("***********outb success*************\n");
	return 0;
}



