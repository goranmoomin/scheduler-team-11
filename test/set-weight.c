#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>

#define SCHED_WRR 7

#define NR_SCHED_SET_WEIGHT 294
#define NR_SCHED_GET_WEIGHT 295

int getcpu(unsigned int *cpu, unsigned int *node);

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s pid weight\n", argv[0]);
		return -1;
	}

	pid_t pid = atoi(argv[1]);
	int weight = atoi(argv[2]);

	printf("set weight of pid %d to %d\n", pid, weight);
	long ret = syscall(NR_SCHED_SET_WEIGHT, pid, weight);
	if (ret < 0) {
		perror("failed to set weight");
		return ret;
	}

	return 0;
}
