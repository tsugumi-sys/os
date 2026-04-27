package main

import (
	"flag"
	"fmt"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"
)

type queue interface {
	Enqueue(int)
	Dequeue() (int, bool)
}

type globalNode struct {
	value int
	next  *globalNode
}

type globalLockQueue struct {
	mu   sync.Mutex
	head *globalNode
	tail *globalNode
}

func newGlobalLockQueue() *globalLockQueue {
	dummy := &globalNode{}
	return &globalLockQueue{head: dummy, tail: dummy}
}

func (q *globalLockQueue) Enqueue(value int) {
	n := &globalNode{value: value}

	q.mu.Lock()
	q.tail.next = n
	q.tail = n
	q.mu.Unlock()
}

func (q *globalLockQueue) Dequeue() (int, bool) {
	q.mu.Lock()
	defer q.mu.Unlock()

	next := q.head.next
	if next == nil {
		return 0, false
	}

	q.head = next
	return next.value, true
}

type twoLockNode struct {
	value int
	next  atomic.Pointer[twoLockNode]
}

type twoLockQueue struct {
	headMu sync.Mutex
	tailMu sync.Mutex
	head   *twoLockNode
	tail   *twoLockNode
}

func newTwoLockQueue() *twoLockQueue {
	dummy := &twoLockNode{}
	return &twoLockQueue{head: dummy, tail: dummy}
}

func (q *twoLockQueue) Enqueue(value int) {
	n := &twoLockNode{value: value}

	q.tailMu.Lock()
	q.tail.next.Store(n)
	q.tail = n
	q.tailMu.Unlock()
}

func (q *twoLockQueue) Dequeue() (int, bool) {
	q.headMu.Lock()
	defer q.headMu.Unlock()

	next := q.head.next.Load()
	if next == nil {
		return 0, false
	}

	q.head = next
	return next.value, true
}

type benchmarkConfig struct {
	name       string
	workload   string
	operations int
	workers    int
	repeat     int
}

type benchmarkResult struct {
	config      benchmarkConfig
	duration    time.Duration
	throughput  float64
	enqueues    int64
	dequeues    int64
	emptyReads  int64
	valueSample int64
}

func main() {
	var (
		operationList = flag.String("ops", "10000,20000,50000,100000,1000000", "comma-separated operation counts")
		workerList    = flag.String("workers", strconv.Itoa(runtime.GOMAXPROCS(0)), "comma-separated worker/thread counts")
		workloads     = flag.String("workloads", "all", "comma-separated workloads: enqueue,dequeue,mixed,all")
		repeats       = flag.Int("repeats", 3, "number of benchmark repetitions per case")
	)
	flag.Parse()

	ops, err := parsePositiveInts(*operationList)
	if err != nil {
		panic(err)
	}
	workers, err := parsePositiveInts(*workerList)
	if err != nil {
		panic(err)
	}
	selectedWorkloads, err := parseWorkloads(*workloads)
	if err != nil {
		panic(err)
	}

	fmt.Println("queue,workload,operations,workers,repeat,duration_ms,throughput_ops_sec,enqueues,dequeues,empty_dequeues,value_sample")

	for _, workload := range selectedWorkloads {
		for _, operationCount := range ops {
			for _, workerCount := range workers {
				for repeat := 1; repeat <= *repeats; repeat++ {
					printResult(runBenchmark(newGlobalLockQueue(), benchmarkConfig{
						name:       "global-lock",
						workload:   workload,
						operations: operationCount,
						workers:    workerCount,
						repeat:     repeat,
					}))
					printResult(runBenchmark(newTwoLockQueue(), benchmarkConfig{
						name:       "two-lock",
						workload:   workload,
						operations: operationCount,
						workers:    workerCount,
						repeat:     repeat,
					}))
				}
			}
		}
	}
}

func runBenchmark(q queue, config benchmarkConfig) benchmarkResult {
	prefill(q, config)

	var enqueues atomic.Int64
	var dequeues atomic.Int64
	var emptyReads atomic.Int64
	var valueSample atomic.Int64

	startGate := make(chan struct{})
	var wg sync.WaitGroup

	for workerID := 0; workerID < config.workers; workerID++ {
		count := operationsForWorker(config.operations, config.workers, workerID)
		wg.Add(1)

		go func(workerID, count int) {
			defer wg.Done()
			<-startGate

			baseValue := workerID * config.operations
			for i := 0; i < count; i++ {
				switch config.workload {
				case "enqueue":
					q.Enqueue(baseValue + i)
					enqueues.Add(1)
				case "dequeue":
					if value, ok := q.Dequeue(); ok {
						dequeues.Add(1)
						valueSample.Add(int64(value))
					} else {
						emptyReads.Add(1)
					}
				case "mixed":
					if i%2 == 0 {
						q.Enqueue(baseValue + i)
						enqueues.Add(1)
					} else if value, ok := q.Dequeue(); ok {
						dequeues.Add(1)
						valueSample.Add(int64(value))
					} else {
						emptyReads.Add(1)
					}
				}
			}
		}(workerID, count)
	}

	start := time.Now()
	close(startGate)
	wg.Wait()
	duration := time.Since(start)

	return benchmarkResult{
		config:      config,
		duration:    duration,
		throughput:  float64(config.operations) / duration.Seconds(),
		enqueues:    enqueues.Load(),
		dequeues:    dequeues.Load(),
		emptyReads:  emptyReads.Load(),
		valueSample: valueSample.Load(),
	}
}

func prefill(q queue, config benchmarkConfig) {
	prefillCount := 0
	switch config.workload {
	case "dequeue":
		prefillCount = config.operations
	case "mixed":
		prefillCount = config.operations / 2
	}

	for i := 0; i < prefillCount; i++ {
		q.Enqueue(i)
	}
}

func operationsForWorker(total, workers, workerID int) int {
	base := total / workers
	if workerID < total%workers {
		return base + 1
	}
	return base
}

func printResult(result benchmarkResult) {
	fmt.Printf("%s,%s,%d,%d,%d,%.3f,%.0f,%d,%d,%d,%d\n",
		result.config.name,
		result.config.workload,
		result.config.operations,
		result.config.workers,
		result.config.repeat,
		float64(result.duration.Microseconds())/1000.0,
		result.throughput,
		result.enqueues,
		result.dequeues,
		result.emptyReads,
		result.valueSample,
	)
}

func parsePositiveInts(input string) ([]int, error) {
	parts := strings.Split(input, ",")
	values := make([]int, 0, len(parts))

	for _, part := range parts {
		part = strings.TrimSpace(part)
		value, err := strconv.Atoi(part)
		if err != nil {
			return nil, fmt.Errorf("parse %q: %w", part, err)
		}
		if value <= 0 {
			return nil, fmt.Errorf("value must be positive: %d", value)
		}
		values = append(values, value)
	}

	return values, nil
}

func parseWorkloads(input string) ([]string, error) {
	parts := strings.Split(input, ",")
	workloads := make([]string, 0, len(parts))

	for _, part := range parts {
		part = strings.TrimSpace(strings.ToLower(part))
		if part == "all" {
			return []string{"enqueue", "dequeue", "mixed"}, nil
		}

		switch part {
		case "enqueue", "dequeue", "mixed":
			workloads = append(workloads, part)
		default:
			return nil, fmt.Errorf("unknown workload %q", part)
		}
	}

	return workloads, nil
}
