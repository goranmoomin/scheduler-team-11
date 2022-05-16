# Project 3: Bye, Scheduler!

## Building the project

## Implementation overview

The following files were modified:

- `include/linux/sched.h`: Add
- `include/linux/sched/wrr.h`:
- `include/linux/syscalls.h`, `include/uapi/asm-generic/unistd.h`:
  Assign the new syscalls `sched_getweight` and `sched_setweight`
`include/uapi/linux/sched.h`
`init/init_task.c`
- `kernel/sched/core.c`, `kernel/sched/rt.c`: Replace the fair
  scheduler to WRR

- `kernel/sched/sched.h`:
- `kernel/sched/wrr.c`: TODO: Bulk of the logic


The following data structures were defined:

- `struct sched_wrr_entity`: Task specific state, embedded as `.wrr`
  in `struct task_struct`.
- `struct wrr_rq`: Per-CPU WRR runqueue, embedded as `.wrr` in `struct rq`.

The overall scheduler implementation works as described below:

- The WRR runqueue manages all enqueued tasks, including the running
  task as one linked list.
- Each task has it’s own time slice state, and the scheduler
  decrements it on each timer tick.
- Once the time slice is all used, the scheduler resets its slice and
  move it to the end of the list.
- `enqueue` and `dequeue` are simple linked list `add_tail`, `del`
  operations.
- Since the `.tasks` linked list is managed so that the current task is
  at the front, `pick_next` is a simple `first_entry` operation.

Below are additional explanations of the details.

### Implementation of the `sched_setweight` syscall

TODO: Mention that weight is protected by both the runqueue lock and
task pi_lock; both are needed to cover all cases. Mention about the
`.on_rq` property, and modifying total weight only in the case the
task is on the runqueue.

### Implementation of load balancing

Determining the CPU with max and min weight is not critical, it does
not need to be exact. Instead of locking all runqueues on every load
balancing attempt, the implementation reads the weight directly
without locks with `READ_ONCE`.

Once the CPUs are determined, the runqueues are locked with
`double_rq_lock`, and the implementation picks the task for migration
(the task with biggest weight that is runnable on the target cpu and
is not currently running). We move the task’s `sched_wrr_entity`
between the two WRR runqueues, and reschedule the target CPU so that
it can run the added task if there is no tasks running.

Note that since the implementation holds the original runqueue lock,
it doesn’t need to hold the task’s `pi_lock`; the runqueue lock is
sufficient.

The load balancer logic gets triggered by a timer every two seconds.
This timer is initialized with the init call mechanism. Also note that
the timer runs with interrupts disabled; this is required for the
timer callback to be able to safely lock the run queue.

### Replacing the CFS scheduler

Instead of hackily redirecting attempts to use the CFS scheduler, the
implementation assigns the WRR scheduler directly without changing the
task policy. Hence all `SCHED_NORMAL` tasks TODO
