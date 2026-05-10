---
status: PENDING
priority: (COULD, SOONER, MEDIUM, LARGE)
---

# Profiling report

When `--profile` is passed, collect per-function timing and memory data and print a report on exit.

## Trigger

`--profile` flag on the `vo` binary; no effect on execution behaviour.

## Metrics — per function

- Call count
- Total wall time (inclusive — includes callees)
- Self wall time (exclusive — excludes callees)
- Peak memory allocated during the call (bytes)

## Metrics — totals

- Total program wall time
- Total calls
- Peak memory across whole run

## Step 1 — timer and allocator hooks (`interpreter.cpp`)

- Wrap `call_callable` with `std::chrono::steady_clock` timestamps
- Track a call stack depth counter to compute self-time (subtract child time from parent)
- Hook `Value` / `HashInstance` construction/destruction to count live allocations

## Step 2 — profile accumulator (`src/profiler.hpp/.cpp`)

- Map from function name -> `{call_count, total_ns, self_ns, peak_bytes}`
- Thread-safe if concurrency is ever added; for now a plain `std::unordered_map`
- Active only when `--profile` flag is set — zero overhead otherwise

## Step 3 — report printer

On program exit, sort entries by self time descending and print a table to stdout:

```
PROFILE REPORT
──────────────────────────────────────────────────────
 Function          Calls   Total ms   Self ms   Peak KB
──────────────────────────────────────────────────────
 loops.for          1200     48.2ms    12.1ms      4 KB
 draw_scene          600     36.1ms    36.1ms      2 KB
 ...
──────────────────────────────────────────────────────
 TOTAL                        48.2ms              8 KB
```

## Usage

```
./vo --profile mygame.vo
```
