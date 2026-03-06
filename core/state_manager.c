#include "state_manager.h"
#include <string.h>

#define HEBS_DFF_SCALAR_THRESHOLD 64U

static void hebs_sequential_commit_scalar(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;
	uint64_t dff_exec_count = 0U;
	uint64_t write_commit_count = 0U;
	uint64_t tray_exec_count = 0U;
#if HEBS_COMPAT_PROBES_ENABLED
	uint64_t state_change_count = 0U;
#endif

	for (instr_idx = 0U; instr_idx < plan->dff_exec_count; ++instr_idx)
	{
		const hebs_exec_instruction_t* exec_instr = &plan->dff_exec_data[instr_idx];
		const uint64_t next_lane = (ctx->next_signal_trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t shifted_lane = next_lane << exec_instr->dst_shift;
		const uint64_t state_tray = ctx->dff_state_trays[exec_instr->dst_tray];
		const uint64_t signal_tray = ctx->next_signal_trays[exec_instr->dst_tray];
		const uint64_t new_value = (state_tray & ~exec_instr->dst_mask) | shifted_lane;

		++dff_exec_count;
		ctx->dff_state_trays[exec_instr->dst_tray] = new_value;
		ctx->next_signal_trays[exec_instr->dst_tray] = (signal_tray & ~exec_instr->dst_mask) | shifted_lane;
		write_commit_count += 2U;
		tray_exec_count += 2U;
#if HEBS_COMPAT_PROBES_ENABLED
		state_change_count += (uint64_t)(new_value != state_tray);
#endif

	}

	ctx->probe_dff_exec += dff_exec_count;
	ctx->probe_state_write_commit += write_commit_count;
	ctx->probe_tray_exec += tray_exec_count;
#if HEBS_COMPAT_PROBES_ENABLED
	ctx->probe_state_change_commit += state_change_count;
#endif

}

static void hebs_sequential_commit_vectorized(hebs_engine* ctx, const hebs_plan* plan)
{
	uint64_t staged_dff_trays[HEBS_MAX_SIGNAL_TRAYS];
	uint32_t instr_idx;
	uint32_t tray_idx;
	uint64_t dff_exec_count = 0U;
	uint64_t write_commit_count = 0U;
	uint64_t tray_exec_count = 0U;
#if HEBS_COMPAT_PROBES_ENABLED
	uint64_t state_change_count = 0U;
#endif

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
		const uint64_t new_value = (tray_value & ~exec_instr->dst_mask) | shifted_lane;
		++dff_exec_count;
		++write_commit_count;
		++tray_exec_count;
#if HEBS_COMPAT_PROBES_ENABLED
		state_change_count += (uint64_t)(new_value != tray_value);
#endif
		staged_dff_trays[exec_instr->dst_tray] = new_value;

	}

	for (tray_idx = 0U; tray_idx < ctx->tray_count; ++tray_idx)
	{
		const uint64_t committed_tray = staged_dff_trays[tray_idx];
		const uint64_t commit_mask = plan->dff_commit_mask[tray_idx];
		ctx->dff_state_trays[tray_idx] = committed_tray;
		if (commit_mask != 0U)
		{
			const uint64_t merged = (ctx->next_signal_trays[tray_idx] & ~commit_mask) | (committed_tray & commit_mask);
#if HEBS_COMPAT_PROBES_ENABLED
			const uint64_t old_next = ctx->next_signal_trays[tray_idx];
#endif
			ctx->next_signal_trays[tray_idx] = merged;
			++write_commit_count;
			++tray_exec_count;
#if HEBS_COMPAT_PROBES_ENABLED
			state_change_count += (uint64_t)(merged != old_next);
#endif

		}

	}

	ctx->probe_dff_exec += dff_exec_count;
	ctx->probe_state_write_commit += write_commit_count;
	ctx->probe_tray_exec += tray_exec_count;
#if HEBS_COMPAT_PROBES_ENABLED
	ctx->probe_state_change_commit += state_change_count;
#endif

}

void hebs_sequential_commit(hebs_engine* ctx, const hebs_plan* plan)
{
	if (!ctx || !plan)
	{
		return;

	}

	if (plan->dff_exec_count == 0U || !plan->dff_exec_data || !plan->dff_commit_mask)
	{
		return;

	}

	if (plan->dff_exec_count < HEBS_DFF_SCALAR_THRESHOLD)
	{
		hebs_sequential_commit_scalar(ctx, plan);
		return;

	}

	hebs_sequential_commit_vectorized(ctx, plan);

}
