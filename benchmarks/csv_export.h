#ifndef HEBS_CSV_EXPORT_H
#define HEBS_CSV_EXPORT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "report_types.h"

static char* hebs_csv_trim(char* text)
{
	char* end;

	if (!text)
	{
		return text;

	}

	while (*text && isspace((unsigned char)*text))
	{
		++text;

	}

	if (*text == '\0')
	{
		return text;

	}

	end = text + strlen(text) - 1;
	while (end > text && isspace((unsigned char)*end))
	{
		*end = '\0';
		--end;

	}

	return text;

}

static int hebs_parse_metric_line(
	const char* line,
	char* out_suite,
	size_t out_suite_size,
	char* out_bench,
	size_t out_bench_size,
	double* out_icf,
	double* out_geps_p50)
{
	char* copy;
	char* token;
	char* context;
	int column;
	char* suite;
	char* bench;
	double icf;
	double geps_p50;

	if (!line || !out_suite || !out_bench || !out_icf || !out_geps_p50)
	{
		return 0;

	}

	copy = (char*)malloc(strlen(line) + 1U);
	if (!copy)
	{
		return 0;

	}

	strcpy(copy, line);
	column = 0;
	suite = NULL;
	bench = NULL;
	icf = 0.0;
	geps_p50 = 0.0;

	token = strtok_s(copy, ",", &context);
	while (token)
	{
		char* field = hebs_csv_trim(token);
		if (column == 0)
		{
			suite = field;

		}
		else if (column == 1)
		{
			bench = field;

		}
		else if (column == 5)
		{
			icf = strtod(field, NULL);

		}
		else if (column == 7)
		{
			geps_p50 = strtod(field, NULL);
			break;

		}

		++column;
		token = strtok_s(NULL, ",", &context);

	}

	if (!suite || !bench)
	{
		free(copy);
		return 0;

	}

	snprintf(out_suite, out_suite_size, "%s", suite);
	snprintf(out_bench, out_bench_size, "%s", bench);
	*out_icf = icf;
	*out_geps_p50 = geps_p50;
	free(copy);
	return 1;

}

static int lookup_history_for_bench_suite(
	const char* csv_path,
	const char* suite_name,
	const char* benchmark,
	double* out_base_geps,
	double* out_prev_geps,
	double* out_base_icf,
	double* out_prev_icf)
{
	FILE* file;
	char line[4096];
	double base_geps;
	double prev_geps;
	double base_icf;
	double prev_icf;
	int found;

	if (!csv_path || !suite_name || !benchmark || !out_base_geps || !out_prev_geps || !out_base_icf || !out_prev_icf)
	{
		return 0;

	}

	file = fopen(csv_path, "r");
	if (!file)
	{
		return 0;

	}

	base_geps = 0.0;
	prev_geps = 0.0;
	base_icf = 0.0;
	prev_icf = 0.0;
	found = 0;

	while (fgets(line, (int)sizeof(line), file))
	{
		char suite[64];
		char bench[128];
		double icf;
		double geps;

		if (strchr(line, '|') != NULL || strstr(line, "Suite") != NULL || line[0] == '\n' || line[0] == '\r')
		{
			continue;

		}

		if (!hebs_parse_metric_line(line, suite, sizeof(suite), bench, sizeof(bench), &icf, &geps))
		{
			continue;

		}

		if (strcmp(suite, suite_name) == 0 && strcmp(bench, benchmark) == 0)
		{
			if (!found)
			{
				base_geps = geps;
				base_icf = icf;
				found = 1;

			}

			prev_geps = geps;
			prev_icf = icf;

		}

	}

	fclose(file);

	if (!found)
	{
		return 0;

	}

	*out_base_geps = base_geps;
	*out_prev_geps = prev_geps;
	*out_base_icf = base_icf;
	*out_prev_icf = prev_icf;
	return 1;

}

static int lookup_revision_mean_geps(
	const char* csv_path,
	const char* revision_token,
	double* out_mean_geps)
{
	FILE* file;
	char line[4096];
	char token_prefix[256];
	double running_sum;
	uint32_t running_count;
	int in_target_block;

	if (!csv_path || !revision_token || !out_mean_geps)
	{
		return 0;

	}

	file = fopen(csv_path, "r");
	if (!file)
	{
		return 0;

	}

	snprintf(token_prefix, sizeof(token_prefix), "%s |", revision_token);
	running_sum = 0.0;
	running_count = 0U;
	in_target_block = 0;

	while (fgets(line, (int)sizeof(line), file))
	{
		char block_copy[4096];
		char* trimmed;
		if (strchr(line, '|') != NULL && strstr(line, ",") == NULL)
		{
			if (in_target_block && running_count > 0U)
			{
				*out_mean_geps = running_sum / (double)running_count;
				fclose(file);
				return 1;

			}

			snprintf(block_copy, sizeof(block_copy), "%s", line);
			trimmed = hebs_csv_trim(block_copy);
			in_target_block = (strncmp(trimmed, token_prefix, strlen(token_prefix)) == 0);
			running_sum = 0.0;
			running_count = 0U;
			continue;

		}

		if (!in_target_block || strstr(line, "Suite") != NULL || line[0] == '\n' || line[0] == '\r')
		{
			continue;

		}

		{
			char suite[64];
			char bench[128];
			double icf;
			double geps;
			if (hebs_parse_metric_line(line, suite, sizeof(suite), bench, sizeof(bench), &icf, &geps))
			{
				running_sum += geps;
				++running_count;

			}

		}

	}

	if (in_target_block && running_count > 0U)
	{
		*out_mean_geps = running_sum / (double)running_count;
		fclose(file);
		return 1;

	}

	fclose(file);
	return 0;

}

static int append_metrics_history_csv(
	const hebs_metric_row_t* results,
	int count,
	const char* revision_name,
	const char* timestamp,
	const char* date_text,
	const char* git_commit_hash,
	const char* output_path)
{
	FILE* file;
	int i;

	if (!results || count <= 0 || !revision_name || !timestamp || !date_text || !git_commit_hash || !output_path)
	{
		return 0;

	}

	file = fopen(output_path, "a");
	if (!file)
	{
		return 0;

	}

	fprintf(file, "%s | %s | %s | %s\n", revision_name, timestamp, date_text, git_commit_hash);
	fprintf(file, "Suite, Benchmark, Gate_Count, PI_Count, Cycles, ICF, GEPS_Min, GEPS_p50, GEPS_Max, Latency, Propagation_Depth, Fanout_Max, Fingerprint_Stable, Logic_CRC32, Plan_Fingerprint, Base_GEPS, Prev_GEPS, Cur_GEPS, Delta_GEPS_Pct, Base_ICF, Prev_ICF, Cur_ICF, Delta_ICF_Pct, Regression_Gate, Probe_Profile, compat_metrics_enabled\n");

	for (i = 0; i < count; ++i)
	{
		const hebs_metric_row_t* row = &results[i];
		fprintf(
			file,
			"%s, %s, %u, %u, %u, %.6f, %.2f, %.2f, %.2f, %.9f, %u, %u, %u, 0x%08X, 0x%llX, %.2f, %.2f, %.2f, %.2f, %.6f, %.6f, %.6f, %.2f, %s, %s, %u\n",
			row->suite_name,
			row->benchmark,
			row->gate_count,
			row->pi_count,
			row->cycles,
			row->icf,
			row->geps_min,
			row->geps_p50,
			row->geps_max,
			row->runtime_p50,
			row->propagation_depth,
			row->fanout_max,
			row->fingerprint_stable,
			(unsigned int)row->logic_fingerprint,
			(unsigned long long)row->plan_fingerprint,
			row->base_geps_p50,
			row->prev_geps_p50,
			row->geps_p50,
			row->geps_delta_prev_pct,
			row->base_icf,
			row->prev_icf,
			row->icf,
			row->icf_delta_prev_pct,
			row->geps_regression_fail ? "FAIL" : "PASS",
			row->probe_profile,
			row->test_probes_enabled);

	}

	fprintf(file, "\n");
	fclose(file);
	return 1;

}

#endif
