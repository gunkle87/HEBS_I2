# Metric Derivation Technical Specification

## 1. Scope
This document defines derived metric computation as a separate tooling step.
The benchmark runner emits raw rows only.
Derived metrics are computed by `benchmarks/metric_calculator.c`.

## 2. Inputs
Calculator input is raw CSV produced by runner.
Default input path:
- `benchmarks/results/raw_runner_output.csv`

Raw schema contract is defined in `benchmarks/BENCH_RUNNER_SPEC.md`.
Calculator consumes aggregate-mode raw CSV only.
Trace-mode CSV (`trace_cycle.csv`) is out of scope for this calculator.

## 3. Output
Calculator output is derived CSV.
Default output path:
- `benchmarks/results/metrics_derived.csv`

One output row is emitted per `(run_id, suite, bench, mode)` group.
Each row aggregates all iterations in that group.

## 4. CLI Contract
Supported arguments:
- `--input <raw.csv>`
- `--output <derived.csv>`
- `--replace` (overwrite output instead of append)

Default behavior appends to output and uses prior output rows as history baseline.

## 5. Grouping and Ordering
Rows are grouped by:
- `run_id`
- `suite`
- `bench`
- `mode`

Groups are emitted in deterministic order by `(suite, bench, run_id, mode)`.

## 6. Derived Formulas
Per raw row:
- `geps = (gate_count * cycles) / runtime_sec / 1e9` when runtime > 0 else `0`
- `icf = probe_state_change_commit / probe_input_toggle` when `probe_input_toggle > 0` else `0`

Per group (across iterations):
- Runtime: `min`, `p50`, `p90`, `p95`, `p99`, `max`, `mean`, `stddev`
- GEPS: `min`, `p50`, `max`, `mean`
- ICF: `min`, `p50`, `max`, `mean`

Percentiles are linear interpolation over sorted samples.
Variance/stddev use population form (`/N`).

## 7. History Baseline and Deltas
When appending, calculator scans existing derived CSV for matching `(suite, bench, mode)`:
- `base_*` is first seen value
- `prev_*` is most recent seen value

Delta formulas:
- `delta_geps_prev_pct = ((geps_p50 - prev_geps_p50) / prev_geps_p50) * 100` when `prev_geps_p50 > 0` else `0`
- `delta_icf_prev_pct = ((icf_p50 - prev_icf_p50) / prev_icf_p50) * 100` when `prev_icf_p50 > 0` else `0`

## 8. Fingerprint Field
`fingerprint_stable = 1` only when all iteration `logic_crc32` values in the group match.
`logic_crc32` output is the first iteration CRC value for the group.

## 9. Derived CSV Schema
Header order:

`run_id,revision,date,time,git_commit,suite,bench,mode,iterations,cycles,gate_count,pi_count,signal_count,propagation_depth,fanout_max,total_fanout_edges,runtime_min_sec,runtime_p50_sec,runtime_p90_sec,runtime_p95_sec,runtime_p99_sec,runtime_max_sec,runtime_mean_sec,runtime_stddev_sec,geps_min,geps_p50,geps_max,geps_mean,icf_min,icf_p50,icf_max,icf_mean,base_geps_p50,prev_geps_p50,delta_geps_prev_pct,base_icf_p50,prev_icf_p50,delta_icf_prev_pct,fingerprint_stable,logic_crc32,compat_metrics_enabled`

## 10. Boundary Rules
Calculator is non-canonical analysis tooling.
It must not mutate engine internals.
It must consume raw runner output only.
