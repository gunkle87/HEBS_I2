#ifndef HEBS_HTML_REPORT_H
#define HEBS_HTML_REPORT_H

#include <stdio.h>
#include <string.h>
#include "report_types.h"

typedef struct hebs_suite_agg_s
{
	char suite_name[64];
	double base_geps_sum;
	double prev_geps_sum;
	double cur_geps_sum;
	int count;

} hebs_suite_agg_t;

static int generate_master_report(
	const hebs_metric_row_t* results,
	int count,
	const char* revision_name,
	const char* timestamp,
	const char* date_text,
	const char* git_commit_hash,
	const char* output_path)
{
	FILE* file;
	hebs_suite_agg_t suites[64];
	double global_base_geps;
	double global_prev_geps;
	double global_cur_geps;
	int suite_count;
	int i;

	if (!results || count <= 0 || !revision_name || !timestamp || !date_text || !git_commit_hash || !output_path)
	{
		return 0;

	}

	memset(suites, 0, sizeof(suites));
	suite_count = 0;
	global_base_geps = 0.0;
	global_prev_geps = 0.0;
	global_cur_geps = 0.0;

	for (i = 0; i < count; ++i)
	{
		int s;
		int found = 0;
		global_base_geps += results[i].base_geps_p50;
		global_prev_geps += results[i].prev_geps_p50;
		global_cur_geps += results[i].geps_p50;

		for (s = 0; s < suite_count; ++s)
		{
			if (strcmp(suites[s].suite_name, results[i].suite_name) == 0)
			{
				suites[s].base_geps_sum += results[i].base_geps_p50;
				suites[s].prev_geps_sum += results[i].prev_geps_p50;
				suites[s].cur_geps_sum += results[i].geps_p50;
				suites[s].count += 1;
				found = 1;
				break;

			}

		}

		if (!found && suite_count < 64)
		{
			size_t len = strnlen(results[i].suite_name, sizeof(suites[suite_count].suite_name) - 1U);
			memcpy(suites[suite_count].suite_name, results[i].suite_name, len);
			suites[suite_count].suite_name[len] = '\0';
			suites[suite_count].base_geps_sum = results[i].base_geps_p50;
			suites[suite_count].prev_geps_sum = results[i].prev_geps_p50;
			suites[suite_count].cur_geps_sum = results[i].geps_p50;
			suites[suite_count].count = 1;
			++suite_count;

		}

	}

	global_base_geps /= (double)count;
	global_prev_geps /= (double)count;
	global_cur_geps /= (double)count;

	file = fopen(output_path, "w");
	if (!file)
	{
		return 0;

	}

	fprintf(file, "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">");
	fprintf(file, "<title>revision_structure</title>");
	fprintf(file, "<style>");
	fprintf(file, "body{font-family:Segoe UI,Arial,sans-serif;background:#0f172a;color:#e2e8f0;padding:20px;}");
	fprintf(file, "h1{color:#93c5fd;margin:0 0 6px 0;} .meta{color:#94a3b8;margin-bottom:12px;}");
	fprintf(file, "table{width:100%%;border-collapse:collapse;margin:12px 0;background:#1e293b;table-layout:auto;}");
	fprintf(file, "th,td{border:1px solid #334155;padding:8px 10px;white-space:nowrap;text-align:left;}");
	fprintf(file, "th{background:#111827;color:#93c5fd;} .footer-title{margin-top:18px;color:#93c5fd;}");
	fprintf(file, ".fail{background:#3f1111;} .pass{background:#0f2d1a;}");
	fprintf(file, "</style></head><body>");

	fprintf(file, "<h1>Revision: %s</h1>", revision_name);
	fprintf(file, "<div class=\"meta\">Run: %s | Date: %s | Commit: %s</div>", timestamp, date_text, git_commit_hash);

	fprintf(file, "<table><thead><tr>");
	fprintf(file, "<th>Bench</th><th>Gates</th><th>Base GEPS</th><th>Prev. GEPS</th><th>Cur. GEPS</th><th>Delta (%%)</th><th>Base ICF</th><th>Prev. ICF</th><th>Cur. ICF</th><th>Delta ICF</th><th>Latency</th>");
	fprintf(file, "</tr></thead><tbody>");

	for (i = 0; i < count; ++i)
	{
		fprintf(
			file,
			"<tr class=\"%s\"><td>%s/%s</td><td>%u</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.6f</td><td>%.6f</td><td>%.6f</td><td>%.2f</td><td>%.9f</td></tr>",
			results[i].geps_regression_fail ? "fail" : "pass",
			results[i].suite_name,
			results[i].benchmark,
			results[i].gate_count,
			results[i].base_geps_p50,
			results[i].prev_geps_p50,
			results[i].geps_p50,
			results[i].geps_delta_prev_pct,
			results[i].base_icf,
			results[i].prev_icf,
			results[i].icf,
			results[i].icf_delta_prev_pct,
			results[i].runtime_p50);

	}

	fprintf(file, "</tbody></table>");
	fprintf(file, "<div class=\"footer-title\">Categorized Global Footer</div>");
	fprintf(file, "<table><thead><tr><th>Owner</th><th>Base GEPS Avg</th><th>Prev. GEPS Avg</th><th>Cur. GEPS Avg</th><th>Delta (Prev vs Cur)</th></tr></thead><tbody>");

	fprintf(
		file,
		"<tr><td><b>Global</b></td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>",
		global_base_geps,
		global_prev_geps,
		global_cur_geps,
		(global_prev_geps > 0.0) ? (((global_cur_geps - global_prev_geps) / global_prev_geps) * 100.0) : 0.0);

	for (i = 0; i < suite_count; ++i)
	{
		double base_avg = suites[i].base_geps_sum / (double)suites[i].count;
		double prev_avg = suites[i].prev_geps_sum / (double)suites[i].count;
		double cur_avg = suites[i].cur_geps_sum / (double)suites[i].count;
		double delta = (prev_avg > 0.0) ? (((cur_avg - prev_avg) / prev_avg) * 100.0) : 0.0;

		fprintf(
			file,
			"<tr><td><b>%s</b></td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td></tr>",
			suites[i].suite_name,
			base_avg,
			prev_avg,
			cur_avg,
			delta);

	}

	fprintf(file, "</tbody></table>");
	fprintf(file, "</body></html>");
	fclose(file);
	return 1;

}

#endif
