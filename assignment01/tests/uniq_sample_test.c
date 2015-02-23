# include <stdio.h>
# include "syscalls.h"

int main () {
	int uuid;
	int res;
	
	printf("First call\n"); fflush(stdout);
	res = get_unique_id(&uuid);
	printf("Syscall returned %d, uuid is %d\n", res, uuid);

	printf("Second call\n"); fflush(stdout);
	res = get_unique_id(&uuid);
	printf ( "Syscall returned %d, uuid is %d\n", res, uuid);

	printf("Invalid memory call\n"); fflush(stdout);
	res = get_unique_id((int*)47424742); // arbitrary memory address
	printf ( "Syscall returned %d \n", res);

	return 0;
}
