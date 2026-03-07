# Metric Derivation Technical Specification

## 1. Scope
This document defines benchmark metric derivation implemented by tooling code.
It reflects current implementation in `benchmarks/runner.c` and helper headers.
It does not define engine internals.

Primary implementation source:
- `benchmarks/runner.c`
- `benchmarks/protocol_helper.h`
- `benchmarks/report_types.h`
- `benchmarks/csv_export.h`

## 2. Input Signals for Derivation
Per benchmark row derives from:
- runtime samples array of size `ITERATIONS`
- GEPS samples array of size `ITERATIONS`
- topology fields from loaded plan
- probe counters captured from `hebs_get_probes`
- cycle totals accumulated from `engine.current_tick`
- history baselines from CSV lookup helpers

## 3. Statistical Metrics
Implemented with helpers in `protocol_helper.h`:
- `runtime_min = min(runtimes)`
- `runtime_max = max(runtimes)`
- `runtime_p50 = p50(runtimes)`
- `runtime_p90 = percentile(runtimes, 90)`
- `runtime_p95 = percentile(runtimes, 95)`
- `runtime_p99 = percentile(runtimes, 99)`
- `runtime_mean = mean(runtimes)`
- `runtime_variance = variance(runtimes)`
- `runtime_stddev = stddev(runtimes)`

GEPS statistics:
- `geps_min = min(geps_runs)`
- `geps_max = max(geps_runs)`
- `geps_p50 = p50(geps_runs)`
- `gates_per_second = geps_p50`

## 4. Transition Core Metric
- `icf = calculate_icf(internal_transitions, primary_input_transitions)`
- `calculate_icf(a, b)` returns `0` when denominator `b` is zero

## 5. Execution Metrics
Current implementation formulas:
- `total_runtime = runtime_p50`
- `throughput = cycles / runtime_p50` when runtime positive, else `0`
- `latency_per_cycle = runtime_p50 / cycles` when cycles positive, else `0`
- `latency_per_vector = latency_per_cycle`
- `speedup = runtime_max / runtime_p50` when runtime positive, else `0`
- `efficiency = runtime_p50 / runtime_max` when runtime_max positive, else `0`

## 6. Simulation and Activity Metrics
Current implementation formulas:
- `events_per_second = total_toggles / runtime_p50` when runtime positive, else `0`
- `edges_per_second = events_per_second`
- `vector_throughput = throughput`
- `cycles_per_second = throughput`
- `cycles_per_vector = 1.0`
- `gates_per_cycle = gate_count` when cycles positive, else `0`
- `events_per_cycle = total_toggles / cycles` when cycles positive, else `0`
- `events_per_vector = events_per_cycle`
- `net_transitions_per_cycle = events_per_cycle`

Fanout and activity:
- `fanout_avg = total_fanout_edges / signal_count` when signal_count positive, else `0`
- `fanout_activity = fanout_avg * events_per_cycle`
- `toggle_rate = icf`
- `activity_factor = icf`
- `event_density = events_per_cycle / gate_count` when gate_count positive, else `0`
- `work_per_event = gate_count / events_per_cycle` when events_per_cycle positive, else `0`
- `evals_per_event = (gate_count * cycles) / total_toggles` when toggles positive, else `0`

Temporal and topological:
- `simulated_time_per_second = cycles_per_second`
- `wallclock_per_sim_cycle = latency_per_cycle`
- `utilized_edges_ratio = event_density`
- `queue_utilization = event_density`
- `critical_path_eval_rate = propagation_depth / runtime_p50` when runtime positive, else `0`
- `gate_density = gate_count / signal_count` when signal_count positive, else `0`
- `transition_entropy = binary_entropy(activity_factor)`

`binary_entropy(p)` is `0` for `p <= 0` or `p >= 1`, else standard binary entropy.

## 7. Profile-Gated Probe Inputs
Transition counters are profile-dependent:

Compat profile enabled:
- `total_toggles` populated from `probes.input_toggle`
- `primary_input_transitions` populated from `probes.input_toggle`
- `internal_transitions` populated from `probes.state_change_commit`

Perf profile enabled:
- `total_toggles = 0`
- `primary_input_transitions = 0`
- `internal_transitions = 0`

These values directly affect ICF and activity-family metrics.

## 8. History and Regression Fields
History lookup:
- base and previous GEPS/ICF are loaded from CSV via `lookup_history_for_bench_suite`

Anchor override:
- for `Revision_Combinational_` rows, base GEPS may be overridden by anchor mean GEPS when anchor token data is available

Regression gate field:
- `geps_delta_prev_pct = ((geps_p50 - prev_geps_p50) / prev_geps_p50) * 100`
- `geps_regression_fail = 1` when delta is below `-2.0`

## 9. Determinism Metadata
`logic_fingerprint` is CRC32 over final signal tray state.
`fingerprint_stable` is true only when all iteration CRC32 values match first iteration CRC32.

## 10. Output Contract
Derived values are persisted through:
- HTML rendering via `generate_master_report`
- CSV append via `append_metrics_history_csv`

CSV output includes profile metadata columns:
- `Probe_Profile`
- `Compat_Metrics_Enabled`

## 11. Change Control Guidance
If formulas are changed in runner implementation, update this file in the same change.
Keep formula names stable unless there is an approved naming change.