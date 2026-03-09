#include "hebs_engine.h"
#include <string.h>

#define HEBS_STATE_Z 0U
#define HEBS_STATE_S1 1U
#define HEBS_STATE_S0 2U
#define HEBS_STATE_X 3U
#define HEBS_STATE_W1 4U
#define HEBS_STATE_W0 5U
#define HEBS_STATE_WX 6U
#define HEBS_STATE_SX 7U

/* Index (4Bi): [SH, SL, WH, WL]
   Packing: bit6=X, bit5=Logic, bits2:0=3Bn */
static const uint8_t HEBS_FUSED_LUT[16] =
{
	0x00U, 0x05U, 0x24U, 0x46U,
	0x02U, 0x02U, 0x02U, 0x02U,
	0x21U, 0x21U, 0x21U, 0x21U,
	0x47U, 0x47U, 0x47U, 0x47U
};

static uint32_t hebs_net_id_from_tray_shift(uint32_t tray_idx, uint8_t tray_shift)
{
	return (tray_idx * 32U) + ((uint32_t)tray_shift >> 1U);

}

static uint8_t hebs_read_pstate_from_word(uint64_t tray_word, uint8_t tray_shift)
{
	return (uint8_t)((tray_word >> tray_shift) & 0x3ULL);

}

static uint8_t hebs_read_pstate_net(const hebs_engine* ctx, uint32_t net_id)
{
	const uint32_t tray_idx = net_id >> 5U;
	const uint32_t tray_shift = (net_id & 31U) << 1U;
	return (uint8_t)((ctx->signal_trays[tray_idx] >> tray_shift) & 0x3ULL);

}

static void hebs_write_pstate_to_trays(uint64_t* trays, uint32_t net_id, uint8_t pstate)
{
	const uint32_t tray_idx = net_id >> 5U;
	const uint32_t tray_shift = (net_id & 31U) << 1U;
	const uint64_t mask = ~(0x3ULL << tray_shift);
	const uint64_t value = ((uint64_t)(pstate & 0x3U)) << tray_shift;
	trays[tray_idx] = (trays[tray_idx] & mask) | value;

}

static void hebs_write_pstate_net(hebs_engine* ctx, uint32_t net_id, uint8_t pstate)
{
	hebs_write_pstate_to_trays(ctx->signal_trays, net_id, pstate);
	if (ctx->next_signal_trays && ctx->next_signal_trays != ctx->signal_trays)
	{
		hebs_write_pstate_to_trays(ctx->next_signal_trays, net_id, pstate);

	}

}

static void hebs_mailbox_or(hebs_engine* ctx, uint32_t net_id, uint8_t nibble)
{
	const uint8_t drive_nibble = (uint8_t)(nibble & 0x0FU);
	const uint8_t old_mailbox = ctx->net_mailbox[net_id];
	const uint8_t merged_mailbox = (uint8_t)(old_mailbox | drive_nibble);

	if (drive_nibble == 0U)
	{
		return;

	}

	if (old_mailbox == 0U)
	{
		/* Mailbox zero is the first-touch invariant for dirty-list insertion. */
		ctx->net_mailbox[net_id] = drive_nibble;
		ctx->dirty_net_ids[ctx->dirty_count++] = net_id;
		return;

	}

	if (merged_mailbox == old_mailbox)
	{
		return;

	}

	ctx->net_mailbox[net_id] = merged_mailbox;

}

static uint8_t hebs_make_drive_nibble(uint8_t logic_bit, uint8_t x_flag, uint8_t strong_drive)
{
	const uint8_t logic = (uint8_t)(logic_bit & 1U);
	const uint8_t x = (uint8_t)(x_flag & 1U);
	const uint8_t strong = (uint8_t)(strong_drive & 1U);
	const uint8_t base = (uint8_t)(1U + (strong * 3U));
	const uint8_t high = (uint8_t)(base << 1U);
	const uint8_t x_mix = (uint8_t)(base | high);
	const uint8_t value_mix = (uint8_t)(base + (logic * base));
	return (uint8_t)((x * x_mix) + ((1U - x) * value_mix));

}

static uint8_t hebs_logic_to_physical(hebs_logic_t value)
{
	switch (value)
	{
		case HEBS_Z:
			return HEBS_STATE_Z;
		case HEBS_S1:
			return HEBS_STATE_S1;
		case HEBS_S0:
			return HEBS_STATE_S0;
		case HEBS_X:
			return HEBS_STATE_X;
		case HEBS_W1:
			return HEBS_STATE_W1;
		case HEBS_W0:
			return HEBS_STATE_W0;
		case HEBS_WX:
			return HEBS_STATE_WX;
		case HEBS_SX:
		default:
			return HEBS_STATE_SX;

	}

}

static hebs_logic_t hebs_physical_to_logic(uint8_t state_3bn)
{
	static const hebs_logic_t PHYSICAL_TO_LOGIC_LUT[8] =
	{
		HEBS_Z,
		HEBS_S1,
		HEBS_S0,
		HEBS_X,
		HEBS_W1,
		HEBS_W0,
		HEBS_WX,
		HEBS_SX
	};
	return PHYSICAL_TO_LOGIC_LUT[state_3bn & 0x7U];

}

static uint8_t hebs_physical_to_pstate(uint8_t state_3bn)
{
	switch (state_3bn & 0x7U)
	{
		case HEBS_STATE_S1:
		case HEBS_STATE_W1:
			return 0x1U;
		case HEBS_STATE_X:
		case HEBS_STATE_WX:
		case HEBS_STATE_SX:
			return 0x2U;
		case HEBS_STATE_Z:
		case HEBS_STATE_S0:
		case HEBS_STATE_W0:
		default:
			return 0x0U;

	}

}

static uint32_t hebs_crc32_bytes(const uint8_t* data, size_t len)
{
	uint32_t crc;
	size_t idx;
	int bit;

	if (!data)
	{
		return 0U;

	}

	crc = 0xFFFFFFFFU;
	for (idx = 0U; idx < len; ++idx)
	{
		crc ^= (uint32_t)data[idx];
		for (bit = 0; bit < 8; ++bit)
		{
			const uint32_t mask = (uint32_t)(-(int)(crc & 1U));
			crc = (crc >> 1U) ^ (0xEDB88320U & mask);

		}

	}

	return ~crc;

}

static void hebs_execute_and_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		out_l = (uint8_t)(a_l & b_l);
		out_x = (uint8_t)((a_x | b_x) & (a_x | a_l) & (b_x | b_l));
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_or_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		out_l = (uint8_t)(a_l | b_l);
		out_x = (uint8_t)((a_x | b_x) & (uint8_t)(a_l ^ 1U) & (uint8_t)(b_l ^ 1U));
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_xor_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		out_l = (uint8_t)(a_l ^ b_l);
		out_x = (uint8_t)(a_x | b_x);
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_nand_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t tmp_l;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		tmp_l = (uint8_t)(a_l & b_l);
		out_l = (uint8_t)(tmp_l ^ 1U);
		out_x = (uint8_t)((a_x | b_x) & (a_x | a_l) & (b_x | b_l));
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_nor_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t tmp_l;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		tmp_l = (uint8_t)(a_l | b_l);
		out_l = (uint8_t)(tmp_l ^ 1U);
		out_x = (uint8_t)((a_x | b_x) & (uint8_t)(a_l ^ 1U) & (uint8_t)(b_l ^ 1U));
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_not_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		uint64_t src_a_tray;
		uint8_t src_a_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		out_l = (uint8_t)(a_l ^ 1U);
		out_x = a_x;
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_buf_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		uint64_t src_a_tray;
		uint8_t src_a_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		out_l = a_l;
		out_x = a_x;
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_xnor_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t tmp_l;
		uint8_t out_l;
		uint8_t out_x;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		tmp_l = (uint8_t)(a_l ^ b_l);
		out_l = (uint8_t)(tmp_l ^ 1U);
		out_x = (uint8_t)(a_x | b_x);
		drive_nibble = hebs_make_drive_nibble(out_l, out_x, 1U);

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_vcc_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint8_t drive_nibble = 0x8U;

		if (dst_net_id >= ctx->net_count)
		{
			continue;

		}

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_gnd_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint8_t drive_nibble = 0x4U;

		if (dst_net_id >= ctx->net_count)
		{
			continue;

		}

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_execute_pup_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		uint64_t src_a_tray;
		uint8_t src_a_pstate;
		uint8_t a_l;
		uint8_t a_x;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);

		{
			const uint8_t src_is_z = (uint8_t)(ctx->net_physical[src_a_net_id] == HEBS_STATE_Z);
			const uint8_t out_x = a_x;
			const uint8_t out_l = (uint8_t)((a_l | src_is_z) & (uint8_t)(a_x ^ 1U));
			const uint8_t drive_nibble = hebs_make_drive_nibble(out_l, out_x, 0U);
			hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

		}

	}

}

static void hebs_execute_pdn_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		uint64_t src_a_tray;
		uint8_t src_a_pstate;
		uint8_t a_l;
		uint8_t a_x;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);

		{
			const uint8_t src_is_z = (uint8_t)(ctx->net_physical[src_a_net_id] == HEBS_STATE_Z);
			const uint8_t out_x = a_x;
			const uint8_t out_l = (uint8_t)((a_l & (uint8_t)(src_is_z ^ 1U)) & (uint8_t)(a_x ^ 1U));
			const uint8_t drive_nibble = hebs_make_drive_nibble(out_l, out_x, 0U);
			hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

		}

	}

}

static void hebs_execute_tri_span(
	hebs_engine* ctx,
	const hebs_exec_instruction_t* exec_base,
	uint32_t count)
{
	uint32_t local_idx;

	for (local_idx = 0U; local_idx < count; ++local_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &exec_base[local_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		const uint32_t src_a_net_id = hebs_net_id_from_tray_shift(exec_instr->src_a_tray, exec_instr->src_a_shift);
		const uint32_t src_b_net_id = hebs_net_id_from_tray_shift(exec_instr->src_b_tray, exec_instr->src_b_shift);
		uint64_t src_a_tray;
		uint64_t src_b_tray;
		uint8_t src_a_pstate;
		uint8_t src_b_pstate;
		uint8_t a_l;
		uint8_t a_x;
		uint8_t b_l;
		uint8_t b_x;
		uint8_t en_valid;
		uint8_t en_high;
		uint8_t en_x;
		uint8_t m_high;
		uint8_t m_x;
		uint8_t data_drive;
		const uint8_t x_drive = 0xCU;
		uint8_t drive_nibble;

		if (dst_net_id >= ctx->net_count || src_a_net_id >= ctx->net_count || src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		src_b_tray = (exec_instr->src_b_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_b_tray] : 0ULL;
		src_a_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		src_b_pstate = hebs_read_pstate_from_word(src_b_tray, exec_instr->src_b_shift);
		a_l = (uint8_t)(src_a_pstate & 1U);
		a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);
		b_l = (uint8_t)(src_b_pstate & 1U);
		b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);
		en_valid = (uint8_t)(b_x ^ 1U);
		en_high = (uint8_t)(en_valid & b_l);
		en_x = b_x;
		m_high = (uint8_t)(0U - en_high);
		m_x = (uint8_t)(0U - en_x);
		data_drive = hebs_make_drive_nibble(a_l, a_x, 1U);
		drive_nibble = (uint8_t)((m_high & data_drive) | (m_x & x_drive));

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_phase_evaluate_batched(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t span_idx;

	for (span_idx = 0U; span_idx < plan->comb_span_count; ++span_idx)
	{
		const hebs_gate_span_t* const span = &plan->comb_spans[span_idx];
		const hebs_exec_instruction_t* const exec_base = &plan->comb_exec_data[span->start];

		ctx->probe_chunk_exec += 1U;
		ctx->probe_gate_eval += (uint64_t)span->count;

		switch ((hebs_gate_type_t)span->gate_type)
		{
			case HEBS_GATE_AND:
				hebs_execute_and_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_OR:
				hebs_execute_or_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_XOR:
				hebs_execute_xor_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_XNOR:
				hebs_execute_xnor_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_NAND:
				hebs_execute_nand_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_NOR:
				hebs_execute_nor_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_NOT:
				hebs_execute_not_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_BUF:
				hebs_execute_buf_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_VCC:
				hebs_execute_vcc_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_GND:
				hebs_execute_gnd_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_PUP:
				hebs_execute_pup_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_PDN:
				hebs_execute_pdn_span(ctx, exec_base, span->count);
				break;
			case HEBS_GATE_TRI:
				hebs_execute_tri_span(ctx, exec_base, span->count);
				break;
			default:
				break;

		}

	}

}

static void hebs_phase_evaluate_fallback(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;

	for (instr_idx = 0U; instr_idx < plan->gate_count; ++instr_idx)
	{
		const hebs_lep_instruction_t* const instr = &plan->lep_data[instr_idx];
		const hebs_gate_type_t gate_type = (hebs_gate_type_t)instr->gate_type;
		const uint32_t dst_net_id = instr->dst_bit_offset >> 1U;
		const uint32_t src_a_net_id = instr->src_a_bit_offset >> 1U;
		const uint32_t src_b_net_id = instr->src_b_bit_offset >> 1U;
		uint8_t src_a_pstate = 0U;
		uint8_t src_b_pstate = 0U;
		uint8_t a_l = 0U;
		uint8_t a_x = 0U;
		uint8_t b_l = 0U;
		uint8_t b_x = 0U;
		uint8_t drive_nibble = 0xCU;

		if (gate_type == HEBS_GATE_DFF)
		{
			continue;

		}

		if (dst_net_id >= ctx->net_count)
		{
			continue;

		}

		if (instr->input_count > 0U && src_a_net_id >= ctx->net_count)
		{
			continue;

		}

		if (instr->input_count > 1U && src_b_net_id >= ctx->net_count)
		{
			continue;

		}

		if (instr->input_count > 0U)
		{
			src_a_pstate = hebs_read_pstate_net(ctx, src_a_net_id);
			a_l = (uint8_t)(src_a_pstate & 1U);
			a_x = (uint8_t)((src_a_pstate >> 1U) & 1U);

		}

		if (instr->input_count > 1U)
		{
			src_b_pstate = hebs_read_pstate_net(ctx, src_b_net_id);
			b_l = (uint8_t)(src_b_pstate & 1U);
			b_x = (uint8_t)((src_b_pstate >> 1U) & 1U);

		}

		ctx->probe_chunk_exec += 1U;
		ctx->probe_gate_eval += 1U;
		switch (gate_type)
		{
			case HEBS_GATE_AND:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)(a_l & b_l),
					(uint8_t)((a_x | b_x) & (a_x | a_l) & (b_x | b_l)),
					1U);
				break;
			case HEBS_GATE_OR:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)(a_l | b_l),
					(uint8_t)((a_x | b_x) & (uint8_t)(a_l ^ 1U) & (uint8_t)(b_l ^ 1U)),
					1U);
				break;
			case HEBS_GATE_XOR:
				drive_nibble = hebs_make_drive_nibble((uint8_t)(a_l ^ b_l), (uint8_t)(a_x | b_x), 1U);
				break;
			case HEBS_GATE_NOT:
				drive_nibble = hebs_make_drive_nibble((uint8_t)(a_l ^ 1U), a_x, 1U);
				break;
			case HEBS_GATE_NAND:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)((a_l & b_l) ^ 1U),
					(uint8_t)((a_x | b_x) & (a_x | a_l) & (b_x | b_l)),
					1U);
				break;
			case HEBS_GATE_NOR:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)((a_l | b_l) ^ 1U),
					(uint8_t)((a_x | b_x) & (uint8_t)(a_l ^ 1U) & (uint8_t)(b_l ^ 1U)),
					1U);
				break;
			case HEBS_GATE_XNOR:
				drive_nibble = hebs_make_drive_nibble((uint8_t)((a_l ^ b_l) ^ 1U), (uint8_t)(a_x | b_x), 1U);
				break;
			case HEBS_GATE_BUF:
				drive_nibble = hebs_make_drive_nibble(a_l, a_x, 1U);
				break;
			case HEBS_GATE_TRI:
			{
				const uint8_t en_valid = (uint8_t)(b_x ^ 1U);
				const uint8_t en_high = (uint8_t)(en_valid & b_l);
				const uint8_t en_x = b_x;
				const uint8_t m_high = (uint8_t)(0U - en_high);
				const uint8_t m_x = (uint8_t)(0U - en_x);
				const uint8_t data_drive = hebs_make_drive_nibble(a_l, a_x, 1U);
				drive_nibble = (uint8_t)((m_high & data_drive) | (m_x & 0xCU));
				break;
			}
			case HEBS_GATE_VCC:
				drive_nibble = 0x8U;
				break;
			case HEBS_GATE_GND:
				drive_nibble = 0x4U;
				break;
			case HEBS_GATE_PUP:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)((a_l | (uint8_t)(ctx->net_physical[src_a_net_id] == HEBS_STATE_Z)) & (uint8_t)(a_x ^ 1U)),
					a_x,
					0U);
				break;
			case HEBS_GATE_PDN:
				drive_nibble = hebs_make_drive_nibble(
					(uint8_t)((a_l & (uint8_t)(((uint8_t)(ctx->net_physical[src_a_net_id] == HEBS_STATE_Z)) ^ 1U)) & (uint8_t)(a_x ^ 1U)),
					a_x,
					0U);
				break;
			default:
				drive_nibble = 0xCU;
				break;

		}

		hebs_mailbox_or(ctx, dst_net_id, drive_nibble);

	}

}

static void hebs_phase_evaluate(hebs_engine* ctx, const hebs_plan* plan)
{
	if (plan->comb_exec_data && plan->comb_spans && plan->comb_span_count > 0U)
	{
		hebs_phase_evaluate_batched(ctx, plan);
		return;

	}

	hebs_phase_evaluate_fallback(ctx, plan);

}

static void hebs_phase_resolve(hebs_engine* ctx)
{
	uint32_t dirty_idx;
	uint8_t* const net_mailbox = ctx->net_mailbox;
	uint8_t* const net_physical = ctx->net_physical;

	for (dirty_idx = 0U; dirty_idx < ctx->dirty_count; ++dirty_idx)
	{
		const uint32_t net_id = ctx->dirty_net_ids[dirty_idx];
		const uint8_t old_mailbox = net_mailbox[net_id];
		const uint8_t lut_value = HEBS_FUSED_LUT[old_mailbox];
		const uint8_t resolved_3bn = (uint8_t)(lut_value & 0x07U);
		const uint8_t next_pstate = (uint8_t)((lut_value >> 5U) & 0x03U);
#if HEBS_TEST_PROBES
		const uint8_t old_value = net_physical[net_id];
#endif

		net_physical[net_id] = resolved_3bn;
		hebs_write_pstate_net(ctx, net_id, next_pstate);
		net_mailbox[net_id] = 0U;
#if HEBS_TEST_PROBES
		ctx->probe_state_change_commit += (uint64_t)(old_value != resolved_3bn);
#endif

	}

	ctx->dirty_count = 0U;

}

static void hebs_phase_commit_dff(hebs_engine* ctx, const hebs_plan* plan)
{
	uint32_t instr_idx;

	if (!plan->dff_exec_data || plan->dff_exec_count == 0U)
	{
		return;

	}

	for (instr_idx = 0U; instr_idx < plan->dff_exec_count; ++instr_idx)
	{
		const hebs_exec_instruction_t* const exec_instr = &plan->dff_exec_data[instr_idx];
		const uint32_t dst_net_id = hebs_net_id_from_tray_shift(exec_instr->dst_tray, exec_instr->dst_shift);
		uint64_t src_a_tray;
		uint8_t data_pstate;
		uint8_t data_l;
		uint8_t data_x;
		uint8_t capture_x;
		uint8_t next_l;
		uint8_t next_pstate;

		ctx->probe_dff_exec += 1U;

		if (exec_instr->dst_tray >= ctx->tray_count || dst_net_id >= ctx->net_count)
		{
			continue;

		}

		src_a_tray = (exec_instr->src_a_tray < ctx->tray_count) ? ctx->signal_trays[exec_instr->src_a_tray] : 0ULL;
		data_pstate = hebs_read_pstate_from_word(src_a_tray, exec_instr->src_a_shift);
		data_l = (uint8_t)(data_pstate & 1U);
		data_x = (uint8_t)((data_pstate >> 1U) & 1U);
		capture_x = data_x;
		next_l = (uint8_t)(data_l & (uint8_t)(capture_x ^ 1U));
		next_pstate = (uint8_t)(next_l | (uint8_t)(capture_x << 1U));

		hebs_write_pstate_to_trays(ctx->dff_state_trays, dst_net_id, next_pstate);
		hebs_mailbox_or(ctx, dst_net_id, hebs_make_drive_nibble(next_l, capture_x, 1U));

	}

}

hebs_status_t hebs_init_engine(hebs_engine* ctx, hebs_plan* plan)
{
	uint32_t net_id;

	if (!ctx || !plan)
	{
		if (ctx)
		{
			ctx->last_status = HEBS_ERR_LOGIC;

		}
		return HEBS_ERR_LOGIC;

	}

	if (plan->tray_count > HEBS_MAX_SIGNAL_TRAYS ||
		plan->signal_count > HEBS_MAX_SIGNALS ||
		plan->num_primary_inputs > HEBS_MAX_PRIMARY_INPUTS)
	{
		ctx->last_status = HEBS_ERR_LOGIC;
		return HEBS_ERR_LOGIC;

	}

	ctx->current_tick = 0U;
	ctx->tray_count = plan->tray_count;
	ctx->net_count = plan->signal_count;
	ctx->dirty_count = 0U;
	ctx->cycles_executed = 0U;
	ctx->vectors_applied = 0U;
	ctx->probe_input_apply = 0U;
	ctx->probe_input_toggle = 0U;
	ctx->probe_chunk_exec = 0U;
	ctx->probe_gate_eval = 0U;
	ctx->probe_state_change_commit = 0U;
	ctx->probe_dff_exec = 0U;
	memset(ctx->tray_plane_a, 0, sizeof(ctx->tray_plane_a));
	memset(ctx->tray_plane_b, 0, sizeof(ctx->tray_plane_b));
	memset(ctx->dff_state_trays, 0, sizeof(ctx->dff_state_trays));
	memset(ctx->net_physical, 0, sizeof(ctx->net_physical));
	memset(ctx->net_mailbox, 0, sizeof(ctx->net_mailbox));
	memset(ctx->dirty_net_ids, 0, sizeof(ctx->dirty_net_ids));
	memset(ctx->dirty_net_flags, 0, sizeof(ctx->dirty_net_flags));
	ctx->signal_trays = ctx->tray_plane_a;
	ctx->next_signal_trays = ctx->tray_plane_a;

	for (net_id = 0U; net_id < ctx->net_count; ++net_id)
	{
		ctx->net_physical[net_id] = HEBS_STATE_Z;
		hebs_write_pstate_net(ctx, net_id, 0x0U);

	}

	ctx->last_status = HEBS_OK;
	return HEBS_OK;

}

void hebs_tick(hebs_engine* ctx, hebs_plan* plan)
{
	if (!ctx || !plan || !ctx->signal_trays)
	{
		if (ctx)
		{
			ctx->last_status = HEBS_ERR_LOGIC;

		}
		return;

	}

	hebs_phase_evaluate(ctx, plan);
	hebs_phase_resolve(ctx);
	hebs_phase_commit_dff(ctx, plan);
	ctx->cycles_executed += 1U;
	ctx->vectors_applied += 1U;
	ctx->current_tick += 1U;
	ctx->last_status = HEBS_OK;

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
	probes.chunk_exec = ctx->probe_chunk_exec;
	probes.gate_eval = ctx->probe_gate_eval;
	probes.dff_exec = ctx->probe_dff_exec;
#if HEBS_TEST_PROBES
	probes.input_toggle = ctx->probe_input_toggle;
	probes.state_change_commit = ctx->probe_state_change_commit;
#endif
	return probes;

}

int hebs_get_run_status(const hebs_engine* ctx, hebs_run_status* out_status)
{
	if (!ctx || !out_status)
	{
		return 0;

	}

	out_status->last_status = ctx->last_status;
	out_status->current_tick = ctx->current_tick;
	out_status->cycles_executed = ctx->cycles_executed;
	out_status->vectors_applied = ctx->vectors_applied;
	out_status->tray_count = ctx->tray_count;
	out_status->test_probes_enabled = (uint8_t)HEBS_TEST_PROBES;
	return 1;

}

uint64_t hebs_get_state_hash(hebs_engine* ctx)
{
	uint64_t hash;
	uint32_t net_idx;

	if (!ctx)
	{
		return 0U;

	}

	hash = 1469598103934665603ULL;
	hash ^= ctx->current_tick;
	hash *= 1099511628211ULL;

	for (net_idx = 0U; net_idx < ctx->net_count; ++net_idx)
	{
		hash ^= (uint64_t)ctx->net_physical[net_idx];
		hash *= 1099511628211ULL;

	}

	return hash;

}

uint32_t hebs_get_final_crc32(const hebs_engine* ctx)
{
	if (!ctx || ctx->net_count == 0U)
	{
		return 0U;

	}

	return hebs_crc32_bytes(ctx->net_physical, (size_t)ctx->net_count);

}

uint64_t hebs_get_plan_hash(const hebs_plan* plan)
{
	if (!plan)
	{
		return 0U;

	}

	return plan->lep_hash;

}

int hebs_get_plan_metadata(const hebs_plan* plan, hebs_plan_metadata* out_metadata)
{
	if (!plan || !out_metadata)
	{
		return 0;

	}

	memset(out_metadata, 0, sizeof(*out_metadata));
	out_metadata->plan_hash = plan->lep_hash;
	out_metadata->level_count = plan->level_count;
	out_metadata->num_primary_inputs = plan->num_primary_inputs;
	out_metadata->num_primary_outputs = plan->num_primary_outputs;
	out_metadata->signal_count = plan->signal_count;
	out_metadata->gate_count = plan->gate_count;
	out_metadata->tray_count = plan->tray_count;
	out_metadata->propagation_depth = plan->propagation_depth;
	out_metadata->fanout_max = plan->fanout_max;
	out_metadata->total_fanout_edges = plan->total_fanout_edges;
	out_metadata->fanout_avg = plan->fanout_avg;
	out_metadata->dff_exec_count = plan->dff_exec_count;
	out_metadata->comb_instruction_count = plan->comb_instruction_count;
	return 1;

}

hebs_status_t hebs_set_primary_input(hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index, hebs_logic_t value)
{
	uint32_t signal_id;
	uint8_t physical_value;
	uint8_t pstate_value;

	if (!ctx || !plan || input_index >= plan->num_primary_inputs)
	{
		if (ctx)
		{
			ctx->last_status = HEBS_ERR_LOGIC;

		}
		return HEBS_ERR_LOGIC;

	}

	if (value > HEBS_WX)
	{
		ctx->last_status = HEBS_ERR_LOGIC;
		return HEBS_ERR_LOGIC;

	}

	signal_id = plan->primary_input_ids[input_index];
	if (signal_id >= ctx->net_count)
	{
		ctx->last_status = HEBS_ERR_LOGIC;
		return HEBS_ERR_LOGIC;

	}

	physical_value = hebs_logic_to_physical(value);
	pstate_value = hebs_physical_to_pstate(physical_value);
	ctx->probe_input_apply += 1U;
#if HEBS_TEST_PROBES
	ctx->probe_input_toggle += (uint64_t)(ctx->net_physical[signal_id] != physical_value);
#endif
	ctx->net_physical[signal_id] = physical_value;
	hebs_write_pstate_net(ctx, signal_id, pstate_value);
	ctx->last_status = HEBS_OK;
	return HEBS_OK;

}

uint8_t hebs_get_primary_input_state(const hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index)
{
	uint32_t signal_id;

	if (!ctx || !plan || input_index >= plan->num_primary_inputs)
	{
		return 0xFFU;

	}

	signal_id = plan->primary_input_ids[input_index];
	if (signal_id >= ctx->net_count)
	{
		return 0xFFU;

	}

	return (uint8_t)(ctx->net_physical[signal_id] & 0x7U);

}

hebs_logic_t hebs_get_primary_input(const hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index)
{
	const uint8_t physical_value = hebs_get_primary_input_state(ctx, plan, input_index);
	if (physical_value == 0xFFU)
	{
		return HEBS_X;

	}

	return hebs_physical_to_logic(physical_value);

}

const uint64_t* hebs_get_signal_tray(const hebs_engine* ctx, uint32_t tray_index)
{
	if (!ctx || !ctx->signal_trays || tray_index >= ctx->tray_count)
	{
		return NULL;

	}

	return &ctx->signal_trays[tray_index];

}
