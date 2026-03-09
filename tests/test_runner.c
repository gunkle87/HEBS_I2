#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "hebs_engine.h"
#include "primitives.h"
#include "../benchmarks/protocol_helper.h"

static hebs_logic_t read_signal_state(const hebs_engine* engine, uint32_t signal_id)
{
	const uint32_t tray_index = signal_id / 32U;
	const uint32_t bit_position = (signal_id % 32U) * 2U;
	const uint64_t* tray_ptr = hebs_get_signal_tray(engine, tray_index);
	if (!tray_ptr)
	{
		return (hebs_logic_t)0U;

	}

	return (hebs_logic_t)((*tray_ptr >> bit_position) & 0x3U);

}

static uint8_t project_logic_to_pstate(hebs_logic_t value)
{
	switch (value)
	{
		case HEBS_S1:
		case HEBS_W1:
			return 0x1U;
		case HEBS_S0:
		case HEBS_W0:
		case HEBS_Z:
			return 0x0U;
		case HEBS_SX:
		case HEBS_WX:
		case HEBS_X:
		default:
			return 0x2U;

	}

}

static uint8_t eval_and_from_pstate(uint8_t a_pstate, uint8_t b_pstate)
{
	const uint8_t a_l = (uint8_t)(a_pstate & 0x1U);
	const uint8_t b_l = (uint8_t)(b_pstate & 0x1U);
	const uint8_t a_x = (uint8_t)((a_pstate >> 1U) & 0x1U);
	const uint8_t b_x = (uint8_t)((b_pstate >> 1U) & 0x1U);
	const uint8_t out_l = (uint8_t)(a_l & b_l);
	const uint8_t out_x = (uint8_t)((a_x | b_x) & (a_x | a_l) & (b_x | b_l));
	return (uint8_t)((out_x << 1U) | out_l);

}

static uint8_t eval_or_from_pstate(uint8_t a_pstate, uint8_t b_pstate)
{
	const uint8_t a_l = (uint8_t)(a_pstate & 0x1U);
	const uint8_t b_l = (uint8_t)(b_pstate & 0x1U);
	const uint8_t a_x = (uint8_t)((a_pstate >> 1U) & 0x1U);
	const uint8_t b_x = (uint8_t)((b_pstate >> 1U) & 0x1U);
	const uint8_t out_l = (uint8_t)(a_l | b_l);
	const uint8_t out_x = (uint8_t)((a_x | b_x) & (a_l ^ 0x1U) & (b_l ^ 0x1U));
	return (uint8_t)((out_x << 1U) | out_l);

}

static uint32_t net_to_bit(uint32_t net_id)
{
	return net_id * 2U;

}

static uint8_t read_net_pstate(const hebs_engine* engine, uint32_t net_id)
{
	return (uint8_t)read_signal_state(engine, net_id);

}

static uint8_t read_net_logic(const hebs_engine* engine, uint32_t net_id)
{
	return (uint8_t)(read_net_pstate(engine, net_id) & 0x1U);

}

static uint8_t read_net_xflag(const hebs_engine* engine, uint32_t net_id)
{
	return (uint8_t)((read_net_pstate(engine, net_id) >> 1U) & 0x1U);

}

static hebs_logic_t read_net_3bn(const hebs_engine* engine, uint32_t net_id)
{
	hebs_logic_t value = HEBS_X;
	assert(hebs_get_net_physical_state(engine, net_id, &value) == 1);
	return value;

}

typedef enum hebs_proof_drive_kind_e
{
	HEBS_PROOF_DRIVE_NONE = 0,
	HEBS_PROOF_DRIVE_S1 = 1,
	HEBS_PROOF_DRIVE_S0 = 2,
	HEBS_PROOF_DRIVE_SX = 3,
	HEBS_PROOF_DRIVE_W1 = 4,
	HEBS_PROOF_DRIVE_W0 = 5,
	HEBS_PROOF_DRIVE_WX = 6,
	HEBS_PROOF_DRIVE_KIND_COUNT = 7

} hebs_proof_drive_kind_t;

#define HEBS_PROOF_STATE_Z 0U
#define HEBS_PROOF_STATE_S1 1U
#define HEBS_PROOF_STATE_S0 2U
#define HEBS_PROOF_STATE_X 3U
#define HEBS_PROOF_STATE_W1 4U
#define HEBS_PROOF_STATE_W0 5U
#define HEBS_PROOF_STATE_WX 6U
#define HEBS_PROOF_STATE_SX 7U

static const uint8_t HEBS_PROOF_FUSED_LUT[16] =
{
	0x00U, 0x05U, 0x24U, 0x46U,
	0x02U, 0x02U, 0x02U, 0x02U,
	0x21U, 0x21U, 0x21U, 0x21U,
	0x47U, 0x47U, 0x47U, 0x47U
};

static uint8_t hebs_proof_drive_to_nibble(hebs_proof_drive_kind_t drive_kind)
{
	switch (drive_kind)
	{
		case HEBS_PROOF_DRIVE_NONE:
			return 0U;
		case HEBS_PROOF_DRIVE_S1:
			return 0x8U;
		case HEBS_PROOF_DRIVE_S0:
			return 0x4U;
		case HEBS_PROOF_DRIVE_SX:
			return 0xCU;
		case HEBS_PROOF_DRIVE_W1:
			return 0x2U;
		case HEBS_PROOF_DRIVE_W0:
			return 0x1U;
		case HEBS_PROOF_DRIVE_WX:
		default:
			return 0x3U;

	}

}

static uint8_t hebs_proof_resolve_mailbox_state(uint8_t mailbox)
{
	return (uint8_t)(HEBS_PROOF_FUSED_LUT[mailbox & 0x0FU] & 0x07U);

}

static uint8_t hebs_proof_build_accum_lut(uint8_t accum_lut[8][HEBS_PROOF_DRIVE_KIND_COUNT])
{
	uint8_t pending_state;
	uint8_t saw_non_materialized_state = 0U;

	for (pending_state = 0U; pending_state < 8U; ++pending_state)
	{
		uint8_t drive_kind;
		uint8_t representative_count = 0U;

		for (drive_kind = 0U; drive_kind < HEBS_PROOF_DRIVE_KIND_COUNT; ++drive_kind)
		{
			const uint8_t drive_nibble = hebs_proof_drive_to_nibble((hebs_proof_drive_kind_t)drive_kind);
			uint8_t mailbox;
			uint8_t saw_candidate = 0U;
			uint8_t next_state = 0xFFU;

			for (mailbox = 0U; mailbox < 16U; ++mailbox)
			{
				if (hebs_proof_resolve_mailbox_state(mailbox) != pending_state)
				{
					continue;

				}

				representative_count += (uint8_t)(drive_kind == 0U);
				if (saw_candidate == 0U)
				{
					next_state = hebs_proof_resolve_mailbox_state((uint8_t)(mailbox | drive_nibble));
					saw_candidate = 1U;

				}
				else
				{
					assert(next_state == hebs_proof_resolve_mailbox_state((uint8_t)(mailbox | drive_nibble)));

				}

			}

			if (saw_candidate != 0U)
			{
				accum_lut[pending_state][drive_kind] = next_state;
				continue;

			}

			/* HEBS_STATE_X is externally seedable, but the fused LUT never materializes it. */
			assert(pending_state == HEBS_PROOF_STATE_X);
			saw_non_materialized_state = 1U;
			accum_lut[pending_state][drive_kind] = (drive_nibble == 0U) ? HEBS_PROOF_STATE_X : hebs_proof_resolve_mailbox_state(drive_nibble);

		}

		if (pending_state == HEBS_PROOF_STATE_X)
		{
			assert(representative_count == 0U);

		}
		else
		{
			assert(representative_count > 0U);

		}

	}

	return saw_non_materialized_state;

}

static uint8_t hebs_proof_fold_candidate(
	const uint8_t accum_lut[8][HEBS_PROOF_DRIVE_KIND_COUNT],
	uint8_t initial_state,
	const hebs_proof_drive_kind_t* drives,
	uint32_t drive_count)
{
	uint32_t idx;
	uint8_t state = initial_state;

	for (idx = 0U; idx < drive_count; ++idx)
	{
		state = accum_lut[state][drives[idx]];

	}

	return state;

}

static uint8_t hebs_proof_fold_mailbox(const hebs_proof_drive_kind_t* drives, uint32_t drive_count)
{
	uint32_t idx;
	uint8_t mailbox = 0U;

	for (idx = 0U; idx < drive_count; ++idx)
	{
		mailbox = (uint8_t)(mailbox | hebs_proof_drive_to_nibble(drives[idx]));

	}

	return hebs_proof_resolve_mailbox_state(mailbox);

}

static void hebs_proof_expect_sequence_match(
	const uint8_t accum_lut[8][HEBS_PROOF_DRIVE_KIND_COUNT],
	const hebs_proof_drive_kind_t* drives,
	uint32_t drive_count)
{
	assert(hebs_proof_fold_candidate(accum_lut, HEBS_PROOF_STATE_Z, drives, drive_count) == hebs_proof_fold_mailbox(drives, drive_count));

}

static hebs_plan* load_temp_bench_plan(const char* path, const char* bench_text)
{
	FILE* file = fopen(path, "wb");
	hebs_plan* plan;

	assert(file != NULL);
	assert(fputs(bench_text, file) >= 0);
	assert(fclose(file) == 0);
	plan = hebs_load_bench(path);
	remove(path);
	return plan;

}

static hebs_plan* load_single_input_dff_test_plan(const char* path)
{
	static const char* bench_text =
		"INPUT(contam)\n"
		"INPUT(data)\n"
		"OUTPUT(q)\n"
		"q = DFF(data)\n";
	return load_temp_bench_plan(path, bench_text);

}

static void test_phase3_strength_precedence_strong_over_weak(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_GND;
	lep_data[0].input_count = 0U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(0U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[1].input_count = 1U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(2U);

	plan.lep_hash = 1001U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_Z) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 2U) == 0U);
	assert(read_net_xflag(&engine, 2U) == 0U);

}

static void test_and_or_dominance_x_suppression(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_AND;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(1U);
	lep_data[1].dst_bit_offset = net_to_bit(3U);

	plan.lep_hash = 1002U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 4U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_SX) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 2U) == 0U);
	assert(read_net_xflag(&engine, 2U) == 0U);
	assert(read_net_xflag(&engine, 3U) == 1U);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_SX) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 3U) == 1U);
	assert(read_net_xflag(&engine, 3U) == 0U);
	assert(read_net_xflag(&engine, 2U) == 1U);

}

static void test_and_nand_x_dominance(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_AND;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_NAND;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(1U);
	lep_data[1].dst_bit_offset = net_to_bit(3U);

	plan.lep_hash = 1010U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 4U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_SX) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 2U) == 0U);
	assert(read_net_xflag(&engine, 2U) == 1U);
	assert(read_net_logic(&engine, 3U) == 0U);
	assert(read_net_xflag(&engine, 3U) == 1U);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_SX) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 2U) == 0U);
	assert(read_net_xflag(&engine, 2U) == 0U);
	assert(read_net_logic(&engine, 3U) == 1U);
	assert(read_net_xflag(&engine, 3U) == 0U);

}

static void test_or_nor_x_dominance(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_NOR;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(1U);
	lep_data[1].dst_bit_offset = net_to_bit(3U);

	plan.lep_hash = 1011U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 4U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_SX) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_logic(&engine, 2U) == 1U);
	assert(read_net_xflag(&engine, 2U) == 0U);
	assert(read_net_logic(&engine, 3U) == 0U);
	assert(read_net_xflag(&engine, 3U) == 0U);

}

static void test_tristate_disabled_is_zero_drive(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_TRI;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(3U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[1].input_count = 0U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(3U);

	plan.lep_hash = 1003U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 4U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 3U) == 0x1U);
	assert(read_net_logic(&engine, 3U) == 1U);
	assert(read_net_xflag(&engine, 3U) == 0U);

}

static void test_multi_driver_phase1_or_not_overwrite(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_GND;
	lep_data[0].input_count = 0U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(0U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[1].input_count = 0U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(2U);

	plan.lep_hash = 1004U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 2U) == 0x2U);
	assert(read_net_xflag(&engine, 2U) == 1U);

}

static void test_duplicate_strong_drive_is_idempotent(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[2];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[0].input_count = 0U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(0U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[1].input_count = 0U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(2U);

	plan.lep_hash = 10041U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 2U) == 0x1U);
	assert(read_net_logic(&engine, 2U) == 1U);
	assert(read_net_xflag(&engine, 2U) == 0U);

	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 2U) == 0x1U);
	assert(read_net_logic(&engine, 2U) == 1U);
	assert(read_net_xflag(&engine, 2U) == 0U);

}

static void test_full_net_contention_64plus(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[66];
	hebs_plan plan = { 0 };
	uint32_t gate_idx;
	uint32_t cycle;

	for (gate_idx = 0U; gate_idx < 66U; ++gate_idx)
	{
		hebs_gate_type_t gate_type = HEBS_GATE_GND;
		if ((gate_idx % 3U) == 1U)
		{
			gate_type = HEBS_GATE_VCC;

		}
		else if ((gate_idx % 3U) == 2U)
		{
			gate_type = HEBS_GATE_PUP;

		}
		lep_data[gate_idx].gate_type = (uint8_t)gate_type;
		lep_data[gate_idx].input_count = (gate_type == HEBS_GATE_PUP) ? 1U : 0U;
		lep_data[gate_idx].level = 1U;
		lep_data[gate_idx].src_a_bit_offset = net_to_bit(0U);
		lep_data[gate_idx].src_b_bit_offset = net_to_bit(0U);
		lep_data[gate_idx].dst_bit_offset = net_to_bit(2U);

	}

	plan.lep_hash = 1005U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.gate_count = 66U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_Z) == HEBS_OK);
	for (cycle = 0U; cycle < 5U; ++cycle)
	{
		hebs_tick(&engine, &plan);
		assert((uint8_t)read_signal_state(&engine, 2U) == 0x2U);
		assert(read_net_xflag(&engine, 2U) == 1U);

	}

}

static void test_dff_x_poison_chain_latency_10(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 11U };
	hebs_exec_instruction_t dff_exec_data[10];
	hebs_plan plan = { 0 };
	uint32_t idx;
	uint32_t step;

	for (idx = 0U; idx < 10U; ++idx)
	{
		const uint32_t src_net = idx;
		const uint32_t dst_net = idx + 1U;
		dff_exec_data[idx].gate_type = (uint8_t)HEBS_GATE_DFF;
		dff_exec_data[idx].src_a_shift = (uint8_t)(net_to_bit(src_net) % 64U);
		dff_exec_data[idx].src_b_shift = (uint8_t)(net_to_bit(11U) % 64U);
		dff_exec_data[idx].dst_shift = (uint8_t)(net_to_bit(dst_net) % 64U);
		dff_exec_data[idx].src_a_tray = net_to_bit(src_net) / 64U;
		dff_exec_data[idx].src_b_tray = net_to_bit(11U) / 64U;
		dff_exec_data[idx].dst_tray = net_to_bit(dst_net) / 64U;
	}

	plan.lep_hash = 1006U;
	plan.level_count = 1U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 12U;
	plan.gate_count = 0U;
	plan.tray_count = 1U;
	plan.max_level = 0U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = NULL;
	plan.dff_exec_count = 10U;
	plan.dff_exec_data = dff_exec_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_X) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);

	for (step = 1U; step <= 10U; ++step)
	{
		uint32_t stage;
		uint32_t x_count = 0U;
		hebs_tick(&engine, &plan);
		for (stage = 1U; stage <= 10U; ++stage)
		{
			const uint8_t is_x = read_net_xflag(&engine, stage);
			x_count += is_x;
			if (stage == step)
			{
				assert(is_x == 1U);

			}
			else
			{
				assert(is_x == 0U);

			}

		}
		assert(x_count == 1U);

	}

}

static void test_dirty_set_sparse_dense_saturation(void)
{
	{
		hebs_engine engine = { 0 };
		uint32_t primary_inputs[64];
		hebs_lep_instruction_t lep_data[4];
		hebs_plan plan = { 0 };
		uint32_t i;
		uint32_t sparse_idx;
		const uint32_t sparse_inputs[4] = { 0U, 17U, 34U, 51U };

		for (i = 0U; i < 64U; ++i)
		{
			primary_inputs[i] = i;

		}

		for (sparse_idx = 0U; sparse_idx < 4U; ++sparse_idx)
		{
			const uint32_t src = sparse_inputs[sparse_idx];
			lep_data[sparse_idx].gate_type = (uint8_t)HEBS_GATE_BUF;
			lep_data[sparse_idx].input_count = 1U;
			lep_data[sparse_idx].level = 1U;
			lep_data[sparse_idx].src_a_bit_offset = net_to_bit(src);
			lep_data[sparse_idx].src_b_bit_offset = net_to_bit(src);
			lep_data[sparse_idx].dst_bit_offset = net_to_bit(src + 64U);

		}

		plan.lep_hash = 1007U;
		plan.level_count = 2U;
		plan.num_primary_inputs = 64U;
		plan.signal_count = 128U;
		plan.gate_count = 4U;
		plan.tray_count = 4U;
		plan.max_level = 1U;
		plan.primary_input_ids = primary_inputs;
		plan.lep_data = lep_data;

		assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
		for (i = 0U; i < 64U; ++i)
		{
			const hebs_logic_t val = ((i & 1U) != 0U) ? HEBS_S1 : HEBS_S0;
			assert(hebs_set_primary_input(&engine, &plan, i, val) == HEBS_OK);

		}
		hebs_tick(&engine, &plan);
		for (i = 0U; i < 64U; ++i)
		{
			const uint32_t dst = i + 64U;
			const uint8_t expected_selected = (uint8_t)(i == 0U || i == 17U || i == 34U || i == 51U);
			if (expected_selected != 0U)
			{
				assert((uint8_t)read_signal_state(&engine, dst) == (((i & 1U) != 0U) ? 0x1U : 0x0U));

			}
			else
			{
				assert((uint8_t)read_signal_state(&engine, dst) == 0x0U);

			}

		}

	}

	{
		hebs_engine engine = { 0 };
		uint32_t primary_inputs[64];
		hebs_lep_instruction_t lep_data[64];
		hebs_plan plan = { 0 };
		uint32_t i;

		for (i = 0U; i < 64U; ++i)
		{
			primary_inputs[i] = i;
			lep_data[i].gate_type = (uint8_t)HEBS_GATE_BUF;
			lep_data[i].input_count = 1U;
			lep_data[i].level = 1U;
			lep_data[i].src_a_bit_offset = net_to_bit(i);
			lep_data[i].src_b_bit_offset = net_to_bit(i);
			lep_data[i].dst_bit_offset = net_to_bit(i + 64U);

		}

		plan.lep_hash = 1008U;
		plan.level_count = 2U;
		plan.num_primary_inputs = 64U;
		plan.signal_count = 128U;
		plan.gate_count = 64U;
		plan.tray_count = 4U;
		plan.max_level = 1U;
		plan.primary_input_ids = primary_inputs;
		plan.lep_data = lep_data;

		assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
		for (i = 0U; i < 64U; ++i)
		{
			const hebs_logic_t val = ((i & 1U) != 0U) ? HEBS_S1 : HEBS_S0;
			assert(hebs_set_primary_input(&engine, &plan, i, val) == HEBS_OK);

		}
		hebs_tick(&engine, &plan);
		for (i = 0U; i < 64U; ++i)
		{
			const uint32_t dst = i + 64U;
			assert((uint8_t)read_signal_state(&engine, dst) == (((i & 1U) != 0U) ? 0x1U : 0x0U));

		}

	}

}

static void test_cycle_stability_no_temporal_path(void)
{
	hebs_engine engine_a = { 0 };
	hebs_engine engine_b = { 0 };
	uint32_t primary_inputs[4] = { 0U, 1U, 2U, 3U };
	hebs_lep_instruction_t lep_data[4];
	hebs_plan plan = { 0 };
	uint32_t cycle;

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_AND;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(4U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(2U);
	lep_data[1].src_b_bit_offset = net_to_bit(3U);
	lep_data[1].dst_bit_offset = net_to_bit(5U);

	lep_data[2].gate_type = (uint8_t)HEBS_GATE_XOR;
	lep_data[2].input_count = 2U;
	lep_data[2].level = 1U;
	lep_data[2].src_a_bit_offset = net_to_bit(4U);
	lep_data[2].src_b_bit_offset = net_to_bit(5U);
	lep_data[2].dst_bit_offset = net_to_bit(6U);

	lep_data[3].gate_type = (uint8_t)HEBS_GATE_NOT;
	lep_data[3].input_count = 1U;
	lep_data[3].level = 1U;
	lep_data[3].src_a_bit_offset = net_to_bit(6U);
	lep_data[3].src_b_bit_offset = net_to_bit(6U);
	lep_data[3].dst_bit_offset = net_to_bit(7U);

	plan.lep_hash = 1009U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 4U;
	plan.signal_count = 8U;
	plan.gate_count = 4U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine_a, &plan) == HEBS_OK);
	assert(hebs_init_engine(&engine_b, &plan) == HEBS_OK);

	for (cycle = 0U; cycle < 20U; ++cycle)
	{
		const hebs_logic_t in0 = ((cycle & 1U) != 0U) ? HEBS_S1 : HEBS_S0;
		const hebs_logic_t in1 = ((cycle & 2U) != 0U) ? HEBS_SX : HEBS_S0;
		const hebs_logic_t in2 = ((cycle & 4U) != 0U) ? HEBS_S1 : HEBS_S0;
		const hebs_logic_t in3 = ((cycle & 8U) != 0U) ? HEBS_SX : HEBS_S1;
		uint32_t net_id;

		assert(hebs_set_primary_input(&engine_a, &plan, 0U, in0) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_a, &plan, 1U, in1) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_a, &plan, 2U, in2) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_a, &plan, 3U, in3) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_b, &plan, 0U, in0) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_b, &plan, 1U, in1) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_b, &plan, 2U, in2) == HEBS_OK);
		assert(hebs_set_primary_input(&engine_b, &plan, 3U, in3) == HEBS_OK);

		hebs_tick(&engine_a, &plan);
		hebs_tick(&engine_b, &plan);
		assert(hebs_get_state_hash(&engine_a) == hebs_get_state_hash(&engine_b));
		assert(hebs_get_final_crc32(&engine_a) == hebs_get_final_crc32(&engine_b));
		(void)net_id;

	}

}

static void test_loader_contract_from_s27(void)
{
	hebs_plan* plan = hebs_load_bench("benchmarks/benches/ISCAS89/s27.bench");
	hebs_engine engine = { 0 };
	hebs_probes probes;

	assert(plan != NULL);
	assert(plan->lep_hash != 0);
	assert(plan->num_primary_inputs > 0);
	assert(plan->gate_count > 0);
	assert(plan->tray_count > 0);
	assert(plan->propagation_depth > 0);
	assert(plan->fanout_max > 0);
	assert(hebs_init_engine(&engine, plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 0, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, plan);
	probes = hebs_get_probes(&engine);
#if HEBS_TEST_PROBES
	assert(probes.input_toggle > 0);
#else
	(void)probes;
#endif
	hebs_free_plan(plan);

}

static void test_functional_gate_suite(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[2];
	hebs_logic_t states[8] = { HEBS_S0, HEBS_S1, HEBS_W0, HEBS_W1, HEBS_Z, HEBS_X, HEBS_SX, HEBS_WX };
	hebs_plan plan = { 0 };
	int a_idx;
	int b_idx;

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_AND;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = 0U;
	lep_data[0].src_b_bit_offset = 2U;
	lep_data[0].dst_bit_offset = 4U;

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = 0U;
	lep_data[1].src_b_bit_offset = 2U;
	lep_data[1].dst_bit_offset = 6U;

	plan.lep_hash = 1U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 4U;
	plan.gate_count = 2U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);

	for (a_idx = 0; a_idx < 8; ++a_idx)
	{
		for (b_idx = 0; b_idx < 8; ++b_idx)
		{
			hebs_logic_t a = states[a_idx];
			hebs_logic_t b = states[b_idx];
			const uint8_t a_pstate = project_logic_to_pstate(a);
			const uint8_t b_pstate = project_logic_to_pstate(b);
			const uint8_t expect_and = eval_and_from_pstate(a_pstate, b_pstate);
			const uint8_t expect_or = eval_or_from_pstate(a_pstate, b_pstate);
			assert(hebs_set_primary_input(&engine, &plan, 0U, a) == HEBS_OK);
			assert(hebs_set_primary_input(&engine, &plan, 1U, b) == HEBS_OK);
			hebs_tick(&engine, &plan);
			assert((uint8_t)read_signal_state(&engine, 2U) == expect_and);
			assert((uint8_t)read_signal_state(&engine, 3U) == expect_or);

		}

	}

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_W1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 3U) == 0x1U);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 2U) == 0x0U);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 3U) == 0x1U);

}

static void test_interleaved_bit_offset_mapping(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 33U };
	hebs_plan plan = { 0 };

	plan.lep_hash = 2U;
	plan.level_count = 1U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 34U;
	plan.gate_count = 0U;
	plan.tray_count = 2U;
	plan.max_level = 0U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = NULL;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(read_signal_state(&engine, 33U) == HEBS_S1);
	assert(hebs_get_signal_tray(&engine, 0U) != NULL);
	assert(hebs_get_signal_tray(&engine, 1U) != NULL);
	assert(*hebs_get_signal_tray(&engine, 0U) == 0ULL);
	assert(((*hebs_get_signal_tray(&engine, 1U) >> 2U) & 0x3ULL) == 0x1ULL);

}

static void test_icf_math_assertion(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_plan plan = { 0 };
	hebs_probes probes;
	uint32_t cycle;
	double icf;

	plan.lep_hash = 3U;
	plan.level_count = 1U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 1U;
	plan.gate_count = 0U;
	plan.tray_count = 1U;
	plan.max_level = 0U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = NULL;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);

	for (cycle = 0; cycle < 10U; ++cycle)
	{
		hebs_logic_t value = ((cycle % 4U) == 1U || (cycle % 4U) == 2U) ? HEBS_S1 : HEBS_S0;
		assert(hebs_set_primary_input(&engine, &plan, 0U, value) == HEBS_OK);
		hebs_tick(&engine, &plan);

	}

	probes = hebs_get_probes(&engine);
#if HEBS_TEST_PROBES
	assert(probes.input_toggle == 6U);
	assert(probes.state_change_commit == 0U);
	icf = calculate_icf(probes.state_change_commit, probes.input_toggle);
#else
	(void)probes;
	icf = calculate_icf(0U, 0U);
#endif
	assert(fabs(icf - 0.0) < 1e-12);
	icf = calculate_icf(15U, 5U);
	assert(fabs(icf - 3.0) < 1e-12);

}

static void test_parallel_dff_tray_commit(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[1];
	uint32_t dff_indices[1] = { 0U };
	hebs_exec_instruction_t dff_exec_data[1];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_DFF;
	lep_data[0].input_count = 1U;
	lep_data[0].level = 0U;
	lep_data[0].src_a_bit_offset = 0U;
	lep_data[0].src_b_bit_offset = 0U;
	lep_data[0].dst_bit_offset = 2U;

	plan.lep_hash = 4U;
	plan.level_count = 1U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 2U;
	plan.gate_count = 1U;
	plan.tray_count = 1U;
	plan.max_level = 0U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;
	plan.dff_instruction_count = 1U;
	plan.dff_instruction_indices = dff_indices;
	dff_exec_data[0].gate_type = (uint8_t)HEBS_GATE_DFF;
	dff_exec_data[0].src_a_shift = 0U;
	dff_exec_data[0].src_b_shift = 0U;
	dff_exec_data[0].dst_shift = 2U;
	dff_exec_data[0].src_a_tray = 0U;
	dff_exec_data[0].src_b_tray = 0U;
	dff_exec_data[0].dst_tray = 0U;
	plan.dff_exec_count = 1U;
	plan.dff_exec_data = dff_exec_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_signal_state(&engine, 1U) == HEBS_S0);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 1U) == 1U);
	hebs_tick(&engine, &plan);
	assert((uint8_t)read_signal_state(&engine, 1U) == 1U);

}

static void test_pending_state_accumulator_closure_proof(void)
{
	uint8_t accum_lut[8][HEBS_PROOF_DRIVE_KIND_COUNT];
	uint8_t pending_state;
	uint8_t saw_non_materialized_state;
	static const hebs_proof_drive_kind_t seq_s1_w0[2] =
	{
		HEBS_PROOF_DRIVE_S1,
		HEBS_PROOF_DRIVE_W0
	};
	static const hebs_proof_drive_kind_t seq_w1_w0[2] =
	{
		HEBS_PROOF_DRIVE_W1,
		HEBS_PROOF_DRIVE_W0
	};
	static const hebs_proof_drive_kind_t seq_s1_sx[2] =
	{
		HEBS_PROOF_DRIVE_S1,
		HEBS_PROOF_DRIVE_SX
	};
	static const hebs_proof_drive_kind_t seq_w1_sx[2] =
	{
		HEBS_PROOF_DRIVE_W1,
		HEBS_PROOF_DRIVE_SX
	};
	static const hebs_proof_drive_kind_t seq_none_strong[2] =
	{
		HEBS_PROOF_DRIVE_NONE,
		HEBS_PROOF_DRIVE_S1
	};
	static const hebs_proof_drive_kind_t seq_duplicate_s1[2] =
	{
		HEBS_PROOF_DRIVE_S1,
		HEBS_PROOF_DRIVE_S1
	};
	static const hebs_proof_drive_kind_t seq_tri_disabled[2] =
	{
		HEBS_PROOF_DRIVE_NONE,
		HEBS_PROOF_DRIVE_W1
	};

	saw_non_materialized_state = hebs_proof_build_accum_lut(accum_lut);
	assert(saw_non_materialized_state == 1U);

	for (pending_state = 0U; pending_state < 8U; ++pending_state)
	{
		uint8_t drive_kind;

		for (drive_kind = 0U; drive_kind < HEBS_PROOF_DRIVE_KIND_COUNT; ++drive_kind)
		{
			const uint8_t drive_nibble = hebs_proof_drive_to_nibble((hebs_proof_drive_kind_t)drive_kind);
			uint8_t mailbox;
			uint8_t saw_representative = 0U;

			for (mailbox = 0U; mailbox < 16U; ++mailbox)
			{
				if (hebs_proof_resolve_mailbox_state(mailbox) != pending_state)
				{
					continue;

				}

				assert(accum_lut[pending_state][drive_kind] == hebs_proof_resolve_mailbox_state((uint8_t)(mailbox | drive_nibble)));
				saw_representative = 1U;

			}

			if (pending_state == HEBS_PROOF_STATE_X)
			{
				assert(saw_representative == 0U);
				if (drive_kind == HEBS_PROOF_DRIVE_NONE)
				{
					assert(accum_lut[pending_state][drive_kind] == HEBS_PROOF_STATE_X);

				}
				else
				{
					assert(accum_lut[pending_state][drive_kind] == hebs_proof_resolve_mailbox_state(drive_nibble));

				}

			}
			else
			{
				assert(saw_representative == 1U);

			}

		}

	}

	hebs_proof_expect_sequence_match(accum_lut, seq_s1_w0, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_w1_w0, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_s1_sx, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_w1_sx, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_none_strong, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_duplicate_s1, 2U);
	hebs_proof_expect_sequence_match(accum_lut, seq_tri_disabled, 2U);

	assert(accum_lut[HEBS_PROOF_STATE_Z][HEBS_PROOF_DRIVE_NONE] == HEBS_PROOF_STATE_Z);
	assert(accum_lut[HEBS_PROOF_STATE_S1][HEBS_PROOF_DRIVE_W0] == HEBS_PROOF_STATE_S1);
	assert(accum_lut[HEBS_PROOF_STATE_W1][HEBS_PROOF_DRIVE_W0] == HEBS_PROOF_STATE_WX);
	assert(accum_lut[HEBS_PROOF_STATE_S1][HEBS_PROOF_DRIVE_S0] == HEBS_PROOF_STATE_SX);
	assert(accum_lut[HEBS_PROOF_STATE_X][HEBS_PROOF_DRIVE_NONE] == HEBS_PROOF_STATE_X);
	assert(accum_lut[HEBS_PROOF_STATE_X][HEBS_PROOF_DRIVE_S1] == HEBS_PROOF_STATE_S1);

}

static void test_live_pending_accumulator_semantics(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[9];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[0].input_count = 0U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(0U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[1].input_count = 0U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(2U);

	lep_data[2].gate_type = (uint8_t)HEBS_GATE_GND;
	lep_data[2].input_count = 0U;
	lep_data[2].level = 1U;
	lep_data[2].src_a_bit_offset = net_to_bit(0U);
	lep_data[2].src_b_bit_offset = net_to_bit(0U);
	lep_data[2].dst_bit_offset = net_to_bit(3U);

	lep_data[3].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[3].input_count = 1U;
	lep_data[3].level = 1U;
	lep_data[3].src_a_bit_offset = net_to_bit(0U);
	lep_data[3].src_b_bit_offset = net_to_bit(0U);
	lep_data[3].dst_bit_offset = net_to_bit(3U);

	lep_data[4].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[4].input_count = 1U;
	lep_data[4].level = 1U;
	lep_data[4].src_a_bit_offset = net_to_bit(0U);
	lep_data[4].src_b_bit_offset = net_to_bit(0U);
	lep_data[4].dst_bit_offset = net_to_bit(4U);

	lep_data[5].gate_type = (uint8_t)HEBS_GATE_PDN;
	lep_data[5].input_count = 1U;
	lep_data[5].level = 1U;
	lep_data[5].src_a_bit_offset = net_to_bit(0U);
	lep_data[5].src_b_bit_offset = net_to_bit(0U);
	lep_data[5].dst_bit_offset = net_to_bit(4U);

	lep_data[6].gate_type = (uint8_t)HEBS_GATE_TRI;
	lep_data[6].input_count = 2U;
	lep_data[6].level = 1U;
	lep_data[6].src_a_bit_offset = net_to_bit(0U);
	lep_data[6].src_b_bit_offset = net_to_bit(1U);
	lep_data[6].dst_bit_offset = net_to_bit(5U);

	lep_data[7].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[7].input_count = 0U;
	lep_data[7].level = 1U;
	lep_data[7].src_a_bit_offset = net_to_bit(0U);
	lep_data[7].src_b_bit_offset = net_to_bit(0U);
	lep_data[7].dst_bit_offset = net_to_bit(5U);

	lep_data[8].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[8].input_count = 1U;
	lep_data[8].level = 1U;
	lep_data[8].src_a_bit_offset = net_to_bit(0U);
	lep_data[8].src_b_bit_offset = net_to_bit(0U);
	lep_data[8].dst_bit_offset = net_to_bit(6U);

	plan.lep_hash = 11001U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 7U;
	plan.gate_count = 9U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_Z) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);

	assert(read_net_3bn(&engine, 2U) == HEBS_S1);
	assert(read_net_3bn(&engine, 3U) == HEBS_S0);
	assert(read_net_3bn(&engine, 4U) == HEBS_WX);
	assert(read_net_3bn(&engine, 5U) == HEBS_S1);
	assert(read_net_3bn(&engine, 6U) == HEBS_W1);
	assert(read_net_pstate(&engine, 2U) == 0x1U);
	assert(read_net_pstate(&engine, 3U) == 0x0U);
	assert(read_net_pstate(&engine, 4U) == 0x2U);
	assert(read_net_pstate(&engine, 5U) == 0x1U);
	assert(read_net_pstate(&engine, 6U) == 0x1U);

}

static void test_live_pending_batched_fallback_equivalence(void)
{
	hebs_engine fallback_engine = { 0 };
	hebs_engine batched_engine = { 0 };
	uint32_t primary_inputs[3] = { 0U, 1U, 2U };
	hebs_lep_instruction_t lep_data[6];
	hebs_exec_instruction_t exec_data[6];
	hebs_gate_span_t spans[6];
	hebs_plan fallback_plan = { 0 };
	hebs_plan batched_plan = { 0 };
	uint32_t idx;
	uint32_t net_id;

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_AND;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(1U);
	lep_data[0].dst_bit_offset = net_to_bit(3U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[1].input_count = 2U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(1U);
	lep_data[1].dst_bit_offset = net_to_bit(4U);

	lep_data[2].gate_type = (uint8_t)HEBS_GATE_TRI;
	lep_data[2].input_count = 2U;
	lep_data[2].level = 1U;
	lep_data[2].src_a_bit_offset = net_to_bit(0U);
	lep_data[2].src_b_bit_offset = net_to_bit(1U);
	lep_data[2].dst_bit_offset = net_to_bit(5U);

	lep_data[3].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[3].input_count = 1U;
	lep_data[3].level = 1U;
	lep_data[3].src_a_bit_offset = net_to_bit(2U);
	lep_data[3].src_b_bit_offset = net_to_bit(2U);
	lep_data[3].dst_bit_offset = net_to_bit(6U);

	lep_data[4].gate_type = (uint8_t)HEBS_GATE_PDN;
	lep_data[4].input_count = 1U;
	lep_data[4].level = 1U;
	lep_data[4].src_a_bit_offset = net_to_bit(2U);
	lep_data[4].src_b_bit_offset = net_to_bit(2U);
	lep_data[4].dst_bit_offset = net_to_bit(7U);

	lep_data[5].gate_type = (uint8_t)HEBS_GATE_XOR;
	lep_data[5].input_count = 2U;
	lep_data[5].level = 1U;
	lep_data[5].src_a_bit_offset = net_to_bit(1U);
	lep_data[5].src_b_bit_offset = net_to_bit(2U);
	lep_data[5].dst_bit_offset = net_to_bit(8U);

	for (idx = 0U; idx < 6U; ++idx)
	{
		exec_data[idx].gate_type = lep_data[idx].gate_type;
		exec_data[idx].src_a_shift = (uint8_t)(lep_data[idx].src_a_bit_offset & 63U);
		exec_data[idx].src_b_shift = (uint8_t)(lep_data[idx].src_b_bit_offset & 63U);
		exec_data[idx].dst_shift = (uint8_t)(lep_data[idx].dst_bit_offset & 63U);
		exec_data[idx].src_a_tray = lep_data[idx].src_a_bit_offset >> 6U;
		exec_data[idx].src_b_tray = lep_data[idx].src_b_bit_offset >> 6U;
		exec_data[idx].dst_tray = lep_data[idx].dst_bit_offset >> 6U;
		spans[idx].start = idx;
		spans[idx].count = 1U;
		spans[idx].gate_type = lep_data[idx].gate_type;
		spans[idx].reserved0 = 0U;
		spans[idx].reserved1 = 0U;
		spans[idx].reserved2 = 0U;

	}

	fallback_plan.lep_hash = 11002U;
	fallback_plan.level_count = 2U;
	fallback_plan.num_primary_inputs = 3U;
	fallback_plan.signal_count = 9U;
	fallback_plan.gate_count = 6U;
	fallback_plan.tray_count = 1U;
	fallback_plan.max_level = 1U;
	fallback_plan.primary_input_ids = primary_inputs;
	fallback_plan.lep_data = lep_data;

	batched_plan = fallback_plan;
	batched_plan.comb_instruction_count = 6U;
	batched_plan.comb_exec_data = exec_data;
	batched_plan.comb_span_count = 6U;
	batched_plan.comb_spans = spans;

	assert(hebs_init_engine(&fallback_engine, &fallback_plan) == HEBS_OK);
	assert(hebs_init_engine(&batched_engine, &batched_plan) == HEBS_OK);

	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 1U, HEBS_SX) == HEBS_OK);
	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 2U, HEBS_Z) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 1U, HEBS_SX) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 2U, HEBS_Z) == HEBS_OK);
	hebs_tick(&fallback_engine, &fallback_plan);
	hebs_tick(&batched_engine, &batched_plan);

	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 1U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&fallback_engine, &fallback_plan, 2U, HEBS_Z) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 1U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&batched_engine, &batched_plan, 2U, HEBS_Z) == HEBS_OK);
	hebs_tick(&fallback_engine, &fallback_plan);
	hebs_tick(&batched_engine, &batched_plan);

	for (net_id = 0U; net_id < fallback_plan.signal_count; ++net_id)
	{
		assert(read_net_3bn(&fallback_engine, net_id) == read_net_3bn(&batched_engine, net_id));
		assert(read_net_pstate(&fallback_engine, net_id) == read_net_pstate(&batched_engine, net_id));

	}

}

static void test_live_pending_dff_deferred_visibility(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_exec_instruction_t dff_exec_data[1];
	hebs_plan plan = { 0 };

	dff_exec_data[0].gate_type = (uint8_t)HEBS_GATE_DFF;
	dff_exec_data[0].src_a_shift = 0U;
	dff_exec_data[0].src_b_shift = 0U;
	dff_exec_data[0].dst_shift = 2U;
	dff_exec_data[0].src_a_tray = 0U;
	dff_exec_data[0].src_b_tray = 0U;
	dff_exec_data[0].dst_tray = 0U;
	plan.lep_hash = 11003U;
	plan.level_count = 1U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 2U;
	plan.gate_count = 0U;
	plan.tray_count = 1U;
	plan.max_level = 0U;
	plan.primary_input_ids = primary_inputs;
	plan.dff_exec_count = 1U;
	plan.dff_exec_data = dff_exec_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(read_net_3bn(&engine, 1U) == HEBS_Z);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_net_3bn(&engine, 1U) == HEBS_Z);
	assert(read_net_pstate(&engine, 1U) == 0x0U);
	hebs_tick(&engine, &plan);
	assert(read_net_3bn(&engine, 1U) == HEBS_S1);
	assert(read_net_pstate(&engine, 1U) == 0x1U);

}

static void test_protocol_helper_stats(void)
{
	double values[5] = { 5.0, 1.0, 3.0, 4.0, 2.0 };
	assert(fabs(calculate_min(values, 5) - 1.0) < 1e-12);
	assert(fabs(calculate_max(values, 5) - 5.0) < 1e-12);
	assert(fabs(calculate_p50(values, 5) - 3.0) < 1e-12);
	assert(fabs(calculate_percentile(values, 5, 90.0) - 4.6) < 1e-12);
	assert(fabs(calculate_stddev(values, 5) - sqrt(2.0)) < 1e-12);

}

static void test_extended_primitive_suite(void)
{
	assert(hebs_eval_xor(HEBS_S0, HEBS_S0) == HEBS_S0);
	assert(hebs_eval_xor(HEBS_S1, HEBS_S0) == HEBS_S1);
	assert(hebs_eval_nand(HEBS_S1, HEBS_S1) == HEBS_S0);
	assert(hebs_eval_nor(HEBS_S0, HEBS_S0) == HEBS_S1);
	assert(hebs_eval_xnor(HEBS_S1, HEBS_S1) == HEBS_S1);
	assert(hebs_eval_buf(HEBS_W0) == HEBS_W0);
	assert(hebs_eval_weak_pull(HEBS_Z) == HEBS_W1);
	assert(hebs_eval_strong_pull(HEBS_Z) == HEBS_S1);
	assert(hebs_eval_vcc() == HEBS_S1);
	assert(hebs_eval_gnd() == HEBS_S0);
	assert(hebs_eval_tristate(HEBS_S1, HEBS_S1) == HEBS_S1);
	assert(hebs_eval_tristate(HEBS_S1, HEBS_S0) == HEBS_Z);

}

static void test_engine_fact_accessors(void)
{
	hebs_plan* plan = hebs_load_bench("benchmarks/benches/ISCAS85/c17.bench");
	hebs_engine engine = { 0 };
	hebs_plan_metadata metadata;
	hebs_run_status status;
	hebs_logic_t value = HEBS_X;
	uint32_t crc_before;
	uint32_t crc_after;

	assert(plan != NULL);
	assert(hebs_get_plan_hash(plan) == plan->lep_hash);
	assert(hebs_get_plan_metadata(plan, &metadata) == 1);
	assert(metadata.plan_hash == plan->lep_hash);
	assert(metadata.gate_count == plan->gate_count);
	assert(metadata.signal_count == plan->signal_count);
	assert(metadata.tray_count == plan->tray_count);
	assert(metadata.num_primary_inputs == plan->num_primary_inputs);
	assert(hebs_get_plan_metadata(NULL, &metadata) == 0);
	assert(hebs_get_plan_metadata(plan, NULL) == 0);
	assert(hebs_get_plan_hash(NULL) == 0U);
	assert(hebs_get_net_physical_state(NULL, 0U, &value) == 0);
	assert(hebs_get_net_physical_state(&engine, 0U, NULL) == 0);

	assert(hebs_init_engine(&engine, plan) == HEBS_OK);
	assert(hebs_get_net_physical_state(&engine, plan->signal_count, &value) == 0);
	assert(hebs_get_net_physical_state(&engine, 0U, &value) == 1);
	assert(value == HEBS_Z);
	assert(hebs_get_run_status(&engine, &status) == 1);
	assert(status.last_status == HEBS_OK);
	assert(status.current_tick == 0U);
	assert(status.tray_count == plan->tray_count);
	assert(status.test_probes_enabled == (uint8_t)HEBS_TEST_PROBES);
	assert(hebs_get_primary_input_state(NULL, plan, 0U) == 0xFFU);
	assert(hebs_get_primary_input_state(&engine, NULL, 0U) == 0xFFU);
	assert(hebs_get_primary_input_state(&engine, plan, plan->num_primary_inputs) == 0xFFU);
	assert(hebs_get_primary_input(&engine, plan, plan->num_primary_inputs) == HEBS_X);

	crc_before = hebs_get_final_crc32(&engine);
	assert(hebs_set_primary_input(&engine, plan, plan->num_primary_inputs, HEBS_S0) == HEBS_ERR_LOGIC);
	assert(hebs_get_run_status(&engine, &status) == 1);
	assert(status.last_status == HEBS_ERR_LOGIC);

	assert(hebs_set_primary_input(&engine, plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_get_net_physical_state(&engine, plan->primary_input_ids[0U], &value) == 1);
	assert(value == HEBS_S1);
	assert(hebs_get_primary_input_state(&engine, plan, 0U) == 0x1U);
	assert(hebs_get_primary_input(&engine, plan, 0U) == HEBS_S1);
	hebs_tick(&engine, plan);
	assert(hebs_get_run_status(&engine, &status) == 1);
	assert(status.last_status == HEBS_OK);
	assert(status.current_tick == 1U);
	assert(status.cycles_executed == 1U);
	crc_after = hebs_get_final_crc32(&engine);
	assert(crc_before != 0U || crc_after != 0U);
	assert(hebs_get_final_crc32(NULL) == 0U);
	assert(hebs_get_run_status(NULL, &status) == 0);
	assert(hebs_get_run_status(&engine, NULL) == 0);
	hebs_free_plan(plan);

}

static void test_probe_expansion_counters(void)
{
	hebs_engine engine = { 0 };
	hebs_probes probes;
	uint32_t primary_inputs[2] = { 0U, 1U };
	hebs_lep_instruction_t lep_data[5];
	hebs_plan plan = { 0 };

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_VCC;
	lep_data[0].input_count = 0U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(0U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	lep_data[1].gate_type = (uint8_t)HEBS_GATE_GND;
	lep_data[1].input_count = 0U;
	lep_data[1].level = 1U;
	lep_data[1].src_a_bit_offset = net_to_bit(0U);
	lep_data[1].src_b_bit_offset = net_to_bit(0U);
	lep_data[1].dst_bit_offset = net_to_bit(2U);

	lep_data[2].gate_type = (uint8_t)HEBS_GATE_PUP;
	lep_data[2].input_count = 1U;
	lep_data[2].level = 1U;
	lep_data[2].src_a_bit_offset = net_to_bit(0U);
	lep_data[2].src_b_bit_offset = net_to_bit(0U);
	lep_data[2].dst_bit_offset = net_to_bit(3U);

	lep_data[3].gate_type = (uint8_t)HEBS_GATE_PDN;
	lep_data[3].input_count = 1U;
	lep_data[3].level = 1U;
	lep_data[3].src_a_bit_offset = net_to_bit(0U);
	lep_data[3].src_b_bit_offset = net_to_bit(0U);
	lep_data[3].dst_bit_offset = net_to_bit(4U);

	lep_data[4].gate_type = (uint8_t)HEBS_GATE_TRI;
	lep_data[4].input_count = 2U;
	lep_data[4].level = 1U;
	lep_data[4].src_a_bit_offset = net_to_bit(0U);
	lep_data[4].src_b_bit_offset = net_to_bit(1U);
	lep_data[4].dst_bit_offset = net_to_bit(5U);

	plan.lep_hash = 11004U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 2U;
	plan.signal_count = 6U;
	plan.gate_count = 5U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_Z) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	probes = hebs_get_probes(&engine);

	assert(probes.tick_count == 1U);
	assert(probes.state_commit_count == 3U);

#if HEBS_TEST_PROBES
	assert(probes.multi_driver_resolve_count == 1U);
	assert(probes.contention_count == 1U);
	assert(probes.unknown_state_materialize_count == 1U);
	assert(probes.highz_materialize_count == 0U);
	assert(probes.tri_no_drive_count == 1U);
	assert(probes.pup_z_source_count == 1U);
	assert(probes.pdn_z_source_count == 1U);
#endif

}

static void test_loaded_dff_captures_declared_data_only(void)
{
	hebs_plan* plan = load_single_input_dff_test_plan("build/test_loaded_dff_capture.bench");
	hebs_engine engine = { 0 };

	assert(plan != NULL);
	assert(plan->dff_exec_count == 1U);
	assert(plan->dff_exec_data[0].src_a_tray == plan->dff_exec_data[0].src_b_tray);
	assert(plan->dff_exec_data[0].src_a_shift == plan->dff_exec_data[0].src_b_shift);

	assert(hebs_init_engine(&engine, plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 0U, HEBS_X) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 1U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, plan);
	hebs_tick(&engine, plan);
	assert((uint8_t)read_signal_state(&engine, 2U) == 0x1U);

	hebs_free_plan(plan);

}

static void test_loaded_dff_unknown_capture_ignores_unrelated_state(void)
{
	hebs_plan* plan = load_single_input_dff_test_plan("build/test_loaded_dff_unknown.bench");
	hebs_engine engine_a = { 0 };
	hebs_engine engine_b = { 0 };

	assert(plan != NULL);
	assert(hebs_init_engine(&engine_a, plan) == HEBS_OK);
	assert(hebs_init_engine(&engine_b, plan) == HEBS_OK);

	assert(hebs_set_primary_input(&engine_a, plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine_a, plan, 1U, HEBS_X) == HEBS_OK);
	assert(hebs_set_primary_input(&engine_b, plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine_b, plan, 1U, HEBS_X) == HEBS_OK);
	hebs_tick(&engine_a, plan);
	hebs_tick(&engine_a, plan);
	hebs_tick(&engine_b, plan);
	hebs_tick(&engine_b, plan);

	assert((uint8_t)read_signal_state(&engine_a, 2U) == 0x2U);
	assert((uint8_t)read_signal_state(&engine_b, 2U) == 0x2U);

	hebs_free_plan(plan);

}

static void test_loader_batched_specialized_span_execution(void)
{
	static const char* bench_text =
		"INPUT(a)\n"
		"INPUT(b)\n"
		"INPUT(en)\n"
		"n_and = AND(a,b)\n"
		"n_or = OR(a,b)\n"
		"n_xor = XOR(a,b)\n"
		"n_nand = NAND(a,b)\n"
		"n_nor = NOR(a,b)\n"
		"n_not = NOT(a)\n"
		"n_buf = BUF(b)\n"
		"n_xnor = XNOR(a,b)\n"
		"n_vcc = VCC()\n"
		"n_gnd = GND()\n"
		"n_tri = TRI(a,en)\n";
	hebs_plan* plan = load_temp_bench_plan("build/test_batched_spans.bench", bench_text);
	hebs_engine engine = { 0 };

	assert(plan != NULL);
	assert(plan->comb_exec_data != NULL);
	assert(plan->comb_spans != NULL);
	assert(plan->comb_span_count > 0U);
	assert(plan->comb_instruction_count == 11U);

	assert(hebs_init_engine(&engine, plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 1U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, plan, 2U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, plan);

	assert((uint8_t)read_signal_state(&engine, 3U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 4U) == 0x1U);
	assert((uint8_t)read_signal_state(&engine, 5U) == 0x1U);
	assert((uint8_t)read_signal_state(&engine, 6U) == 0x1U);
	assert((uint8_t)read_signal_state(&engine, 7U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 8U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 9U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 10U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 11U) == 0x1U);
	assert((uint8_t)read_signal_state(&engine, 12U) == 0x0U);
	assert((uint8_t)read_signal_state(&engine, 13U) == 0x1U);

	hebs_free_plan(plan);

}

static void test_init_rejects_oversized_signal_count(void)
{
	hebs_engine engine = { 0 };
	hebs_plan plan = { 0 };

	plan.signal_count = HEBS_MAX_SIGNALS + 1U;
	plan.tray_count = 1U;

	assert(hebs_init_engine(&engine, &plan) == HEBS_ERR_LOGIC);
	assert(engine.last_status == HEBS_ERR_LOGIC);

}

static void test_fallback_invalid_src_b_is_safely_skipped(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_lep_instruction_t lep_data[1];
	hebs_plan plan = { 0 };
	hebs_probes probes;

	lep_data[0].gate_type = (uint8_t)HEBS_GATE_OR;
	lep_data[0].input_count = 2U;
	lep_data[0].level = 1U;
	lep_data[0].src_a_bit_offset = net_to_bit(0U);
	lep_data[0].src_b_bit_offset = net_to_bit(99U);
	lep_data[0].dst_bit_offset = net_to_bit(2U);

	plan.lep_hash = 10011U;
	plan.level_count = 2U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.gate_count = 1U;
	plan.tray_count = 1U;
	plan.max_level = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.lep_data = lep_data;
	plan.comb_exec_data = NULL;
	plan.comb_spans = NULL;
	plan.comb_span_count = 0U;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	probes = hebs_get_probes(&engine);

	assert((uint8_t)read_signal_state(&engine, 2U) == 0x0U);
	assert(probes.chunk_exec == 0U);
	assert(probes.gate_eval == 0U);

}

static void test_batched_invalid_src_b_is_safely_skipped(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_exec_instruction_t comb_exec_data[1];
	hebs_gate_span_t comb_spans[1];
	hebs_plan plan = { 0 };
	hebs_probes probes;

	comb_exec_data[0].gate_type = (uint8_t)HEBS_GATE_OR;
	comb_exec_data[0].src_a_shift = 0U;
	comb_exec_data[0].src_b_shift = 0U;
	comb_exec_data[0].dst_shift = 4U;
	comb_exec_data[0].src_a_tray = 0U;
	comb_exec_data[0].src_b_tray = 1U;
	comb_exec_data[0].dst_tray = 0U;
	comb_spans[0].start = 0U;
	comb_spans[0].count = 1U;
	comb_spans[0].gate_type = (uint8_t)HEBS_GATE_OR;
	comb_spans[0].reserved0 = 0U;
	comb_spans[0].reserved1 = 0U;
	comb_spans[0].reserved2 = 0U;

	plan.lep_hash = 10012U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 3U;
	plan.tray_count = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.comb_instruction_count = 1U;
	plan.comb_exec_data = comb_exec_data;
	plan.comb_span_count = 1U;
	plan.comb_spans = comb_spans;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	probes = hebs_get_probes(&engine);

	assert((uint8_t)read_signal_state(&engine, 2U) == 0x0U);
	assert(probes.chunk_exec == 1U);
	assert(probes.gate_eval == 1U);

}

static void test_dff_invalid_destination_is_safely_skipped(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_exec_instruction_t dff_exec_data[1];
	hebs_plan plan = { 0 };
	hebs_probes probes;
	uint32_t crc_before;

	dff_exec_data[0].gate_type = (uint8_t)HEBS_GATE_DFF;
	dff_exec_data[0].src_a_shift = 0U;
	dff_exec_data[0].src_b_shift = 0U;
	dff_exec_data[0].dst_shift = 0U;
	dff_exec_data[0].src_a_tray = 0U;
	dff_exec_data[0].src_b_tray = 0U;
	dff_exec_data[0].dst_tray = 1U;
	plan.lep_hash = 10013U;
	plan.num_primary_inputs = 1U;
	plan.signal_count = 1U;
	plan.tray_count = 1U;
	plan.primary_input_ids = primary_inputs;
	plan.dff_exec_count = 1U;
	plan.dff_exec_data = dff_exec_data;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	crc_before = hebs_get_final_crc32(&engine);
	hebs_tick(&engine, &plan);
	probes = hebs_get_probes(&engine);

	assert((uint8_t)read_signal_state(&engine, 0U) == 0x1U);
	assert(hebs_get_final_crc32(&engine) == crc_before);
	assert(probes.dff_exec == 1U);

}

int main(void)
{
	assert(HEBS_S1 == 1);
	assert(HEBS_WX == 7);
	test_loader_contract_from_s27();
	test_pending_state_accumulator_closure_proof();
	test_live_pending_accumulator_semantics();
	test_live_pending_batched_fallback_equivalence();
	test_live_pending_dff_deferred_visibility();
	test_phase3_strength_precedence_strong_over_weak();
	test_and_or_dominance_x_suppression();
	test_and_nand_x_dominance();
	test_or_nor_x_dominance();
	test_tristate_disabled_is_zero_drive();
	test_multi_driver_phase1_or_not_overwrite();
	test_duplicate_strong_drive_is_idempotent();
	test_full_net_contention_64plus();
	test_dff_x_poison_chain_latency_10();
	test_dirty_set_sparse_dense_saturation();
	test_cycle_stability_no_temporal_path();
	test_functional_gate_suite();
	test_interleaved_bit_offset_mapping();
	test_icf_math_assertion();
	test_parallel_dff_tray_commit();
	test_protocol_helper_stats();
	test_extended_primitive_suite();
	test_engine_fact_accessors();
	test_probe_expansion_counters();
	test_loaded_dff_captures_declared_data_only();
	test_loaded_dff_unknown_capture_ignores_unrelated_state();
	test_loader_batched_specialized_span_execution();
	test_init_rejects_oversized_signal_count();
	test_fallback_invalid_src_b_is_safely_skipped();
	test_batched_invalid_src_b_is_safely_skipped();
	test_dff_invalid_destination_is_safely_skipped();
	return 0;

}
