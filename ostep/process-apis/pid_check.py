import os

pid = os.fork()

x = 1

if pid == 0:
    x = 10
    # 子プロセス
    print(
        "child process, pid:",
        os.getpid(),
        "parent pid:",
        os.getppid(),
        "fork return:",
        pid,
    )
    print("x in child:", x)
else:
    # 親プロセス
    x = 100
    print("parent process, pid:", os.getpid(), "child pid:", pid)
    print("x in parent:", x)
