/*************************************************************************
	> File Name: NcDrv.c
	> Author: yinshangqing
	> Mail: 841668821@qq.com 
	> Created Time: 2017年03月10日 星期六 16时07分08秒
 ************************************************************************/

/*************************************************************************
 *  > 中断向量表
 *	> 
 *	> 
 *  > 
 *  > 
 ************************************************************************/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <asm/io.h>
//#include <asm/arch/regs-gpio.h>
//#include <asm/hardware.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
//#include <asm/system.h>
#include <asm/switch_to.h>//代替asm/system.h头文件
#include <asm/uaccess.h>
#include <linux/timer.h> /*包括timer.h头文件*/
#include <asm/atomic.h> 


#define  CLR8259INT   0x20			/* clear 8259 and allow next one */         
#define  CLR8259DATA  0x20
#define  MASK8259     0x21             
#define  INT_NUMBER   0x20 
#define  IRQ_INT      0x03	    	/* 申请中断号 */
#define  INT_CTRL     0x60 
#define  IRQ_CONTROL  0x60			/* 控制部分的中断号 */
#define  INT_DRIVER	0x0b			/* 0x0b 对应的中断号 */
#define  INTERFACE    "INTER_3"		/* 中断的别名 */
#define  TASK_READY             -1
#define  TASK_CLOSE             0
#define  TASK_SHARE             1
#define  TASK_STK_LEN           26
#define  MAX_TASK               16

#define  E_STACK_OVERFLOW       1 
#define  E_TASK_OVERFLOW        2
#define  E_WHOLE_STACK_OVERFLOW 3
#define  E_TASK_UNKNOWN         4
#define  E_TASK_ERROR_EXIT      5
#define  E_TASK_INDEX_ERROR     6

#define MAX_TASK	 16
//#define IRQ_INT      3		    /* 申请中断号 */
//#define IRQ_INT      0x0b		    /* 申请中断号 */
#define INTERFACE    "INTER_3"  	/* 中断的别名 */

typedef struct {
  unsigned sp,               
           bp,               
           ss;
} REG;

typedef struct {
  REG reg;
  int flag;								/* =0 Normal;=1 Emergent;=-1 Close */
  //void (__cdecl __far *addr)();
  unsigned counter_l,					/* Idle counter initial value   */ 
           counter,						/* Current idle counter */
           stk_top;						/* Stack top addr */
} TASK_REC;

typedef struct {
  int      emg,
           now,                         /* Current active task */
           total,                       /* Total login task */ 
          int_no;						/* Interruput no */
  unsigned stk_top,						/* Total stack top addr */
           err_cs,
           err_ip;
} TASK;

//void (_interrupt _far *old_driver[INT_NUMBER])();
TASK_REC task_rec[MAX_TASK];
REG      int_rec,old_rec;
TASK     task;
unsigned old_mask,new_mask;


/* 定义并初始化等待队列头 */
static DECLARE_WAIT_QUEUE_HEAD(NcDrv_waitq);

static int ev_press = 0;
static struct fasync_struct *NcDrv_async;

//static int can_read;

static struct class	 *NcDrv_class;
static struct device     *NcDrv_device;
/* 中断申请号 */
static int irq;
char *interface;
int irq_irq = 1;
/* 传递的数据值 */
int ifun_run = 1;
/* 是否给用户发送的标志 */
unsigned int can_send;
static irqreturn_t NcDrv_irq(int irq,void *dev);

static int NcDrv_open(struct inode * inode, struct file * filp)  
{  
	unsigned char no = 0x0b;
	unsigned char old_mask=inb(MASK8259);               /* get hardware interrupt mask */
	unsigned char bit=~(1<<(no-8));
	unsigned char new_mask=old_mask & bit;              /* enable the INT by setting to 0 */
	printk(KERN_NOTICE "======== NcDrv_open\n");  
	/* 中断部分 */
	if(request_irq(irq,NcDrv_irq,IRQF_SHARED | IRQF_TRIGGER_LOW,interface,&irq) != 0){
		//printk(KERN_ERR "%s interrupt can't register %d IRQ\n",interface,irq);
		return -EIO;
	}
	
	disable_irq(irq);
//	outp(MASK8259,new_mask);              /* send this new mask to 8259 */
	outb(new_mask,MASK8259);              /* send this new mask to 8259 */
	
	enable_irq(irq);
	
	
	/*
	if(request_irq()){
	
	}
	*/
	return 0;  
}

static ssize_t NcDrv_read(struct file *file, char __user *user, size_t size,loff_t *ppos)  
{  
	 
	if (size != 1)
		return -EINVAL;
	//printk(KERN_NOTICE "======== NcDrv_read\n"); 
	/* 如果没有按键动作, 休眠 */
	wait_event_interruptible(NcDrv_waitq, ev_press);  
	//copy_NcDrv(user, &ifun_run, 4);
	/* 将ev_press清零 */
	ev_press = 0;	
	return 1;     
}  

/*文件释放，上层对此设备调用close时会执行*/  
int NcDrv_release(struct inode *inode, struct file *filp)      
{   
	//printk("the irq is bye bye!\n");
	free_irq(irq,&irq);	
	return 0;  
}  

static unsigned NcDrv_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &NcDrv_waitq, wait); // 不会立即休眠

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int NcDrv_fasync (int fd, struct file *filp, int on)
{
	//printk("driver: NcDrv_fasync\n");
	//初始化/释放 fasync_struct 结构体 (fasync_struct->fa_file->f_owner->pid)
	return fasync_helper (fd, filp, on, &NcDrv_async);
}

/* File operations struct for character device */  
static const struct file_operations NcDrv_fops = {  
	.owner         = THIS_MODULE,   /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open          = NcDrv_open,  
	.read          = NcDrv_read,  
	.release       = NcDrv_release,
	.poll    	   = NcDrv_poll,
	.fasync	 	   = NcDrv_fasync,
};   

int major;
/* 驱动入口函数 */
static int NcDrv_init(void)
{
	/* 数据初始化部分 */
	irq            = IRQ_INT;
	interface      = INTERFACE;
	//printk(KERN_NOTICE "======== NcDrv_init\n");
	/* 主设备号设置为0表示由系统自动分配主设备号 */ 
	major          = register_chrdev(0,"NcDrv_drv",&NcDrv_fops); 
	/* 创建 NcDrv 类 */  
	NcDrv_class  = class_create(THIS_MODULE, "NcDrv");   /* 名字不能一样 */  	  
	/* 在 NcDrv 类下创建 NcDrv_dev 设备，供应用程序打开设备*/
	NcDrv_device = device_create(NcDrv_class, NULL, MKDEV(major, 0), NULL, "NcDrv_dev");  
		  
	return 0;
}

static irqreturn_t NcDrv_irq(int irq,void *dev)
{
	//printk("%d IRQ is working...\n",irq);

	/* 此处给用户发送执行信号 */
	/* copy_NcDrv(); */
	
	ev_press	   = 1;							 	/* 表示中断已经发生 */
	wake_up_interruptible(&NcDrv_waitq);   	/* 唤醒休眠的进程 */

	//发送信号SIGIO信号给fasync_struct 结构体所描述的PID，触发应用程序的SIGIO信号处理函数
	kill_fasync (&NcDrv_async, SIGIO, POLL_IN);
	
	return IRQ_RETVAL(IRQ_HANDLED);
}

/* 驱动出口函数 */  
static void NcDrv_exit(void)  
{  
	//printk(KERN_NOTICE "======== NcDrv_exit\n"); 
	unregister_chrdev(major, "NcDrv_drv");  
	device_unregister(NcDrv_device);  /* 卸载类下的设备 */  
	class_destroy(NcDrv_class);       /* 卸载类 */ 
} 


module_init(NcDrv_init);  //用于修饰入口函数  
module_exit(NcDrv_exit);  //用于修饰出口函数      
  
MODULE_AUTHOR("YSQ");  
MODULE_DESCRIPTION("Just for Demon");  
MODULE_LICENSE("GPL");  //遵循GPL协议
//module_param(interface,charp,0644);
//module_param(irq,int,0644);






