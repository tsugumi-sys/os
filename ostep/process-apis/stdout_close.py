import os
import sys

pid = os.fork()

if pid == 0:
    os.close(1)

    try:
        print("child: this goes to stdout")
        sys.stdout.flush()
    except OSError as e:
        os.write(2, f"child: print to closed stdout failed: {e}\n".encode())

    os._exit(0)
else:
    os.waitpid(pid, 0)
    print("parent: child finished")
