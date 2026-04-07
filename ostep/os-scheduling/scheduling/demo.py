from .models import TaskSpec


SIMULATION_TICKS = 84


def build_demo_tasks() -> list[TaskSpec]:
    return [
        TaskSpec(name="A", runtime=84, weight=10),
        TaskSpec(name="B", runtime=84, weight=20),
        TaskSpec(name="C", runtime=84, weight=40),
    ]
