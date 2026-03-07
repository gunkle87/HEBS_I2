# Benchmark Runner Technical Specification

## 1. Scope
This document defines the benchmark runner in `benchmarks/runner.c`.
It defines run discovery, run execution, and raw CSV output only.

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

`all` discovers every suite directory below `bench_root` and every `.bench` file in each suite.
`suite` discovers every `.bench` file in one suite.
`bench` discovers one named `.bench` file in one suite.

No benchmark names are hardcoded in execution logic.

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
7. Append one raw CSV row.

## 7. Raw CSV Schema (Locked)
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

## 8. Failure Semantics
Runner returns non-zero when any target iteration fails due to:

- invalid CLI contract
- discovery failure
- plan load or engine init failure
- PI set failure
- CSV open/write failure

Runner does not enforce quality thresholds or regression gates.

## 9. Artifact Boundary
Canonical runner output is raw CSV only.
Derived metrics and reports are out of scope for this executable and should be implemented in separate tooling.

Legacy metrics/history/report artifacts are non-canonical and must not be extended by this runner.

## 10. Tooling Boundary Rules
Runner is a tooling client of engine public API.
It must not mutate core internals directly.
