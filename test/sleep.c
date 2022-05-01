#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7

int main(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int err = sched_setscheduler(getpid(), SCHED_WRR, &param);
	if (err) {
		perror("failed to set policy to SCHED_WRR");
		return err;
	}

	for (int i = 0; i < 5; i++) {
		printf("hello, wrr!\n");
		sleep(1);
	}

	return 0;
}
