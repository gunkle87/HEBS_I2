# BENCH_METRICS.md

This document defines benchmark metrics and their formulas used by the
benchmark runner. All metrics are calculated outside the engine using
runner data, probes, and timing results.

------------------------------------------------------------------------

# Core Transition Metric

ICF Formula: ICF = internal_node_transitions / primary_input_transitions

------------------------------------------------------------------------

# Statistical Metrics

p50 (median) Formula: p50 = median(runtime_samples)

p90 Formula: p90 = percentile(runtime_samples, 90)

p95 Formula: p95 = percentile(runtime_samples, 95)

p99 Formula: p99 = percentile(runtime_samples, 99)

min Formula: min = minimum(runtime_samples)

max Formula: max = maximum(runtime_samples)

mean Formula: mean = sum(runtime_samples) / N

variance Formula: variance = Σ(x - mean)\^2 / N

stddev Formula: stddev = sqrt(variance)

------------------------------------------------------------------------

# Execution Metrics

total_runtime Formula: total_runtime = end_time - start_time

throughput Formula: throughput = work_units / total_runtime

latency_per_cycle Formula: latency_per_cycle = total_runtime /
total_cycles

latency_per_vector Formula: latency_per_vector = total_runtime /
total_vectors

speedup Formula: speedup = baseline_runtime / current_runtime

efficiency Formula: efficiency = speedup / number_of_cores

------------------------------------------------------------------------

# Simulation Metrics

GEPS (Gates Evaluated Per Second) Formula: GEPS = total_gates_evaluated
/ total_runtime

gates_per_second Formula: gates_per_second = total_gates_evaluated /
total_runtime

events_per_second Formula: events_per_second = total_events /
total_runtime

edges_per_second Formula: edges_per_second = total_signal_edges /
total_runtime

vector_throughput Formula: vector_throughput = vectors_processed /
total_runtime

cycles_per_second Formula: cycles_per_second = total_cycles /
total_runtime

------------------------------------------------------------------------

# Logic Activity Metrics

cycles_per_vector Formula: cycles_per_vector = total_cycles /
total_vectors

gates_per_cycle Formula: gates_per_cycle = total_gates_evaluated /
total_cycles

events_per_cycle Formula: events_per_cycle = total_events / total_cycles

events_per_vector Formula: events_per_vector = total_events /
total_vectors

net_transitions_per_cycle Formula: net_transitions_per_cycle =
total_net_transitions / total_cycles

------------------------------------------------------------------------

# Activity Metrics

fanout_activity Formula: fanout_activity = total_events /
total_fanout_edges

toggle_rate Formula: toggle_rate = total_net_transitions / total_nets

activity_factor Formula: activity_factor = total_net_transitions /
(total_nets \* total_cycles)

event_density Formula: event_density = total_events / total_gates

work_per_event Formula: work_per_event = total_gates_evaluated /
total_events

evals_per_event Formula: evals_per_event = total_gate_evaluations /
total_events

------------------------------------------------------------------------

# System / Cache Metrics

instructions_per_cycle Formula: instructions_per_cycle =
instructions_retired / cpu_cycles

cycles_per_gate_eval Formula: cycles_per_gate_eval = cpu_cycles /
total_gates_evaluated

cycles_per_event Formula: cycles_per_event = cpu_cycles / total_events

instructions_per_gate_eval Formula: instructions_per_gate_eval =
instructions_retired / total_gates_evaluated

instructions_per_event Formula: instructions_per_event =
instructions_retired / total_events

Other system metrics (best effort): cache_miss_rate
branch_mispredict_rate memory_bandwidth scheduler_utilization
stall_ratio compute_ratio

These depend on hardware counters when available.

------------------------------------------------------------------------

# Temporal Metrics

simulated_time_per_second Formula: simulated_time_per_second =
simulated_time / total_runtime

wallclock_per_sim_cycle Formula: wallclock_per_sim_cycle = total_runtime
/ total_cycles

utilized_edges_ratio Formula: utilized_edges_ratio = active_edges /
total_edges

queue_utilization Formula: queue_utilization = queue_active_time /
queue_total_time

------------------------------------------------------------------------

# Topological Metrics

propagation_depth Formula: propagation_depth = maximum_logic_level

critical_path_eval_rate Formula: critical_path_eval_rate =
critical_path_evaluations / total_runtime

fanout_avg Formula: fanout_avg = total_fanout_edges / total_nets

fanout_max Formula: fanout_max = max(fanout(net))

gate_density Formula: gate_density = total_gates / total_nets

transition_entropy Formula: transition_entropy = - Σ(p_i \* log2(p_i))

Where p_i is the probability of each transition type.
