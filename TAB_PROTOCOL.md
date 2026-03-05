# HEBS Canonical Testing & Benchmarking Protocol
Version: 1.1.0
Status: MANDATORY
Authority: HEBS_SNAPSHOT.md

## 1. Canonical Build Policy
* **Requirement**: All binaries for testing and benchmarking must use the same build process.
* **Profile**: Use the `perf` profile: `gcc -O3 -march=native -DNDEBUG -static -fno-plt`.
* **Environment**: Benchmarks must be run via `taskset -c <core_id>` on a single physical core with OS "Performance" governor active.
* **Constraint**: No environment-specific overrides or variant parameters are permitted to prevent results skew.

## 2. Testing Framework (The Stress Runner)
* **Scope**: Comprehensive testing of gates, combinational loops, oscillation, and 7-state resolution.
* **Execution**: Triggered at every core code edit to prevent logic regression.
* **Reporting**: Record results only at the completion of a minor or major revision phase; no intermittent logging.
* **8-State Integrity**: Mandatory PopCount check for all 8 states (U, X, 0, 1, Z, W, L, H) to ensure no state collapse occurs during optimization.
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