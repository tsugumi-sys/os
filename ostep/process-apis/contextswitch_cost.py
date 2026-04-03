import argparse
import os
import statistics
import time


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Measure the approximate cost of a process context switch."
    )
    parser.add_argument(
        "-n",
        "--iterations",
        type=int,
        default=100_000,
        help="number of ping-pong round trips to measure (default: 100,000)",
    )
    parser.add_argument(
        "-r",
        "--repeats",
        type=int,
        default=7,
        help="number of benchmark samples to collect (default: 7)",
    )
    return parser.parse_args()


def run_ping_pong(iterations: int) -> float:
    parent_read_fd, child_write_fd = os.pipe()
    child_read_fd, parent_write_fd = os.pipe()
    pid = os.fork()

    if pid == 0:
        try:
            os.close(parent_read_fd)
            os.close(parent_write_fd)

            for _ in range(iterations):
                os.read(child_read_fd, 1)
                os.write(child_write_fd, b"x")
        finally:
            os.close(child_read_fd)
            os.close(child_write_fd)
            os._exit(0)

    os.close(child_read_fd)
    os.close(child_write_fd)

    try:
        start = time.perf_counter_ns()
        for _ in range(iterations):
            os.write(parent_write_fd, b"x")
            os.read(parent_read_fd, 1)
        end = time.perf_counter_ns()
    finally:
        os.close(parent_read_fd)
        os.close(parent_write_fd)
        os.waitpid(pid, 0)

    # Each round trip includes two forced context switches:
    # parent -> child and child -> parent.
    return (end - start) / (iterations * 2)


def benchmark(iterations: int, repeats: int) -> list[float]:
    samples = []
    for _ in range(repeats):
        samples.append(run_ping_pong(iterations))
    return samples


def main() -> None:
    args = parse_args()
    samples = benchmark(args.iterations, args.repeats)

    mean_ns = statistics.mean(samples)
    stddev_ns = statistics.pstdev(samples)

    print("context switch benchmark: parent/child pipe ping-pong")
    print(f"iterations/sample: {args.iterations:,} round trips")
    print(f"repeats: {args.repeats}")
    print(f"mean: {mean_ns:.1f} ns/context switch")
    print(f"stddev: {stddev_ns:.1f} ns/context switch")
    print()
    print(
        "Note: this estimate includes pipe read/write syscall overhead in addition "
        "to scheduling latency, so it is an upper bound on pure context-switch cost."
    )


if __name__ == "__main__":
    main()
