#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7
int getcpu(unsigned int *cpu, unsigned int *node);

int main(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int err = sched_setscheduler(0, SCHED_WRR, &param);
	unsigned int cpu;

	if (err) {
		perror("failed to set policy to SCHED_WRR");
		return err;
	}

	while (1) {
		err = getcpu(&cpu, NULL);
		if (err) {
			perror("failed to get current cpu number");
			return err;
		}
		if (cpu >= 2) {
			return 0;
		}
		printf("hello, wrr %d!\n", cpu);
		sleep(1);
	}

	return 0;
}
