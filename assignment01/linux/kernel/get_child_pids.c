#include<linux/linkage.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/thread_info.h>
#include<asm-generic/current.h>
#include<linux/spinlock.h>
#include<linux/uaccess.h>
#include<linux/list.h>

spinlock_t lock;

static int __init hi(void)
{
	spin_lock_init(&lock);

	return 0;
}

module_init(hi);

asmlinkage long sys_get_child_pids(pid_t *list, size_t limit,
	size_t *num_children)
{
	/* return value of the function */
	long res;

	res = 0;

	/* Counter for children tasks */
	size_t children_count;
<<<<<<< HEAD
=======

>>>>>>> d6e16e97c7352833ecc0a0412bde68e9a1d8eadd
	children_count = 0;

	/* First check on memory validity */
	if (list == NULL && limit != 0) {
		printk(KERN_ERR "List is NULL whereas limit is nonzero!\n");
		return -EFAULT;
	}

	/* Lock tasklist while iterating */
	read_lock(&tasklist_lock);

	/* Iterate on children */
	struct task_struct *child;

	list_for_each_entry(child, &current->children, sibling) {

		/* Extract pid of child */
		pid_t child_id;
<<<<<<< HEAD
=======

>>>>>>> d6e16e97c7352833ecc0a0412bde68e9a1d8eadd
		child_id = child->pid;

		printk(KERN_DEBUG "Looking at child pid %zu\n", child_id);

		/* Increment number of children */
		children_count++;

		if (children_count <= limit) {
			/* Store children pid in the list */
			if (put_user(child_id, list) == -EFAULT) {
				printk(KERN_ERR "Error while writing children id\n");

				read_unlock(&tasklist_lock);

				return -EFAULT;
			}
			/* Moves pointer to next memory slot */
			list++;
		}
	}

	read_unlock(&tasklist_lock);

	if (children_count > limit) {
		/* Need to return -ENOBUFFS; */
		printk(KERN_DEBUG "List is too big to fit !\n");
		res = -ENOBUFS;
	}

<<<<<<< HEAD
	printk(KERN_DEBUG "Trying to write nr %zu\n", children_count);

	/* Write the number of children in num_children */
	if (put_user(children_count, num_children) == -EFAULT) {
		printk(KERN_ERR, "Cannot write number of children\n");
=======
	/* Lock for the writing in num_children */
	spin_lock(&lock);

	if (put_user(children_count, num_children) == -EFAULT) {
		printk(KERN_ERR "Cannot write number of children\n");

		spin_unlock(&lock);

>>>>>>> d6e16e97c7352833ecc0a0412bde68e9a1d8eadd
		return -EFAULT;
	}

	spin_unlock(&lock);

	return res;
}
