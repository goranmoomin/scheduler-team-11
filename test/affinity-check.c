#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7
int getcpu(unsigned int *cpu, unsigned int *node);

int main(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int err = sched_setscheduler(0, SCHED_WRR, &param);

	if (err) {
		perror("failed to set policy to SCHED_WRR");
		return err;
	}

    unsigned int cpu;
    err = getcpu(&cpu, NULL);
    if (err) {
        perror("failed to get current cpu");
        return err;
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    err = sched_setaffinity(0, sizeof(set), &set);
    if (err) {
        perror("failed to set cpu affinity");
        return err;
    }


	for (;;) {
		err = sched_yield();
		if (err) {
			perror("failed to yield");
		}
	}

	return 0;
}
