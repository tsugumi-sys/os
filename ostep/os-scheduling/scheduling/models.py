from __future__ import annotations

from dataclasses import dataclass


@dataclass
class TaskSpec:
    name: str
    runtime: int
    weight: int


@dataclass
class TaskState:
    name: str
    total_runtime: int
    remaining: int
    weight: int
    pass_value: float = 0.0
    vruntime: float = 0.0

    @classmethod
    def from_spec(cls, spec: TaskSpec) -> "TaskState":
        return cls(
            name=spec.name,
            total_runtime=spec.runtime,
            remaining=spec.runtime,
            weight=spec.weight,
        )


@dataclass
class ScheduleResult:
    policy_name: str
    timeline: list[str]
    task_specs: list[TaskSpec]

