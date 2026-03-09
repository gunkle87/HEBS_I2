#ifndef HEBS_PROBE_PROFILE_H
#define HEBS_PROBE_PROFILE_H

#if defined(HEBS_PROBE_PROFILE_PERF) && defined(HEBS_PROBE_PROFILE_TEST)
#error "Exactly one HEBS probe profile must be selected."
#endif

#if !defined(HEBS_PROBE_PROFILE_PERF) && !defined(HEBS_PROBE_PROFILE_TEST)
#define HEBS_PROBE_PROFILE_PERF 1
#endif

#if !defined(HEBS_TEST_PROBES)
#if defined(HEBS_PROBE_PROFILE_TEST)
#define HEBS_TEST_PROBES 1
#else
#define HEBS_TEST_PROBES 0
#endif
#endif

#endif
