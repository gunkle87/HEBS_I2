#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "hebs_engine.h"
#include "timing_helper.h"

#ifndef DEFAULT_ITERATIONS
#define DEFAULT_ITERATIONS 10U
#endif
#ifndef DEFAULT_SIM_CYCLES
#define DEFAULT_SIM_CYCLES 1000U
#endif
#ifndef DEFAULT_WARMUP_SECONDS
#define DEFAULT_WARMUP_SECONDS 5.0
#endif
#ifndef DEFAULT_BENCH_ROOT
#define DEFAULT_BENCH_ROOT "benchmarks/benches"
#endif
#ifndef RAW_CSV_PATH
#define RAW_CSV_PATH "benchmarks/results/raw_runner_output.csv"
#endif
#ifndef TRACE_CSV_PATH
#define TRACE_CSV_PATH "benchmarks/results/trace_cycle.csv"
#endif
#ifndef REVISION_NAME
#define REVISION_NAME "Raw_Runner_v01"
#endif

typedef enum hebs_scope_e
{
	HEBS_SCOPE_ALL = 0,
	HEBS_SCOPE_SUITE = 1,
	HEBS_SCOPE_BENCH = 2

} hebs_scope_t;

typedef enum hebs_record_mode_e
{
	HEBS_RECORD_MODE_AGGREGATE = 0,
	HEBS_RECORD_MODE_TRACE = 1,
	HEBS_RECORD_MODE_NONE = 2

} hebs_record_mode_t;

typedef enum hebs_record_overwrite_e
{
	HEBS_RECORD_OVERWRITE_APPEND = 0,
	HEBS_RECORD_OVERWRITE_OVERWRITE = 1

} hebs_record_overwrite_t;

typedef struct hebs_cli_options_s
{
	hebs_scope_t scope;
	char suite_filter[64];
	char bench_filter[128];
	char bench_root[260];
	char output_csv[260];
	char trace_csv[260];
	uint32_t iterations;
	uint32_t cycles;
	double warmup_seconds;
	hebs_record_mode_t record_mode;
	hebs_record_overwrite_t record_overwrite_rule;
	uint8_t record_meta_head;
	uint32_t record_interval;

} hebs_cli_options_t;

typedef struct hebs_bench_target_s
{
	char suite_name[64];
	char bench_file[128];
	char bench_path[260];

} hebs_bench_target_t;

static const char* hebs_probe_profile_name(void)
{
#if HEBS_COMPAT_PROBES_ENABLED
	return "compat";
#else
	return "perf";
#endif

}

static const char* hebs_scope_name(hebs_scope_t scope)
{
	if (scope == HEBS_SCOPE_ALL)
	{
		return "all";

	}

	if (scope == HEBS_SCOPE_SUITE)
	{
		return "suite";

	}

	return "bench";

}

static const char* hebs_record_mode_name(hebs_record_mode_t mode)
{
	if (mode == HEBS_RECORD_MODE_AGGREGATE)
	{
		return "aggregate";

	}

	if (mode == HEBS_RECORD_MODE_TRACE)
	{
		return "trace";

	}

	return "none";

}

static int hebs_is_dot_dir(const char* name)
{
	if (!name)
	{
		return 0;

	}

	return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0);

}

static int hebs_has_bench_extension(const char* name)
{
	const char* dot;

	if (!name)
	{
		return 0;

	}

	dot = strrchr(name, '.');
	if (!dot)
	{
		return 0;

	}

	return (_stricmp(dot, ".bench") == 0);

}

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

static void hebs_warmup(double warmup_seconds)
{
	hebs_timer_t timer;

	printf("HEBS_CLEAN: Thermal Warm-up (%.0f s)...\n", warmup_seconds);
	timer_start(&timer);
	do
	{
		timer_stop(&timer);

	} while (timer_elapsed_sec(&timer) < warmup_seconds);

}

static void hebs_print_usage(const char* exe)
{
	printf("Usage:\n");
	printf("  %s [--scope all|suite|bench] [--suite NAME] [--bench FILE] [--iterations N] [--cycles N] [--warmup-seconds X] [--bench-root PATH] [--output PATH]\n", exe ? exe : "hebs_cli");
	printf("     [--record-mode aggregate|trace|none] [--record-overwrite append|overwrite] [--record-meta-head on|off]\n");
	printf("     [--record-interval N] [--trace-output PATH]\n");
	printf("Examples:\n");
	printf("  hebs_cli --scope all\n");
	printf("  hebs_cli --scope suite --suite <suite_name>\n");
	printf("  hebs_cli --scope bench --suite <suite_name> --bench <bench_file.bench>\n");
	printf("  hebs_cli --record-mode trace --record-interval 1000 --trace-output benchmarks/results/trace_cycle.csv\n");
	printf("  hebs_cli --record-mode none\n");

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

static int hebs_parse_scope(const char* text, hebs_scope_t* out_scope)
{
	if (!text || !out_scope)
	{
		return 0;

	}

	if (_stricmp(text, "all") == 0)
	{
		*out_scope = HEBS_SCOPE_ALL;
		return 1;

	}

	if (_stricmp(text, "suite") == 0)
	{
		*out_scope = HEBS_SCOPE_SUITE;
		return 1;

	}

	if (_stricmp(text, "bench") == 0)
	{
		*out_scope = HEBS_SCOPE_BENCH;
		return 1;

	}

	return 0;

}

static int hebs_parse_record_mode(const char* text, hebs_record_mode_t* out_mode)
{
	if (!text || !out_mode)
	{
		return 0;

	}

	if (_stricmp(text, "aggregate") == 0)
	{
		*out_mode = HEBS_RECORD_MODE_AGGREGATE;
		return 1;

	}

	if (_stricmp(text, "trace") == 0)
	{
		*out_mode = HEBS_RECORD_MODE_TRACE;
		return 1;

	}

	if (_stricmp(text, "none") == 0)
	{
		*out_mode = HEBS_RECORD_MODE_NONE;
		return 1;

	}

	return 0;

}

static int hebs_parse_record_overwrite(const char* text, hebs_record_overwrite_t* out_rule)
{
	if (!text || !out_rule)
	{
		return 0;

	}

	if (_stricmp(text, "append") == 0)
	{
		*out_rule = HEBS_RECORD_OVERWRITE_APPEND;
		return 1;

	}

	if (_stricmp(text, "overwrite") == 0)
	{
		*out_rule = HEBS_RECORD_OVERWRITE_OVERWRITE;
		return 1;

	}

	return 0;

}

static int hebs_parse_on_off(const char* text, uint8_t* out_value)
{
	if (!text || !out_value)
	{
		return 0;

	}

	if (_stricmp(text, "on") == 0 || _stricmp(text, "true") == 0 || strcmp(text, "1") == 0)
	{
		*out_value = 1U;
		return 1;

	}

	if (_stricmp(text, "off") == 0 || _stricmp(text, "false") == 0 || strcmp(text, "0") == 0)
	{
		*out_value = 0U;
		return 1;

	}

	return 0;

}

static int hebs_parse_cli(int argc, char** argv, hebs_cli_options_t* opts)
{
	int idx;

	if (!opts)
	{
		return 0;

	}

	memset(opts, 0, sizeof(*opts));
	opts->scope = HEBS_SCOPE_ALL;
	opts->iterations = DEFAULT_ITERATIONS;
	opts->cycles = DEFAULT_SIM_CYCLES;
	opts->warmup_seconds = DEFAULT_WARMUP_SECONDS;
	opts->record_mode = HEBS_RECORD_MODE_AGGREGATE;
	opts->record_overwrite_rule = HEBS_RECORD_OVERWRITE_APPEND;
	opts->record_meta_head = 0U;
	opts->record_interval = 1U;
	snprintf(opts->bench_root, sizeof(opts->bench_root), "%s", DEFAULT_BENCH_ROOT);
	snprintf(opts->output_csv, sizeof(opts->output_csv), "%s", RAW_CSV_PATH);
	snprintf(opts->trace_csv, sizeof(opts->trace_csv), "%s", TRACE_CSV_PATH);

	for (idx = 1; idx < argc; ++idx)
	{
		const char* arg = argv[idx];
		if (strcmp(arg, "--scope") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_scope(argv[idx + 1], &opts->scope))
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--suite") == 0)
		{
			if (idx + 1 >= argc)
			{
				return 0;

			}
			snprintf(opts->suite_filter, sizeof(opts->suite_filter), "%s", argv[idx + 1]);
			++idx;
			continue;

		}

		if (strcmp(arg, "--bench") == 0)
		{
			if (idx + 1 >= argc)
			{
				return 0;

			}
			snprintf(opts->bench_filter, sizeof(opts->bench_filter), "%s", argv[idx + 1]);
			++idx;
			continue;

		}

		if (strcmp(arg, "--iterations") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_u32(argv[idx + 1], &opts->iterations) || opts->iterations == 0U)
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--cycles") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_u32(argv[idx + 1], &opts->cycles) || opts->cycles == 0U)
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--warmup-seconds") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_double(argv[idx + 1], &opts->warmup_seconds) || opts->warmup_seconds < 0.0)
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--bench-root") == 0)
		{
			if (idx + 1 >= argc)
			{
				return 0;

			}
			snprintf(opts->bench_root, sizeof(opts->bench_root), "%s", argv[idx + 1]);
			++idx;
			continue;

		}

		if (strcmp(arg, "--output") == 0)
		{
			if (idx + 1 >= argc)
			{
				return 0;

			}
			snprintf(opts->output_csv, sizeof(opts->output_csv), "%s", argv[idx + 1]);
			++idx;
			continue;

		}

		if (strcmp(arg, "--trace-output") == 0)
		{
			if (idx + 1 >= argc)
			{
				return 0;

			}
			snprintf(opts->trace_csv, sizeof(opts->trace_csv), "%s", argv[idx + 1]);
			++idx;
			continue;

		}

		if (strcmp(arg, "--record-mode") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_record_mode(argv[idx + 1], &opts->record_mode))
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--record-overwrite") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_record_overwrite(argv[idx + 1], &opts->record_overwrite_rule))
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--record-meta-head") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_on_off(argv[idx + 1], &opts->record_meta_head))
			{
				return 0;

			}
			++idx;
			continue;

		}

		if (strcmp(arg, "--record-interval") == 0)
		{
			if (idx + 1 >= argc || !hebs_parse_u32(argv[idx + 1], &opts->record_interval) || opts->record_interval == 0U)
			{
				return 0;

			}
			++idx;
			continue;

		}

		return 0;

	}

	if (opts->scope == HEBS_SCOPE_SUITE && opts->suite_filter[0] == '\0')
	{
		return 0;

	}

	if (opts->scope == HEBS_SCOPE_BENCH && (opts->suite_filter[0] == '\0' || opts->bench_filter[0] == '\0'))
	{
		return 0;

	}

	if (opts->record_mode == HEBS_RECORD_MODE_TRACE && opts->record_interval == 0U)
	{
		return 0;

	}

	return 1;

}

static int hebs_reserve_targets(hebs_bench_target_t** targets, uint32_t needed, uint32_t* capacity)
{
	hebs_bench_target_t* resized;
	uint32_t new_capacity;

	if (!targets || !capacity)
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

	resized = (hebs_bench_target_t*)realloc(*targets, (size_t)new_capacity * sizeof(hebs_bench_target_t));
	if (!resized)
	{
		return 0;

	}

	*targets = resized;
	*capacity = new_capacity;
	return 1;

}

static int hebs_append_target(
	hebs_bench_target_t** targets,
	uint32_t* count,
	uint32_t* capacity,
	const char* suite_name,
	const char* bench_file,
	const char* bench_path)
{
	hebs_bench_target_t* slot;

	if (!targets || !count || !capacity || !suite_name || !bench_file || !bench_path)
	{
		return 0;

	}

	if (!hebs_reserve_targets(targets, *count + 1U, capacity))
	{
		return 0;

	}

	slot = &(*targets)[*count];
	if (!hebs_copy_text_fit(slot->suite_name, sizeof(slot->suite_name), suite_name) ||
		!hebs_copy_text_fit(slot->bench_file, sizeof(slot->bench_file), bench_file) ||
		!hebs_copy_text_fit(slot->bench_path, sizeof(slot->bench_path), bench_path))
	{
		return 0;

	}

	++(*count);
	return 1;

}

static int hebs_compare_targets(const void* lhs, const void* rhs)
{
	const hebs_bench_target_t* a = (const hebs_bench_target_t*)lhs;
	const hebs_bench_target_t* b = (const hebs_bench_target_t*)rhs;
	int suite_cmp = strcmp(a->suite_name, b->suite_name);
	if (suite_cmp != 0)
	{
		return suite_cmp;

	}

	return strcmp(a->bench_file, b->bench_file);

}

static int hebs_discover_suite_targets(
	const char* bench_root,
	const char* suite_name,
	const char* bench_filter,
	hebs_bench_target_t** out_targets,
	uint32_t* out_count,
	uint32_t* out_capacity)
{
	char pattern[512];
	WIN32_FIND_DATAA find_data;
	HANDLE find_handle;

	if (!bench_root || !suite_name || !out_targets || !out_count || !out_capacity)
	{
		return 0;

	}

	snprintf(pattern, sizeof(pattern), "%s/%s/%s", bench_root, suite_name, bench_filter ? bench_filter : "*.bench");
	find_handle = FindFirstFileA(pattern, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
	{
		return 1;

	}

	do
	{
		char bench_path[512];
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;

		}

		if (!hebs_has_bench_extension(find_data.cFileName))
		{
			continue;

		}

		snprintf(bench_path, sizeof(bench_path), "%s/%s/%s", bench_root, suite_name, find_data.cFileName);
		if (!hebs_append_target(out_targets, out_count, out_capacity, suite_name, find_data.cFileName, bench_path))
		{
			FindClose(find_handle);
			return 0;

		}

	} while (FindNextFileA(find_handle, &find_data) != 0);

	FindClose(find_handle);
	return 1;

}

static int hebs_discover_all_targets(
	const char* bench_root,
	hebs_bench_target_t** out_targets,
	uint32_t* out_count,
	uint32_t* out_capacity)
{
	char pattern[512];
	WIN32_FIND_DATAA find_data;
	HANDLE find_handle;

	if (!bench_root || !out_targets || !out_count || !out_capacity)
	{
		return 0;

	}

	snprintf(pattern, sizeof(pattern), "%s/*", bench_root);
	find_handle = FindFirstFileA(pattern, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
	{
		return 1;

	}

	do
	{
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0U)
		{
			continue;

		}

		if (hebs_is_dot_dir(find_data.cFileName))
		{
			continue;

		}

		if (!hebs_discover_suite_targets(bench_root, find_data.cFileName, NULL, out_targets, out_count, out_capacity))
		{
			FindClose(find_handle);
			return 0;

		}

	} while (FindNextFileA(find_handle, &find_data) != 0);

	FindClose(find_handle);
	return 1;

}

static int hebs_discover_targets(const hebs_cli_options_t* opts, hebs_bench_target_t** out_targets, uint32_t* out_count)
{
	hebs_bench_target_t* targets;
	uint32_t count;
	uint32_t capacity;

	if (!opts || !out_targets || !out_count)
	{
		return 0;

	}

	targets = NULL;
	count = 0U;
	capacity = 0U;

	if (opts->scope == HEBS_SCOPE_ALL)
	{
		if (!hebs_discover_all_targets(opts->bench_root, &targets, &count, &capacity))
		{
			free(targets);
			return 0;

		}

	}
	else if (opts->scope == HEBS_SCOPE_SUITE)
	{
		if (!hebs_discover_suite_targets(opts->bench_root, opts->suite_filter, NULL, &targets, &count, &capacity))
		{
			free(targets);
			return 0;

		}

	}
	else
	{
		if (!hebs_discover_suite_targets(opts->bench_root, opts->suite_filter, opts->bench_filter, &targets, &count, &capacity))
		{
			free(targets);
			return 0;

		}

	}

	if (count > 1U)
	{
		qsort(targets, count, sizeof(targets[0]), hebs_compare_targets);

	}

	*out_targets = targets;
	*out_count = count;
	return 1;

}

static int hebs_csv_file_is_empty(const char* output_path)
{
	FILE* file;
	long size;

	if (!output_path)
	{
		return 1;

	}

	file = fopen(output_path, "rb");
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

static int hebs_open_record_file(const char* path, hebs_record_overwrite_t overwrite_rule, FILE** out_file, int* out_file_was_empty)
{
	if (!path || !out_file || !out_file_was_empty)
	{
		return 0;

	}

	if (overwrite_rule == HEBS_RECORD_OVERWRITE_OVERWRITE)
	{
		*out_file = fopen(path, "w");
		*out_file_was_empty = 1;
		return (*out_file != NULL);

	}

	*out_file_was_empty = hebs_csv_file_is_empty(path);
	*out_file = fopen(path, "a");
	return (*out_file != NULL);

}

static int hebs_write_record_meta_head(
	FILE* out_file,
	const char* artifact_name,
	const char* run_id,
	const char* revision,
	const char* date_text,
	const char* time_text,
	const char* git_hash,
	const hebs_cli_options_t* opts)
{
	if (!out_file || !artifact_name || !run_id || !revision || !date_text || !time_text || !git_hash || !opts)
	{
		return 0;

	}

	fprintf(
		out_file,
		"# meta artifact=%s run_id=%s revision=%s date=%s time=%s git_commit=%s probe_profile=%s scope=%s iterations=%u cycles=%u record_mode=%s record_interval=%u warmup_seconds=%.3f\n",
		artifact_name,
		run_id,
		revision,
		date_text,
		time_text,
		git_hash,
		hebs_probe_profile_name(),
		hebs_scope_name(opts->scope),
		opts->iterations,
		opts->cycles,
		hebs_record_mode_name(opts->record_mode),
		opts->record_interval,
		opts->warmup_seconds);

	return (ferror(out_file) == 0);

}

static int hebs_write_raw_header(FILE* csv_file)
{
	if (!csv_file)
	{
		return 0;

	}

	fprintf(
		csv_file,
		"run_id,revision,date,time,git_commit,suite,bench,mode,iteration,iterations_total,cycles,runtime_sec,plan_fingerprint,logic_crc32,probe_input_apply,probe_input_toggle,probe_chunk_exec,probe_gate_eval,probe_state_change_commit,probe_dff_exec,pi_count,gate_count,signal_count,propagation_depth,fanout_max,total_fanout_edges,compat_metrics_enabled\n");
	return (ferror(csv_file) == 0);

}

static int hebs_write_trace_header(FILE* csv_file)
{
	if (!csv_file)
	{
		return 0;

	}

	fprintf(
		csv_file,
		"run_id,revision,date,time,git_commit,suite,bench,mode,iteration,iterations_total,cycle,cycles_total,runtime_sec,plan_fingerprint,logic_crc32,probe_input_apply,probe_input_toggle,probe_chunk_exec,probe_gate_eval,probe_state_change_commit,probe_dff_exec,compat_metrics_enabled,pi_count,gate_count,signal_count\n");
	return (ferror(csv_file) == 0);

}

static int hebs_write_raw_row(
	FILE* csv_file,
	const char* run_id,
	const char* revision,
	const char* date_text,
	const char* time_text,
	const char* git_hash,
	const char* suite,
	const char* bench,
	const char* mode,
	uint32_t iteration_index,
	uint32_t iterations_total,
	uint32_t cycles,
	double runtime_sec,
	uint64_t plan_fingerprint,
	uint32_t logic_crc32,
	const hebs_probes* probes,
	const hebs_plan_metadata* plan_metadata)
{
	if (!csv_file || !run_id || !revision || !date_text || !time_text || !git_hash || !suite || !bench || !mode || !probes || !plan_metadata)
	{
		return 0;

	}

	fprintf(
		csv_file,
		"%s,%s,%s,%s,%s,%s,%s,%s,%u,%u,%u,%.9f,0x%llX,0x%08X,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%u,%u\n",
		run_id,
		revision,
		date_text,
		time_text,
		git_hash,
		suite,
		bench,
		mode,
		iteration_index,
		iterations_total,
		cycles,
		runtime_sec,
		(unsigned long long)plan_fingerprint,
		(unsigned int)logic_crc32,
		(unsigned long long)probes->input_apply,
		(unsigned long long)probes->input_toggle,
		(unsigned long long)probes->chunk_exec,
		(unsigned long long)probes->gate_eval,
		(unsigned long long)probes->state_change_commit,
		(unsigned long long)probes->dff_exec,
		plan_metadata->num_primary_inputs,
		plan_metadata->gate_count,
		plan_metadata->signal_count,
		plan_metadata->propagation_depth,
		plan_metadata->fanout_max,
		plan_metadata->total_fanout_edges,
		(unsigned int)HEBS_COMPAT_PROBES_ENABLED);

	return (ferror(csv_file) == 0);

}

static int hebs_write_trace_row(
	FILE* csv_file,
	const char* run_id,
	const char* revision,
	const char* date_text,
	const char* time_text,
	const char* git_hash,
	const char* suite,
	const char* bench,
	const char* mode,
	uint32_t iteration_index,
	uint32_t iterations_total,
	uint32_t cycle_index,
	uint32_t cycles_total,
	double runtime_sec,
	uint64_t plan_fingerprint,
	uint32_t logic_crc32,
	const hebs_probes* probes,
	const hebs_plan_metadata* plan_metadata)
{
	if (!csv_file || !run_id || !revision || !date_text || !time_text || !git_hash || !suite || !bench || !mode || !probes || !plan_metadata)
	{
		return 0;

	}

	fprintf(
		csv_file,
		"%s,%s,%s,%s,%s,%s,%s,%s,%u,%u,%u,%u,%.9f,0x%llX,0x%08X,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u\n",
		run_id,
		revision,
		date_text,
		time_text,
		git_hash,
		suite,
		bench,
		mode,
		iteration_index,
		iterations_total,
		cycle_index,
		cycles_total,
		runtime_sec,
		(unsigned long long)plan_fingerprint,
		(unsigned int)logic_crc32,
		(unsigned long long)probes->input_apply,
		(unsigned long long)probes->input_toggle,
		(unsigned long long)probes->chunk_exec,
		(unsigned long long)probes->gate_eval,
		(unsigned long long)probes->state_change_commit,
		(unsigned long long)probes->dff_exec,
		(unsigned int)HEBS_COMPAT_PROBES_ENABLED,
		plan_metadata->num_primary_inputs,
		plan_metadata->gate_count,
		plan_metadata->signal_count);

	return (ferror(csv_file) == 0);

}

static int hebs_run_single_bench(
	const hebs_cli_options_t* opts,
	const hebs_bench_target_t* target,
	const char* run_id,
	const char* revision,
	const char* date_text,
	const char* time_text,
	const char* git_hash,
	FILE* aggregate_file,
	FILE* trace_file)
{
	uint32_t iter;

	if (!opts || !target || !run_id || !revision || !date_text || !time_text || !git_hash)
	{
		return 0;

	}

	if (opts->record_mode == HEBS_RECORD_MODE_AGGREGATE && !aggregate_file)
	{
		return 0;

	}

	if (opts->record_mode == HEBS_RECORD_MODE_TRACE && !trace_file)
	{
		return 0;

	}

	printf("\nBENCH: %s/%s\n", target->suite_name, target->bench_file);

	for (iter = 0U; iter < opts->iterations; ++iter)
	{
		hebs_plan* plan = hebs_load_bench(target->bench_path);
		hebs_engine engine = { 0 };
		hebs_plan_metadata plan_metadata;
		hebs_run_status run_status;
		hebs_timer_t timer;
		hebs_probes probes;
		uint64_t plan_hash;
		uint32_t crc32;
		double elapsed;
		uint32_t cycle;

		if (!plan || hebs_init_engine(&engine, plan) != HEBS_OK)
		{
			printf("Iteration %u: init failed\n", iter + 1U);
			hebs_free_plan(plan);
			return 0;

		}

		if (!hebs_get_plan_metadata(plan, &plan_metadata))
		{
			printf("Iteration %u: plan metadata query failed\n", iter + 1U);
			hebs_free_plan(plan);
			return 0;

		}

		plan_hash = hebs_get_plan_hash(plan);

		timer_start(&timer);
		for (cycle = 0U; cycle < opts->cycles; ++cycle)
		{
			uint32_t pi;
			for (pi = 0U; pi < plan_metadata.num_primary_inputs && pi < HEBS_MAX_PRIMARY_INPUTS; ++pi)
			{
				hebs_logic_t value = ((cycle + pi) & 1U) ? HEBS_S1 : HEBS_S0;
				if (hebs_set_primary_input(&engine, plan, pi, value) != HEBS_OK)
				{
					printf("Iteration %u: input set failed\n", iter + 1U);
					hebs_free_plan(plan);
					return 0;

				}

			}

			hebs_tick(&engine, plan);

			if (opts->record_mode == HEBS_RECORD_MODE_TRACE && trace_file)
			{
				const uint32_t cycle_index = cycle + 1U;
				const int record_due = ((cycle_index % opts->record_interval) == 0U) || (cycle_index == opts->cycles);
				if (record_due)
				{
					timer_stop(&timer);
					elapsed = timer_elapsed_sec(&timer);
					probes = hebs_get_probes(&engine);
					crc32 = hebs_get_final_crc32(&engine);
					if (!hebs_write_trace_row(
						trace_file,
						run_id,
						revision,
						date_text,
						time_text,
						git_hash,
						target->suite_name,
						target->bench_file,
						hebs_probe_profile_name(),
						iter + 1U,
						opts->iterations,
						cycle_index,
						opts->cycles,
						elapsed,
						plan_hash,
						crc32,
						&probes,
						&plan_metadata))
					{
						printf("Iteration %u: trace CSV write failed\n", iter + 1U);
						hebs_free_plan(plan);
						return 0;

					}

				}

			}

		}
		timer_stop(&timer);

		elapsed = timer_elapsed_sec(&timer);
		probes = hebs_get_probes(&engine);
		crc32 = hebs_get_final_crc32(&engine);
		if (!hebs_get_run_status(&engine, &run_status) || run_status.last_status != HEBS_OK)
		{
			printf("Iteration %u: run status query failed\n", iter + 1U);
			hebs_free_plan(plan);
			return 0;

		}
		printf("Iteration %u: %.9f sec\n", iter + 1U, elapsed);

		if (opts->record_mode == HEBS_RECORD_MODE_AGGREGATE && aggregate_file &&
			!hebs_write_raw_row(
				aggregate_file,
				run_id,
				revision,
				date_text,
				time_text,
				git_hash,
				target->suite_name,
				target->bench_file,
				hebs_probe_profile_name(),
				iter + 1U,
				opts->iterations,
				opts->cycles,
				elapsed,
				plan_hash,
				crc32,
				&probes,
				&plan_metadata))
		{
			printf("Iteration %u: raw CSV write failed\n", iter + 1U);
			hebs_free_plan(plan);
			return 0;

		}

		hebs_free_plan(plan);

	}

	return 1;

}

int main(int argc, char** argv)
{
	hebs_cli_options_t options;
	hebs_bench_target_t* targets;
	uint32_t target_count;
	char timestamp[32];
	char date_text[32];
	char git_hash[64];
	char run_id[256];
	FILE* aggregate_file;
	FILE* trace_file;
	int processed;
	int failures;
	uint32_t target_idx;
	int aggregate_file_was_empty;
	int trace_file_was_empty;

	if (!hebs_parse_cli(argc, argv, &options))
	{
		hebs_print_usage((argc > 0) ? argv[0] : "hebs_cli");
		return 1;

	}

	targets = NULL;
	target_count = 0U;
	if (!hebs_discover_targets(&options, &targets, &target_count))
	{
		printf("Target discovery failed.\n");
		return 1;

	}

	if (target_count == 0U)
	{
		printf("No benchmark targets discovered.\n");
		free(targets);
		return 1;

	}

	hebs_get_run_clock(timestamp, sizeof(timestamp), date_text, sizeof(date_text));
	hebs_get_git_commit_hash(git_hash, sizeof(git_hash));
	snprintf(run_id, sizeof(run_id), "%s|%s|%s|%s|%s", REVISION_NAME, date_text, timestamp, git_hash, hebs_probe_profile_name());

	hebs_warmup(options.warmup_seconds);

	aggregate_file = NULL;
	trace_file = NULL;
	aggregate_file_was_empty = 1;
	trace_file_was_empty = 1;

	if (options.record_mode == HEBS_RECORD_MODE_AGGREGATE)
	{
		if (!hebs_open_record_file(options.output_csv, options.record_overwrite_rule, &aggregate_file, &aggregate_file_was_empty))
		{
			printf("Failed to open raw CSV output: %s\n", options.output_csv);
			free(targets);
			return 1;

		}

		if (aggregate_file_was_empty && !hebs_write_raw_header(aggregate_file))
		{
			printf("Failed to write raw CSV header: %s\n", options.output_csv);
			fclose(aggregate_file);
			free(targets);
			return 1;

		}

		if (options.record_meta_head &&
			!hebs_write_record_meta_head(aggregate_file, "aggregate_raw", run_id, REVISION_NAME, date_text, timestamp, git_hash, &options))
		{
			printf("Failed to write raw CSV metadata head: %s\n", options.output_csv);
			fclose(aggregate_file);
			free(targets);
			return 1;

		}

	}
	else if (options.record_mode == HEBS_RECORD_MODE_TRACE)
	{
		if (!hebs_open_record_file(options.trace_csv, options.record_overwrite_rule, &trace_file, &trace_file_was_empty))
		{
			printf("Failed to open trace CSV output: %s\n", options.trace_csv);
			free(targets);
			return 1;

		}

		if (trace_file_was_empty && !hebs_write_trace_header(trace_file))
		{
			printf("Failed to write trace CSV header: %s\n", options.trace_csv);
			fclose(trace_file);
			free(targets);
			return 1;

		}

		if (options.record_meta_head &&
			!hebs_write_record_meta_head(trace_file, "trace_cycle", run_id, REVISION_NAME, date_text, timestamp, git_hash, &options))
		{
			printf("Failed to write trace CSV metadata head: %s\n", options.trace_csv);
			fclose(trace_file);
			free(targets);
			return 1;

		}

	}

	processed = 0;
	failures = 0;
	for (target_idx = 0U; target_idx < target_count; ++target_idx)
	{
		const hebs_bench_target_t* target = &targets[target_idx];
		if (!hebs_run_single_bench(&options, target, run_id, REVISION_NAME, date_text, timestamp, git_hash, aggregate_file, trace_file))
		{
			++failures;
			continue;

		}

		++processed;

	}

	if (aggregate_file)
	{
		fclose(aggregate_file);

	}

	if (trace_file)
	{
		fclose(trace_file);

	}

	free(targets);

	printf("\nRUN SUMMARY: processed=%d failed=%d\n", processed, failures);
	printf("Record mode: %s\n", hebs_record_mode_name(options.record_mode));
	if (options.record_mode == HEBS_RECORD_MODE_AGGREGATE)
	{
		printf("Raw CSV: %s\n", options.output_csv);

	}
	else if (options.record_mode == HEBS_RECORD_MODE_TRACE)
	{
		printf("Trace CSV: %s\n", options.trace_csv);

	}
	else
	{
		printf("Artifacts: disabled\n");

	}

	return (failures == 0) ? 0 : 1;

}
