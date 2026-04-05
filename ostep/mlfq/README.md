# Simple MLFQ Simulator

This project simulates a simple Multi-Level Feedback Queue scheduler with:

- 3 queues
- time slice of `25ms`
- per-queue CPU allotments of `100ms`, `200ms`, and `300ms`
- priority boost interval of `200ms` by default

## Assumptions

- All tasks arrive at time `0`
- New tasks start in `Q0`
- Round-robin scheduling is used within each queue
- A task is demoted after consuming its queue's full allotment
- All runnable tasks are boosted back to `Q0` every boost interval
- No I/O blocking

## Run

```bash
python3 main.py
```

Custom task execution times:

```bash
python3 main.py --tasks 80 160 260 420
```

Disable boosting:

```bash
python3 main.py --boost 0
```

Choose a custom boost interval:

```bash
python3 main.py --tasks 200 500 --boost 300
```

## Output

The script prints:

- configuration summary
- task list
- queue visualization over time
- CPU execution timeline

Each column is one `25ms` scheduling slot.
The `event` row uses `B` to show a priority boost where all runnable tasks move back to `Q0`.
