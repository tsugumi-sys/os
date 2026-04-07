from scheduling.demo import build_demo_tasks
from scheduling.policies import run_cfs, run_lottery, run_stride
from scheduling.visualizer import render_comparison, render_lottery_variability


def main() -> None:
    tasks = build_demo_tasks()
    lottery_seeds = [1, 2, 3, 4, 5, 6, 7, 8]
    lottery_results = [run_lottery(tasks, seed=seed) for seed in lottery_seeds]
    results = [
        lottery_results[-1],
        run_stride(tasks),
        run_cfs(tasks),
    ]
    print(
        "\n\n".join(
            [
                render_comparison(results),
                render_lottery_variability(lottery_results),
            ]
        )
    )


if __name__ == "__main__":
    main()
