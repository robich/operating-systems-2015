#include<linux/linkage.h>
#include<linux/uaccess.h>

asmlinkage long sys_get_unique_id(int *uuid)
{
	// Deal with concurrency !
	// HELLO_BITCHES
	// Use put_user
	int x = 10;
	return put_user(x, uuid);
	//Working here (Alain)
}

