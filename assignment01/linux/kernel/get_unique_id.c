#include<linux/linkage.h>
#include<linux/uaccess.h>
#include<linux/spinlock.h>
asmlinkage long sys_get_unique_id(int *uuid)
{
	// Hi guys! (from Robin, with <3)
	// Deal with concurrency !
	// HELLO_BITCHES
	// Use put_user
	int x = 10;
	return put_user(x, uuid);
	//Working here (Alain)
}

