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
	uint64_t chunk_exec;
	uint64_t gate_eval;
	uint64_t dff_exec;
	uint64_t tick_count;
	uint64_t state_commit_count;
#if HEBS_TEST_PROBES
	uint64_t input_toggle;
	uint64_t state_change_commit;
	uint64_t contention_count;
	uint64_t unknown_state_materialize_count;
	uint64_t highz_materialize_count;
	uint64_t multi_driver_resolve_count;
	uint64_t tri_no_drive_count;
	uint64_t pup_z_source_count;
	uint64_t pdn_z_source_count;
#endif

} hebs_probes;

#endif
