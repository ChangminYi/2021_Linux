#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>


#define THREAD_NUM	4


int global_var = 0;

int print_global_var(void *_arg) {
	int thr_num = *((int *)_arg);
	int copy_var = __sync_fetch_and_add(&global_var, 1);
	printk("Thread number %d: global_var = %d\n", thr_num, copy_var);
	kfree(_arg);
	
	return 0;
}

void do_atomic(void) {
	int i;
	for(i = 0; i < THREAD_NUM; i++) {
		int *arg = kmalloc(sizeof(int), GFP_KERNEL);
		*arg = i;
		kthread_run(&print_global_var, arg, "test_thread");
	}
}

int __init atomic_operation_init(void) {
	printk("Module inserted\n");
	do_atomic();
	return 0;
}

void __exit atomic_operation_cleanup(void) {
	printk("Module unloaded\n");
}


module_init(atomic_operation_init);
module_exit(atomic_operation_cleanup);
MODULE_LICENSE("GPL");
