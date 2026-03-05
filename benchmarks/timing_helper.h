#ifndef HEBS_TIMING_HELPER_H
#define HEBS_TIMING_HELPER_H

#include <windows.h>

typedef struct hebs_timer_s
{
	LARGE_INTEGER start;
	LARGE_INTEGER stop;
	LARGE_INTEGER frequency;

} hebs_timer_t;

static void timer_start(hebs_timer_t* t)
{
	QueryPerformanceFrequency(&t->frequency);
	QueryPerformanceCounter(&t->start);
	t->stop = t->start;

}

static void timer_stop(hebs_timer_t* t)
{
	QueryPerformanceCounter(&t->stop);

}

static double timer_elapsed_sec(const hebs_timer_t* t)
{
	return (double)(t->stop.QuadPart - t->start.QuadPart) / (double)t->frequency.QuadPart;

}

#endif
