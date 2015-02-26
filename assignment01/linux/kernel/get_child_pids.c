#include<linux/linkage.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/thread_info.h>
#include<asm-generic/current.h>
#include<linux/spinlock.h>
#include<linux/uaccess.h>
#include<linux/list.h>

asmlinkage long sys_get_child_pids(pid_t *list, size_t limit, size_t *num_children)
{
	/* return value of the function */
	long res;

	res = 0;

	/* Counter for children tasks */
	size_t children;

	children = 0;

	/* First check on memory validity */
	if (list == NULL && limit != 0) {
		printk(KERN_ERR "List is NULL whereas limit is nonzero!\n");
		return -EFAULT;
	}

	/* TODO : Lock pids list here */

	/* Iterate on children */
	struct task_struct *child;

	list_for_each_entry(child, &current->children, sibling) {

		/* Extract pid of child */
		pid_t child_id;
		child_id = child->pid;

		printk(KERN_DEBUG "Looking at child pid %zu\n", child_id);

		/* Increment number of children */
		children = children + 1;

		if (children <= limit) {
			/* Store children pid in the list */
			if (put_user(child_id, list) == -EFAULT) {
				printk(KERN_ERR "Error while writing children id\n");
				return -EFAULT;
			}
			/* Moves pointer to next memory slot */
			list++;
		}
	}

	if (children <= limit)
		/* Need to return -ENOBUFFS; */
		res = -ENOBUFS;

	/* Write the number of children in num_children */
	if (put_user(children, num_children) == -EFAULT)
		printk(KERN_ERR, "Cannot write number of children\n");

	return res;
}
