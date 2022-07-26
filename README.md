# Project 3: Bye, Scheduler!

## Building the project

The project can be built conventionally; the usual sequence of
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

- `struct sched_wrr_entity`: Task-specific state, embedded as `.wrr`
  in `struct task_struct`.
- `struct wrr_rq`: Per-CPU WRR runqueue, embedded as `.wrr` in `struct rq`.

The overall scheduler implementation works as described below:

```
              ┌─────────┐
    task_tick │  timer  │
    ┌─────────│interrupt│   put_prev
    │         └─────────┘  yield_task
    │  ┌───────────────────────────────────────┐
 ┌ ─│─ ┼ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┼ ─ ─ ┐
    ▼  │                                       ▼
 │ ┌───┴───┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌ ─ ─ ─ ┐ │
   │       ├┐│       ├┐│       ├┐│       ├┐
 │ │RUNNING│││ READY │││ READY │││ READY │││ READY │ │
   │       │└┤       │└┤       │└┤       │└─
 │ └───┬───┘ └───────┘ └───────┘ └───────┘ └ ─ ─ ─ ┘ │
       │             WRR runqueue              ▲
 └ ─ ─ ┼ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┼ ─ ─ ┘
       ▼                                       │
 dequeue_task                            enqueue_task

```

- The WRR runqueue manages all enqueued tasks, including the running
  task as one linked list.
- Each task has its time slice state, and the scheduler decrements it
  on each timer tick.
- Once the time slice is all used, the scheduler resets its slice and
  move it to the end of the list.
- `enqueue` and `dequeue` are simple linked list `add_tail`, `del`
  operations.
- Since the `.tasks` linked list is managed so that the current task is
  at the front, `pick_next` is a simple `first_entry` operation.

Below are additional explanations of the details.

### WRR scheduler entity `struct sched_wrr_entity`

The WRR scheduler entity stores task-specific states, in particular,
the weight, the time slice, and whether it is on the WRR runqueue or
not.

The mutable state is protected by either the runqueue the task resides
on if it is runnable, or the `pi_lock` that the task has.

The syscall `sched_setweight` takes advantage of the helper function
`task_rq_lock` that locks both of the locks properly, including saving
interrupt state.

Note that since the runqueue is shared globally, the syscall must
disable interrupts and prevent hardware interrupt handlers to try
locking the runqueue, leading to deadlocks.

### WRR runqueue `struct wrr_rq`

The WRR runqueue stores CPU-specific state, in particular, the total
weight of the current enqueued WRR tasks, and the enqueued task list
itself. The `.total_weight` property is manually synchronized on every
task enqueue, dequeue, and weight change; the syscall
`sched_setweight` must check whether the target task is enqueued in a
specific runqueue, and update `.total_weight` of the WRR runqueue.

The task list is ordered as a simple FIFO. The first task is always
the task that will be running on the next tick; as soon as the time
slice gets used, the task is moved to the tail of the task list.

The WRR runqueue is protected by the runqueue lock; it does not have a
separate lock. Mutating the WRR runqueue requires the runqueue lock
that it resides to. Iterating the WRR runqueue task list also requires
the runqueue lock.

However, since `.total_weight` is a 32-bit sized integer, reading it
is atomic. The scheduler takes advantage of this and avoids locking
just to read `.total_weight` in places where exactness is not needed.

### Implementation of load balancing

Determining the CPU with max and min weight is not critical, it does
not need to be exact. Instead of locking all runqueues on every load
balancing attempt, the implementation reads the weight directly
without locks with `READ_ONCE`. As mentioned above, reading the weight
is atomic; no value-tearing can happen.

Once the CPUs are determined, the runqueues are locked with
`double_rq_lock`, and the implementation picks the task for migration
(the task with the biggest weight that is runnable on the target CPU
and is not currently running). We move the task’s `sched_wrr_entity`
between the two WRR runqueues, and reschedule the target CPU so that
it can run the added task if no tasks are running.

Note that since the implementation holds the lock of the original
runqueue the task once resided, it doesn’t need to hold `pi_lock`.
Also note that, since we find the target task after we lock the
runqueue, load balancing cannot race with `sched_setweight`.

The load balancer logic gets triggered by a timer every two seconds.
This timer is initialized with the init call mechanism. Also, note
that the timer runs with interrupts disabled; this is required for the
timer callback to be able to safely lock the run queue.

### Replacing the CFS scheduler

Instead of hackily redirecting attempts to use the CFS scheduler, the
implementation assigns the WRR scheduler directly without changing the
task policy. Hence all `SCHED_NORMAL` tasks are also considered WRR;
WRR-specific syscalls also work on them.

## Testing and Performance

### Test programs

We have written various programs in the `test/` directory. Test
programs are described below.

- `crazy.c`: Infinite loop without any syscalls.
- `kindly-crazy.c`: Infinite loop that yields on every iteration.
- `sleep.c`: Infinite loop that calls syscalls on every iteration.
- `factorization.c`: Fix CPU affinity to current CPU, fork 20
  instances of itself, set its WRR weight and repeatedly factorize
  every number from 1 to 997*991. The program prints how much time the
  factorization has taken.
- `affinity-check.c`: Infinite loop that yields on every iteration,
  but with CPU affinity fixed.

All programs that end with `-n` simply fix CPU affinity to current
CPU, forks `atoi(argv[1])` instances that do the work, and simply
exits.

The reason `*-n` and `factorization` programs fix their affinity is to
prevent inaccurate conclusions based on how the weights get
distributed between different CPUs; the programs are fixed to run in
one CPU hence can compare stats more meaningfully.


### Performance

The following graph depicts how the turnaround time of the
factorization program varies depending on weight. Since the program
fixes its CPU affinity, these processes get CPU time proportionally to
their weight. Indeed, the graph shows that the turnaround time is
quite exactly the inverse of the weight.

![Factorization turnaround time per weight on single CPU](https://user-images.githubusercontent.com/37990858/169116555-55b06740-deaa-4d2a-9265-f1df5ca01127.png)

## Discussion and Lessons

### Discussion

- Efficient runqueue management and faster load balancing are at odds;
  Pre-calculating information for load balancing almost always slows
  down runqueue operations. Our implementation prioritizes `O(1)`
  runqueue operations at the cost of `O(N)` load balancing. In the
  case where load balancing should be more prioritized, we can imagine
  an implementation that uses red-black trees with runqueue operations
  being `O(log N)`; we must consider tradeoffs between different
  implementation strategies.

- Sharing task state is done by either the runqueue lock or `pi_lock`;
  the implementation must carefully consider what it should lock.

### Lessons

- The kernel is sensitive; each missing line translates to quite a few
  hours of debugging.
- But using GDB might reduce that few hours to half or more.
- Evil concurrency issues always stab us in the back, and the kernel
  doesn’t help you anymore.
- Wizardry optimizations for CFS scheduler are cool… until you have to
  understand it.

- The assignment specification might be just annoying… or it might be
  forecasting the troubles we are going through. If an issue is
  holding back for a day or more… maybe the specification might have
  something to say?
