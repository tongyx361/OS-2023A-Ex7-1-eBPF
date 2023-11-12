#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOOP 100 // 定义LOOP的值，例如5

int main()
{
	pid_t pid;
	int i;
	for (i = 0; i < LOOP; i++) {
		/* fork another process */
		pid = fork();
		if (pid < 0) { /* error occurred */
			fprintf(stderr, "Fork Failed\n");
			exit(-1);
		} else if (pid == 0) { /* child process */
			fprintf(stdout, "i = %d, pid = %d, parent pid = %d\n",
				i, getpid(), getppid());
			sleep(1); // 等待1秒钟
			exit(0); // 确保子进程在输出后退出
		}
	}

	// 父进程等待所有子进程结束
	for (i = 0; i < LOOP; i++) {
		wait(NULL);
	}

	exit(0);
}