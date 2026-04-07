from ..models import ScheduleResult, TaskSpec, TaskState
from ..simulator import run_schedule


TARGET_LATENCY = 6


def run_cfs(task_specs: list[TaskSpec]) -> ScheduleResult:
    base_weight = min(spec.weight for spec in task_specs)

    def chooser(runnable: list[TaskState], _: int) -> tuple[TaskState, int]:
        current = min(runnable, key=lambda task: (task.vruntime, task.name))
        total_weight = sum(task.weight for task in runnable)
        slice_length = max(1, round(TARGET_LATENCY * current.weight / total_weight))
        current.vruntime += slice_length * (base_weight / current.weight)
        return current, slice_length

    return run_schedule(task_specs, policy_name="Simple CFS", chooser=chooser)

