#include "sched.h"

#define WRR_TIMER_DELAY_MS 2000

char _debug_str_wrr[256];
int _debug_str_wrr_idx = 0;

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

static inline struct rq *rq_of_wrr_se(struct sched_wrr_entity *wrr_se)
{
	struct task_struct *p = wrr_task_of(wrr_se);
	return task_rq(p);
}

void wrr_timer_callback(struct timer_list *);

static struct timer_list wrr_timer;

void wrr_timer_callback(struct timer_list *timer) {
	struct rq *rq0, *rq1;
	struct sched_wrr_entity *wrr_se;
	struct task_struct *p = NULL;

	rq0 = cpu_rq(0);
	rq1 = cpu_rq(1);

	double_rq_lock(rq0, rq1);

	if (!list_empty(&rq0->wrr.tasks)) {
		wrr_se = list_last_entry(&rq0->wrr.tasks, struct sched_wrr_entity, run_list);
		p = wrr_task_of(wrr_se);
		if (p == rq0->curr) {
			p = NULL;
		}
	}

	if (p) {
		deactivate_task(rq0, p, 0);
		set_task_cpu(p, 1);
		activate_task(rq1, p, 0);
		resched_curr(rq0);
	}

	double_rq_unlock(rq0, rq1);
	mod_timer(&wrr_timer, jiffies + msecs_to_jiffies(WRR_TIMER_DELAY_MS));
}

inline static void mark_debug_str_wrr(char c)
{
	_debug_str_wrr[_debug_str_wrr_idx++ % 256] = c;
}

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
	INIT_LIST_HEAD(&wrr_rq->tasks);
	wrr_rq->total_weight = 0;
}


static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_se = &p->wrr;

	mark_debug_str_wrr('e');
	
	printk(KERN_INFO "enqueue_task_wrr rq=%px p=%px flags=%d\n", rq, p,
	       flags);

	list_add_tail(&wrr_se->run_list, &wrr_rq->tasks);
	wrr_se->on_rq = 1;
	add_nr_running(rq, 1);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	mark_debug_str_wrr('d');

	printk(KERN_INFO "dequeue_task_wrr rq=%px p=%px flags=%d\n", rq, p,
	       flags);

	list_del(&wrr_se->run_list);
	wrr_se->on_rq = 0;
	sub_nr_running(rq, 1);
}

static void yield_task_wrr(struct rq *rq)
{
	mark_debug_str_wrr('y');
	printk(KERN_INFO "yield_task_wrr rq=%px\n", rq);
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

	printk(KERN_INFO "pick_next_task_wrr rq=%px prev=%px rf=%px\n", rq,
	       prev, rf);

	put_prev_task(rq, prev);

	wrr_se = list_first_entry_or_null(&wrr_rq->tasks,
				  struct sched_wrr_entity, run_list);
	if (wrr_se) {
		p = wrr_task_of(wrr_se);
	}

	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
	struct wrr_rq *wrr_rq = &rq->wrr;

	mark_debug_str_wrr('p');
	printk(KERN_INFO "put_prev_task_wrr rq=%px prev=%px\n", rq, prev);

	if (prev->wrr.on_rq) {
		list_move_tail(&prev->wrr.run_list, &wrr_rq->tasks);
	}
}

static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag,
			      int flags)
{
	mark_debug_str_wrr('s');
	printk(KERN_INFO
	       "select_task_rq_wrr p=%px cpu=%d sd_flag=%d flags=%d\n",
	       p, cpu, sd_flag, flags);
	return cpu;
}

static void set_curr_task_wrr(struct rq *rq)
{
	mark_debug_str_wrr('s');
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct wrr_rq *wrr_rq = &rq->wrr;

	mark_debug_str_wrr('k');
	printk(KERN_INFO "task_tick_wrr rq=%px p=%px queued=%d\n", rq, p,
	       queued);

	if (--p->wrr.time_slice)
		return;

	p->wrr.time_slice = WRR_BASE_TIMESLICE * p->wrr.weight;
	if (p->wrr.on_rq) {
		list_move_tail(&p->wrr.run_list, &wrr_rq->tasks);
	}

	resched_curr(rq);
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

void init_sched_wrr_class(void) {
	timer_setup(&wrr_timer, wrr_timer_callback, TIMER_IRQSAFE);
	mod_timer(&wrr_timer, jiffies + msecs_to_jiffies(WRR_TIMER_DELAY_MS));
}
early_initcall(init_sched_wrr_class);
