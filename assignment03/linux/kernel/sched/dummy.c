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

static void check_preempt_curr_dummy(struct rq *rq, struct task_struct *p, int flags);

unsigned int sysctl_sched_dummy_timeslice = DUMMY_TIMESLICE;
static inline unsigned int get_timeslice(void)
{
	return sysctl_sched_dummy_timeslice;
}

unsigned int sysctl_sched_dummy_age_threshold = DUMMY_AGE_THRESHOLD;
static inline unsigned int get_age_threshold(void)
{
	return sysctl_sched_dummy_age_threshold;
}

/*
 * Init
 */

void init_dummy_rq(struct dummy_rq *dummy_rq, struct rq *rq)
{
	int i = 0;
	for (i = 0; i < NR_OF_DUMMY_PRIORITIES; i++) {
		INIT_LIST_HEAD(&dummy_rq->queues[i]);
	}
	
	// Some initialization also happen in include/linux/init_task.h
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
	
	if (!dummy_prio(p->prio)) {
		// defined in include/linux/sched.h
		// Dealing with prio outside of dumy.c's range. Do nothing.
		return;
	}
	
	struct dummy_rq *dummy_rq = &rq->dummy;
	struct sched_dummy_entity *dummy_se = &p->dummy_se;
	
	/* Put task into the right queue according to the dynamic prio */
	struct list_head *queue = &dummy_rq->queues[p->prio - MIN_DUMMY_PRIO];
	
	list_add_tail(&dummy_se->run_list, queue);
	
	dummy_se->age_tick_count = 0;
	
	int flags = 0;
	check_preempt_curr_dummy(rq, p, flags);
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
	_dequeue_task_dummy(p);
	_enqueue_task_dummy(rq, p);
}

static void yield_task_dummy(struct rq *rq)
{
	unsigned int flags = 0;
	requeue_task_dummy(rq, rq->curr, flags);
	resched_curr(rq);
}

static void check_preempt_curr_dummy(struct rq *rq, struct task_struct *p, int flags)
{
	#ifdef KERNEL_DEBUG
	printk_deferred(KERN_ALERT "[call] check_preempt_curr_dumm(rq=%p, p=%p)\n", rq, p);
	printk_deferred(KERN_ALERT "[info] p->prio =%d\n", p->prio);
	printk_deferred(KERN_ALERT "[info] rq->curr-prio =%d\n", p->prio);
	#endif
	
	/* Preempt current task if prio is lower (only need to reschedule in this case) */
	if (rq->curr == NULL || p->prio < rq->curr->prio) {
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
	
	if (p->prio != oldprio) {
		unsigned int flags = 0;
		requeue_task_dummy(rq, p, flags);	
	}
}

static struct task_struct *pick_next_task_dummy(struct rq *rq, struct task_struct* prev)
{
	
	struct dummy_rq *dummy_rq = &(rq->dummy);
	struct sched_dummy_entity *next;
	struct task_struct *next_task;

	
	int i = 0;
	/* Iterate over the different priorities until we find a task */
	for (i = 0; i < NR_OF_DUMMY_PRIORITIES; i++) {
		struct list_head *queue = &(dummy_rq->queues[i]);
		
		if (!list_empty(queue)) {
			next = list_first_entry(queue, struct sched_dummy_entity, run_list);
			next_task = dummy_task_of(next);
			put_prev_task(rq, prev);
			
			if (next_task != dummy_rq->current_task) {
				
				next_task->prio = next_task->normal_prio;
				next_task->dummy_se.timeslice = 0;
				next_task->dummy_se.age_tick_count = 0;

				dummy_rq->current_task = next_task;
			}
			
			return next_task;
		}
	}
	
	/* if nothing is found */
	return NULL;
	
}

static void put_prev_task_dummy(struct rq *rq, struct task_struct *prev)
{
	// Nothing
}

static void set_curr_task_dummy(struct rq *rq)
{
	_dequeue_task_dummy(rq->curr);
}

static void task_tick_dummy(struct rq *rq, struct task_struct *curr, int queued)
{
	// TODO change
	int i = 0;
	struct sched_dummy_entity *next;
	struct dummy_rq *dummy_rq = &rq->dummy;
	struct list_head *p, *n;
	struct task_struct *next_task;

	/* Task preemption */
	if (curr->dummy_se.timeslice >= get_timeslice()) {
		requeue_task_dummy(rq, curr, 0);
		resched_curr(rq);
	} else {
		curr->dummy_se.timeslice += 1;
	}

	/* Aging with bad O(n) algorithm */
	for (i = 0; i < NR_OF_DUMMY_PRIORITIES; ++i) {
		list_for_each_safe(p, n, &dummy_rq->queues[i]) {
			next = list_entry(p, struct sched_dummy_entity, run_list);
			next_task = dummy_task_of(next);

			if (next != &curr->dummy_se)
				next->age_tick_count += 1;

			if (next->age_tick_count >= get_age_threshold()
					&& next_task->prio > MIN_DUMMY_PRIO) {
				next_task->prio -= 1;
				unsigned int flags = 0;
				requeue_task_dummy(rq, next_task, flags);
			}
		}
	}
}

static void switched_from_dummy(struct rq *rq, struct task_struct *p)
{
	// Nothing
}

static void switched_to_dummy(struct rq *rq, struct task_struct *p)
{
	if (!p->on_rq) return;
	
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
	// Nothing
}
#endif
/*
 * Scheduling class
 */
static void update_curr_dummy(struct rq*rq)
{
	// Nothing
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
