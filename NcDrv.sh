#########################################################################
# File Name: NcDrv.sh
# Author: yinshangqing
# mail: 841668821@qq.com
# Created Time: 2017年05月05日 星期五 15时16分23秒
#########################################################################
#!/bin/bash

make clean
make

clear

sudo rmmod NcDrv
sudo insmod NcDrv.ko
echo "###########################"

gcc Nc_test.c -o Nc_test

sudo ./Nc_test

#while(true) do

#dmesg | tail

#/bin/sleep 2

#done


