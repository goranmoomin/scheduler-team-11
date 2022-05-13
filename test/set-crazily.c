#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NR_SCHED_SET_WEIGHT 294

int getcpu(unsigned int *cpu, unsigned int *node);

int main(void)
{
	int weight = 10;
	while (1) {
		syscall(NR_SCHED_SET_WEIGHT, 0, weight);
		weight = weight % 20 + 1;
	}

	return 0;
}
