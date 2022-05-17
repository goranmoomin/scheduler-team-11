# Project 3: Bye, Scheduler!

## Building the project

The project can be built in a conventional way; the usual sequence of
`build-rpi3.sh`, `./setup-images.sh`, and `./qemu.sh` builds and boot
up from the built kernel and the Tizen userspace.

The test programs in the `test` directory must be compiled with
`aarch64-linux-gnu-gcc` with the `-static` option, as we have
implemented the syscalls as 64-bit only.

## Implementation overview

The following files were modified:

- `kernel/sched/sched.h`: Define the WRR runqueue `struct wrr_rq` and
  add it as property `.wrr` in the runqueue `struct rq`.
- `include/linux/sched.h`: Define `struct sched_wrr_entity` and add it
  as property `.wrr` in `struct task_struct`.
- `init/init_task.c`: Add WRR-specific initialization to `init_task`.

- `kernel/sched/wrr.c`: Define the scheduler class `wrr_sched_class`
  and implement functions of it.
- `kernel/sched/core.c`, `kernel/sched/rt.c`: Replace the fair
  scheduler class to WRR (`&wrr_sched_class`).

- `include/linux/sched/wrr.h`, `include/uapi/linux/sched.h`: Add
  constants and helpers for the WRR scheduler.
- `include/linux/syscalls.h`, `include/uapi/asm-generic/unistd.h`:
  Assign the new syscalls `sched_getweight` and `sched_setweight`.

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

### WRR scheduler entity `struct sched_wrr_entity`

The WRR scheduler entity stores task-specific state, in particular the
weight, the time slice, and whether it is on the WRR runqueue or not.

The mutable state is protected by either the runqueue lock the task
resides on if it is runnable, or the `pi_lock` that the task has.

The syscall `sched_setweight` takes advantage of the helper function
`task_rq_lock` that locks both of the locks properly, including saving
interrupt state.

Note that since the runqueue is shared globally, the syscall must
disable interrupts and prevent hardware interrupt handlers to try
locking the runqueue, leading to deadlocks.

### WRR runqueue `struct wrr_rq`

The WRR runqueue stores CPU-specific state, in particular the total
weight of the current enqueued WRR tasks, and the enqueued task list
itself. The `.total_weight` property is manually synchronized on every
task enqueue, dequeue, and weight change; the syscall
`sched_setweight` must check whether the target task is enqueued in a
specific runqueue, and update `.total_weight` of the WRR runqueue.

The task list is ordered as a simple FIFO. The first task is always
the task that will be running on the next tick; as soon as the time
slice gets used, the task is moved to the tail of the task list.

The WRR runqueue is protected by the runqueue lock; it does not have
a separate lock. Mutating the WRR runqueue requires the runqueue lock
that it resides to. Iterating the WRR runqueue task list also requires
the runqueue lock.

However, since `.total_weight` is a 32-bit sized integer, reading it
is atomic. The scheduler takes advantage of this, and avoids locking
just to read `.total_weight` in places where exactness is not needed.

### Implementation of load balancing

Determining the CPU with max and min weight is not critical, it does
not need to be exact. Instead of locking all runqueues on every load
balancing attempt, the implementation reads the weight directly
without locks with `READ_ONCE`. As mentioned above, reading the weight
is atomic; no value-tearing can happen.

Once the CPUs are determined, the runqueues are locked with
`double_rq_lock`, and the implementation picks the task for migration
(the task with biggest weight that is runnable on the target cpu and
is not currently running). We move the task’s `sched_wrr_entity`
between the two WRR runqueues, and reschedule the target CPU so that
it can run the added task if there is no tasks running.

Note that since the implementation holds the lock of the original
runqueue the task once resided, it doesn’t need to hold `pi_lock`.

The load balancer logic gets triggered by a timer every two seconds.
This timer is initialized with the init call mechanism. Also note that
the timer runs with interrupts disabled; this is required for the
timer callback to be able to safely lock the run queue.

### Replacing the CFS scheduler

Instead of hackily redirect attempts to use the CFS scheduler, the
implementation assigns the WRR scheduler directly without changing the
task policy. Hence all `SCHED_NORMAL` tasks are also considered WRR;
WRR-specific syscalls also work on them.
