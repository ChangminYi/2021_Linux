#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>


#define BILLION	1000000000
#define DATA_SIZE	100000	// 데이터 양
#define HASHTABLE_BIT	8	// 해시테이블 크기의 log값


// 테스트를 위해서 만든 구조체. key와 value는 같은 값, 정수.
struct my_node {
	int key, value;
	struct rb_node node;
};

// my_node 메모리 할당받아 초기화한 후 반환
struct my_node *get_my_node(int _key, int _value) {
	struct my_node *ret = kmalloc(sizeof(struct my_node), GFP_KERNEL);
	ret->key = _key;
	ret->value = _value;
	
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

// 레드-블랙 트리 삽입
void rbtree_insert(struct rb_root *root, struct my_node *data) {
	struct rb_node **new_node = &(root->rb_node);
	struct rb_node *parent = NULL;
	
	while(*new_node) {
		struct my_node *this = rb_entry(*new_node, struct my_node, node);
		parent = *new_node;
		
		if(data->key < this->key) {
			new_node = &((*new_node)->rb_left);
		}
		else if(data->key > this->key) {
			new_node = &((*new_node)->rb_right);
		}
		else {
			return;
		}
	}
	
	rb_link_node(&data->node, parent, new_node);
	rb_insert_color(&data->node, root);
}

// 레드-블랙 트리 탐색. key값으로 탐색을 진행하여 해당 my_node 포인터 반환
struct my_node *rbtree_search(struct rb_root *root, int key) {
	struct rb_node *cur = root->rb_node;
	
	while(cur) {
		struct my_node *data = rb_entry(cur, struct my_node, node);
		
		if(key == data->key) {
			return data;
		}
		else{
			if(key < data->key) {
				cur = cur->rb_left;
			}
			else{
				cur = cur->rb_right;
			}
		}
	}
	
	return NULL;
}


// 각 버킷이 레드-블랙 트리인 해시테이블의 삽입, 탐색, 삭제 실행하는 함수
void hash_rbtree_manip(void) {
	// 시간 측정용 변수들 ////////////////////////////
	unsigned long long total_time = 0, total_count = 0;
	unsigned long long calc_t;
	struct timespec spclock[2];
	////////////////////////////////////////////////
	
	unsigned bkt_idx;	// 해시테이블 접근 시 필요한 인덱스. hash_min으로 얻음.
	int i, count;		// i는 for문에서 사용, count는 성공적으로 실행한 횟수 카운트.
	struct my_node *tmp;	// 임시변수. 레드블랙 트리의 탐색, 삭제 등에서 사용.
	
	// 해시테이블 선언 및 초기화
	struct rb_root tbl[1 << HASHTABLE_BIT];
	for(i = 0; i < (1 << HASHTABLE_BIT); i++) {
		tbl[i] = RB_ROOT;
	}
	
	// 삽입. 메모리 할당 시간은 제외하고 측정 /////////////////////////////////////////
	calc_t = 0, count = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		tmp = get_my_node(i, i);
		
		getnstimeofday(&spclock[0]);
		
		bkt_idx = hash_min(tmp->key, HASHTABLE_BIT);
		rbtree_insert(&tbl[bkt_idx], tmp);
		
		getnstimeofday(&spclock[1]);
		calc_t += calclock3(spclock, &total_time, &total_count);
		count++;
	}
	printk("Insert time: %lld ns, count: %d\n", calc_t, count);
	
	
	// 탐색. key를 해싱하여 해시테이블 인덱스를 얻고, 해당 rbtree 탐색 /////////////////
	calc_t = 0, count = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		getnstimeofday(&spclock[0]);
		
		bkt_idx = hash_min(i, HASHTABLE_BIT);
		tmp = rbtree_search(&tbl[bkt_idx], i);
		if(tmp && i == tmp->value) {
			count++;
		}
		
		getnstimeofday(&spclock[1]);
		calc_t += calclock3(spclock, &total_time, &total_count);
	}
	printk("Search time: %lld ns, count: %d\n", calc_t, count);
	
	
	// 삭제. 삭제할 key를 해싱하여 인덱스 얻고, 해당 노드 삭제 ////////////////////////
	calc_t = 0, count = 0;
	for(i = 0; i < DATA_SIZE; i++) {
		getnstimeofday(&spclock[0]);
		
		bkt_idx = hash_min(i, HASHTABLE_BIT);
		tmp = rbtree_search(&tbl[bkt_idx], i);
		if(tmp && i == tmp->value) {
			rb_erase(&tmp->node, &tbl[bkt_idx]);
			kfree(tmp);
			count++;
		}
		
		getnstimeofday(&spclock[1]);
		calc_t += calclock3(spclock, &total_time, &total_count);
	}
	printk("Delete time: %lld ns, count: %d\n", calc_t, count);
}


int __init hash_rbtree_init(void) {
	printk("Hashtable with Red-Black tree module inserted.\n");
	printk("Table size is %d and Data size is %d\n", 1 << HASHTABLE_BIT, DATA_SIZE);
	
	hash_rbtree_manip();
	
	printk("Hashtable manipulation ended.\n");
	
	return 0;
}


void __exit hash_rbtree_cleanup(void) {
	printk("Hashtable module unloaded.\n");
}




module_init(hash_rbtree_init);
module_exit(hash_rbtree_cleanup);
MODULE_LICENSE("GPL");
