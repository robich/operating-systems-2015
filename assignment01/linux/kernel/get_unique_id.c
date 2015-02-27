#include<linux/linkage.h>
#include<linux/uaccess.h>
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
}

module_init(hi);

asmlinkage long sys_get_unique_id(int *uuid)
{
	/* Lock */
	spin_lock(&lock);

	/* Try to write the ID */
	long res;

	res = put_user(id, uuid);

	spin_unlock(&lock);

	if (res == -EFAULT) {
		printk(KERN_ERR "Trying to access an illegal memory location !\n");
	} else {
		/* Only increment uuid if call is valid */
		id++;
	}

	return res;
}
