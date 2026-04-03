import argparse
import os
import statistics
import time
from collections.abc import Callable


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Measure the time cost of completing a simple system call."
    )
    parser.add_argument(
        "-n",
        "--iterations",
        type=int,
        default=1_000_000,
        help="number of timed iterations per sample (default: 1,000,000)",
    )
    parser.add_argument(
        "-r",
        "--repeats",
        type=int,
        default=7,
        help="number of benchmark samples to collect (default: 7)",
    )
    parser.add_argument(
        "--syscall",
        choices=("getpid", "fstat", "write"),
        default="fstat",
        help="which syscall-shaped operation to benchmark (default: fstat)",
    )
    return parser.parse_args()


def measure_average_ns(fn: Callable[[], None], iterations: int) -> float:
    start = time.perf_counter_ns()
    for _ in range(iterations):
        fn()
    end = time.perf_counter_ns()
    return (end - start) / iterations


def empty_operation() -> None:
    return None


def benchmark(fn: Callable[[], None], iterations: int, repeats: int) -> list[float]:
    # Warm up the Python and libc call paths before measuring.
    for _ in range(10_000):
        fn()

    samples = []
    for _ in range(repeats):
        samples.append(measure_average_ns(fn, iterations))
    return samples


def main() -> None:
    args = parse_args()

    devnull_fd = os.open("/dev/null", os.O_WRONLY)

    try:
        if args.syscall == "getpid":
            syscall_name = "os.getpid()"

            def syscall_operation() -> None:
                os.getpid()

        elif args.syscall == "fstat":
            syscall_name = "os.fstat(devnull_fd)"

            def syscall_operation() -> None:
                os.fstat(devnull_fd)

        else:
            syscall_name = "os.write(devnull_fd, b'x')"

            def syscall_operation() -> None:
                os.write(devnull_fd, b"x")

        baseline_samples = benchmark(empty_operation, args.iterations, args.repeats)
        syscall_samples = benchmark(syscall_operation, args.iterations, args.repeats)
    finally:
        os.close(devnull_fd)

    baseline_mean = statistics.mean(baseline_samples)
    baseline_stddev = statistics.pstdev(baseline_samples)
    syscall_mean = statistics.mean(syscall_samples)
    syscall_stddev = statistics.pstdev(syscall_samples)
    adjusted_mean = syscall_mean - baseline_mean

    print(f"syscall benchmark: {syscall_name}")
    print(f"iterations/sample: {args.iterations:,}")
    print(f"repeats: {args.repeats}")
    print(f"baseline mean: {baseline_mean:.1f} ns/call")
    print(f"baseline stddev: {baseline_stddev:.1f} ns/call")
    print(f"measured mean: {syscall_mean:.1f} ns/call")
    print(f"measured stddev: {syscall_stddev:.1f} ns/call")
    print(f"adjusted mean: {adjusted_mean:.1f} ns/call")
    print()
    print(
        "Note: this is a Python-level benchmark. The adjusted mean is a rough estimate "
        "of syscall completion cost after subtracting loop overhead."
    )
    if args.syscall == "getpid":
        print("Note: getpid() may be optimized by libc and can under-report kernel entry cost.")


if __name__ == "__main__":
    main()
