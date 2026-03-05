#include "hebs_engine.h"
#include <string.h>
#include "primitives.h"

static hebs_logic_t hebs_read_logic_at_offset(const hebs_engine* ctx, uint32_t bit_offset)
{
	uint32_t tray_index;
	uint32_t bit_position;
	uint64_t tray;

	if (!ctx)
	{
		return HEBS_S0;

	}

	tray_index = bit_offset / 64U;
	bit_position = bit_offset % 64U;
	if (tray_index >= ctx->tray_count)
	{
		return HEBS_S0;

	}

	tray = ctx->signal_trays[tray_index];
	return (hebs_logic_t)((tray >> bit_position) & 0x3U);

}

static void hebs_write_logic_at_offset(hebs_engine* ctx, uint32_t bit_offset, hebs_logic_t value)
{
	uint32_t tray_index;
	uint32_t bit_position;
	uint64_t mask;

	if (!ctx)
	{
		return;

	}

	tray_index = bit_offset / 64U;
	bit_position = bit_offset % 64U;
	if (tray_index >= ctx->tray_count)
	{
		return;

	}

	mask = ~(0x3ULL << bit_position);
	ctx->signal_trays[tray_index] = (ctx->signal_trays[tray_index] & mask) | (((uint64_t)value & 0x3ULL) << bit_position);

}

hebs_status_t hebs_init_engine(hebs_engine* ctx, hebs_plan* plan)
{
	if (!ctx || !plan)
	{
		return HEBS_ERR_LOGIC;

	}

	if (plan->tray_count > HEBS_MAX_SIGNAL_TRAYS)
	{
		return HEBS_ERR_LOGIC;

	}

	if (plan->num_primary_inputs > HEBS_MAX_PRIMARY_INPUTS)
	{
		return HEBS_ERR_LOGIC;

	}

	ctx->current_tick = 0;
	ctx->tray_count = plan->tray_count;
	ctx->input_toggle_count = 0;
	memset(ctx->signal_trays, 0, sizeof(ctx->signal_trays));
	memset(ctx->previous_input_state, 0, sizeof(ctx->previous_input_state));
	return HEBS_OK;

}

void hebs_tick(hebs_engine* ctx, hebs_plan* plan)
{
	uint32_t input_idx;
	uint32_t instr_idx;

	if (!ctx || !plan)
	{
		return;

	}

	for (input_idx = 0; input_idx < plan->num_primary_inputs && input_idx < HEBS_MAX_PRIMARY_INPUTS; ++input_idx)
	{
		uint32_t signal_id = plan->primary_input_ids[input_idx];
		uint32_t bit_offset = signal_id * 2U;
		uint8_t current_value = (uint8_t)hebs_read_logic_at_offset(ctx, bit_offset);

		if (ctx->previous_input_state[input_idx] != current_value)
		{
			++ctx->input_toggle_count;
			ctx->previous_input_state[input_idx] = current_value;

		}

	}

	for (instr_idx = 0; instr_idx < plan->gate_count; ++instr_idx)
	{
		const hebs_lep_instruction_t* instr = &plan->lep_data[instr_idx];
		hebs_logic_t a = hebs_read_logic_at_offset(ctx, instr->src_a_bit_offset);
		hebs_logic_t b = hebs_read_logic_at_offset(ctx, instr->src_b_bit_offset);
		hebs_logic_t result = HEBS_X;

		switch ((hebs_gate_type_t)instr->gate_type)
		{
			case HEBS_GATE_AND:
				result = hebs_eval_and(a, b);
				break;
			case HEBS_GATE_OR:
				result = hebs_eval_or(a, b);
				break;
			case HEBS_GATE_NOT:
				result = hebs_eval_not(a);
				break;
			case HEBS_GATE_NAND:
				result = hebs_eval_not(hebs_eval_and(a, b));
				break;
			case HEBS_GATE_NOR:
				result = hebs_eval_not(hebs_eval_or(a, b));
				break;
			case HEBS_GATE_BUF:
				result = a;
				break;
			default:
				result = HEBS_X;
				break;

		}

		hebs_write_logic_at_offset(ctx, instr->dst_bit_offset, result);

	}

	ctx->current_tick++;

}

uint64_t hebs_get_state_hash(hebs_engine* ctx)
{
	uint64_t hash;
	uint32_t tray_idx;

	if (!ctx)
	{
		return 0;

	}

	hash = 1469598103934665603ULL;
	hash ^= ctx->current_tick;
	hash *= 1099511628211ULL;

	for (tray_idx = 0; tray_idx < ctx->tray_count; ++tray_idx)
	{
		hash ^= ctx->signal_trays[tray_idx];
		hash *= 1099511628211ULL;

	}

	return hash;

}

hebs_status_t hebs_set_primary_input(hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index, hebs_logic_t value)
{
	uint32_t signal_id;
	uint32_t bit_offset;

	if (!ctx || !plan || input_index >= plan->num_primary_inputs)
	{
		return HEBS_ERR_LOGIC;

	}

	if (value > HEBS_WX)
	{
		return HEBS_ERR_LOGIC;

	}

	signal_id = plan->primary_input_ids[input_index];
	bit_offset = signal_id * 2U;
	hebs_write_logic_at_offset(ctx, bit_offset, value);
	return HEBS_OK;

}

hebs_logic_t hebs_get_primary_input(const hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index)
{
	uint32_t signal_id;
	uint32_t bit_offset;

	if (!ctx || !plan || input_index >= plan->num_primary_inputs)
	{
		return HEBS_X;

	}

	signal_id = plan->primary_input_ids[input_index];
	bit_offset = signal_id * 2U;
	return hebs_read_logic_at_offset(ctx, bit_offset);

}
