#include "state_manager.h"

static hebs_logic_t hebs_read_tray_logic(const uint64_t* trays, uint32_t tray_count, uint32_t bit_offset)
{
	uint32_t tray_index;
	uint32_t bit_position;

	if (!trays)
	{
		return HEBS_S0;

	}

	tray_index = bit_offset / 64U;
	bit_position = bit_offset % 64U;
	if (tray_index >= tray_count)
	{
		return HEBS_S0;

	}

	return (hebs_logic_t)((trays[tray_index] >> bit_position) & 0x3U);

}

static void hebs_write_tray_logic(uint64_t* trays, uint32_t tray_count, uint32_t bit_offset, hebs_logic_t value)
{
	uint32_t tray_index;
	uint32_t bit_position;
	uint64_t mask;

	if (!trays)
	{
		return;

	}

	tray_index = bit_offset / 64U;
	bit_position = bit_offset % 64U;
	if (tray_index >= tray_count)
	{
		return;

	}

	mask = ~(0x3ULL << bit_position);
	trays[tray_index] = (trays[tray_index] & mask) | (((uint64_t)value & 0x3ULL) << bit_position);

}

void hebs_sequential_commit(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;

	if (!ctx || !plan)
	{
		return;

	}

	if (plan->dff_instruction_count == 0U || !plan->dff_instruction_indices)
	{
		return;

	}

	for (instr_idx = 0; instr_idx < plan->dff_instruction_count; ++instr_idx)
	{
		const uint32_t dff_idx = plan->dff_instruction_indices[instr_idx];
		const hebs_lep_instruction_t* instr = &plan->lep_data[dff_idx];
		hebs_logic_t dff_state_value;
		hebs_logic_t dff_next_value;
		uint64_t delta_bits;

		dff_state_value = hebs_read_tray_logic(ctx->dff_state_trays, ctx->tray_count, instr->dst_bit_offset);
		dff_next_value = hebs_read_tray_logic(ctx->next_signal_trays, ctx->tray_count, instr->src_a_bit_offset);
		delta_bits = ((uint64_t)dff_state_value ^ (uint64_t)dff_next_value) & 0x3ULL;
		ctx->internal_transition_count += (uint64_t)__builtin_popcountll(delta_bits);
		hebs_write_tray_logic(ctx->dff_state_trays, ctx->tray_count, instr->dst_bit_offset, dff_next_value);
		++ctx->signal_writes_committed;

	}

	for (instr_idx = 0; instr_idx < plan->dff_instruction_count; ++instr_idx)
	{
		const uint32_t dff_idx = plan->dff_instruction_indices[instr_idx];
		const hebs_lep_instruction_t* instr = &plan->lep_data[dff_idx];
		hebs_logic_t committed_value;

		committed_value = hebs_read_tray_logic(ctx->dff_state_trays, ctx->tray_count, instr->dst_bit_offset);
		hebs_write_tray_logic(ctx->next_signal_trays, ctx->tray_count, instr->dst_bit_offset, committed_value);
		++ctx->signal_writes_committed;

	}

}
