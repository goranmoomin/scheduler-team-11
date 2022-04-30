#include "sched.h"

char _debug_str_wrr[256];
int _debug_str_wrr_idx = 0;

inline static void mark_debug_str_wrr(char c)
{
	_debug_str_wrr[_debug_str_wrr_idx++ % 256] = c;
}

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	INIT_LIST_HEAD(&wrr_rq->tasks);
}

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

static inline struct rq *rq_of_wrr_se(struct sched_wrr_entity *wrr_se)
{
	struct task_struct *p = wrr_task_of(wrr_se);
	return task_rq(p);
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_se = &p->wrr;

	mark_debug_str_wrr('e');

	printk(KERN_INFO "enqueue_task_wrr rq=%p p=%p flags=%d\n", rq, p,
	       flags);

	list_add_tail(&wrr_se->run_list, &wrr_rq->tasks);
	add_nr_running(rq, 1);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	mark_debug_str_wrr('d');

	printk(KERN_INFO "dequeue_task_wrr rq=%p p=%p flags=%d\n", rq, p,
	       flags);

	list_del(&wrr_se->run_list);
	sub_nr_running(rq, 1);
}

static void yield_task_wrr(struct rq *rq)
{
	mark_debug_str_wrr('y');
	printk(KERN_INFO "yield_task_wrr rq=%p\n", rq);
}

static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p,
			      bool preempt)
{
	mark_debug_str_wrr('t');
	printk(KERN_INFO "yield_to_task_wrr rq=%p p=%p preempt=%d\n", rq, p,
	       preempt);
	yield_task_wrr(rq);
	return true;
}

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p,
				   int flags)
{
}

static struct task_struct *
pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p = NULL;

	mark_debug_str_wrr('n');

	printk(KERN_INFO "pick_next_task_wrr rq=%p prev=%p rf=%p\n", rq, prev,
	       rf);

	if (prev->on_rq) {
		put_prev_task(rq, prev);
	}

	if (!list_empty(&wrr_rq->tasks)) {
		wrr_se = list_first_entry(&wrr_rq->tasks,
					  struct sched_wrr_entity, run_list);
		p = wrr_task_of(wrr_se);
	}

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
	struct wrr_rq *wrr_rq = &rq->wrr;

	mark_debug_str_wrr('p');
	printk(KERN_INFO "put_prev_task_wrr rq=%p prev=%p\n", rq, prev);
	if (prev->on_rq) {
		list_add_tail(&prev->wrr.run_list, &wrr_rq->tasks);
	}
}

static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag,
			      int flags)
{
	mark_debug_str_wrr('s');
	printk(KERN_INFO "select_task_rq_wrr p=%p cpu=%d sd_flag=%d flags=%d\n",
	       p, cpu, sd_flag, flags);
	return cpu;
}

static void task_woken_wrr(struct rq *rq, struct task_struct *p)
{
	mark_debug_str_wrr('w');
}

static void switched_from_wrr(struct rq *rq, struct task_struct *p)
{
	mark_debug_str_wrr('f');
}

static void set_curr_task_wrr(struct rq *rq)
{
	mark_debug_str_wrr('s');
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	static int counter = 0;

	mark_debug_str_wrr('k');
	printk(KERN_INFO "task_tick_wrr rq=%p p=%p queued=%d\n", rq, p, queued);

	if (counter++ >= 1000) {
		counter = 0;
		list_add_tail(&p->wrr.run_list, &wrr_rq->tasks);
		resched_curr(rq);
	}
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	return 0;
}

static void update_curr_wrr(struct rq *rq)
{
}

const struct sched_class wrr_sched_class = {
	.next = &fair_sched_class,
	.enqueue_task = enqueue_task_wrr,
	.dequeue_task = dequeue_task_wrr,
	.yield_task = yield_task_wrr,

	.check_preempt_curr = check_preempt_curr_wrr,

	.pick_next_task = pick_next_task_wrr,
	.put_prev_task = put_prev_task_wrr,

#ifdef CONFIG_SMP
	.select_task_rq = select_task_rq_wrr,
	.set_cpus_allowed = set_cpus_allowed_common,
#endif

	.set_curr_task = set_curr_task_wrr,
	.task_tick = task_tick_wrr,

	.prio_changed = prio_changed_wrr,
	.switched_to = switched_to_wrr,

	.get_rr_interval = get_rr_interval_wrr,

	.update_curr = update_curr_wrr,
};
