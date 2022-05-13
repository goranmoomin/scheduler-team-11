#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7

#define NR_SCHED_SET_WEIGHT 294
#define NR_SCHED_GET_WEIGHT 295

int main(void)
{
	struct sched_param param = { .sched_priority = 0 };
	int err = sched_setscheduler(0, SCHED_WRR, &param);
	if (err) {
		perror("failed to set policy to SCHED_WRR");
		return err;
	}

	long ret = syscall(NR_SCHED_SET_WEIGHT, 0, 20);
	if (ret < 0) {
		perror("failed to set weight to 20");
		return ret;
	}

	ret = syscall(NR_SCHED_GET_WEIGHT, 0);
	if (ret < 0) {
		perror("failed to get weight");
		return ret;
	}

	printf("sched_get_weight: %ld\n", ret);

	return 0;
}
