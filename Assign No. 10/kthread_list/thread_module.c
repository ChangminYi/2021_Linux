#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/time.h>

#include <linux/list.h>

#include <linux/kthread.h>

#include <linux/spinlock.h>
#include <linux/mutex.h>


#define BILLION	1000000000
#define SIZE		100000
#define THREAD_NUM	4

struct my_node {
	struct list_head list;
	int value;
};

// spinlock_t list_lock;
// struct rw_semaphore list_lock;
struct mutex list_lock;

struct list_head my_list;

int list_entry = 0;
int ins_cnt = 0, ser_cnt = 0, del_cnt = 0;
unsigned long long ins_t = 0, ser_t = 0, del_t = 0;

// 시간 측정 함수
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

/*
 * 리스트에 원소 삽입, 탐색, 삭제.
 * atomic operation인 __sync_fetch_and_add로 삽입 시작할 숫자 얻음.
 * 그 후로는, 각 스레드가 담당한 구역의 원소들만 탐색하고, 삭제함.
*/
int manip_list(void *_junk) {
	unsigned long long total_time, total_count;
	struct timespec spclock[2];
	int i;
	int base = __sync_fetch_and_add(&list_entry, SIZE / THREAD_NUM);
	struct my_node *cur;
	
	// 삽입.
	for(i = base ; i < base + (SIZE / THREAD_NUM); i++) {
		struct my_node *new_node = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		new_node->value = i;
		
		// spin_lock(&list_lock);
		// down_write(&list_lock);
		mutex_lock(&list_lock);
		
		getnstimeofday(&spclock[0]);
		list_add(&new_node->list, &my_list);
		getnstimeofday(&spclock[1]);
		
		mutex_unlock(&list_lock);
		// up_write(&list_lock);
		// spin_unlock(&list_lock);
		
		__sync_fetch_and_add(&ins_t, calclock3(spclock, &total_time, &total_count));
	}
	__sync_fetch_and_add(&ins_cnt, 1);
	if(ins_cnt == THREAD_NUM) {
		printk("Insert: %lld ns\n", ins_t);
	}
	
	// 탐색.
	for(i = base; i < base + SIZE / THREAD_NUM; i++) {
		// spin_lock(&list_lock);
		// down_read(&list_lock);
		mutex_lock(&list_lock);
		
		getnstimeofday(&spclock[0]);
		list_for_each_entry(cur, &my_list, list) {
			if(cur->value == i) {
				break;
			}
		}
		getnstimeofday(&spclock[1]);
		
		mutex_unlock(&list_lock);
		// up_read(&list_lock);
		// spin_unlock(&list_lock);
		
		__sync_fetch_and_add(&ser_t, calclock3(spclock, &total_time, &total_count));
	}
	__sync_fetch_and_add(&ser_cnt, 1);
	if(ser_cnt == THREAD_NUM) {
		printk("Search: %lld ns\n", ser_t);
	}
	
	// 삭제.
	for(i = base; i < base + SIZE / THREAD_NUM; i++) {
		// spin_lock(&list_lock);
		// down_write(&list_lock);
		mutex_lock(&list_lock);
		
		getnstimeofday(&spclock[0]);
		list_for_each_entry(cur, &my_list, list) {
			if(cur->value == i) {
				list_del(&cur->list);
				kfree(cur);
				break;
			}
		}
		getnstimeofday(&spclock[1]);
		
		mutex_unlock(&list_lock);
		// up_write(&list_lock);
		// spin_unlock(&list_lock);
		
		__sync_fetch_and_add(&del_t, calclock3(spclock, &total_time, &total_count));
	}
	__sync_fetch_and_add(&del_cnt, 1);
	if(del_cnt == THREAD_NUM) {
		printk("Delete: %lld ns\n", del_t);
	}
	
	return 0;
}

int __init thread_module_init(void) {
	int i;
	
	printk("Module inserted, using Mutex\n");	

	INIT_LIST_HEAD(&my_list);
	
	//spin_lock_init(&list_lock);
	//init_rwsem(&list_lock);
	mutex_init(&list_lock);
	
	for(i = 0; i < THREAD_NUM; i++) {
		kthread_run(&manip_list, NULL, "test_thread");
	}
	
	return 0;
}

void __exit thread_module_cleanup(void) {
	printk("Module unloaded\n");
}



module_init(thread_module_init);
module_exit(thread_module_cleanup);
MODULE_LICENSE("GPL");
