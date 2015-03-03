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

	/* Try to write the ID */
	long _id;
	
	spin_lock(&uuid_lock);
	_id = id;
	id++;
	spin_unlock(&uuid_lock);

	return put_user(_id, uuid);
}
