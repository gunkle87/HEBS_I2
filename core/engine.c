#include "hebs_engine.h"
#include <string.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
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

#define HEBS_LANE_LUT_INDEX(a, b) ((((uint32_t)(a) & 0x3U) << 2U) | ((uint32_t)(b) & 0x3U))

/* 2-bit lane LUTs preserve current packed-lane gate behavior while reducing hot-loop decode math. */
static const uint8_t HEBS_LUT_AND_2B[16] =
{
	0U, 0U, 2U, 2U, 0U, 1U, 2U, 3U, 2U, 2U, 2U, 2U, 2U, 3U, 2U, 3U
};

static const uint8_t HEBS_LUT_OR_2B[16] =
{
	0U, 1U, 0U, 1U, 1U, 1U, 1U, 1U, 0U, 1U, 2U, 3U, 1U, 1U, 3U, 3U
};

static const uint8_t HEBS_LUT_XOR_2B[16] =
{
	0U, 1U, 2U, 3U, 1U, 0U, 3U, 2U, 2U, 3U, 2U, 3U, 3U, 2U, 3U, 2U
};

static const uint8_t HEBS_LUT_NAND_2B[16] =
{
	1U, 1U, 3U, 3U, 1U, 0U, 3U, 2U, 3U, 3U, 3U, 3U, 3U, 2U, 3U, 2U
};

static const uint8_t HEBS_LUT_NOR_2B[16] =
{
	1U, 0U, 1U, 0U, 0U, 0U, 0U, 0U, 1U, 0U, 3U, 2U, 0U, 0U, 2U, 2U
};

static const uint8_t HEBS_LUT_XNOR_2B[16] =
{
	1U, 0U, 3U, 2U, 0U, 1U, 2U, 3U, 3U, 2U, 3U, 2U, 2U, 3U, 2U, 3U
};

static const uint8_t HEBS_LUT_TRI_2B[16] =
{
	0U, 0U, 0U, 0U, 0U, 1U, 0U, 1U, 0U, 2U, 0U, 2U, 0U, 3U, 0U, 3U
};

static const uint8_t HEBS_LUT_NOT_2B[4] = { 1U, 0U, 3U, 2U };
static const uint8_t HEBS_LUT_BUF_2B[4] = { 0U, 1U, 2U, 3U };
static const uint8_t HEBS_LUT_PUP_2B[4] = { 1U, 1U, 3U, 3U };
static const uint8_t HEBS_LUT_PDN_2B[4] = { 0U, 0U, 2U, 2U };
static const uint8_t HEBS_LUT_VCC_2B = 1U;
static const uint8_t HEBS_LUT_GND_2B = 0U;

static inline uint64_t hebs_commit_lane_write(
	uint64_t* trays,
	const hebs_exec_instruction_t* exec_instr,
	uint64_t out_lane)
{
	const uint64_t shifted_lane = (out_lane & 0x3ULL) << exec_instr->dst_shift;
	const uint64_t old_value = trays[exec_instr->dst_tray];
	const uint64_t new_value = (old_value & ~exec_instr->dst_mask) | shifted_lane;

	trays[exec_instr->dst_tray] = new_value;
#if HEBS_COMPAT_PROBES_ENABLED
	return (uint64_t)(new_value != old_value);
#else
	return 0U;
#endif

}

#define HEBS_FLUSH_CHUNK_PROBES(ctx, writes, changes) \
	do \
	{ \
		(ctx)->probe_chunk_exec += 1U; \
		(ctx)->probe_gate_eval += (uint64_t)(writes); \
		if (HEBS_COMPAT_PROBES_ENABLED) \
		{ \
			(ctx)->probe_state_change_commit += (changes); \
		} \
	} while (0)

static void hebs_execute_and_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_AND_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_or_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_OR_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_not_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_NOT_2B[(uint32_t)a_lane & 0x3U];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_nand_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_NAND_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_nor_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_NOR_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_buf_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_BUF_2B[(uint32_t)a_lane & 0x3U];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_xor_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_XOR_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_xnor_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t b_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_XNOR_2B[HEBS_LANE_LUT_INDEX(a_lane, b_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_tri_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t data_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t enable_lane = (trays[exec_instr->src_b_tray] >> exec_instr->src_b_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_TRI_2B[HEBS_LANE_LUT_INDEX(data_lane, enable_lane)];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_vcc_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t out_lane = (uint64_t)HEBS_LUT_VCC_2B;
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_gnd_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t out_lane = (uint64_t)HEBS_LUT_GND_2B;
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_pup_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_PUP_2B[(uint32_t)a_lane & 0x3U];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

}

static void hebs_execute_pdn_chunk(
	hebs_engine* ctx,
	const hebs_plan* plan,
	uint32_t chunk_start,
	uint32_t chunk_count )
{
	uint32_t idx;
	const hebs_exec_instruction_t* exec_base;
	uint64_t* trays;
	uint64_t state_change_count = 0U;

	if (chunk_count == 0U)
	{
		return;

	}

	exec_base = &plan->comb_exec_data[chunk_start];
	trays = ctx->next_signal_trays;

#pragma GCC unroll 4
	for (idx = 0U; idx < chunk_count; ++idx)
	{
		const hebs_exec_instruction_t* exec_instr = &exec_base[idx];
		const uint64_t a_lane = (trays[exec_instr->src_a_tray] >> exec_instr->src_a_shift) & 0x3ULL;
		const uint64_t out_lane = (uint64_t)HEBS_LUT_PDN_2B[(uint32_t)a_lane & 0x3U];
		state_change_count += hebs_commit_lane_write(trays, exec_instr, out_lane);

	}

	HEBS_FLUSH_CHUNK_PROBES(ctx, chunk_count, state_change_count);

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
				hebs_execute_and_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_OR:
				hebs_execute_or_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_XOR:
				hebs_execute_xor_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_NOT:
				hebs_execute_not_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_NAND:
				hebs_execute_nand_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_NOR:
				hebs_execute_nor_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_XNOR:
				hebs_execute_xnor_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_BUF:
				hebs_execute_buf_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_TRI:
				hebs_execute_tri_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_VCC:
				hebs_execute_vcc_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_GND:
				hebs_execute_gnd_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_PUP:
				hebs_execute_pup_chunk(ctx, plan, span->start, span->count );
				break;
			case HEBS_GATE_PDN:
				hebs_execute_pdn_chunk(ctx, plan, span->start, span->count );
				break;
			default:
				break;

		}

	}

}

static void hebs_execute_combinational_fallback(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;
#if HEBS_COMPAT_PROBES_ENABLED
	uint64_t state_change_count = 0U;
#endif
	uint64_t write_count = 0U;

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
			case HEBS_GATE_XOR:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_xor(a, b);
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
			case HEBS_GATE_XNOR:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_xnor(a, b);
				break;
			}
			case HEBS_GATE_BUF:
				result = a;
				break;
			case HEBS_GATE_TRI:
			{
				hebs_logic_t b = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->src_b_bit_offset);
				result = hebs_eval_tristate(a, b);
				break;
			}
			case HEBS_GATE_VCC:
				result = hebs_eval_vcc();
				break;
			case HEBS_GATE_GND:
				result = hebs_eval_gnd();
				break;
			case HEBS_GATE_PUP:
				result = hebs_eval_weak_pull(a);
				break;
			case HEBS_GATE_PDN:
				result = hebs_eval_weak_pull_down(a);
				break;
			case HEBS_GATE_DFF:
				continue;
			default:
				result = HEBS_X;
				break;

		}

		++write_count;
		#if HEBS_COMPAT_PROBES_ENABLED
		{
			const hebs_logic_t old_value = hebs_read_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->dst_bit_offset);
			hebs_write_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->dst_bit_offset, result);
			state_change_count += (uint64_t)(old_value != result);

		}
		#else
		hebs_write_logic_at_offset(ctx->next_signal_trays, ctx->tray_count, instr->dst_bit_offset, result);
		#endif

	}

	ctx->probe_gate_eval += write_count;
	ctx->probe_chunk_exec += write_count;
#if HEBS_COMPAT_PROBES_ENABLED
	ctx->probe_state_change_commit += state_change_count;
#endif

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
	ctx->cycles_executed = 0;
	ctx->vectors_applied = 0;
	ctx->probe_input_apply = 0;
	ctx->probe_input_toggle = 0;
	ctx->probe_chunk_exec = 0;
	ctx->probe_state_change_commit = 0;
	ctx->probe_dff_exec = 0;
	ctx->probe_gate_eval = 0;
	memset(ctx->tray_plane_a, 0, sizeof(ctx->tray_plane_a));
	memset(ctx->tray_plane_b, 0, sizeof(ctx->tray_plane_b));
	memset(ctx->dff_state_trays, 0, sizeof(ctx->dff_state_trays));
	ctx->signal_trays = ctx->tray_plane_a;
	ctx->next_signal_trays = ctx->tray_plane_b;
	return HEBS_OK;

}

void hebs_tick(hebs_engine* ctx, hebs_plan* plan)
{
	if (!ctx || !plan)
	{
		return;

	}
	memcpy(ctx->next_signal_trays, ctx->signal_trays, (size_t)ctx->tray_count * sizeof(uint64_t));
	if (plan->comb_exec_data && plan->comb_spans && plan->comb_span_count > 0U)
	{
		hebs_execute_combinational_batched(ctx, plan );

	}
	else
	{
		hebs_execute_combinational_fallback(ctx, plan );

	}

	hebs_sequential_commit(ctx, plan);
	hebs_swap_active_trays(ctx);
	++ctx->cycles_executed;
	++ctx->vectors_applied;
	ctx->current_tick++;

}

hebs_probes hebs_get_probes(const hebs_engine* ctx)
{
	hebs_probes probes;

	memset(&probes, 0, sizeof(probes));
	if (!ctx)
	{
		return probes;

	}

	probes.input_apply = ctx->probe_input_apply;
	probes.input_toggle = ctx->probe_input_toggle;
	probes.chunk_exec = ctx->probe_chunk_exec;
	probes.gate_eval = ctx->probe_gate_eval;
	probes.state_change_commit = ctx->probe_state_change_commit;
	probes.dff_exec = ctx->probe_dff_exec;
	return probes;

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
#if HEBS_COMPAT_PROBES_ENABLED
	hebs_logic_t old_value;
#endif

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
	++ctx->probe_input_apply;
#if HEBS_COMPAT_PROBES_ENABLED
	old_value = hebs_read_logic_at_offset(ctx->signal_trays, ctx->tray_count, bit_offset);
	if (old_value != value)
	{
		++ctx->probe_input_toggle;

	}
#endif
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
