#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define __NR_get_unique_id 358
#define __NR_get_child_pids 359

inline long get_unique_id(int* uuid) {
	return syscall(__NR_get_unique_id, uuid) ? errno : 0;
}

inline long get_child_pids(pid_t* buf, size_t limit, size_t* num_children) {
	return syscall(__NR_get_child_pids, buf, limit, num_children) ? errno : 0;
}
