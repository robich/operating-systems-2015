#include<linux/linkage.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/thread_info.h>
#include<asm-generic/current.h>
#include<linux/spinlock.h>
#include<linux/uaccess.h>
#include<linux/list.h>

asmlinkage long sys_get_child_pids(pid_t *list, size_t limit,
	size_t *num_children)
{
	/* return value of the function */
	long res;

	res = 0;

	/* Counter for children tasks */
	size_t children_count;

	children_count = 0;

	/* First check on memory validity */
	if (list == NULL && limit != 0) {
		/* printk(KERN_ERR "List is NULL whereas limit is nonzero!\n"); */
		return -EFAULT;
	}

	struct task_struct *_current;
	
	/* We need to lock tasklist_lock because other processes might work on that list */
	
	read_lock(&tasklist_lock);
	
	int array[limit];
	int i;
	
	i = 0;

	/* Iterate on children */
	struct task_struct *child;

	list_for_each_entry(child, &current->children, sibling) {

		/* Extract pid of child */
		pid_t child_id;

		child_id = child->pid;

		/* Increment number of children */
		children_count++;

		if (children_count <= limit) {
			/* Store children pid in the list */
			array[i] = child_id;
			i++;
		}
	}
	
	read_unlock(&tasklist_lock);
	
	int j;
	for (j = 0; j < limit; j++) {
		put_user(array[j], list);
		list++;
	}

	if (children_count > limit) {
		/* Need to return -ENOBUFFS; */
		/* printk(KERN_ERR "List is too big to fit !\n"); */
		res = -ENOBUFS;
	}

	/* Lock for the writing in num_children */

	if (put_user(children_count, num_children) == -EFAULT) {
		/* printk(KERN_ERR "Cannot write number of children\n"); */

		return -EFAULT;
	}

	return res;
}
