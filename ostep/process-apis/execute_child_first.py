import os

read_fd, write_fd = os.pipe()
pid = os.fork()

if pid == 0:
    os.close(read_fd)
    os.write(1, b"hello\n")
    # Signal the parent to continue, without using wait().
    os.write(write_fd, b"x")
    os.close(write_fd)
else:
    os.close(write_fd)
    # Block until the child signals that its work is done.
    os.read(read_fd, 1)
    os.close(read_fd)
    os.write(1, b"goodbye\n")
