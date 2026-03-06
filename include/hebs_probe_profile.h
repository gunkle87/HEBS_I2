#ifndef HEBS_PROBE_PROFILE_H
#define HEBS_PROBE_PROFILE_H

#if defined(HEBS_PROBE_PROFILE_PERF) && defined(HEBS_PROBE_PROFILE_COMPAT)
#error "Exactly one HEBS probe profile must be selected."
#endif

#if !defined(HEBS_PROBE_PROFILE_PERF) && !defined(HEBS_PROBE_PROFILE_COMPAT)
#define HEBS_PROBE_PROFILE_PERF 1
#endif

#if defined(HEBS_PROBE_PROFILE_COMPAT)
#define HEBS_COMPAT_PROBES_ENABLED 1
#else
#define HEBS_COMPAT_PROBES_ENABLED 0
#endif

#endif
