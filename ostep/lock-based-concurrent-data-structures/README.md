# Lock-Based Concurrent Queue Benchmark

This project compares two linked-list queue implementations under concurrent load:

- `global-lock`: one mutex protects both enqueue and dequeue.
- `two-lock`: separate enqueue and dequeue mutexes, using an atomic `next` pointer so the empty-queue boundary is safe in Go.

Run the default benchmark:

```sh
go run .
```

Default operation counts are:

```text
10000,20000,50000,100000,1000000
```

Use custom worker/thread counts:

```sh
go run . -workers 1,2,4,8
```

Run only the mixed insert/delete workload:

```sh
go run . -workloads mixed -workers 1,2,4,8 -repeats 5
```

The output is CSV:

```text
queue,workload,operations,workers,repeat,duration_ms,throughput_ops_sec,enqueues,dequeues,empty_dequeues,value_sample
```

`throughput_ops_sec` is based on requested operations. `empty_dequeues` should normally be zero for the default `dequeue` and `mixed` workloads because the queue is prefilled before timing starts.
