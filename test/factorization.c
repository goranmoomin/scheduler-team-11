#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define SCHED_WRR 7

#define NR_SCHED_SET_WEIGHT 294
#define NR_SCHED_GET_WEIGHT 295

int getcpu(unsigned int *cpu, unsigned int *node);

int main(int argc, char **argv)
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

	int weight;

	for (weight = 1; weight <= 20; weight++) {
		pid_t pid = fork();
		if (pid == -1) {
			perror("failed to fork");
			return -1;
		} else if (pid == 0) {
			long ret = syscall(NR_SCHED_SET_WEIGHT, 0, weight);
			if (ret < 0) {
				perror("failed to set weight");
				return ret;
			}
			printf("set weight to %d\n", weight);
			break;
		} else {
			if (weight == 20) {
				printf("parent has finished it's job\n");
				return 0;
			}
			continue;
		}
	}

	struct timespec start;

	if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
		perror("clock_gettime error");
		exit(EXIT_FAILURE);
	}
	intmax_t cnt;

	for (int val = 2; val <= 997 * 991; val++) {
		int tmp = val;
		for (int i = 2; i * i <= tmp;) {
			if (tmp % i) {
				i++;
			} else {
				tmp /= i;
			}
		}
		cnt += tmp;
	}

	struct timespec end;

	if (clock_gettime(CLOCK_REALTIME, &end) == -1) {
		perror("clock_gettime error");
		exit(EXIT_FAILURE);
	}

	printf("weight=%d duration=%f cnt=%jd\n", weight,
	       (double)(end.tv_sec + (double)end.tv_nsec / 1000000000) -
		       (double)(start.tv_sec +
				(double)start.tv_nsec / 1000000000),
	       cnt);

	return 0;
}
