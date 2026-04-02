import os
import time

OUTPUT_FILE = "fork_shared_fd.txt"


def main() -> None:
    fd = os.open(OUTPUT_FILE, os.O_CREAT | os.O_TRUNC | os.O_WRONLY, 0o644)

    print("opened file descriptor:", fd, "in pid:", os.getpid())

    pid = os.fork()

    role = "child" if pid == 0 else "parent"
    other_pid = os.getppid() if pid == 0 else pid

    for i in range(5):
        line = (
            f"{role} write {i} | pid={os.getpid()} | "
            f"other_pid={other_pid} | fd={fd}\n"
        )
        os.write(fd, line.encode())
        time.sleep(
            0.05
        )  # If we comment out this line, the whole write operations will be executed then the context is switched to the child process!

    os.close(fd)

    if pid == 0:
        os._exit(0)

    os.waitpid(pid, 0)

    print("\nfile contents:")
    with open(OUTPUT_FILE, "r", encoding="utf-8") as f:
        print(f.read(), end="")


if __name__ == "__main__":
    main()
