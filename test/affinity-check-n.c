#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#define SCHED_WRR 7
int getcpu(unsigned int *cpu, unsigned int *node);

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s n\n", argv[0]);
        return -1;
    }
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

    int i;
    for (i = 0; i < atoi(argv[1]); i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("failed to fork");
            return -1;
        } else if (pid == 0) {
	        for (;;) {
		        err = sched_yield();
		        if (err) {
			        perror("failed to yield");
		        }
	        }
        }
    }

	return 0;
}
