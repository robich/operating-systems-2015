#include<linux/linkage.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/thread_info.h>
#include<asm-um/current.h>
#include<linux/spinlock.h>
#include<linux/uaccess.h>
#include<linux/list.h>

#define current (current_thread_info()->task)

asmlinkage long sys_get_child_pids(pid_t* list, size_t limit, size_t* num_children)
{
	// return value of the function
	long res = 0;

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
		return -EFAULT;
	}

	// TODO : Lock pids list here

	// Iterate on children
	struct list_head *i;
	list_for_each_entry(i, &children_tasks_list) {
		// Get element in the list
		struct task_struct *child = list_entry(i, struct task_struct, node);

		// Extract pid of child
		pid_t child_id;
		child_id = ???;

		// Increment number of children
		children = children + 1;

		if (children <= limit) {
			// Store children pid in the list...
		}
	}

	if (children <= limit) {
		// Need to return -ENOBUFFS;
		res = -ENOBUFFS;
	}

	// Write the number of children in num_children
	long write = put_user(&num_children, children);

	// We make the choice to say that EFAULT is more important than
	// ENOBUFFS in case both happen at the same time
	if (write == -EFAULT) {
		res = -EFAULT;
	}

	return res;
}
