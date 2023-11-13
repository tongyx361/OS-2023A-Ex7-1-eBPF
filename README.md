# OS-Ex7.1 eBPF Report

> [课后练习](https://www.yuque.com/xyong-9fuoz/qczol5/uzf18vbnscar3hzi#xrhqT)
> “[wakeuptime.py](https://github.com/iovisor/bcc/blob/master/tools/wakeuptime.py)”是一个 eBPF 应用，可以动态进程执行过程中的进入等待和等待事件出现等相关信息。“[wakeuptime_example.txt](https://github.com/iovisor/bcc/blob/master/tools/wakeuptime_example.txt)”这是这个小工具的使用说明和示例。请依据自己的兴趣和时间情况，选择完成如下部分或全部任务。
>
> 1. 安装 eBPF 工具链，并确信使用说明中的示例是工作；描述安装过程中遇到的问题和解决方法。
> 2. 在 MacOS 或 Linux 平台上运行课程中的“[循环创建进程示例](http://learningos.cn/os-lectures/lec7/p1-process-overview.html#39)”，用“[wakeuptime.py](https://github.com/iovisor/bcc/blob/master/tools/wakeuptime.py)”跟踪示例运行过程；解释跟踪等待事件对应的示例代码。
> 3. “[example](https://github.com/zoidbergwill/awesome-ebpf#examples)”目录中有多种基于 eBPF 的动态跟踪小工具，选一个有兴趣的工具，并跟踪相关的内核事件。

## 安装 eBPF 工具链

### WSL2 (Ubuntu 18.04) 安装

根据 [在 WSL2 环境下安装 BPF 工具链 | 时间之外，地球往事](https://oftime.net/2021/01/16/win-bpf/)，需要下载 WSL2 Linux 内核源码，从其中安装头文件。过于复杂，故放弃。

### VMWare (Ubuntu 22.04) 安装

#### 使用 Ubuntu 的包管理器安装

根据 [INSTALL.md](https://github.com/iovisor/bcc/blob/master/INSTALL.md)，（配置好虚拟机上的网络后）输入

```Shell
sudo apt-get install bpfcc-tools linux-headers-$(uname -r)
```

即可安装。

安装完成后，`bcc` 工具会放到 `/usr/share/bcc/tools` 目录中。

输入 `<tool>-bpfcc` 即可使用。

#### 根据源码安装

在官方的 [GitHub issue](https://github.com/iovisor/bcc/issues/3348) 里提到，推荐的安装方法是通过源码进行编译安装，因为 `repo.iovisor.org` 上的版本老旧，且存在 bug。具体方法此处省略。

## 运行课程示例并使用 `wakeuptime` 跟踪

### 课程示例

课程中的“[循环创建进程示例](http://learningos.cn/os-lectures/lec7/p1-process-overview.html#39)”内容为

```C
int main()
{
	pid_t pid;
	int i;
	for (i = 0; i < LOOP; i++) {
		/* fork  another  process  */
		pid = fork();
		if (pid < 0) { /*error  occurred  */
			fprintf(stderr, “Fork Failed”);
			exit(-1);
		} else if (pid == 0) { /* child process */
			fprintf(stdout, “i = % d, pid = % d,
				parent pid = % d\n”, I, getpid(), getppid());
		}
	}
	wait(NULL);
	exit(0);
}
```

修改为在正常 C 环境下可运行的形式，详见 `fork-eg.c`

```C
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOOP 5 // 定义LOOP的值，例如5

int main() {
    pid_t pid;
    int i;
    for (i = 0; i < LOOP; i++) {
        /* fork another process */
        pid = fork();
        if (pid < 0) { /* error occurred */
            fprintf(stderr, "Fork Failed\n");
            exit(-1);
        } else if (pid == 0) { /* child process */
            fprintf(stdout, "i = %d, pid = %d, parent pid = %d\n", i, getpid(), getppid());
            exit(0); // 确保子进程在输出后退出
        }
    }

    // 父进程等待所有子进程结束
    for (i = 0; i < LOOP; i++) {
        wait(NULL);
    }

    exit(0);
}
```

### 运行并使用 `wakeuptime` 跟踪

`gcc` 编译后，输入以下命令执行并跟踪

```Shell
/a.out &; sudo wakeuptime-bpfcc -p $!
```

对应输出为

```
    target:          a.out
    ffffffffafa0007c entry_SYSCALL_64_after_hwframe
    ffffffffaf94ae7c do_syscall_64
    ffffffffaecbd7f8 __x64_sys_exit_group
    ffffffffaecbd74b do_group_exit
    ffffffffaecbd543 do_exit
    ffffffffaecbbd1b exit_notify
    ffffffffaeccb1e4 do_notify_parent
    ffffffffaecbd821 __wake_up_parent
    ffffffffaed1cb50 __wake_up_sync_key
    ffffffffaed1c92c __wake_up_common_lock
    ffffffffaed1c7ed __wake_up_common
    ffffffffaecbaf23 child_wait_callback
    waker:           a.out
        12879
```

### 解释输出

1. **`entry_SYSCALL_64_after_hwframe` 和 `do_syscall_64`：** 这些函数是内核中的系统调用入口路径的一部分。当程序调用 `fork()`、`wait()` 或 `exit()` 时，它会进行系统调用，从这一点进入内核。
2. **`__x64_sys_exit_group` 和 `do_exit`：** 这些函数涉及到进程退出时的操作，对应于子进程中的 `exit(0);`。`__x64_sys_exit_group` 是终止进程组中所有线程的系统调用，而 `do_exit` 执行实际的进程终止工作。
3. **`exit_notify` 和 `do_notify_parent`：** 这些函数在进程退出并需要通知其父进程时被调用。在程序中，每个子进程在退出时通知其父进程（主进程）。
4. **`__wake_up_parent`：** 此函数唤醒等待其子进程退出的父进程。在程序中，这对应于父进程中的 `wait(NULL);` 循环。这个循环的每次迭代都等待一个子进程的退出。
5. **`__wake_up_sync_key`、`__wake_up_common_lock` 和 `__wake_up_common`：** 这些是用于唤醒进程的更低级别的函数。它们处理唤醒休眠进程的实际机制，而在情况下，这是父进程在等待 `wait()` 时的情况。
6. **`child_wait_callback`：** 这可能是在子进程状态发生变化时调用的回调函数，例如当子进程退出时。在程序中，这与父进程被通知子进程退出相关。

## 其它工具：`tcptop`

`tcptop` 按主机和端口汇总吞吐量，详见 [tcptop\_example.txt](https://github.com/iovisor/bcc/blob/master/tools/tcptop_example.txt)

```
 PID    COMM         LADDR               RADDR               RX_KB  TX_KB
14237   firefox   192.168.0.103:33990   14.215.177.38:443   7      2
14237   firefox   192.168.0.103:42368   14.215.177.39:443   4      1
```

可以看到 `tcptop` 抓取到了 tcp 报文。
