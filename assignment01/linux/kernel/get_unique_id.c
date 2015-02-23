#include<linux/linkage.h>
#include<linux/uaccess.h>

asmlinkage long sys_get_unique_id(int *uuid)
{
	printk("*************** salut *******************");
	return -17;
}
