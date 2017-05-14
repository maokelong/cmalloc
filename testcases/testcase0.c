/*
	检查编译链接能否顺利完成，并进行一次简单的内存复用
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
	int *ptr1, *ptr2;
	ptr1 = (int *)cmalloc_malloc(1);
	ptr2 = (int *)cmalloc_malloc(1);
	printf("%p - %p\n", ptr1, ptr2);
	cmalloc_free(ptr1);
	cmalloc_free(ptr2);
	printf("%p - ", cmalloc_malloc(1));
	printf("%p\n", cmalloc_malloc(1));
	return 0;
}