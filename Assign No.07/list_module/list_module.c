#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/time.h>

#define SMALL 1000
#define MIDDLE 10000
#define LARGE 100000
#define BILLION 1000000000

struct my_node {
	struct list_head list;
	int data;
};

unsigned long long calclock3(struct timespec *spclock, unsigned long long *total_time, unsigned long long *total_count) {
	long tmp, tmp_n;
	unsigned long long timedelay = 0;
	
	if(spclock[1].tv_nsec >= spclock[0].tv_nsec) {
		tmp = spclock[1].tv_sec - spclock[0].tv_sec;
		tmp_n = spclock[1].tv_nsec - spclock[0].tv_nsec;
	}
	else {
		tmp = spclock[1].tv_sec - spclock[0].tv_sec - 1;
		tmp_n = BILLION + spclock[1].tv_nsec - spclock[0].tv_nsec;
	}
	timedelay = BILLION * tmp + tmp_n;
	
	__sync_fetch_and_add(total_time, timedelay);
	__sync_fetch_and_add(total_count, 1);
	
	return timedelay;
}

void struct_example(void) {
	int list_size = SMALL;	// size of list; SMALL, MIDDLE, LARGE
	struct list_head my_list;	// head of list of my_node
	struct my_node *cur, *tmp;	// temporal variable for iteration
	unsigned long long total_time = 0, total_count = 0;
	unsigned long long time_to_print = 0;
	struct timespec spclock[2];
	int i;	// for 'for' statement
	
	
	INIT_LIST_HEAD(&my_list);
	printk("List size is %d\n", list_size);


	/* adding elements to list */
	for(i = 0; i < list_size; i++){
		getnstimeofday(&spclock[0]);
	
		struct my_node *new_node = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		new_node->data = i;
		list_add(&new_node->list, &my_list);
		
		
		getnstimeofday(&spclock[1]);
		time_to_print += calclock3(spclock, &total_time, &total_count);
	}
	printk("Addition time: %lld, count: %lld\n", time_to_print, total_count);
	
	
	/* searching list for all elements */
	total_time = 0, total_count = 0, time_to_print = 0;
	for(i = 0; i < list_size; i++){
		getnstimeofday(&spclock[0]);
		
		list_for_each_entry(cur, &my_list, list) {
			if(cur->data == list_size / 2) {
				break;
			}
		}
		
		getnstimeofday(&spclock[1]);
		time_to_print += calclock3(spclock, &total_time, &total_count);
	}
	printk("Searching time: %lld, count: %lld\n", time_to_print, total_count);
	
	
	/* delete all the elements and free */
	total_time = 0, total_count = 0, time_to_print = 0;
	list_for_each_entry_safe(cur, tmp, &my_list, list) {
		getnstimeofday(&spclock[0]);
		
		list_del(&cur->list);
		kfree(cur);
		
		getnstimeofday(&spclock[1]);
		time_to_print += calclock3(spclock, &total_time, &total_count);
	}
	printk("Deletion time: %lld, count: %lld\n", time_to_print, total_count);
}

int __init hello_module_init(void) {
	printk("Module inserted.\n");
	
	// list manipulation entry
	struct_example();
	printk("Linked-list manipulation ended.\n");
	
	return 0;
}

void __exit hello_module_cleanup(void) {
	printk("Module unloaded.\n");
}

module_init(hello_module_init);
module_exit(hello_module_cleanup);
MODULE_LICENSE("GPL");
