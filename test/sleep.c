#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7

int main(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int err = sched_setscheduler(getpid(), SCHED_WRR, &param);
	int cpu;
	if (err) {
		perror("failed to set policy to SCHED_WRR");
		return err;
	}


	while(1) {
		err = getcpu(&cpu, NULL);
		if (err || cpu)
			return err;
		printf("hello, wrr %d!\n", cpu);
		sleep(1);
	}

	return 0;
}
