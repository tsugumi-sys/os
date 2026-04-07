from typing import Callable

from .demo import SIMULATION_TICKS
from .models import ScheduleResult, TaskSpec, TaskState


def clone_tasks(task_specs: list[TaskSpec]) -> list[TaskState]:
    return [TaskState.from_spec(spec) for spec in task_specs]


def all_done(tasks: list[TaskState]) -> bool:
    return all(task.remaining == 0 for task in tasks)


def active_tasks(tasks: list[TaskState]) -> list[TaskState]:
    return [task for task in tasks if task.remaining > 0]


def run_schedule(
    task_specs: list[TaskSpec],
    policy_name: str,
    chooser: Callable[[list[TaskState], int], tuple[TaskState, int]],
    max_ticks: int = SIMULATION_TICKS,
) -> ScheduleResult:
    tasks = clone_tasks(task_specs)
    timeline: list[str] = []
    tick = 0

    while tick < max_ticks and not all_done(tasks):
        runnable = active_tasks(tasks)
        current, requested_slice = chooser(runnable, tick)
        actual_slice = min(requested_slice, current.remaining, max_ticks - tick)
        current.remaining -= actual_slice
        timeline.extend([current.name] * actual_slice)
        tick += actual_slice

    return ScheduleResult(policy_name=policy_name, timeline=timeline, task_specs=task_specs)

