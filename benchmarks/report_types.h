#ifndef HEBS_REPORT_TYPES_H
#define HEBS_REPORT_TYPES_H

#include <stdint.h>

typedef struct hebs_metric_row_s
{
	char suite_name[64];
	char benchmark[128];
	uint32_t pi_count;
	uint32_t cycles;
	uint32_t gate_count;
	uint32_t signal_count;
	uint32_t propagation_depth;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	uint64_t plan_fingerprint;
	uint32_t logic_fingerprint;
	uint8_t fingerprint_stable;
	uint64_t total_toggles;
	uint64_t primary_input_transitions;
	uint64_t internal_transitions;
	double runtime_min;
	double runtime_max;
	double runtime_p50;
	double runtime_p90;
	double runtime_p95;
	double runtime_p99;
	double runtime_mean;
	double runtime_variance;
	double runtime_stddev;
	double total_runtime;
	double throughput;
	double latency_per_cycle;
	double latency_per_vector;
	double speedup;
	double efficiency;
	double gates_per_second;
	double geps_min;
	double geps_p50;
	double geps_max;
	double events_per_second;
	double edges_per_second;
	double vector_throughput;
	double cycles_per_second;
	double cycles_per_vector;
	double gates_per_cycle;
	double events_per_cycle;
	double events_per_vector;
	double net_transitions_per_cycle;
	double fanout_activity;
	double toggle_rate;
	double activity_factor;
	double event_density;
	double work_per_event;
	double evals_per_event;
	double simulated_time_per_second;
	double wallclock_per_sim_cycle;
	double utilized_edges_ratio;
	double queue_utilization;
	double critical_path_eval_rate;
	double fanout_avg;
	double gate_density;
	double transition_entropy;
	double icf;
	double base_geps_p50;
	double prev_geps_p50;
	double base_icf;
	double prev_icf;
	double geps_delta_prev_pct;
	double icf_delta_prev_pct;
	uint8_t geps_regression_fail;
	char probe_profile[16];
	uint8_t test_probes_enabled;

} hebs_metric_row_t;

#endif
