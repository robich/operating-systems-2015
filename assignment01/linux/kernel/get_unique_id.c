#include<linux/linkage.h>
#include<linux/uaccess.h>
<<<<<<< HEAD
#include<linux/module.h>
#include<linux/init.h>
#include<linux/spinlock.h>

spinlock_t lock;
long id;

static int __init hi(void)
{
	spin_lock_init(&lock);
	id = 0;

	return 0;
=======
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
>>>>>>> d02acfa27ed8b70a848e2a5c4d83982889c1a144
}

module_init(hi);

asmlinkage long sys_get_unique_id(int *uuid)
{
	// Lock
	spin_lock(&lock);

	// Try to write the ID
	long res;
	res = put_user(id, uuid);
	id++;

	spin_unlock(&lock);

	if(res == -EFAULT) {
		printk(KERN_ERR "Trying to access an illegal memory location !\n");
	}

	return res;
}
