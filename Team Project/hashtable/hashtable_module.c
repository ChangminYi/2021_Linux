#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/hashtable.h>


#define BILLION	1000000000
#define DATA_SIZE	100000	// 데이터 양
#define HASHTABLE_BIT	8	// 해시테이블 크기의 log값


// 테스트를 위해서 만든 구조체. key와 value는 같은 값, 정수.
struct my_node {
	int key, value;
	struct hlist_node hnode;
};

// my_node 메모리 할당받아 초기화한 후 반환
struct my_node *get_my_node(int _key, int _value) {
	struct my_node *ret = kmalloc(sizeof(struct my_node), GFP_KERNEL);
	ret->key = _key;
	ret->value = _value;
	memset(&ret->hnode, 0, sizeof(struct hlist_node));
	
	return ret;
}

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


// 리눅스 커널 해시테이블 사용하여 삽입, 탐색, 삭제 실행
void hashtable_manip(void) {
	// 해시테이블 선언
	DEFINE_HASHTABLE(tbl, HASHTABLE_BIT);
	
	// 시간 측정용 변수들 ////////////////////////////
	unsigned long long calc_t;
	unsigned long long total_time = 0, total_count = 0;
	struct timespec spclock[2];
	////////////////////////////////////////////////
	
	int i, count;	// i는 for문에서 사용, 
	unsigned bkt;		// 해시테이블 접근 시 필요한 인덱스. hash_min으로 얻음.
	struct my_node *tmp;	// 임시 변수. 데이터 생성 등에서 사용.

	// 해시테이블 초기화
	hash_init(tbl);	

	// 삽입. 메모리 할당에 걸리는 시간은 제외하고 측정 //////////////////////////////////////
	count = 0, calc_t = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		tmp = get_my_node(i, i);
		getnstimeofday(&spclock[0]);
		
		hash_add(tbl, &tmp->hnode, tmp->key);
		
		getnstimeofday(&spclock[1]);
		__sync_fetch_and_add(&calc_t, calclock3(spclock, &total_time, &total_count));
		count++;
	}
	printk("Insert time: %lld ns, count: %d\n",  calc_t, count);
	
	// 탐색. key를 해싱하여 버킷 인덱스를 얻고, 해당 버킷에서 hlist 순차탐색 /////////////////
	count = 0, calc_t = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		getnstimeofday(&spclock[0]);
		
		bkt = hash_min(i, HASHTABLE_BIT);
		hlist_for_each_entry(tmp, &tbl[bkt], hnode) {
			if(tmp->value == i) {
				count++;
				break;
			}
		}
		
		getnstimeofday(&spclock[1]);
		__sync_fetch_and_add(&calc_t, calclock3(spclock, &total_time, &total_count));
	}
	printk("Search time: %lld ns, count: %d\n",  calc_t, count);
	
	// 삭제. key를 해싱하여 해당 버킷에서 탐색하여 해당 노드 삭제 ///////////////////////////
	count = 0, calc_t = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		getnstimeofday(&spclock[0]);
		
		bkt = hash_min(i, HASHTABLE_BIT);
		hlist_for_each_entry(tmp, &tbl[bkt], hnode) {
			if(tmp->value == i) {
				hash_del(&tmp->hnode);
				count++;
				break;
			}
		}
		
		getnstimeofday(&spclock[1]);
		__sync_fetch_and_add(&calc_t, calclock3(spclock, &total_time, &total_count));
	}
	printk("Delete time: %lld ns, count: %d\n",  calc_t, count);
}


int __init hashtable_module_init(void) {
	printk("Linux kernel default hashtable module inserted.\n");
	printk("Table size is %d and Data size is %d\n", 1 << HASHTABLE_BIT, DATA_SIZE);
	
	hashtable_manip();
	
	printk("Hashtable manipulation ended.\n");
	
	return 0;
}


void __exit hashtable_module_cleanup(void) {
	printk("Hashtable module unloaded.\n");
}




module_init(hashtable_module_init);
module_exit(hashtable_module_cleanup);
MODULE_LICENSE("GPL");
