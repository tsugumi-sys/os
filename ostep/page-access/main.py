import random
import time

PAGE_SIZE = 16384
STRIDE = PAGE_SIZE // 8  # int64


def benchmark(num_pages):
    size = num_pages * STRIDE
    arr = [0] * size

    pages = list(range(num_pages))
    random.shuffle(pages)

    start = time.perf_counter_ns()

    s = 0
    for _ in range(10):
        for p in pages:
            s += arr[p * STRIDE]

    end = time.perf_counter_ns()

    return (end - start) / (10 * num_pages)


for pages in [32, 64, 128, 256, 512, 1024, 2048]:
    ns = benchmark(pages)
    print(pages, ns)
