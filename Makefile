ifeq ($(KERNELRELEASE),)  
  
KERNELDIR ?= /lib/modules/$(shell uname -r)/build   
#KERNELDIR ?= ~/wor_lip/linux-3.4.112  
PWD := $(shell pwd)  
  
modules:  
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules  
  
modules_install:  
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install  
  
clean:  
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules* Module*  
  
.PHONY: modules modules_install clean  
  
else  
    obj-m := NcDrv.o  
endif 


