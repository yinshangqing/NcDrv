/*************************************************************************
	> File Name: inb_test.c
	> Author: yinshangqing
	> Mail: 841668821@qq.com 
	> Created Time: 2017年05月06日 星期六 14时40分28秒
 ************************************************************************/

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{

	int ret = iopl(3);
	if(ret < 0){
		perror("can't open!!!");
	}
	unsigned char value = inb(0x20);
	

	printf("value: 0x%0x\n",value);

	return 0;
}




