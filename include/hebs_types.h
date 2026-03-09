#ifndef HEBS_TYPES_H
#define HEBS_TYPES_H

#include <stdint.h>
#include "hebs_probe_profile.h"

#ifndef HEBS_TEST_PROBES
#define HEBS_TEST_PROBES 0
#endif

typedef struct hebs_probes_s
{
	uint64_t input_apply;
#if HEBS_TEST_PROBES
	uint64_t input_toggle;
#endif
	uint64_t chunk_exec;
	uint64_t gate_eval;
#if HEBS_TEST_PROBES
	uint64_t state_change_commit;
#endif
	uint64_t dff_exec;

} hebs_probes;

#endif
