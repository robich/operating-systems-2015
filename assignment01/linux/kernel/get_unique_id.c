#include<linux/linkage.h>
#include<linux/uaccess.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/spinlock.h>

spinlock_t uuid_lock;
DEFINE_SPINLOCK(uuid_lock);
long id;

asmlinkage long sys_get_unique_id(int *uuid)
{
	/* Lock */
	spin_lock(&uuid_lock);

	/* Try to write the ID */
	long res;

	res = put_user(id, uuid);

	spin_unlock(&uuid_lock);

	if (res == -EFAULT) {
		/* printk(KERN_ERR "Trying to access an illegal memory location !\n"); */
	} else {
		/* Only increment uuid if call is valid */
		id++;
	}

	return res;
}
