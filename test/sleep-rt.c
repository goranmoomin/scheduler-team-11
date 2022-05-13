#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_RR 2

int main(void)
{
	struct sched_param param = { .sched_priority = 50 };
	int err = sched_setscheduler(0, SCHED_RR, &param);
	unsigned int cpu;

	if (err) {
		perror("failed to set policy to SCHED_RR");
		return err;
	}

	while (1) {
		sleep(1);
	}

	return 0;
}
