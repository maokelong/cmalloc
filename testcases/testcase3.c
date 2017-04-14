/*
	迭代分配
*/

#include "../sources/includes/cmalloc.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
	int i;
	for (i = 0; i < 40960; ++i){
		void *ptr = malloc(i);
        free(ptr);
    }
	return 0;
}