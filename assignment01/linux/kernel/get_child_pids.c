#include<linux/linkage.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/thread_info.h>
#include<asm-generic/current.h>
#include<linux/spinlock.h>
#include<linux/uaccess.h>
#include<linux/list.h>

#define current (current_thread_info()->task)

asmlinkage long sys_get_child_pids(pid_t* list, size_t limit, size_t* num_children)
{
	// return value of the function
	long res;
	res = 0;

	// Counter for children tasks
	size_t children;
	children = 0;

	// Structures for current task and a list for its children
	struct task_struct* current_task;
	struct list_head children_tasks_list;
	current_task = current;
	children_tasks = current_task->children;

	// First check on memory validity
	if (list == NULL && limit != 0) {
		printk(KERN_ERR "List is NULL whereas limit is nonzero!");
		return -EFAULT;
	}

	// TODO : Lock pids list here

	// Iterate on children
	struct task_struct *child;
	list_for_each_entry(child, &children_tasks_list, *next) {

		// Extract pid of child
		pid_t child_id;
		child_id = child->pid;

		printk(KERN_DEBUG "Looking at child pid %ld" child_id);

		// Increment number of children
		children = children + 1;

		if (children <= limit) {
			// Store children pid in the list
			if(put_user(child_id, list[children-1]) == -EFAULT){
				printk(KERN_ERR "Error while writing children id");
				return -EFAULT;
			}
		}
	}

	if (children <= limit) {
		// Need to return -ENOBUFFS;
		res = -ENOBUFS;
	}

	// Write the number of children in num_children
	long write = put_user(children, num_children);

	// We make the choice to say that EFAULT is more important than
	// ENOBUFFS in case both happen at the same time
	if (write == -EFAULT) {
		res = -EFAULT;
	}

	return res;
}
