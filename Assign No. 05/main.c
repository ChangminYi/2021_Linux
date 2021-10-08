#include <stdio.h>
#include <sys/syscall.h>

#define MY_SYSCALL 335

/*
 * Written by YiChangmin
 * Linux Assignment No.05: Making own syscall
 * */
int main(void) {
	long int return_val = syscall(MY_SYSCALL);
	printf("System call returned: %ld\n", return_val);

	return 0;
}

