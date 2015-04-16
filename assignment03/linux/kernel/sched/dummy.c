/*
 * Dummy scheduling class, mapped to range of 5 levels of SCHED_NORMAL policy
 */

#include "sched.h"

/*
 * Timeslice and age threshold are repsented in jiffies. Default timeslice
 * is 100ms. Both parameters can be tuned from /proc/sys/kernel.
 */

#define DUMMY_TIMESLICE		(100 * HZ / 1000)
#define DUMMY_AGE_THRESHOLD	(3 * DUMMY_TIMESLICE)

//#define KERNEL_DEBUG

unsigned int sysctl_sched_dummy_timeslice = DUMMY_TIMESLICE;
static inline unsigned int get_timeslice(void)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] get_timeslice [return] %u\n", sysctl_sched_dummy_timeslice);
	#endif
	return sysctl_sched_dummy_timeslice;
}

unsigned int sysctl_sched_dummy_age_threshold = DUMMY_AGE_THRESHOLD;
static inline unsigned int get_age_threshold(void)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] get_dummy_age_threshhold [return] %u\n", sysctl_sched_dummy_age_threshold);
	#endif
	return sysctl_sched_dummy_age_threshold;
}

/*
 * Init
 */

void init_dummy_rq(struct dummy_rq *dummy_rq, struct rq *rq)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] init_dummy_rq\n");
	#endif
	
	int i = 0;
	for (i = 0; i < NR_OF_DUMMY_PRIORITIES; i++) {
		INIT_LIST_HEAD(&dummy_rq->queues[i]);
	}
}

/*
 * Helper functions
 */

static inline struct task_struct *dummy_task_of(struct sched_dummy_entity *dummy_se)
{
	return container_of(dummy_se, struct task_struct, dummy_se);
}

static inline void _enqueue_task_dummy(struct rq *rq, struct task_struct *p)
{
	
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] _enqueue_task_dummy(rq=%p, p=%p)\n", rq, p);
	printk_deferred(KERN_ALERT "[info] task has prio %d", p->prio);
	printk_deferred(KERN_ALERT "[info] timeslice put to 0", p->prio);
	printk_deferred(KERN_ALERT "[info] age put to 0", p->prio);
	#endif
	
	struct dummy_rq *dummy_rq = &rq->dummy;
	
	/* Set timeslice & age_tick_count to 0 in the scheduling entity */
	struct sched_dummy_entity *dummy_se = &p->dummy_se;
	dummy_se->timeslice = 0;
	dummy_se->age_tick_count = 0;
	
	/* Put task into the right queue according to the dynamic prio */
	struct list_head *queue = &dummy_rq->queues[p->prio - MIN_DUMMY_PRIO];
	
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[info] put task in queue %p (nr=%d)", queue, p->prio - MIN_DUMMY_PRIO);
	#endif
	
	list_add_tail(&dummy_se->run_list, queue);
}

static inline void _dequeue_task_dummy(struct task_struct *p)
{
	struct sched_dummy_entity *dummy_se = &p->dummy_se;
	list_del_init(&dummy_se->run_list);
}

/*
 * Scheduling class functions to implement
 */

static void enqueue_task_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	_enqueue_task_dummy(rq, p);
	add_nr_running(rq,1);
}

static void dequeue_task_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	_dequeue_task_dummy(p);
	sub_nr_running(rq,1);
}

static void requeue_task_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] requeue_task_dummy(rq=%p, p=%p)\n", rq, p);
	printk_deferred(KERN_ALERT "[info] task has prio %d", p->prio);
	#endif
	
	dequeue_task_dummy(rq, p, flags);
	enqueue_task_dummy(rq, p, flags);
	
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[info] call resched_curr(%p)\n", rq);
	#endif
	
	resched_curr(rq);
}

static void yield_task_dummy(struct rq *rq)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] yield_tasl_dummy(rq=%p, p=%p)\n", rq);
	printk_deferred(KERN_ALERT "[info] call requeue_task_dummy\n", rq);
	#endif
	
	unsigned int flags = 0;
	requeue_task_dummy(rq, rq->curr, flags);
}

static void check_preempt_curr_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] check_preempt_curr_dumm(rq=%p, p=%p)\n", rq, p);
	printk_deferred(KERN_ALERT "[info] p->prio =%d\n", p->prio);
	printk_deferred(KERN_ALERT "[info] rq->curr-prio =%d\n", p->prio);
	#endif
	
	/* Preempt current task if prio is lower (only need to reschedule in this case) */
	if (p->prio < rq->curr->prio) {
		#ifdef KERNEL_DEBUG
		printk_deferred(KERN_ALERT "[info] preempting, call resched_curr\n", rq, p);
		#endif
		resched_curr(rq);
	}
}

static void prio_changed_dummy(struct rq*rq, struct task_struct *p, int oldprio)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] prio_changed_dummy(rq=%p, p=%p, oldprio=%d)\n", rq, p, oldprio);
	printk_deferred(KERN_ALERT "[info] call check_preempt_curr_dummy", p->prio);
	#endif
	
	unsigned int flags = 0;
	check_preempt_curr_dummy(rq, p, flags);
}

static struct task_struct *pick_next_task_dummy(struct rq *rq, struct task_struct* prev)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] pick_next_task_dummy(rq=%p, prev=%p)\n", rq, prev);
	#endif
	
	struct dummy_rq *dummy_rq = &(rq->dummy);
	struct sched_dummy_entity *next;
	
	int i;
	/* Iterate over the different priorities until we find a task */
	for (i = 0; i < NR_OF_DUMMY_PRIORITIES; i++) {
		#ifdef KERNEL_DEBUG
		printk_deferred(KERN_ALERT "[info] testing priority %d\n", i);
		#endif
		struct list_head *queue = &(dummy_rq->queues[i]);
		
		if (!list_empty(queue)) {
			next = list_first_entry(queue, struct sched_dummy_entity, run_list);
			#ifdef KERNEL_DEBUG
			printk_deferred(KERN_ALERT "[info] list non_empty");
			#endif
			
			put_prev_task(rq, prev);
			
			return dummy_task_of(next);
		}
	}

	/* if nothing is found */
	return NULL;
	
}

static void put_prev_task_dummy(struct rq *rq, struct task_struct *prev)
{
}

static void set_curr_task_dummy(struct rq *rq)
{
}

static void task_tick_dummy(struct rq *rq, struct task_struct *curr, int queued)
{
	
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] task_tick_dummy(rq=%p, curr=%p, queued=%d)\n", rq, curr, queued);
	#endif
	
	struct dummy_rq *dummy_rq = &rq->dummy;
	
	int i;
	/* Increment age & test for threshhold */
	for (i = 1; i < NR_OF_DUMMY_PRIORITIES; i++) {
		
		#ifdef KERNEL_DEBUG
		printk_deferred(KERN_ALERT "[info] iterating on priority %d\n", i);
		#endif
		
		struct list_head *p, *n;
		struct sched_dummy_entity *current_se;
		
		list_for_each_safe(p, n, &dummy_rq->queues[i]) {
			current_se = list_entry(p, struct sched_dummy_entity, run_list);
			current_se->age_tick_count++;
			
			/* Get corresponding task_struct */
			struct task_struct *current_task = dummy_task_of(current_se);
				
			#ifdef KERNEL_DEBUG
			printk_deferred(KERN_ALERT "[info] iterating on task_struct %u\n", current_task->pid);
			printk_deferred(KERN_ALERT "[info] increment age, now at %u\n", current_se->age_tick_count);
			#endif
			
			if (current_se->age_tick_count >= get_age_threshold()) {
				
				/* Set new priority and change queue */
				unsigned int new_prio = i - 1 + MIN_DUMMY_PRIO;
				current_se->prio_saved = current_task->prio;
				current_task->prio = new_prio;
				list_move_tail(p, &dummy_rq->queues[i-1]);
				/* Callback */
				// Try to fix test4
				resched_curr(rq);
				prio_changed_dummy(rq, current_task, new_prio + 1);
			}
		}
	}
	
	curr->dummy_se.timeslice++;
	if (curr->dummy_se.timeslice >= get_timeslice()) {
		unsigned int flags = 0;
		if (curr->prio != curr->dummy_se.prio_saved) {
			// Need to put it back to its old priority
			curr->prio = curr->dummy_se.prio_saved;
		}
		requeue_task_dummy(rq, curr, flags);
	}
}

static void switched_from_dummy(struct rq *rq, struct task_struct *p)
{
}

static void switched_to_dummy(struct rq *rq, struct task_struct *p)
{
	if (rq->curr == p) {
		resched_curr(rq);
	} else {
		unsigned int flags = 0;
		check_preempt_curr_dummy(rq, p, flags);
	}
}

static unsigned int get_rr_interval_dummy(struct rq* rq, struct task_struct *p)
{
	return get_timeslice();
}
#ifdef CONFIG_SMP
/*
 * SMP related functions	
 */

static inline int select_task_rq_dummy(struct task_struct *p, int cpu, int sd_flags, int wake_flags)
{
	int new_cpu = smp_processor_id();
	
	return new_cpu; //set assigned CPU to zero
}


static void set_cpus_allowed_dummy(struct task_struct *p,  const struct cpumask *new_mask)
{
}
#endif
/*
 * Scheduling class
 */
static void update_curr_dummy(struct rq*rq)
{
}
const struct sched_class dummy_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_dummy,
	.dequeue_task		= dequeue_task_dummy,
	.yield_task		= yield_task_dummy,

	.check_preempt_curr	= check_preempt_curr_dummy,
	
	.pick_next_task		= pick_next_task_dummy,
	.put_prev_task		= put_prev_task_dummy,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_dummy,
	.set_cpus_allowed	= set_cpus_allowed_dummy,
#endif

	.set_curr_task		= set_curr_task_dummy,
	.task_tick		= task_tick_dummy,

	.switched_from		= switched_from_dummy,
	.switched_to		= switched_to_dummy,
	.prio_changed		= prio_changed_dummy,

	.get_rr_interval	= get_rr_interval_dummy,
	.update_curr		= update_curr_dummy,
};
