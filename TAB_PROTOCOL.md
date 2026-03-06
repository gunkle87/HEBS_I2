# HEBS Canonical Testing & Benchmarking Protocol
Version: 1.4.1
Status: MANDATORY
Authority: HEBS_SNAPSHOT.md

## 1. Canonical Build Policy
* **Requirement**: All binaries for testing and benchmarking must use the same build process.
* **Profile**: Use the `perf` profile: `gcc -O3 -march=native -DNDEBUG -static -fno-plt`.
* **Environment**: Benchmarks must be run via `taskset -c <core_id>` on a single physical core with OS "Performance" governor active.
* **Constraint**: No environment-specific overrides or variant parameters are permitted to prevent results skew.

## 2. Testing Framework (The Stress Runner)
* **Scope**: Comprehensive testing of gates, combinational loops, oscillation, and canonical operational-state resolution.
* **Execution**: Triggered at every core code edit to prevent logic regression.
* **Reporting**: Record results only at the completion of a minor or major revision phase; no intermittent logging.
* **Operational Collapse Rule**: HEBS remains 8-state capable, but canonical protocol runs treat `SX` and `WX` as `X` unless a future revision explicitly enables strength-aware mode.
* **Resolution Check**: Verify contention logic (e.g., Strong vs. Weak) matches the resolution matrix.

## 3. Benchmarking Framework (The Performance Runner)
* **Discovery**: Scans `root/benchmarks/benches` for all `.bench` files regardless of content.
* **Execution Model**:
    * **Iterated Cold Start**: Each test starts and stops independently to prevent cache/state stacking.
    * **Warm-up**: 5-second dummy load required prior to iterations to settle CPU thermal and frequency states.
* **Iteration Policy**: Default is a 10-iteration cold start per test; runner must support manual iteration overrides via parameters.
* **Suites**: Supports running individual files, specific suites (e.g., ISCAS85), or the entire directory.

## 4. Metrics & Calculation
* **Statistical Method**: Calculate results based on the **median** of the 10-iteration cold start.
* **Required Output**: Return Median (p50), High (max), and Low (min) for every metric.
* **Metadata**: Every report must include system info, time, date, and the current completed Git revision.
* **ICF Canonical Formula**: `ICF = internal_node_transitions / primary_input_transitions`.
* **ICF Comparability Rule**: ICF values generated before this formula lock are historical-only and not directly comparable to post-lock ICF values without re-running benchmarks under the locked formula.
* **Metric List**:
    * **Statistical**: p50, p90, p95, p99, min, max, mean, variance, stddev.
    * **Execution**: total_runtime, throughput, latency_per_cycle, latency_per_vector, speedup, efficiency.
    * **Simulation**: gates_per_second, GEPS, events_per_second, edges_per_second, vector_throughput, cycles_per_second.
    * **Logic Stats**: cycles_per_vector, gates_per_cycle, events_per_cycle, events_per_vector, net_transitions_per_cycle.
    * **Activity**: fanout_activity, toggle_rate, activity_factor, event_density, work_per_event, evals_per_event.
    * **System/Cache** (Best Effort): event_queue_pressure, scheduler_utilization, stall_ratio, compute_ratio, memory_bandwidth, memory_per_gate, memory_per_net, cache_miss_rate, branch_mispredict_rate, instructions_per_cycle, cycles_per_gate_eval, cycles_per_event, instructions_per_gate_eval, instructions_per_event.
    * **Temporal**: simulated_time_per_second, wallclock_per_sim_cycle, utilized_edges_ratio, queue_utilization.
    * **Topological**: propagation_depth, critical_path_eval_rate, fanout_avg, fanout_max, gate_density, transition_entropy.

## 5. Determinism & Integrity Fingerprint
* **Logic Fingerprint**: CRC32 of the final signal tray after the last tick.
* **Plan Fingerprint**: Hash of the Linear Execution Plan (LEP) generated from the source `.bench`.
* **Acceptance Gate**: Any revision that alters the Fingerprint on a stable benchmark or regresses p50 GEPS by >2% is a hard failure.

## 6. Artifact Naming & HTML Lifecycle (Mandatory)
* **Revision Token Source**: The active revision name (for example `Revision_Structure_v04`) is the naming root for generated report artifacts.
* **Naming Rule**: New generated report files must use `<RevisionName>_vNN.html` with minor numbering starting at `v01` for each revision.
* **Minor Revision Append Rule**: Each minor benchmark run writes a new HTML file and increments `NN`.
* **Immutability Rule**: Existing HTML files must never be overwritten or deleted by the benchmark runner.
* **Cross-Revision Isolation**: A subsequent major revision must continue to write new revision-named files and may not modify any prior revision HTML artifact.
* **Final Minor Metrics Rule**: The final minor code change of a revision must be the revision metrics-point update.
* **History Rule**: CSV history remains append-only and must continue to carry revision metadata per run header.

## 7. Engine Probe Constraint

The engine may expose **raw probes only**.
The engine must not compute, derive, aggregate, scan for, or assemble benchmark or test metrics inside the engine.

### Allowed

Raw probe updates at **natural execution points** only.

A **natural execution point** is an already-existing simulation path where the engine is performing required simulation work, and the probe can be updated in **constant cost O(1)** without adding any extra scan, sweep, recomputation, aggregation pass, or control flow introduced solely for testing or benchmarking.

Probe APIs may expose raw probe structs only.
Probe APIs must not return derived metrics.

Examples:

```c
ctx->probe_gate_eval++;
ctx->probe_event_dispatch++;
ctx->probe_state_write++;
```

### Forbidden

Any **additional** control flow introduced **solely** for testing, benchmarking, auditing, or reporting, including:

* scan passes
* sweeps over engine state
* recomputation of transitions
* aggregation logic
* dirty-set construction for metrics
* metric assembly inside the engine

Examples of forbidden patterns:

```c
for (...) { ... }          // added only to measure
if (...) { metric++; }     // added only to classify/count for reporting
metric += popcount(...);   // derived analysis
```

### Boundary

* **Engine:** emits facts
* **Tools / runners / harnesses:** derive conclusions

The engine may report:

* raw counters
* raw event markers
* raw state observations already produced by execution

The engine must not report:

* derived benchmark metrics
* interpreted activity summaries
* counts that require post-execution reconstruction inside the engine

### Compliance Test

If code exists to measure engine behavior rather than perform simulation behavior, it does not belong in the engine unless it is a raw O(1) probe on an already-existing execution path.

## 8. Probe Profile Canon (probe_fix_v03-v05)
* **Build Profiles**:
    * Exactly one profile must be active at build time: `HEBS_PROBE_PROFILE_PERF` or `HEBS_PROBE_PROFILE_COMPAT`.
    * Default canonical build profile is `HEBS_PROBE_PROFILE_PERF`.
    * `HEBS_COMPAT_PROBES_ENABLED` is the derived compatibility switch for runner/test behavior.
* **Probe Classification**:
    * **Permanent (all profiles)**: `input_apply`, `chunk_exec`, `gate_eval`, `dff_exec`.
    * **Compatibility (compat profile only)**: `input_toggle`, `state_change_commit`.
    * **Removed in v03**: `state_write_attempt`.
    * **Deferred out of v03**: `tray_exec`, `state_write_commit`.
* **Benchmark Suite Output Rules**:
    * Under `HEBS_PROBE_PROFILE_PERF`, compatibility-derived fields must emit numeric `0`.
    * Under `HEBS_PROBE_PROFILE_COMPAT`, compatibility-derived fields are computed normally.
    * Benchmark output rows must append (do not reorder existing columns):
        * `Probe_Profile` (`perf` or `compat`)
        * `Compat_Metrics_Enabled` (`0` or `1`)
* **Canonical Artifact Separation**:
    * Canonical perf-profile benchmark runs append only to `benchmarks/results/metrics_history.csv`.
    * Compat-profile benchmark runs append only to `benchmarks/results/metrics_history_compat.csv`.
    * Canonical regression history must contain perf-profile runs only.
* **Regression Authority**:
    * Canonical regression gates remain based on GEPS, latency/runtime statistics, and CRC/correctness stability.
    * Compatibility-derived metrics are informational only and must not gate canonical acceptance.
* **Suite Terminology Rule**:
    * Normative language must use explicit terms: `test suite` and `benchmark suite`.
    * Avoid mixed generic language where required behavior differs between suites.
* **Test Suite Rules**:
    * Perf profile test suite: must not require compatibility probe values.
    * Compat profile test suite: may assert compatibility probe values (`input_toggle`, `state_change_commit`).
* **Measurement and Acceptance**:
    * Probe acceptance decisions use median-of-10 on same host/build/inputs/runtime conditions.
    * A probe may remain permanent only if it causes <=1.0% median GEPS regression and <=2.5% regression on any individual benchmark unless explicitly approved.
* **Minor Revision Naming Rule**:
    * Probe-fix minor revisions must use flat tokens (`probe_fix_vNN`).
    * Nested suffix tokens (for example `probe_fix_v03_v01`) are non-canonical.
