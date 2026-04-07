from .models import ScheduleResult, TaskSpec


TIMELINE_COLUMNS = 28
BAR_WIDTH = 24


def task_counts(result: ScheduleResult) -> dict[str, int]:
    return {spec.name: result.timeline.count(spec.name) for spec in result.task_specs}


def expected_share(weight: int, total_weight: int) -> float:
    return weight / total_weight


def bar(share: float) -> str:
    filled = round(share * BAR_WIDTH)
    return "#" * filled + "." * (BAR_WIDTH - filled)


def render_timeline(result: ScheduleResult) -> list[str]:
    lines: list[str] = []

    for start in range(0, len(result.timeline), TIMELINE_COLUMNS):
        stop = min(start + TIMELINE_COLUMNS, len(result.timeline))
        labels = range(start, stop)
        chunk = result.timeline[start:stop]
        lines.append("time : " + " ".join(f"{tick:>2}" for tick in labels))
        lines.append("task : " + " ".join(f"{name:>2}" for name in chunk))

    return lines


def render_result(result: ScheduleResult) -> str:
    counts = task_counts(result)
    total_ticks = len(result.timeline)
    total_weight = sum(spec.weight for spec in result.task_specs)
    summary = [
        (
            f"{spec.name}: ran={counts[spec.name]:>2}/{total_ticks}, "
            f"weight={spec.weight:>2}, "
            f"actual={counts[spec.name] / total_ticks:>5.1%}, "
            f"ideal={expected_share(spec.weight, total_weight):>5.1%}, "
            f"delta={(counts[spec.name] / total_ticks) - expected_share(spec.weight, total_weight):>+6.1%}, "
            f"[{bar(counts[spec.name] / total_ticks)}]"
        )
        for spec in result.task_specs
    ]

    return "\n".join(
        [
            result.policy_name,
            *render_timeline(result),
            *summary,
        ]
    )


def render_expected_share(task_specs: list[TaskSpec]) -> str:
    total_weight = sum(spec.weight for spec in task_specs)
    lines = ["Expected weighted share"]

    for spec in task_specs:
        share = expected_share(spec.weight, total_weight)
        lines.append(
            f"{spec.name}: weight={spec.weight:>2}, ideal_share={share:>5.1%}, [{bar(share)}]"
        )

    return "\n".join(lines)


def render_lottery_variability(results: list[ScheduleResult]) -> str:
    task_specs = results[0].task_specs
    total_ticks = len(results[0].timeline)
    counts_by_task = {
        spec.name: [task_counts(result)[spec.name] for result in results] for spec in task_specs
    }
    lines = [f"Lottery variability across {len(results)} seeds"]

    for spec in task_specs:
        counts = counts_by_task[spec.name]
        average = sum(counts) / len(counts)
        lines.append(
            f"{spec.name}: avg={average / total_ticks:>5.1%}, "
            f"min={min(counts) / total_ticks:>5.1%}, "
            f"max={max(counts) / total_ticks:>5.1%}"
        )

    return "\n".join(lines)


def render_comparison(results: list[ScheduleResult]) -> str:
    task_table = "\n".join(
        f"- {task.name}: runtime={task.runtime}, weight={task.weight}"
        for task in results[0].task_specs
    )
    sections = "\n\n".join(render_result(result) for result in results)
    return "\n".join(
        [
            "Demo tasks",
            task_table,
            "",
            render_expected_share(results[0].task_specs),
            "",
            sections,
        ]
    )
