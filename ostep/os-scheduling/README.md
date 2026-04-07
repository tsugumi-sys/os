# OS Scheduling Demo

Simple CPU scheduling demos for:

- Lottery scheduling
- Stride scheduling
- Simple CFS-style scheduling

The demo uses the same task set for every policy:

- `A`: weight `10`
- `B`: weight `20`
- `C`: weight `40`

Each scheduler runs over the same fixed observation window of `84` ticks. That window is intentional: with weights `10:20:40`, the ideal weighted split is exactly:

- `A`: `12` ticks
- `B`: `24` ticks
- `C`: `48` ticks

The result is rendered with shared visualization logic:

- `time`: tick index
- `task`: which task ran at that tick
- summary lines: actual share, ideal share, and delta
- simple ASCII bars for quick visual comparison
- lottery variability summary across multiple seeds

## Run

```bash
python3 main.py
```

This prints a side-by-side textual comparison that is easy to extend when adding new policies.

## Structure

- `main.py`: entry point
- `scheduling/demo.py`: shared demo task definitions
- `scheduling/models.py`: common task and result models
- `scheduling/simulator.py`: shared scheduling loop
- `scheduling/visualizer.py`: common timeline rendering
- `scheduling/policies/`: lottery, stride, and simple CFS policies
