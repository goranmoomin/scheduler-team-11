#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#define NR_SCHED_SET_WEIGHT 294

int getcpu(unsigned int *cpu, unsigned int *node);

int main(void)
{
	int r;
	int weight;
	unsigned int cpu;
	while (1) {
		r = rand() % 3;
		switch (r) {
		case 0:
			getcpu(&cpu, NULL);
			break;
		case 1:
			weight = rand() % 20 + 1;
			syscall(NR_SCHED_SET_WEIGHT, 0, weight);
			break;
		case 2:
			sched_yield();
			break;
		default:
			break;
		}
	}

	return 0;
}
