#include "hebs_engine.h"
#include <string.h>
#include "primitives.h"
#include "state_manager.h"

static hebs_logic_t hebs_read_logic_at_offset(const uint64_t* trays, uint32_t tray_count, uint32_t bit_offset)
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

static void hebs_write_logic_at_offset(uint64_t* trays, uint32_t tray_count, uint32_t bit_offset, hebs_logic_t value)
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

static void hebs_swap_active_trays(hebs_engine* ctx)
{
	uint64_t* swap;

	if (!ctx)
	{
		return;

	}

	swap = ctx->signal_trays;
	ctx->signal_trays = ctx->next_signal_trays;
	ctx->next_signal_trays = swap;

}

typedef uint64_t (*hebs_binary_gate_simd_fn)(uint64_t tray_a, uint64_t tray_b);
typedef uint64_t (*hebs_unary_gate_simd_fn)(uint64_t tray_a);

static void hebs_execute_binary_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count,
	hebs_binary_gate_simd_fn eval_fn)
{
	uint32_t idx;
	uint64_t* trays;

	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &plan->comb_exec_data[chunk_start + idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = eval_fn(a_lane, b_lane) & 0x3ULL;
		const uint64_t shifted_lane = out_lane << exec_instr->dst_shift;
		const uint64_t tray_value = trays[exec_instr->dst_tray];
		trays[exec_instr->dst_tray] = (tray_value & ~exec_instr->dst_mask) | shifted_lane;

	}

	ctx->gate_evals += chunk_count;
	ctx->signal_writes_committed += chunk_count;

}

static void hebs_execute_unary_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count,
	hebs_unary_gate_simd_fn eval_fn)
{
	uint32_t idx;
	uint64_t* trays;

	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &plan->comb_exec_data[chunk_start + idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t out_lane = eval_fn(a_lane) & 0x3ULL;
		const uint64_t shifted_lane = out_lane << exec_instr->dst_shift;
		const uint64_t tray_value = trays[exec_instr->dst_tray];
		trays[exec_instr->dst_tray] = (tray_value & ~exec_instr->dst_mask) | shifted_lane;

	}

	ctx->gate_evals += chunk_count;
	ctx->signal_writes_committed += chunk_count;

}

static void hebs_execute_combinational_batched(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t span_idx;

	for (span_idx = 0U; span_idx < plan->comb_span_count; ++span_idx)
	{
		const hebs_gate_span_t* span = &plan->comb_spans[span_idx];
		switch ((hebs_gate_type_t)span->gate_type)
		{
			case HEBS_GATE_AND:
				hebs_execute_binary_chunk(ctx, plan, span->start, span->count, hebs_gate_and_simd);
				break;
			case HEBS_GATE_OR:
				hebs_execute_binary_chunk(ctx, plan, span->start, span->count, hebs_gate_or_simd);
				break;
			case HEBS_GATE_NOT:
				hebs_execute_unary_chunk(ctx, plan, span->start, span->count, hebs_gate_not_simd);
				break;
			case HEBS_GATE_NAND:
				hebs_execute_binary_chunk(ctx, plan, span->start, span->count, hebs_gate_nand_simd);
				break;
			case HEBS_GATE_NOR:
				hebs_execute_binary_chunk(ctx, plan, span->start, span->count, hebs_gate_nor_simd);
				break;
			case HEBS_GATE_BUF:
				hebs_execute_unary_chunk(ctx, plan, span->start, span->count, hebs_gate_buf_simd);
				break;
			default:
				break;

		}

	}

}

static void hebs_execute_combinational_fallback(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;

	for (instr_idx = 0U; instr_idx < plan->gate_count; ++instr_idx)
	{
		const hebs_lep_instruction_t* instr = &plan->lep_data[instr_idx];
		hebs_logic_t a = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_a_bit_offset);
		hebs_logic_t result = HEBS_X;

		switch ((hebs_gate_type_t)instr->gate_type)
		{
			case HEBS_GATE_AND:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_and(a, b);
				break;
			}
			case HEBS_GATE_OR:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_or(a, b);
				break;
			}
			case HEBS_GATE_NOT:
				result = hebs_eval_not(a);
				break;
			case HEBS_GATE_NAND:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_not(hebs_eval_and(a, b));
				break;
			}
			case HEBS_GATE_NOR:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_not(hebs_eval_or(a, b));
				break;
			}
			case HEBS_GATE_BUF:
				result = a;
				break;
			case HEBS_GATE_DFF:
				continue;
			default:
				result = HEBS_X;
				break;

		}

		++ctx->gate_evals;
		hebs_write_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->dst_bit_offset, result);
		++ctx->signal_writes_committed;

	}

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
	ctx->internal_transition_count = 0;
	ctx->cycles_executed = 0;
	ctx->vectors_applied = 0;
	ctx->gate_evals = 0;
	ctx->signal_writes_committed = 0;
	memset(ctx->tray_plane_a, 0, sizeof(ctx->tray_plane_a));
	memset(ctx->tray_plane_b, 0, sizeof(ctx->tray_plane_b));
	memset(ctx->dff_state_trays, 0, sizeof(ctx->dff_state_trays));
	ctx->signal_trays = ctx->tray_plane_a;
	ctx->next_signal_trays = ctx->tray_plane_b;
	memset(ctx->previous_input_state, 0, sizeof(ctx->previous_input_state));
	return HEBS_OK;

}

void hebs_tick(hebs_engine* ctx, hebs_plan* plan)
{
	uint32_t input_idx;

	if (!ctx || !plan)
	{
		return;

	}

	for (input_idx = 0; input_idx < plan->num_primary_inputs && input_idx < HEBS_MAX_PRIMARY_INPUTS; ++input_idx)
	{
		uint32_t signal_id = plan->primary_input_ids[input_idx];
		uint32_t bit_offset = signal_id * 2U;
		uint8_t current_value = (uint8_t)hebs_read_logic_at_offset(ctx->signal_trays, ctx->tray_count, bit_offset);

		if (ctx->previous_input_state[input_idx] != current_value)
		{
			++ctx->input_toggle_count;
			ctx->previous_input_state[input_idx] = current_value;

		}

	}

	memcpy(ctx->next_signal_trays, ctx->signal_trays, (size_t)ctx->tray_count * sizeof(uint64_t));
	if (plan->comb_exec_data && plan->comb_spans && plan->comb_span_count > 0U)
	{
		hebs_execute_combinational_batched(ctx, plan);

	}
	else
	{
		hebs_execute_combinational_fallback(ctx, plan);

	}

	hebs_sequential_commit(ctx, plan);
	hebs_swap_active_trays(ctx);
	++ctx->cycles_executed;
	++ctx->vectors_applied;
	ctx->current_tick++;

}

hebs_metrics hebs_get_metrics(const hebs_engine* ctx, const hebs_plan* plan)
{
	hebs_metrics metrics;

	memset(&metrics, 0, sizeof(metrics));
	if (!ctx || !plan)
	{
		return metrics;

	}

	metrics.gate_count = plan->gate_count;
	metrics.net_count = plan->signal_count;
	metrics.pi_count = plan->num_primary_inputs;
	metrics.po_count = plan->num_primary_outputs;
	metrics.fanout_avg = plan->fanout_avg;
	metrics.fanout_max = plan->fanout_max;
	metrics.level_depth = plan->propagation_depth;

	metrics.cycles_executed = ctx->cycles_executed;
	metrics.vectors_applied = ctx->vectors_applied;
	metrics.gate_evals = ctx->gate_evals;
	metrics.signal_writes_committed = ctx->signal_writes_committed;

	metrics.primary_input_transitions = ctx->input_toggle_count;
	metrics.internal_node_transitions = ctx->internal_transition_count;
	return metrics;

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
	hebs_write_logic_at_offset(ctx->signal_trays, ctx->tray_count, bit_offset, value);
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
	return hebs_read_logic_at_offset(ctx->signal_trays, ctx->tray_count, bit_offset);

}

const uint64_t* hebs_get_signal_tray(const hebs_engine* ctx, uint32_t tray_index)
{
	if (!ctx || !ctx->signal_trays || tray_index >= ctx->tray_count)
	{
		return NULL;

	}

	return &ctx->signal_trays[tray_index];

}
