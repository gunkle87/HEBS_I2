#ifndef HEBS_PROTOCOL_HELPER_H
#define HEBS_PROTOCOL_HELPER_H

#include <stdint.h>
#include <math.h>

static void hebs_sort_ascending(double* values, int count)
{
	int i;
	int j;

	for (i = 1; i < count; ++i)
	{
		double key = values[i];
		j = i - 1;

		while (j >= 0 && values[j] > key)
		{
			values[j + 1] = values[j];
			--j;

		}

		values[j + 1] = key;

	}

}

static double calculate_p50(const double* values, int count)
{
	double sorted[512];
	double index;
	int lower;
	int upper;
	double fraction;
	int i;

	if (!values || count <= 0 || count > 512)
	{
		return 0.0;

	}

	for (i = 0; i < count; ++i)
	{
		sorted[i] = values[i];

	}

	hebs_sort_ascending(sorted, count);
	index = 0.5 * (double)(count - 1);
	lower = (int)index;
	upper = (lower + 1 < count) ? (lower + 1) : lower;
	fraction = index - (double)lower;
	return sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;

}

static double calculate_percentile(const double* values, int count, double pct)
{
	double sorted[512];
	double index;
	int lower;
	int upper;
	double fraction;
	int i;

	if (!values || count <= 0 || count > 512)
	{
		return 0.0;

	}

	if (pct < 0.0)
	{
		pct = 0.0;

	}
	else if (pct > 100.0)
	{
		pct = 100.0;

	}

	for (i = 0; i < count; ++i)
	{
		sorted[i] = values[i];

	}

	hebs_sort_ascending(sorted, count);
	index = (pct / 100.0) * (double)(count - 1);
	lower = (int)index;
	upper = (lower + 1 < count) ? (lower + 1) : lower;
	fraction = index - (double)lower;
	return sorted[lower] + (sorted[upper] - sorted[lower]) * fraction;

}

static double calculate_min(const double* values, int count)
{
	double min_value;
	int i;

	if (!values || count <= 0)
	{
		return 0.0;

	}

	min_value = values[0];
	for (i = 1; i < count; ++i)
	{
		if (values[i] < min_value)
		{
			min_value = values[i];

		}

	}

	return min_value;

}

static double calculate_max(const double* values, int count)
{
	double max_value;
	int i;

	if (!values || count <= 0)
	{
		return 0.0;

	}

	max_value = values[0];
	for (i = 1; i < count; ++i)
	{
		if (values[i] > max_value)
		{
			max_value = values[i];

		}

	}

	return max_value;

}

static double calculate_icf(uint64_t total_toggles, uint32_t num_pi, uint32_t cycles)
{
	if (num_pi == 0 || cycles == 0)
	{
		return 0.0;

	}

	return (double)total_toggles / ((double)num_pi * (double)cycles);

}

static double calculate_mean(const double* values, int count)
{
	double sum;
	int i;

	if (!values || count <= 0)
	{
		return 0.0;

	}

	sum = 0.0;
	for (i = 0; i < count; ++i)
	{
		sum += values[i];

	}

	return sum / (double)count;

}

static double calculate_variance(const double* values, int count)
{
	double mean;
	double accum;
	int i;

	if (!values || count <= 0)
	{
		return 0.0;

	}

	mean = calculate_mean(values, count);
	accum = 0.0;
	for (i = 0; i < count; ++i)
	{
		double d = values[i] - mean;
		accum += d * d;

	}

	return accum / (double)count;

}

static double calculate_stddev(const double* values, int count)
{
	return sqrt(calculate_variance(values, count));

}

#endif
