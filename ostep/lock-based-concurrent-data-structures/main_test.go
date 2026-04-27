package main

import (
	"sync"
	"testing"
)

func TestQueuesPreserveFIFOOrder(t *testing.T) {
	tests := []struct {
		name string
		q    queue
	}{
		{name: "global-lock", q: newGlobalLockQueue()},
		{name: "two-lock", q: newTwoLockQueue()},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			for i := 0; i < 100; i++ {
				tt.q.Enqueue(i)
			}

			for expected := 0; expected < 100; expected++ {
				actual, ok := tt.q.Dequeue()
				if !ok {
					t.Fatalf("dequeue failed at value %d", expected)
				}
				if actual != expected {
					t.Fatalf("expected %d, got %d", expected, actual)
				}
			}

			if _, ok := tt.q.Dequeue(); ok {
				t.Fatal("expected empty queue")
			}
		})
	}
}

func TestQueuesHandleConcurrentMixedOperations(t *testing.T) {
	tests := []struct {
		name string
		q    queue
	}{
		{name: "global-lock", q: newGlobalLockQueue()},
		{name: "two-lock", q: newTwoLockQueue()},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			const workers = 8
			const operationsPerWorker = 1000

			for i := 0; i < workers*operationsPerWorker/2; i++ {
				tt.q.Enqueue(i)
			}

			var wg sync.WaitGroup
			for workerID := 0; workerID < workers; workerID++ {
				wg.Add(1)
				go func(workerID int) {
					defer wg.Done()
					for i := 0; i < operationsPerWorker; i++ {
						if i%2 == 0 {
							tt.q.Enqueue(workerID*operationsPerWorker + i)
						} else {
							tt.q.Dequeue()
						}
					}
				}(workerID)
			}
			wg.Wait()

			for {
				if _, ok := tt.q.Dequeue(); !ok {
					return
				}
			}
		})
	}
}
