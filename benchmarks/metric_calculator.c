#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef RAW_INPUT_PATH
#define RAW_INPUT_PATH "benchmarks/results/raw_runner_output.csv"
#endif

#ifndef DERIVED_OUTPUT_PATH
#define DERIVED_OUTPUT_PATH "benchmarks/results/metrics_derived.csv"
#endif

#define HEBS_RAW_COL_COUNT 27
#define HEBS_DERIVED_COL_COUNT 41

typedef struct hebs_calc_options_s
{
	char input_csv[260];
	char output_csv[260];
	int append_output;

} hebs_calc_options_t;

typedef struct hebs_metric_group_s
{
	char run_id[256];
	char revision[64];
	char date_text[32];
	char time_text[32];
	char git_commit[64];
	char suite_name[64];
	char bench_name[128];
	char mode_name[16];
	uint32_t cycles;
	uint32_t gate_count;
	uint32_t pi_count;
	uint32_t signal_count;
	uint32_t propagation_depth;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	uint8_t test_probes_enabled;
	double* runtimes_sec;
	double* geps_values;
	double* icf_values;
	uint32_t* logic_crc_values;
	uint32_t sample_count;
	uint32_t sample_capacity;
	double base_geps_p50;
	double prev_geps_p50;
	double base_icf_p50;
	double prev_icf_p50;
	int history_found;

} hebs_metric_group_t;

enum hebs_raw_col_index_e
{
	HEBS_RAW_RUN_ID = 0,
	HEBS_RAW_REVISION = 1,
	HEBS_RAW_DATE = 2,
	HEBS_RAW_TIME = 3,
	HEBS_RAW_GIT_COMMIT = 4,
	HEBS_RAW_SUITE = 5,
	HEBS_RAW_BENCH = 6,
	HEBS_RAW_MODE = 7,
	HEBS_RAW_ITERATION = 8,
	HEBS_RAW_ITERATIONS_TOTAL = 9,
	HEBS_RAW_CYCLES = 10,
	HEBS_RAW_RUNTIME_SEC = 11,
	HEBS_RAW_PLAN_FINGERPRINT = 12,
	HEBS_RAW_LOGIC_CRC32 = 13,
	HEBS_RAW_PROBE_INPUT_APPLY = 14,
	HEBS_RAW_PROBE_INPUT_TOGGLE = 15,
	HEBS_RAW_PROBE_CHUNK_EXEC = 16,
	HEBS_RAW_PROBE_GATE_EVAL = 17,
	HEBS_RAW_PROBE_STATE_CHANGE_COMMIT = 18,
	HEBS_RAW_PROBE_DFF_EXEC = 19,
	HEBS_RAW_PI_COUNT = 20,
	HEBS_RAW_GATE_COUNT = 21,
	HEBS_RAW_SIGNAL_COUNT = 22,
	HEBS_RAW_PROPAGATION_DEPTH = 23,
	HEBS_RAW_FANOUT_MAX = 24,
	HEBS_RAW_TOTAL_FANOUT_EDGES = 25,
	HEBS_RAW_TEST_PROBES_ENABLED = 26
};

enum hebs_derived_col_index_e
{
	HEBS_DERIVED_RUN_ID = 0,
	HEBS_DERIVED_REVISION = 1,
	HEBS_DERIVED_DATE = 2,
	HEBS_DERIVED_TIME = 3,
	HEBS_DERIVED_GIT_COMMIT = 4,
	HEBS_DERIVED_SUITE = 5,
	HEBS_DERIVED_BENCH = 6,
	HEBS_DERIVED_MODE = 7,
	HEBS_DERIVED_ITERATIONS = 8,
	HEBS_DERIVED_CYCLES = 9,
	HEBS_DERIVED_GATE_COUNT = 10,
	HEBS_DERIVED_PI_COUNT = 11,
	HEBS_DERIVED_SIGNAL_COUNT = 12,
	HEBS_DERIVED_PROPAGATION_DEPTH = 13,
	HEBS_DERIVED_FANOUT_MAX = 14,
	HEBS_DERIVED_TOTAL_FANOUT_EDGES = 15,
	HEBS_DERIVED_RUNTIME_MIN = 16,
	HEBS_DERIVED_RUNTIME_P50 = 17,
	HEBS_DERIVED_RUNTIME_P90 = 18,
	HEBS_DERIVED_RUNTIME_P95 = 19,
	HEBS_DERIVED_RUNTIME_P99 = 20,
	HEBS_DERIVED_RUNTIME_MAX = 21,
	HEBS_DERIVED_RUNTIME_MEAN = 22,
	HEBS_DERIVED_RUNTIME_STDDEV = 23,
	HEBS_DERIVED_GEPS_MIN = 24,
	HEBS_DERIVED_GEPS_P50 = 25,
	HEBS_DERIVED_GEPS_MAX = 26,
	HEBS_DERIVED_GEPS_MEAN = 27,
	HEBS_DERIVED_ICF_MIN = 28,
	HEBS_DERIVED_ICF_P50 = 29,
	HEBS_DERIVED_ICF_MAX = 30,
	HEBS_DERIVED_ICF_MEAN = 31,
	HEBS_DERIVED_BASE_GEPS = 32,
	HEBS_DERIVED_PREV_GEPS = 33,
	HEBS_DERIVED_DELTA_GEPS_PCT = 34,
	HEBS_DERIVED_BASE_ICF = 35,
	HEBS_DERIVED_PREV_ICF = 36,
	HEBS_DERIVED_DELTA_ICF_PCT = 37,
	HEBS_DERIVED_FINGERPRINT_STABLE = 38,
	HEBS_DERIVED_LOGIC_CRC32 = 39,
	HEBS_DERIVED_TEST_PROBES_ENABLED = 40
};

static int hebs_copy_text_fit(char* out, size_t out_size, const char* text)
{
	size_t length;

	if (!out || out_size == 0U || !text)
	{
		return 0;

	}

	length = strlen(text);
	if (length >= out_size)
	{
		return 0;

	}

	memcpy(out, text, length + 1U);
	return 1;

}

static char* hebs_trim_in_place(char* text)
{
	char* end;

	if (!text)
	{
		return text;

	}

	while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
	{
		++text;

	}

	if (*text == '\0')
	{
		return text;

	}

	end = text + strlen(text) - 1;
	while (end > text && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
	{
		*end = '\0';
		--end;

	}

	return text;

}

static int hebs_is_compat_metrics_header(const char* token)
{
	if (!token)
	{
		return 0;

	}

	return (strcmp(token, "compat_metrics_enabled") == 0 || strcmp(token, "test_probes_enabled") == 0);

}

static int hebs_split_csv_simple(char* line, char** fields, int max_fields)
{
	int count;
	char* cursor;

	if (!line || !fields || max_fields <= 0)
	{
		return 0;

	}

	count = 0;
	cursor = line;
	fields[count++] = cursor;

	while (*cursor)
	{
		if (*cursor == ',')
		{
			*cursor = '\0';
			if (count >= max_fields)
			{
				return count;

			}

			fields[count++] = cursor + 1;

		}

		++cursor;

	}

	return count;

}

static int hebs_parse_u32(const char* text, uint32_t* out_value)
{
	char* end_ptr;
	unsigned long value;

	if (!text || !out_value)
	{
		return 0;

	}

	value = strtoul(text, &end_ptr, 10);
	if (*text == '\0' || *end_ptr != '\0' || value > 0xFFFFFFFFUL)
	{
		return 0;

	}

	*out_value = (uint32_t)value;
	return 1;

}

static int hebs_parse_u64(const char* text, uint64_t* out_value)
{
	char* end_ptr;
	unsigned long long value;

	if (!text || !out_value)
	{
		return 0;

	}

	value = strtoull(text, &end_ptr, 10);
	if (*text == '\0' || *end_ptr != '\0')
	{
		return 0;

	}

	*out_value = (uint64_t)value;
	return 1;

}

static int hebs_parse_hex_u32(const char* text, uint32_t* out_value)
{
	char* end_ptr;
	unsigned long value;

	if (!text || !out_value)
	{
		return 0;

	}

	value = strtoul(text, &end_ptr, 0);
	if (*text == '\0' || *end_ptr != '\0' || value > 0xFFFFFFFFUL)
	{
		return 0;

	}

	*out_value = (uint32_t)value;
	return 1;

}

static int hebs_parse_double(const char* text, double* out_value)
{
	char* end_ptr;
	double value;

	if (!text || !out_value)
	{
		return 0;

	}

	value = strtod(text, &end_ptr);
	if (*text == '\0' || *end_ptr != '\0')
	{
		return 0;

	}

	*out_value = value;
	return 1;

}

static int hebs_file_is_empty(const char* path)
{
	FILE* file;
	long size;

	if (!path)
	{
		return 1;

	}

	file = fopen(path, "rb");
	if (!file)
	{
		return 1;

	}

	if (fseek(file, 0L, SEEK_END) != 0)
	{
		fclose(file);
		return 1;

	}

	size = ftell(file);
	fclose(file);
	return (size <= 0L);

}

static int hebs_reserve_groups(hebs_metric_group_t** groups, uint32_t* capacity, uint32_t needed)
{
	hebs_metric_group_t* resized;
	uint32_t new_capacity;

	if (!groups || !capacity)
	{
		return 0;

	}

	if (needed <= *capacity)
	{
		return 1;

	}

	new_capacity = (*capacity == 0U) ? 16U : (*capacity * 2U);
	while (new_capacity < needed)
	{
		new_capacity *= 2U;

	}

	resized = (hebs_metric_group_t*)realloc(*groups, (size_t)new_capacity * sizeof(hebs_metric_group_t));
	if (!resized)
	{
		return 0;

	}

	*groups = resized;
	*capacity = new_capacity;
	return 1;

}

static int hebs_reserve_samples(hebs_metric_group_t* group, uint32_t needed)
{
	double* runtimes;
	double* geps;
	double* icf;
	uint32_t* crc;
	uint32_t new_capacity;

	if (!group)
	{
		return 0;

	}

	if (needed <= group->sample_capacity)
	{
		return 1;

	}

	new_capacity = (group->sample_capacity == 0U) ? 16U : (group->sample_capacity * 2U);
	while (new_capacity < needed)
	{
		new_capacity *= 2U;

	}

	runtimes = (double*)realloc(group->runtimes_sec, (size_t)new_capacity * sizeof(double));
	geps = (double*)realloc(group->geps_values, (size_t)new_capacity * sizeof(double));
	icf = (double*)realloc(group->icf_values, (size_t)new_capacity * sizeof(double));
	crc = (uint32_t*)realloc(group->logic_crc_values, (size_t)new_capacity * sizeof(uint32_t));
	if (!runtimes || !geps || !icf || !crc)
	{
		free(runtimes);
		free(geps);
		free(icf);
		free(crc);
		return 0;

	}

	group->runtimes_sec = runtimes;
	group->geps_values = geps;
	group->icf_values = icf;
	group->logic_crc_values = crc;
	group->sample_capacity = new_capacity;
	return 1;

}

static void hebs_free_groups(hebs_metric_group_t* groups, uint32_t group_count)
{
	uint32_t idx;

	if (!groups)
	{
		return;

	}

	for (idx = 0U; idx < group_count; ++idx)
	{
		free(groups[idx].runtimes_sec);
		free(groups[idx].geps_values);
		free(groups[idx].icf_values);
		free(groups[idx].logic_crc_values);

	}

	free(groups);

}

static int hebs_groups_match(const hebs_metric_group_t* group, const char* run_id, const char* suite_name, const char* bench_name, const char* mode_name)
{
	if (!group || !run_id || !suite_name || !bench_name || !mode_name)
	{
		return 0;

	}

	return (strcmp(group->run_id, run_id) == 0 &&
		strcmp(group->suite_name, suite_name) == 0 &&
		strcmp(group->bench_name, bench_name) == 0 &&
		strcmp(group->mode_name, mode_name) == 0);

}

static int hebs_create_group(
	hebs_metric_group_t** groups,
	uint32_t* group_count,
	uint32_t* group_capacity,
	const char** fields,
	hebs_metric_group_t** out_group)
{
	hebs_metric_group_t* slot;
	uint32_t cycles;
	uint32_t gate_count;
	uint32_t pi_count;
	uint32_t signal_count;
	uint32_t propagation_depth;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	uint32_t test_probes_enabled;

	if (!groups || !group_count || !group_capacity || !fields || !out_group)
	{
		return 0;

	}

	if (!hebs_parse_u32(fields[HEBS_RAW_CYCLES], &cycles) ||
		!hebs_parse_u32(fields[HEBS_RAW_GATE_COUNT], &gate_count) ||
		!hebs_parse_u32(fields[HEBS_RAW_PI_COUNT], &pi_count) ||
		!hebs_parse_u32(fields[HEBS_RAW_SIGNAL_COUNT], &signal_count) ||
		!hebs_parse_u32(fields[HEBS_RAW_PROPAGATION_DEPTH], &propagation_depth) ||
		!hebs_parse_u32(fields[HEBS_RAW_FANOUT_MAX], &fanout_max) ||
		!hebs_parse_u32(fields[HEBS_RAW_TOTAL_FANOUT_EDGES], &total_fanout_edges) ||
		!hebs_parse_u32(fields[HEBS_RAW_TEST_PROBES_ENABLED], &test_probes_enabled))
	{
		return 0;

	}

	if (!hebs_reserve_groups(groups, group_capacity, *group_count + 1U))
	{
		return 0;

	}

	slot = &(*groups)[*group_count];
	memset(slot, 0, sizeof(*slot));

	if (!hebs_copy_text_fit(slot->run_id, sizeof(slot->run_id), fields[HEBS_RAW_RUN_ID]) ||
		!hebs_copy_text_fit(slot->revision, sizeof(slot->revision), fields[HEBS_RAW_REVISION]) ||
		!hebs_copy_text_fit(slot->date_text, sizeof(slot->date_text), fields[HEBS_RAW_DATE]) ||
		!hebs_copy_text_fit(slot->time_text, sizeof(slot->time_text), fields[HEBS_RAW_TIME]) ||
		!hebs_copy_text_fit(slot->git_commit, sizeof(slot->git_commit), fields[HEBS_RAW_GIT_COMMIT]) ||
		!hebs_copy_text_fit(slot->suite_name, sizeof(slot->suite_name), fields[HEBS_RAW_SUITE]) ||
		!hebs_copy_text_fit(slot->bench_name, sizeof(slot->bench_name), fields[HEBS_RAW_BENCH]) ||
		!hebs_copy_text_fit(slot->mode_name, sizeof(slot->mode_name), fields[HEBS_RAW_MODE]))
	{
		return 0;

	}

	slot->cycles = cycles;
	slot->gate_count = gate_count;
	slot->pi_count = pi_count;
	slot->signal_count = signal_count;
	slot->propagation_depth = propagation_depth;
	slot->fanout_max = fanout_max;
	slot->total_fanout_edges = total_fanout_edges;
	slot->test_probes_enabled = (uint8_t)test_probes_enabled;
	*out_group = slot;
	++(*group_count);
	return 1;

}

static int hebs_append_group_sample(hebs_metric_group_t* group, const char** fields)
{
	double runtime_sec;
	uint64_t input_toggle;
	uint64_t state_change_commit;
	uint32_t logic_crc32;
	double geps;
	double icf;

	if (!group || !fields)
	{
		return 0;

	}

	if (!hebs_parse_double(fields[HEBS_RAW_RUNTIME_SEC], &runtime_sec) ||
		!hebs_parse_u64(fields[HEBS_RAW_PROBE_INPUT_TOGGLE], &input_toggle) ||
		!hebs_parse_u64(fields[HEBS_RAW_PROBE_STATE_CHANGE_COMMIT], &state_change_commit) ||
		!hebs_parse_hex_u32(fields[HEBS_RAW_LOGIC_CRC32], &logic_crc32))
	{
		return 0;

	}

	geps = 0.0;
	if (runtime_sec > 0.0)
	{
		geps = ((double)group->gate_count * (double)group->cycles) / runtime_sec / 1000000000.0;

	}

	icf = (input_toggle > 0U) ? ((double)state_change_commit / (double)input_toggle) : 0.0;

	if (!hebs_reserve_samples(group, group->sample_count + 1U))
	{
		return 0;

	}

	group->runtimes_sec[group->sample_count] = runtime_sec;
	group->geps_values[group->sample_count] = geps;
	group->icf_values[group->sample_count] = icf;
	group->logic_crc_values[group->sample_count] = logic_crc32;
	++group->sample_count;
	return 1;

}

static int hebs_group_cmp(const void* lhs, const void* rhs)
{
	const hebs_metric_group_t* a;
	const hebs_metric_group_t* b;
	int suite_cmp;
	int bench_cmp;
	int run_cmp;

	a = (const hebs_metric_group_t*)lhs;
	b = (const hebs_metric_group_t*)rhs;

	suite_cmp = strcmp(a->suite_name, b->suite_name);
	if (suite_cmp != 0)
	{
		return suite_cmp;

	}

	bench_cmp = strcmp(a->bench_name, b->bench_name);
	if (bench_cmp != 0)
	{
		return bench_cmp;

	}

	run_cmp = strcmp(a->run_id, b->run_id);
	if (run_cmp != 0)
	{
		return run_cmp;

	}

	return strcmp(a->mode_name, b->mode_name);

}

static int hebs_double_cmp(const void* lhs, const void* rhs)
{
	double a;
	double b;

	a = *(const double*)lhs;
	b = *(const double*)rhs;

	if (a < b)
	{
		return -1;

	}

	if (a > b)
	{
		return 1;

	}

	return 0;

}

static double hebs_calculate_mean(const double* values, uint32_t count)
{
	double sum;
	uint32_t idx;

	if (!values || count == 0U)
	{
		return 0.0;

	}

	sum = 0.0;
	for (idx = 0U; idx < count; ++idx)
	{
		sum += values[idx];

	}

	return sum / (double)count;

}

static double hebs_calculate_stddev(const double* values, uint32_t count, double mean)
{
	double variance_sum;
	uint32_t idx;

	if (!values || count == 0U)
	{
		return 0.0;

	}

	variance_sum = 0.0;
	for (idx = 0U; idx < count; ++idx)
	{
		double delta = values[idx] - mean;
		variance_sum += delta * delta;

	}

	return sqrt(variance_sum / (double)count);

}

static double hebs_calculate_percentile(const double* values, uint32_t count, double percentile)
{
	double* sorted;
	double index;
	uint32_t lower;
	uint32_t upper;
	double fraction;
	double result;

	if (!values || count == 0U)
	{
		return 0.0;

	}

	if (percentile < 0.0)
	{
		percentile = 0.0;

	}
	else if (percentile > 100.0)
	{
		percentile = 100.0;

	}

	sorted = (double*)malloc((size_t)count * sizeof(double));
	if (!sorted)
	{
		return 0.0;

	}

	memcpy(sorted, values, (size_t)count * sizeof(double));
	qsort(sorted, count, sizeof(double), hebs_double_cmp);

	index = (percentile / 100.0) * (double)(count - 1U);
	lower = (uint32_t)index;
	upper = (lower + 1U < count) ? (lower + 1U) : lower;
	fraction = index - (double)lower;
	result = sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;

	free(sorted);
	return result;

}

static double hebs_calculate_min(const double* values, uint32_t count)
{
	double min_value;
	uint32_t idx;

	if (!values || count == 0U)
	{
		return 0.0;

	}

	min_value = values[0];
	for (idx = 1U; idx < count; ++idx)
	{
		if (values[idx] < min_value)
		{
			min_value = values[idx];

		}

	}

	return min_value;

}

static double hebs_calculate_max(const double* values, uint32_t count)
{
	double max_value;
	uint32_t idx;

	if (!values || count == 0U)
	{
		return 0.0;

	}

	max_value = values[0];
	for (idx = 1U; idx < count; ++idx)
	{
		if (values[idx] > max_value)
		{
			max_value = values[idx];

		}

	}

	return max_value;

}

static int hebs_is_fingerprint_stable(const hebs_metric_group_t* group, uint32_t* out_crc32)
{
	uint32_t idx;
	uint32_t reference;

	if (!group || group->sample_count == 0U || !out_crc32)
	{
		return 0;

	}

	reference = group->logic_crc_values[0];
	for (idx = 1U; idx < group->sample_count; ++idx)
	{
		if (group->logic_crc_values[idx] != reference)
		{
			*out_crc32 = reference;
			return 0;

		}

	}

	*out_crc32 = reference;
	return 1;

}

static int hebs_parse_cli(int argc, char** argv, hebs_calc_options_t* opts)
{
	int idx;

	if (!opts)
	{
		return 0;

	}

	memset(opts, 0, sizeof(*opts));
	opts->append_output = 1;
	snprintf(opts->input_csv, sizeof(opts->input_csv), "%s", RAW_INPUT_PATH);
	snprintf(opts->output_csv, sizeof(opts->output_csv), "%s", DERIVED_OUTPUT_PATH);

	for (idx = 1; idx < argc; ++idx)
	{
		const char* arg = argv[idx];

		if (strcmp(arg, "--input") == 0)
		{
			if (idx + 1 >= argc || !hebs_copy_text_fit(opts->input_csv, sizeof(opts->input_csv), argv[idx + 1]))
			{
				return 0;

			}

			++idx;
			continue;

		}

		if (strcmp(arg, "--output") == 0)
		{
			if (idx + 1 >= argc || !hebs_copy_text_fit(opts->output_csv, sizeof(opts->output_csv), argv[idx + 1]))
			{
				return 0;

			}

			++idx;
			continue;

		}

		if (strcmp(arg, "--replace") == 0)
		{
			opts->append_output = 0;
			continue;

		}

		if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
		{
			return 0;

		}

		return 0;

	}

	return 1;

}

static void hebs_print_usage(const char* exe_name)
{
	printf("Usage:\n");
	printf("  %s [--input raw.csv] [--output metrics_derived.csv] [--replace]\n", exe_name ? exe_name : "hebs_metrics");
	printf("Examples:\n");
	printf("  hebs_metrics --input benchmarks/results/raw_runner_output.csv\n");
	printf("  hebs_metrics --input benchmarks/results/raw_runner_output.csv --output benchmarks/results/metrics_derived.csv\n");
	printf("  hebs_metrics --input benchmarks/results/raw_runner_output.csv --output benchmarks/results/metrics_derived.csv --replace\n");

}

static int hebs_load_raw_groups(const char* input_csv, hebs_metric_group_t** out_groups, uint32_t* out_group_count)
{
	FILE* file;
	char line[8192];
	char* fields[HEBS_RAW_COL_COUNT + 2];
	hebs_metric_group_t* groups;
	uint32_t group_count;
	uint32_t group_capacity;

	if (!input_csv || !out_groups || !out_group_count)
	{
		return 0;

	}

	file = fopen(input_csv, "r");
	if (!file)
	{
		return 0;

	}

	groups = NULL;
	group_count = 0U;
	group_capacity = 0U;

	while (fgets(line, (int)sizeof(line), file))
	{
		int field_count;
		uint32_t idx;
		int found_index;
		hebs_metric_group_t* group;

		if (line[0] == '\n' || line[0] == '\r')
		{
			continue;

		}

		field_count = hebs_split_csv_simple(line, fields, HEBS_RAW_COL_COUNT + 2);
		if (field_count != HEBS_RAW_COL_COUNT)
		{
			continue;

		}

		for (idx = 0U; idx < HEBS_RAW_COL_COUNT; ++idx)
		{
			fields[idx] = hebs_trim_in_place(fields[idx]);

		}

		if (strcmp(fields[HEBS_RAW_RUN_ID], "run_id") == 0)
		{
			if (!hebs_is_compat_metrics_header(fields[HEBS_RAW_TEST_PROBES_ENABLED]))
			{
				hebs_free_groups(groups, group_count);
				fclose(file);
				return 0;

			}
			continue;

		}

		found_index = -1;
		for (idx = 0U; idx < group_count; ++idx)
		{
			if (hebs_groups_match(&groups[idx], fields[HEBS_RAW_RUN_ID], fields[HEBS_RAW_SUITE], fields[HEBS_RAW_BENCH], fields[HEBS_RAW_MODE]))
			{
				found_index = (int)idx;
				break;

			}

		}

		if (found_index < 0)
		{
			if (!hebs_create_group(&groups, &group_count, &group_capacity, (const char**)fields, &group))
			{
				hebs_free_groups(groups, group_count);
				fclose(file);
				return 0;

			}

		}
		else
		{
			group = &groups[found_index];

		}

		if (!hebs_append_group_sample(group, (const char**)fields))
		{
			hebs_free_groups(groups, group_count);
			fclose(file);
			return 0;

		}

	}

	fclose(file);

	if (group_count > 1U)
	{
		qsort(groups, group_count, sizeof(groups[0]), hebs_group_cmp);

	}

	*out_groups = groups;
	*out_group_count = group_count;
	return 1;

}

static void hebs_apply_history(const char* output_csv, hebs_metric_group_t* groups, uint32_t group_count)
{
	FILE* file;
	char line[8192];
	char* fields[HEBS_DERIVED_COL_COUNT + 4];

	if (!output_csv || !groups || group_count == 0U)
	{
		return;

	}

	file = fopen(output_csv, "r");
	if (!file)
	{
		return;

	}

	while (fgets(line, (int)sizeof(line), file))
	{
		int count;
		uint32_t idx;
		char* suite;
		char* bench;
		char* mode;
		double geps_p50;
		double icf_p50;

		if (line[0] == '\n' || line[0] == '\r')
		{
			continue;

		}

		count = hebs_split_csv_simple(line, fields, HEBS_DERIVED_COL_COUNT + 4);
		if (count != HEBS_DERIVED_COL_COUNT)
		{
			continue;

		}

		for (idx = 0U; idx < HEBS_DERIVED_COL_COUNT; ++idx)
		{
			fields[idx] = hebs_trim_in_place(fields[idx]);

		}

		if (strcmp(fields[HEBS_DERIVED_RUN_ID], "run_id") == 0)
		{
			continue;

		}

		suite = fields[HEBS_DERIVED_SUITE];
		bench = fields[HEBS_DERIVED_BENCH];
		mode = fields[HEBS_DERIVED_MODE];
		geps_p50 = strtod(fields[HEBS_DERIVED_GEPS_P50], NULL);
		icf_p50 = strtod(fields[HEBS_DERIVED_ICF_P50], NULL);

		for (idx = 0U; idx < group_count; ++idx)
		{
			hebs_metric_group_t* group = &groups[idx];
			if (strcmp(group->suite_name, suite) == 0 &&
				strcmp(group->bench_name, bench) == 0 &&
				strcmp(group->mode_name, mode) == 0)
			{
				if (!group->history_found)
				{
					group->base_geps_p50 = geps_p50;
					group->base_icf_p50 = icf_p50;
					group->history_found = 1;

				}

				group->prev_geps_p50 = geps_p50;
				group->prev_icf_p50 = icf_p50;

			}

		}

	}

	fclose(file);

}

static int hebs_write_derived_header(FILE* output_file)
{
	return (fprintf(
		output_file,
		"run_id,revision,date,time,git_commit,suite,bench,mode,iterations,cycles,gate_count,pi_count,signal_count,propagation_depth,fanout_max,total_fanout_edges,"
		"runtime_min_sec,runtime_p50_sec,runtime_p90_sec,runtime_p95_sec,runtime_p99_sec,runtime_max_sec,runtime_mean_sec,runtime_stddev_sec,"
		"geps_min,geps_p50,geps_max,geps_mean,icf_min,icf_p50,icf_max,icf_mean,"
		"base_geps_p50,prev_geps_p50,delta_geps_prev_pct,base_icf_p50,prev_icf_p50,delta_icf_prev_pct,fingerprint_stable,logic_crc32,compat_metrics_enabled\n") > 0);

}

static int hebs_write_derived_rows(FILE* output_file, const hebs_metric_group_t* groups, uint32_t group_count)
{
	uint32_t idx;

	if (!output_file || !groups)
	{
		return 0;

	}

	for (idx = 0U; idx < group_count; ++idx)
	{
		const hebs_metric_group_t* group = &groups[idx];
		double runtime_min;
		double runtime_p50;
		double runtime_p90;
		double runtime_p95;
		double runtime_p99;
		double runtime_max;
		double runtime_mean;
		double runtime_stddev;
		double geps_min;
		double geps_p50;
		double geps_max;
		double geps_mean;
		double icf_min;
		double icf_p50;
		double icf_max;
		double icf_mean;
		double delta_geps_prev_pct;
		double delta_icf_prev_pct;
		uint32_t crc32;
		int fingerprint_stable;

		if (group->sample_count == 0U)
		{
			continue;

		}

		runtime_min = hebs_calculate_min(group->runtimes_sec, group->sample_count);
		runtime_p50 = hebs_calculate_percentile(group->runtimes_sec, group->sample_count, 50.0);
		runtime_p90 = hebs_calculate_percentile(group->runtimes_sec, group->sample_count, 90.0);
		runtime_p95 = hebs_calculate_percentile(group->runtimes_sec, group->sample_count, 95.0);
		runtime_p99 = hebs_calculate_percentile(group->runtimes_sec, group->sample_count, 99.0);
		runtime_max = hebs_calculate_max(group->runtimes_sec, group->sample_count);
		runtime_mean = hebs_calculate_mean(group->runtimes_sec, group->sample_count);
		runtime_stddev = hebs_calculate_stddev(group->runtimes_sec, group->sample_count, runtime_mean);

		geps_min = hebs_calculate_min(group->geps_values, group->sample_count);
		geps_p50 = hebs_calculate_percentile(group->geps_values, group->sample_count, 50.0);
		geps_max = hebs_calculate_max(group->geps_values, group->sample_count);
		geps_mean = hebs_calculate_mean(group->geps_values, group->sample_count);

		icf_min = hebs_calculate_min(group->icf_values, group->sample_count);
		icf_p50 = hebs_calculate_percentile(group->icf_values, group->sample_count, 50.0);
		icf_max = hebs_calculate_max(group->icf_values, group->sample_count);
		icf_mean = hebs_calculate_mean(group->icf_values, group->sample_count);

		delta_geps_prev_pct = 0.0;
		if (group->prev_geps_p50 > 0.0)
		{
			delta_geps_prev_pct = ((geps_p50 - group->prev_geps_p50) / group->prev_geps_p50) * 100.0;

		}

		delta_icf_prev_pct = 0.0;
		if (group->prev_icf_p50 > 0.0)
		{
			delta_icf_prev_pct = ((icf_p50 - group->prev_icf_p50) / group->prev_icf_p50) * 100.0;

		}

		fingerprint_stable = hebs_is_fingerprint_stable(group, &crc32);

		if (fprintf(
			output_file,
			"%s,%s,%s,%s,%s,%s,%s,%s,%u,%u,%u,%u,%u,%u,%u,%u,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f,%.6f,%.6f,%.2f,%u,0x%08X,%u\n",
			group->run_id,
			group->revision,
			group->date_text,
			group->time_text,
			group->git_commit,
			group->suite_name,
			group->bench_name,
			group->mode_name,
			group->sample_count,
			group->cycles,
			group->gate_count,
			group->pi_count,
			group->signal_count,
			group->propagation_depth,
			group->fanout_max,
			group->total_fanout_edges,
			runtime_min,
			runtime_p50,
			runtime_p90,
			runtime_p95,
			runtime_p99,
			runtime_max,
			runtime_mean,
			runtime_stddev,
			geps_min,
			geps_p50,
			geps_max,
			geps_mean,
			icf_min,
			icf_p50,
			icf_max,
			icf_mean,
			group->base_geps_p50,
			group->prev_geps_p50,
			delta_geps_prev_pct,
			group->base_icf_p50,
			group->prev_icf_p50,
			delta_icf_prev_pct,
			(unsigned int)fingerprint_stable,
			(unsigned int)crc32,
			(unsigned int)group->test_probes_enabled) <= 0)
		{
			return 0;

		}

	}

	return 1;

}

int main(int argc, char** argv)
{
	hebs_calc_options_t options;
	hebs_metric_group_t* groups;
	uint32_t group_count;
	FILE* output_file;
	int output_was_empty;

	if (!hebs_parse_cli(argc, argv, &options))
	{
		hebs_print_usage((argc > 0) ? argv[0] : "hebs_metrics");
		return 1;

	}

	groups = NULL;
	group_count = 0U;
	if (!hebs_load_raw_groups(options.input_csv, &groups, &group_count))
	{
		printf("Failed to load raw CSV: %s\n", options.input_csv);
		return 1;

	}

	if (group_count == 0U)
	{
		printf("No raw rows found in input: %s\n", options.input_csv);
		hebs_free_groups(groups, group_count);
		return 1;

	}

	if (options.append_output)
	{
		hebs_apply_history(options.output_csv, groups, group_count);

	}

	output_was_empty = hebs_file_is_empty(options.output_csv);
	output_file = fopen(options.output_csv, options.append_output ? "a" : "w");
	if (!output_file)
	{
		printf("Failed to open output CSV: %s\n", options.output_csv);
		hebs_free_groups(groups, group_count);
		return 1;

	}

	if (!options.append_output || output_was_empty)
	{
		if (!hebs_write_derived_header(output_file))
		{
			fclose(output_file);
			hebs_free_groups(groups, group_count);
			return 1;

		}

	}

	if (!hebs_write_derived_rows(output_file, groups, group_count))
	{
		fclose(output_file);
		hebs_free_groups(groups, group_count);
		return 1;

	}

	fclose(output_file);
	hebs_free_groups(groups, group_count);

	printf("Derived metrics rows written: %u\n", group_count);
	printf("Input raw CSV: %s\n", options.input_csv);
	printf("Output metrics CSV: %s\n", options.output_csv);
	return 0;

}
