#include<linux/linkage.h>
#include<linux/uaccess.h>

asmlinkage long sys_get_unique_id(int *uuid)
{
	return 17L;
}
