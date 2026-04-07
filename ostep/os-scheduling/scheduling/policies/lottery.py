import random

from ..models import ScheduleResult, TaskSpec, TaskState
from ..simulator import run_schedule


def run_lottery(task_specs: list[TaskSpec], seed: int = 8) -> ScheduleResult:
    rng = random.Random(seed)

    def chooser(runnable: list[TaskState], _: int) -> tuple[TaskState, int]:
        return rng.choices(runnable, weights=[task.weight for task in runnable], k=1)[
            0
        ], 1

    return run_schedule(
        task_specs, policy_name=f"Lottery Scheduling (seed={seed})", chooser=chooser
    )
