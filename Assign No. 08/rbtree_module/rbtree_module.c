#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/time.h>

#define SMALL 1000
#define MIDDLE 10000
#define LARGE 100000
#define BILLION 1000000000

struct my_node {
	struct rb_node node;
	int value;
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

struct rb_node *rbtree_search(struct rb_root *root, int num) {
	struct rb_node *cur = root->rb_node;
	while(cur) {
		struct my_node *data = rb_entry(cur, struct my_node, node);
		
		if(num < data->value) {
			cur = cur->rb_left;
		}
		else if(num > data->value) {
			cur = cur->rb_right;
		}
		else {
			return cur;
		}
	}
	
	return NULL;
}

void rbtree_insert(struct rb_root *root, struct my_node *data) {
	struct rb_node **new_node = &(root->rb_node);
	struct rb_node *parent = NULL;
	
	while(*new_node) {
		struct my_node *this = rb_entry(*new_node, struct my_node, node);
		parent = *new_node;
		
		if(data->value < this->value) {
			new_node = &((*new_node)->rb_left);
		}
		else if(data->value > this->value) {
			new_node = &((*new_node)->rb_right);
		}
		else {
			return;
		}
	}
	
	rb_link_node(&data->node, parent, new_node);
	rb_insert_color(&data->node, root);
}

void rbtree_example(void) {
	unsigned long long total_time = 0, total_count = 0;
	unsigned long long time_to_print = 0;
	struct timespec spclock[2];
	struct rb_root root_node = RB_ROOT;
	int size = SMALL;
	int i;
	
	
	printk("Data size is %d\n", size);
	
	
	// insertion
	for(i = 0; i < size; i++) {
		getnstimeofday(&spclock[0]);
		
		struct my_node *tmp = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		tmp->value = i;
		rbtree_insert(&root_node, tmp);
	
		getnstimeofday(&spclock[1]);
		time_to_print += calclock3(spclock, &total_time, &total_count);
	}
	printk("Addition time: %lld, count: %lld\n", time_to_print, total_count);
	
	
	// searching
	total_time = 0, total_count = 0, time_to_print = 0;
	for(i = 0; i < size; i++) {
		getnstimeofday(&spclock[0]);
		
		struct rb_node *res = rbtree_search(&root_node, i);
		if(res){
			getnstimeofday(&spclock[1]);
			time_to_print += calclock3(spclock, &total_time, &total_count);
		}
	}
	printk("Searching elements: %lld, count: %lld\n", time_to_print, total_count);
	
	
	// deletion
	total_time = 0, total_count = 0, time_to_print = 0;
	for(i = 0; i < size; i++) {
		getnstimeofday(&spclock[0]);
		
		struct rb_node *res = rbtree_search(&root_node, i);
		if(res) {
			rb_erase(res, &root_node);
			kfree(res);
			
			getnstimeofday(&spclock[1]);
			time_to_print += calclock3(spclock, &total_time, &total_count);
		}
	}
	printk("Deletion time: %lld, count: %lld\n", time_to_print, total_count);
}

int __init rbtree_module_init(void) {
	printk("Module loaded.\n");
	rbtree_example();
	printk("Red-Black tree manipulation ended.\n");
	return 0;
}

void __exit rbtree_module_cleanup(void) {
	printk("Module unloaded.\n");
}

module_init(rbtree_module_init);
module_exit(rbtree_module_cleanup);
MODULE_LICENSE("GPL");
