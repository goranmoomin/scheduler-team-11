#include "linux/list.h"
#include "sched.h"

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
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr;

	list_add(&wrr_se->run_list, &wrr_rq->tasks);
	add_nr_running(rq, 1);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;

	list_del(&wrr_se->run_list);
	sub_nr_running(rq, 1);
}

static void yield_task_wrr(struct rq *rq)
{
}

static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p,
			      bool preempt)
{
	return false;
}

static struct task_struct *
pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	return NULL;
}

static void put_prev_task_fair(struct rq *rq, struct task_struct *prev)
{
}

const struct sched_class wrr_sched_class = {
	.next = &fair_sched_class,
	.enqueue_task = enqueue_task_wrr,
	.dequeue_task = dequeue_task_wrr,
	.yield_task = yield_task_wrr,
	.yield_to_task = yield_to_task_wrr,

	.pick_next_task = pick_next_task_wrr,
	.put_prev_task = put_prev_task_fair,
};
