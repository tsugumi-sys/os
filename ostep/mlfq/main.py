from __future__ import annotations

import argparse
from collections import deque
from dataclasses import dataclass, field


TIMESLICE_MS = 25
QUEUE_ALLOTMENTS_MS = [100, 200, 300]
DEFAULT_BOOST_INTERVAL_MS = 200


@dataclass(slots=True)
class Task:
    name: str
    execution_time_ms: int
    remaining_ms: int = field(init=False)
    current_queue: int = field(default=0)
    used_in_queue_ms: int = field(default=0)

    def __post_init__(self) -> None:
        self.remaining_ms = self.execution_time_ms


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Simple MLFQ simulator with 3 queues and ASCII timeline output."
    )
    parser.add_argument(
        "--tasks",
        nargs="+",
        type=int,
        default=[80, 160, 260, 420],
        help="Execution time for each task in ms. Example: --tasks 80 160 260 420",
    )
    parser.add_argument(
        "--boost",
        type=int,
        default=DEFAULT_BOOST_INTERVAL_MS,
        help=(
            "Priority boost interval in ms. Must be a positive multiple of "
            f"{TIMESLICE_MS}. Use 0 to disable boosting."
        ),
    )
    return parser


def apply_priority_boost(queues: list[deque[Task]]) -> None:
    boosted_tasks: list[Task] = []

    for queue in queues:
        while queue:
            task = queue.popleft()
            task.current_queue = 0
            task.used_in_queue_ms = 0
            boosted_tasks.append(task)

    queues[0].extend(boosted_tasks)


def simulate(
    task_execution_times: list[int], boost_interval_ms: int
) -> tuple[list[Task], list[list[str]], list[str], list[str], int]:
    tasks = [Task(name=f"T{i + 1}", execution_time_ms=duration) for i, duration in enumerate(task_execution_times)]
    queues = [deque(tasks), deque(), deque()]
    queue_timelines = [[] for _ in range(3)]
    task_timeline: list[str] = []
    event_timeline: list[str] = []
    current_time_ms = 0

    while any(queue for queue in queues):
        if boost_interval_ms > 0 and current_time_ms > 0 and current_time_ms % boost_interval_ms == 0:
            apply_priority_boost(queues)
            event_timeline.append("B")
        else:
            event_timeline.append(".")

        current_queue = next(index for index, queue in enumerate(queues) if queue)
        task = queues[current_queue].popleft()

        runtime_ms = min(TIMESLICE_MS, task.remaining_ms)
        task.remaining_ms -= runtime_ms
        task.used_in_queue_ms += runtime_ms
        current_time_ms += runtime_ms

        for index in range(3):
            queue_timelines[index].append(task.name if index == current_queue else ".")
        task_timeline.append(task.name)

        if task.remaining_ms == 0:
            continue

        queue_allotment_ms = QUEUE_ALLOTMENTS_MS[current_queue]
        if task.used_in_queue_ms >= queue_allotment_ms and current_queue < 2:
            task.current_queue += 1
            task.used_in_queue_ms = 0
            queues[task.current_queue].append(task)
        else:
            queues[current_queue].append(task)

    return tasks, queue_timelines, task_timeline, event_timeline, current_time_ms


def format_axis(slots: int) -> str:
    labels = [f"{slot * TIMESLICE_MS:>4}" for slot in range(slots)]
    return "time ".ljust(8) + "".join(labels)


def format_row(label: str, values: list[str]) -> str:
    cells = [f"{value:>4}" for value in values]
    return f"{label:<8}" + "".join(cells)


def print_summary(tasks: list[Task], boost_interval_ms: int) -> None:
    print("Configuration")
    print(f"  queues: 3")
    print(f"  timeslice: {TIMESLICE_MS}ms")
    print(f"  allotment per queue: {QUEUE_ALLOTMENTS_MS[0]}ms, {QUEUE_ALLOTMENTS_MS[1]}ms, {QUEUE_ALLOTMENTS_MS[2]}ms")
    if boost_interval_ms > 0:
        print(f"  priority boost interval: {boost_interval_ms}ms")
    else:
        print("  priority boost interval: disabled")
    print()
    print("Tasks")
    for task in tasks:
        print(f"  {task.name}: execution time={task.execution_time_ms}ms")
    print()


def print_timeline(
    queue_timelines: list[list[str]], task_timeline: list[str], event_timeline: list[str], total_time_ms: int
) -> None:
    slots = len(task_timeline)
    print("Execution Timeline")
    print(format_axis(slots))
    print(format_row("event", event_timeline))
    print(format_row("Q0", queue_timelines[0]))
    print(format_row("Q1", queue_timelines[1]))
    print(format_row("Q2", queue_timelines[2]))
    print(format_row("CPU", task_timeline))
    print()
    print("Legend: B = priority boost to Q0")
    print(f"Total completion time: {total_time_ms}ms")


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    if args.boost < 0 or args.boost % TIMESLICE_MS != 0:
        parser.error(f"--boost must be 0 or a positive multiple of {TIMESLICE_MS}ms")

    tasks, queue_timelines, task_timeline, event_timeline, total_time_ms = simulate(args.tasks, args.boost)
    print_summary(tasks, args.boost)
    print_timeline(queue_timelines, task_timeline, event_timeline, total_time_ms)


if __name__ == "__main__":
    main()
