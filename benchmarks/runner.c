#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <windows.h>
#include "hebs_engine.h"
#include "timing_helper.h"
#include "protocol_helper.h"
#include "report_types.h"
#include "html_report.h"
#include "csv_export.h"

#define ITERATIONS 10
#define WARMUP_SECONDS 5.0
#define SIM_CYCLES 100
#define BENCH_ROOT "benchmarks/benches/"
#define MAX_BENCH_RESULTS 256
#define REVISION_NAME "Revision_Structure_v07"
#define METRICS_CSV_PATH "benchmarks/results/metrics_history.csv"
#define REPORT_HTML_PATH "benchmarks/results/revision_structure_v07.html"

static const char* hebs_basename_ptr(const char* path)
{
	const char* slash_a;
	const char* slash_b;
	const char* base;

	if (!path)
	{
		return "unknown";

	}

	slash_a = strrchr(path, '/');
	slash_b = strrchr(path, '\\');
	base = path;
	if (slash_a && slash_b)
	{
		base = (slash_a > slash_b) ? (slash_a + 1) : (slash_b + 1);

	}
	else if (slash_a)
	{
		base = slash_a + 1;

	}
	else if (slash_b)
	{
		base = slash_b + 1;

	}

	return base;

}

static void hebs_strip_extension(const char* in, char* out, size_t out_size)
{
	const char* dot;
	size_t copy_len;

	if (!out || out_size == 0U)
	{
		return;

	}

	if (!in)
	{
		out[0] = '\0';
		return;

	}

	dot = strrchr(in, '.');
	copy_len = dot ? (size_t)(dot - in) : strlen(in);
	if (copy_len >= out_size)
	{
		copy_len = out_size - 1U;

	}

	memcpy(out, in, copy_len);
	out[copy_len] = '\0';

}

static void hebs_get_run_clock(char* timestamp, size_t timestamp_size, char* date_text, size_t date_text_size)
{
	time_t now;
	struct tm* local;

	now = time(NULL);
	local = localtime(&now);
	if (!local)
	{
		snprintf(timestamp, timestamp_size, "00:00:00");
		snprintf(date_text, date_text_size, "1970-01-01");
		return;

	}

	strftime(timestamp, timestamp_size, "%H:%M:%S", local);
	strftime(date_text, date_text_size, "%Y-%m-%d", local);

}

static void hebs_get_git_commit_hash(char* out, size_t out_size)
{
	FILE* pipe;

	if (!out || out_size == 0U)
	{
		return;

	}

	snprintf(out, out_size, "unknown");
	pipe = _popen("git rev-parse --short HEAD 2>nul", "r");
	if (!pipe)
	{
		return;

	}

	if (fgets(out, (int)out_size, pipe))
	{
		size_t len = strlen(out);
		while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
		{
			out[len - 1] = '\0';
			--len;

		}

	}

	_pclose(pipe);

}

static void hebs_warmup(void)
{
	hebs_timer_t timer;
	printf("HEBS_CLEAN: Thermal Warm-up (%.0f s)...\n", WARMUP_SECONDS);
	timer_start(&timer);
	do
	{
		timer_stop(&timer);

	} while (timer_elapsed_sec(&timer) < WARMUP_SECONDS);

}

static uint32_t hebs_crc32_bytes(const uint8_t* data, size_t len)
{
	uint32_t crc;
	size_t i;
	int bit;

	crc = 0xFFFFFFFFU;
	for (i = 0; i < len; ++i)
	{
		crc ^= (uint32_t)data[i];
		for (bit = 0; bit < 8; ++bit)
		{
			uint32_t mask = (uint32_t)(-(int)(crc & 1U));
			crc = (crc >> 1U) ^ (0xEDB88320U & mask);

		}

	}

	return ~crc;

}

static double hebs_binary_entropy(double p)
{
	if (p <= 0.0 || p >= 1.0)
	{
		return 0.0;

	}

	return -(p * (log(p) / log(2.0)) + (1.0 - p) * (log(1.0 - p) / log(2.0)));

}

static void hebs_finalize_metric_row(hebs_metric_row_t* row, const double* runtimes, const double* geps_runs)
{
	double cycles_d;
	double gates_d;
	double toggles_d;
	double fanout_avg;

	cycles_d = (double)row->cycles;
	gates_d = (double)row->gate_count;
	toggles_d = (double)row->total_toggles;

	row->runtime_min = calculate_min(runtimes, ITERATIONS);
	row->runtime_max = calculate_max(runtimes, ITERATIONS);
	row->runtime_p50 = calculate_p50(runtimes, ITERATIONS);
	row->runtime_p90 = calculate_percentile(runtimes, ITERATIONS, 90.0);
	row->runtime_p95 = calculate_percentile(runtimes, ITERATIONS, 95.0);
	row->runtime_p99 = calculate_percentile(runtimes, ITERATIONS, 99.0);
	row->runtime_mean = calculate_mean(runtimes, ITERATIONS);
	row->runtime_variance = calculate_variance(runtimes, ITERATIONS);
	row->runtime_stddev = calculate_stddev(runtimes, ITERATIONS);

	row->geps_min = calculate_min(geps_runs, ITERATIONS);
	row->geps_max = calculate_max(geps_runs, ITERATIONS);
	row->geps_p50 = calculate_p50(geps_runs, ITERATIONS);
	row->gates_per_second = row->geps_p50;
	row->icf = calculate_icf(row->internal_transitions, row->primary_input_transitions);

	row->total_runtime = row->runtime_p50;
	row->throughput = (row->runtime_p50 > 0.0) ? (cycles_d / row->runtime_p50) : 0.0;
	row->latency_per_cycle = (cycles_d > 0.0) ? (row->runtime_p50 / cycles_d) : 0.0;
	row->latency_per_vector = row->latency_per_cycle;
	row->speedup = (row->runtime_p50 > 0.0) ? (row->runtime_max / row->runtime_p50) : 0.0;
	row->efficiency = (row->runtime_max > 0.0) ? (row->runtime_p50 / row->runtime_max) : 0.0;

	row->events_per_second = (row->runtime_p50 > 0.0) ? (toggles_d / row->runtime_p50) : 0.0;
	row->edges_per_second = row->events_per_second;
	row->vector_throughput = row->throughput;
	row->cycles_per_second = row->throughput;

	row->cycles_per_vector = 1.0;
	row->gates_per_cycle = (cycles_d > 0.0) ? (gates_d) : 0.0;
	row->events_per_cycle = (cycles_d > 0.0) ? (toggles_d / cycles_d) : 0.0;
	row->events_per_vector = row->events_per_cycle;
	row->net_transitions_per_cycle = row->events_per_cycle;

	fanout_avg = (row->signal_count > 0U) ? ((double)row->total_fanout_edges / (double)row->signal_count) : 0.0;
	row->fanout_avg = fanout_avg;
	row->fanout_activity = fanout_avg * row->events_per_cycle;
	row->toggle_rate = row->icf;
	row->activity_factor = row->icf;
	row->event_density = (gates_d > 0.0) ? (row->events_per_cycle / gates_d) : 0.0;
	row->work_per_event = (row->events_per_cycle > 0.0) ? (gates_d / row->events_per_cycle) : 0.0;
	row->evals_per_event = (toggles_d > 0.0) ? ((gates_d * cycles_d) / toggles_d) : 0.0;

	row->simulated_time_per_second = row->cycles_per_second;
	row->wallclock_per_sim_cycle = row->latency_per_cycle;
	row->utilized_edges_ratio = row->event_density;
	row->queue_utilization = row->event_density;

	row->critical_path_eval_rate = (row->runtime_p50 > 0.0) ? ((double)row->propagation_depth / row->runtime_p50) : 0.0;
	row->gate_density = (row->signal_count > 0U) ? ((double)row->gate_count / (double)row->signal_count) : 0.0;
	row->transition_entropy = hebs_binary_entropy(row->activity_factor);

}

static void hebs_apply_history_and_guardrail(hebs_metric_row_t* row)
{
	double base_geps;
	double prev_geps;
	double base_icf;
	double prev_icf;

	row->base_geps_p50 = row->geps_p50;
	row->prev_geps_p50 = row->geps_p50;
	row->base_icf = row->icf;
	row->prev_icf = row->icf;
	row->geps_delta_prev_pct = 0.0;
	row->icf_delta_prev_pct = 0.0;
	row->geps_regression_fail = 0U;

	if (!lookup_history_for_bench_suite(
		METRICS_CSV_PATH,
		row->suite_name,
		row->benchmark,
		&base_geps,
		&prev_geps,
		&base_icf,
		&prev_icf))
	{
		return;

	}

	row->base_geps_p50 = base_geps;
	row->prev_geps_p50 = prev_geps;
	row->base_icf = base_icf;
	row->prev_icf = prev_icf;

	if (prev_geps > 0.0)
	{
		row->geps_delta_prev_pct = ((row->geps_p50 - prev_geps) / prev_geps) * 100.0;
		if (row->geps_delta_prev_pct < -2.0)
		{
			row->geps_regression_fail = 1U;

		}

	}

	if (prev_icf > 0.0)
	{
		row->icf_delta_prev_pct = ((row->icf - prev_icf) / prev_icf) * 100.0;

	}

}

static int hebs_run_single_bench(const char* suite_name, const char* bench_path, hebs_metric_row_t* out_row)
{
	double runtimes[ITERATIONS];
	double geps_runs[ITERATIONS];
	uint32_t crc_runs[ITERATIONS];
	hebs_metrics metrics;
	uint64_t total_toggles;
	uint64_t total_primary_input_transitions;
	uint64_t total_internal_transitions;
	uint32_t total_cycles;
	uint32_t stable_crc;
	int crc_stable;
	int i;

	if (!suite_name || !bench_path || !out_row)
	{
		return 0;

	}

	memset(out_row, 0, sizeof(*out_row));
	snprintf(out_row->suite_name, sizeof(out_row->suite_name), "%s", suite_name);
	total_toggles = 0U;
	total_primary_input_transitions = 0U;
	total_internal_transitions = 0U;
	total_cycles = 0U;
	crc_stable = 1;
	stable_crc = 0U;

	printf("\nBENCH: %s/%s\n", suite_name, bench_path);

	for (i = 0; i < ITERATIONS; ++i)
	{
		hebs_plan* plan = hebs_load_bench(bench_path);
		hebs_engine engine = { 0 };
		hebs_timer_t timer;
		uint32_t cycle;

		if (!plan || hebs_init_engine(&engine, plan) != HEBS_OK)
		{
			printf("Iteration %d: init failed\n", i + 1);
			hebs_free_plan(plan);
			return 0;

		}

		timer_start(&timer);
		for (cycle = 0; cycle < SIM_CYCLES; ++cycle)
		{
			uint32_t pi;
			for (pi = 0; pi < plan->num_primary_inputs && pi < HEBS_MAX_PRIMARY_INPUTS; ++pi)
			{
				hebs_logic_t value = ((cycle + pi) & 1U) ? HEBS_S1 : HEBS_S0;
				if (hebs_set_primary_input(&engine, plan, pi, value) != HEBS_OK)
				{
					printf("Iteration %d: input set failed\n", i + 1);
					hebs_free_plan(plan);
					return 0;
				}

			}

			hebs_tick(&engine, plan);

		}

		timer_stop(&timer);
		runtimes[i] = timer_elapsed_sec(&timer);
		geps_runs[i] = (runtimes[i] > 0.0) ? (((double)plan->gate_count * (double)SIM_CYCLES) / runtimes[i]) : 0.0;
		crc_runs[i] = hebs_crc32_bytes((const uint8_t*)engine.signal_trays, (size_t)engine.tray_count * sizeof(uint64_t));
		metrics = hebs_get_metrics(&engine, plan);

		if (i == 0)
		{
			stable_crc = crc_runs[i];

		}
		else if (crc_runs[i] != stable_crc)
		{
			crc_stable = 0;

		}

		total_toggles += metrics.primary_input_transitions;
		total_primary_input_transitions += metrics.primary_input_transitions;
		total_internal_transitions += metrics.internal_node_transitions;
		total_cycles += (uint32_t)metrics.cycles_executed;
		printf("Iteration %d: %.9f sec\n", i + 1, runtimes[i]);

		if (i == ITERATIONS - 1)
		{
			char bench_name[128];
			hebs_strip_extension(hebs_basename_ptr(bench_path), bench_name, sizeof(bench_name));
			snprintf(out_row->benchmark, sizeof(out_row->benchmark), "%s", bench_name);
			out_row->pi_count = metrics.pi_count;
			out_row->cycles = total_cycles;
			out_row->gate_count = metrics.gate_count;
			out_row->signal_count = metrics.net_count;
			out_row->propagation_depth = metrics.level_depth;
			out_row->fanout_max = metrics.fanout_max;
			out_row->total_fanout_edges = plan->total_fanout_edges;
			out_row->plan_fingerprint = plan->lep_hash;

		}

		hebs_free_plan(plan);

	}

	out_row->logic_fingerprint = stable_crc;
	out_row->fingerprint_stable = (uint8_t)crc_stable;
	out_row->total_toggles = total_toggles;
	out_row->primary_input_transitions = total_primary_input_transitions;
	out_row->internal_transitions = total_internal_transitions;
	hebs_finalize_metric_row(out_row, runtimes, geps_runs);
	hebs_apply_history_and_guardrail(out_row);

	printf("\n--- FINAL REPORT (p50 Median) ---\n");
	printf("Runtime Low: %.9f sec\n", out_row->runtime_min);
	printf("Runtime High: %.9f sec\n", out_row->runtime_max);
	printf("Median Runtime: %.9f sec\n", out_row->runtime_p50);
	printf("Active Primary Inputs: %u\n", out_row->pi_count);
	printf("Programmatic Gate Count: %u\n", out_row->gate_count);
	printf("Propagation Depth: %u\n", out_row->propagation_depth);
	printf("Fanout Max: %u\n", out_row->fanout_max);
	printf("Base/Prev/Cur GEPS: %.2f / %.2f / %.2f\n", out_row->base_geps_p50, out_row->prev_geps_p50, out_row->geps_p50);
	printf("Base/Prev/Cur ICF: %.6f / %.6f / %.6f\n", out_row->base_icf, out_row->prev_icf, out_row->icf);
	printf("GEPS Delta Prev vs Cur: %.2f%%\n", out_row->geps_delta_prev_pct);
	printf("Logic CRC32: 0x%08X (stable=%s)\n", (unsigned int)out_row->logic_fingerprint, out_row->fingerprint_stable ? "YES" : "NO");
	printf("---------------------------------\n");
	return 1;

}

int main(void)
{
	WIN32_FIND_DATAA suite_find_data;
	HANDLE suite_find_handle;
	hebs_metric_row_t registry[MAX_BENCH_RESULTS];
	char suite_glob[512];
	char bench_glob[512];
	char bench_path[512];
	char timestamp[32];
	char date_text[32];
	char git_hash[64];
	int result_count;
	int failures;

	result_count = 0;
	failures = 0;

	hebs_warmup();
	hebs_get_run_clock(timestamp, sizeof(timestamp), date_text, sizeof(date_text));
	hebs_get_git_commit_hash(git_hash, sizeof(git_hash));

	snprintf(suite_glob, sizeof(suite_glob), "%s*", BENCH_ROOT);
	suite_find_handle = FindFirstFileA(suite_glob, &suite_find_data);
	if (suite_find_handle == INVALID_HANDLE_VALUE)
	{
		printf("No suite directories found under %s\n", BENCH_ROOT);
		return 1;

	}

	do
	{
		WIN32_FIND_DATAA bench_find_data;
		HANDLE bench_find_handle;
		char suite_name[64];

		if ((suite_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0U)
		{
			continue;

		}

		if (strcmp(suite_find_data.cFileName, ".") == 0 || strcmp(suite_find_data.cFileName, "..") == 0)
		{
			continue;

		}

		{
			size_t len = strnlen(suite_find_data.cFileName, sizeof(suite_name) - 1U);
			memcpy(suite_name, suite_find_data.cFileName, len);
			suite_name[len] = '\0';
		}
		snprintf(bench_glob, sizeof(bench_glob), "%s%s/*.bench", BENCH_ROOT, suite_name);
		bench_find_handle = FindFirstFileA(bench_glob, &bench_find_data);
		if (bench_find_handle == INVALID_HANDLE_VALUE)
		{
			continue;

		}

		do
		{
			if ((bench_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0U)
			{
				continue;

			}

			if (result_count >= MAX_BENCH_RESULTS)
			{
				++failures;
				break;

			}

			snprintf(bench_path, sizeof(bench_path), "%s%s/%s", BENCH_ROOT, suite_name, bench_find_data.cFileName);
			if (!hebs_run_single_bench(suite_name, bench_path, &registry[result_count]))
			{
				++failures;
				continue;

			}

			if (registry[result_count].geps_regression_fail)
			{
				printf("REGRESSION FAIL: %s/%s p50 GEPS dropped %.2f%% versus previous.\n", registry[result_count].suite_name, registry[result_count].benchmark, -registry[result_count].geps_delta_prev_pct);
				++failures;

			}

			if (!registry[result_count].fingerprint_stable)
			{
				printf("FINGERPRINT FAIL: %s/%s tray CRC32 unstable.\n", registry[result_count].suite_name, registry[result_count].benchmark);
				++failures;

			}

			++result_count;

		} while (FindNextFileA(bench_find_handle, &bench_find_data) != 0);

		FindClose(bench_find_handle);

	} while (FindNextFileA(suite_find_handle, &suite_find_data) != 0);

	FindClose(suite_find_handle);

	if (result_count > 0)
	{
		if (!generate_master_report(
			registry,
			result_count,
			REVISION_NAME,
			timestamp,
			date_text,
			git_hash,
			REPORT_HTML_PATH))
		{
			printf("Failed to write report HTML\n");
			++failures;

		}

		if (!append_metrics_history_csv(
			registry,
			result_count,
			REVISION_NAME,
			timestamp,
			date_text,
			git_hash,
			METRICS_CSV_PATH))
		{
			printf("Failed to append metrics history CSV\n");
			++failures;

		}

	}
	else
	{
		++failures;

	}

	printf("\nSUITE SUMMARY: processed=%d failed=%d\n", result_count, failures);
	printf("Revision HTML: %s\n", REPORT_HTML_PATH);
	printf("CSV Ledger: %s\n", METRICS_CSV_PATH);
	return (failures == 0) ? 0 : 1;

}
