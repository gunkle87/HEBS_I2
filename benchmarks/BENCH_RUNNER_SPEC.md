# Benchmark Runner Technical Specification

## 1. Scope
This document defines the benchmark runner in `benchmarks/runner.c`.
It defines run discovery, run execution, and raw recording output modes.

It does not define:
- engine architecture
- engine correctness policy
- derived metric policy
- report rendering policy

## 2. Entry Point
Runner entry point is `main` in `benchmarks/runner.c`.
The runner discovers selected `.bench` files, executes runs, and appends raw rows.

## 3. Build-Time Defaults
Defaults can be overridden at compile time.

- `DEFAULT_ITERATIONS` default `10`
- `DEFAULT_SIM_CYCLES` default `1000`
- `DEFAULT_WARMUP_SECONDS` default `5.0`
- `DEFAULT_BENCH_ROOT` default `benchmarks/benches`
- `RAW_CSV_PATH` default `benchmarks/results/raw_runner_output.csv`
- `TRACE_CSV_PATH` default `benchmarks/results/trace_cycle.csv`
- `REVISION_NAME` default `Raw_Runner_v01`

Probe profile label is selected by `HEBS_COMPAT_PROBES_ENABLED`.

## 4. CLI Contract
Supported arguments:

- `--scope all|suite|bench`
- `--suite <name>` required for `suite` and `bench` scopes
- `--bench <file.bench>` required for `bench` scope
- `--iterations <N>`
- `--cycles <N>`
- `--warmup-seconds <seconds>`
- `--bench-root <path>`
- `--output <path>`
- `--trace-output <path>`
- `--record-mode aggregate|trace|none`
- `--record-overwrite append|overwrite`
- `--record-meta-head on|off`
- `--record-interval <N>`

`all` discovers every suite directory below `bench_root` and every `.bench` file in each suite.
`suite` discovers every `.bench` file in one suite.
`bench` discovers one named `.bench` file in one suite.

No benchmark names are hardcoded in execution logic.

Recording defaults:
- `record_mode=aggregate`
- `record_overwrite=append`
- `record_meta_head=off`
- `record_interval=1`

## 5. Target Discovery and Ordering
Targets are discovered from the filesystem at runtime.
Each target stores:

- `suite_name`
- `bench_file`
- `bench_path`

Targets are sorted deterministically by `(suite_name, bench_file)`.

## 6. Execution Protocol
For each target, runner executes `iterations` independent runs.
Per iteration:

1. Load plan with `hebs_load_bench`.
2. Initialize engine with `hebs_init_engine`.
3. Execute configured cycle count using deterministic PI pattern `(cycle + pi) & 1`.
4. Measure wall time.
5. Capture probes with `hebs_get_probes`.
6. Capture final tray CRC with `hebs_crc32_signal_trays`.
7. Record output according to `record_mode`.

Mode behavior:
- `aggregate`: append one raw row per iteration to aggregate CSV.
- `trace`: append cycle snapshot rows to trace CSV, gated by `record_interval`.
- `none`: execute workload without artifact writes.

## 7. Raw CSV Schema (Locked)
Applies to `record_mode=aggregate`.
Runner writes exactly one row per `(target, iteration)` with this header order:

`run_id,revision,date,time,git_commit,suite,bench,mode,iteration,iterations_total,cycles,runtime_sec,plan_fingerprint,logic_crc32,probe_input_apply,probe_input_toggle,probe_chunk_exec,probe_gate_eval,probe_state_change_commit,probe_dff_exec,pi_count,gate_count,signal_count,propagation_depth,fanout_max,total_fanout_edges,compat_metrics_enabled`

Field policy:

- Metadata fields: `run_id`, `revision`, `date`, `time`, `git_commit`, `suite`, `bench`, `mode`.
- Run controls: `iteration`, `iterations_total`, `cycles`.
- Raw timing/output: `runtime_sec`, `plan_fingerprint`, `logic_crc32`.
- Approved raw probe fields only:
  - `probe_input_apply`
  - `probe_input_toggle`
  - `probe_chunk_exec`
  - `probe_gate_eval`
  - `probe_state_change_commit`
  - `probe_dff_exec`
- Topology metadata from plan:
  - `pi_count`
  - `gate_count`
  - `signal_count`
  - `propagation_depth`
  - `fanout_max`
  - `total_fanout_edges`
- Profile indicator: `compat_metrics_enabled`.

The runner must not add derived metrics (for example GEPS, ICF, deltas, percentiles).
The runner must not add or remove unrelated raw CSV fields outside explicitly approved schema changes.

## 8. Trace CSV Schema
Applies to `record_mode=trace`.
Runner writes trace rows with this header order:

`run_id,revision,date,time,git_commit,suite,bench,mode,iteration,iterations_total,cycle,cycles_total,runtime_sec,plan_fingerprint,logic_crc32,probe_input_apply,probe_input_toggle,probe_chunk_exec,probe_gate_eval,probe_state_change_commit,probe_dff_exec,compat_metrics_enabled,pi_count,gate_count,signal_count`

Trace rows are written only when:
- `cycle % record_interval == 0`
- or final cycle of the iteration
## 9. Failure Semantics
Runner returns non-zero when any target iteration fails due to:

- invalid CLI contract
- discovery failure
- plan load or engine init failure
- PI set failure
- CSV open/write failure

Runner does not enforce quality thresholds or regression gates.

## 10. Artifact Boundary
Canonical runner output for metric derivation is aggregate raw CSV only.
Derived metrics and reports are out of scope for this executable and should be implemented in separate tooling.
Trace output is optional debug tooling output and must remain separate from aggregate raw CSV.

Legacy metrics/history/report artifacts are non-canonical and must not be extended by this runner.

## 11. Tooling Boundary Rules
Runner is a tooling client of engine public API.
It must not mutate core internals directly.
