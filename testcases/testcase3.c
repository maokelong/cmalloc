/*
	多线程分配
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

long total_time;//
pthread_spinlock_t spin_lock;

void malloc_test(void)
{
	// 计时器
	struct timeval start, end;

	// 线程ID
	pthread_t tid = pthread_self();
	printf("thread %lu\tstarted.\n", tid);

	// 内存分配
	gettimeofday(&start, NULL);
	int i;
	for (i = 0; i < 10000; i += 10) {
		void* ptr = malloc(i);
		if(ptr) *(int*)ptr = 1;
		gettimeofday(&end, NULL);
		printf("thread:%lu\tmalloc:%d\treturn:%p\tSDB index:%lu\n",
			tid, i, ptr, ((size_t)ptr - 0x700000000000) / 65536);
	}

	// 计时器
	long time_con = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;

	pthread_spin_lock(&spin_lock);
	total_time += time_con;
	printf("thread %lu\tended => Time consumption(cur round): %f sec of total %f sec.\n",
		tid, time_con / 1000000.0, total_time / 1000000.0);
	pthread_spin_unlock(&spin_lock);
}
int main(int argc, char *argv[]) {
	pthread_spin_init(&spin_lock, 0);

	// 线程池
	pthread_t threads_pool[100];
	int i;
	for (i = 0; i < 100; ++i) {
		pthread_create(&threads_pool[i], NULL, (void*)malloc_test, NULL);
	}
	for (i = 0; i < 100; ++i) {
		pthread_join(threads_pool[i], NULL);
	}

	getchar();
	return 0;
}
