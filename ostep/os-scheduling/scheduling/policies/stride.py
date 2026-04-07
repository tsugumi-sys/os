from ..models import ScheduleResult, TaskSpec, TaskState
from ..simulator import run_schedule


BIG_STRIDE = 10_000


def run_stride(task_specs: list[TaskSpec]) -> ScheduleResult:
    def chooser(runnable: list[TaskState], _: int) -> tuple[TaskState, int]:
        current = min(runnable, key=lambda task: (task.pass_value, task.name))
        current.pass_value += BIG_STRIDE / current.weight
        return current, 1

    return run_schedule(task_specs, policy_name="Stride Scheduling", chooser=chooser)

