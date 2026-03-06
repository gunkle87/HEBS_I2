#include "state_manager.h"
#include <string.h>

void hebs_sequential_commit(hebs_engine* ctx, const hebs_plan* plan)
{
	uint64_t staged_dff_trays[HEBS_MAX_SIGNAL_TRAYS];
	uint32_t instr_idx;
	uint32_t tray_idx;

	if (!ctx || !plan)
	{
		return;

	}

	if (plan->dff_exec_count == 0U || !plan->dff_exec_data || !plan->dff_commit_mask)
	{
		return;

	}

	if (ctx->tray_count > HEBS_MAX_SIGNAL_TRAYS)
	{
		return;

	}

	memcpy(staged_dff_trays, ctx->dff_state_trays, (size_t)ctx->tray_count * sizeof(uint64_t));

	for (instr_idx = 0U; instr_idx < plan->dff_exec_count; ++instr_idx)
	{
		const hebs_exec_instruction_t* exec_instr = &plan->dff_exec_data[instr_idx];
		const uint64_t src_lane = (ctx->next_signal_trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t shifted_lane = src_lane << exec_instr->dst_shift;
		const uint64_t tray_value = staged_dff_trays[exec_instr->dst_tray];
		staged_dff_trays[exec_instr->dst_tray] = (tray_value & ~exec_instr->dst_mask) | shifted_lane;
		++ctx->signal_writes_committed;

	}

	for (tray_idx = 0U; tray_idx < ctx->tray_count; ++tray_idx)
	{
		const uint64_t committed_tray = staged_dff_trays[tray_idx];
		const uint64_t delta_tray = ctx->dff_state_trays[tray_idx] ^ committed_tray;
		const uint64_t commit_mask = plan->dff_commit_mask[tray_idx];
		ctx->internal_transition_count += (uint64_t)__builtin_popcountll(delta_tray);
		ctx->dff_state_trays[tray_idx] = committed_tray;
		if (commit_mask != 0U)
		{
			const uint64_t merged = (ctx->next_signal_trays[tray_idx] & ~commit_mask) | (committed_tray & commit_mask);
			ctx->next_signal_trays[tray_idx] = merged;
			++ctx->signal_writes_committed;

		}

	}

}
