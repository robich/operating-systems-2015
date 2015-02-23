#include<linux/linkage.h>
#include<linux/uaccess.h>

asmlinkage long sys_get_unique_id(int *uuid)
{
	printk("***HEHEHEHEHE******** salut *******************");
	return -17;
}
