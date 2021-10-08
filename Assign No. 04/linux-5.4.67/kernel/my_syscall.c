#include <linux/kernel.h>

asmlinkage long sys_mycall(void){
	printk("20170454 YiChangmin syscall");
	return 0;
}
