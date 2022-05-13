#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>

#define NR_SCHED_SET_WEIGHT 294

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s count\n", argv[0]);
		return -1;
	}
	int count = atoi(argv[1]);
	pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * count);
	for (int i = 0; i < count; i++) {
		pid_t pid = fork();
		if (pid < 0) {
			perror("failed to fork");
			return -1;
		} else if (pid == 0) {
			for (;;) {
				write(STDIN_FILENO, "", 0);
			}
			return 0;
		} else {
			pids[i] = pid;
		}
	}
	for (int i = 0;; i++) {
		pid_t pid = pids[i % count];
		long ret = syscall(NR_SCHED_SET_WEIGHT, pid, i % 20 + 1);
		if (ret < 0) {
			perror("failed to set weight");
			return ret;
		}
	}

	return 0;
}
