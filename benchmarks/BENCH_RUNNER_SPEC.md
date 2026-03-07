# Benchmark Runner Technical Specification

## 1. Scope
This document specifies the benchmark runner tooling in `benchmarks/`.
It does not define engine architecture or engine correctness behavior.
Engine behavior is consumed through public APIs only.

Primary implementation source:
- `benchmarks/runner.c`
- `benchmarks/report_types.h`
- `benchmarks/protocol_helper.h`
- `benchmarks/csv_export.h`
- `benchmarks/html_report.h`
- `benchmarks/timing_helper.h`

## 2. Entry Point
Runner entry point is `main` in `benchmarks/runner.c`.
The runner executes a fixed benchmark target set and emits benchmark artifacts.

## 3. Build-Time Configuration
Current configuration is compile-time controlled.

- `ITERATIONS` default `10`
- `WARMUP_SECONDS` default `5.0`
- `SIM_CYCLES` default `1000`
- `HEBS_ENABLE_TITAN_BENCHES` default `0`
- `HEBS_SKIP_ARTIFACTS` default `0`
- `REVISION_NAME` default `Revision_Combinational_v05`
- `METRICS_CSV_PATH` profile-dependent:
  - perf profile: `benchmarks/results/metrics_history.csv`
  - compat profile: `benchmarks/results/metrics_history_compat.csv`
- `REPORT_HTML_PATH` default `benchmarks/results/revision_combinational_v05.html`

Probe profile selection is provided by `HEBS_COMPAT_PROBES_ENABLED` from `include/hebs_probe_profile.h`.

## 4. Benchmark Target Model
The runner currently uses a static benchmark target table.
Target records are `hebs_bench_target_s` with:
- `suite_name`
- `bench_path`

Default active targets:
- ISCAS85: `c17`, `c432`, `c499`, `c6288`, `c880`
- ISCAS89: `s27`, `s298`, `s382`, `s526`, `s820`

Optional Titan targets are compiled in only when `HEBS_ENABLE_TITAN_BENCHES=1`.

## 5. Execution Pipeline
High-level flow:
1. Run thermal warmup for `WARMUP_SECONDS`.
2. Optionally load anchor GEPS mean from CSV using `lookup_revision_mean_geps`.
3. Run hot-path profiling helper for `c6288` and print overhead diagnostics.
4. Collect timestamp/date and git short hash.
5. Iterate benchmark targets and run each via `hebs_run_single_bench`.
6. Evaluate row-level regression and fingerprint checks.
7. Emit HTML and CSV artifacts unless `HEBS_SKIP_ARTIFACTS=1`.
8. Return exit code `0` on clean pass, `1` on any failure.

## 6. Per-Benchmark Run Protocol
Each benchmark run performs `ITERATIONS` independent runs.
Per iteration:
1. Load plan from `.bench` via `hebs_load_bench`.
2. Initialize engine via `hebs_init_engine`.
3. For each cycle in `SIM_CYCLES`:
   - Apply deterministic PI pattern based on `(cycle + pi) & 1U`.
   - Call `hebs_tick`.
4. Measure elapsed runtime with high-resolution timer helpers.
5. Capture GEPS sample and final logic CRC32 fingerprint.
6. Capture probes via `hebs_get_probes`.

## 7. Derived Reporting Row
Row schema is `hebs_metric_row_t` in `benchmarks/report_types.h`.
The runner populates identity, topology, runtime statistics, activity metrics, history baselines, and probe profile metadata.

Profile-dependent behavior:
- Compat profile enabled: transition-based counters are populated from probes.
- Compat profile disabled: transition-based counters are forced to numeric `0`.

## 8. Validation and Failure Gates
Current failure conditions in runner summary:
- GEPS regression gate: fail if `geps_delta_prev_pct < -2.0`.
- Determinism gate: fail if iteration CRC32 fingerprints are not stable.
- Initialization or execution setup failure for a target is counted as failure.
- Artifact write failure is counted as failure when artifact output is enabled.

## 9. Artifact Contracts
When artifact output is enabled:
- HTML report is written by `generate_master_report`.
- CSV history is appended by `append_metrics_history_csv`.

CSV append behavior:
- Writes a revision metadata header line.
- Writes fixed column header line.
- Appends one row per benchmark result.
- Leaves prior history rows intact.

## 10. Tooling Boundary Rules
Runner code must remain outside core engine implementation.
Runner may read engine outputs and probes through public API only.
Runner must not mutate engine internals directly.

## 11. Non-Goals
This specification does not define:
- engine state model
- engine execution semantics
- probe implementation internals
Those belong to engine-side technical documentation.