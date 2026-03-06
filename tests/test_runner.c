#include <assert.h>
#include <math.h>
#include "hebs_engine.h"
#include "primitives.h"
#include "../benchmarks/protocol_helper.h"

static hebs_logic_t read_signal_state(const hebs_engine* engine, uint32_t signal_id)
{
	uint32_t tray_index = signal_id / 32U;
	uint32_t bit_position = (signal_id % 32U) * 2U;
	uint64_t tray = engine->signal_trays[tray_index];
	return (hebs_logic_t)((tray >> bit_position) & 0x3U);

}

static hebs_logic_t pack2(hebs_logic_t value)
{
	return (hebs_logic_t)((uint32_t)value & 0x3U);

}

static void test_loader_contract_from_s27(void)
{
	hebs_plan* plan = hebs_load_bench("benchmarks/benches/ISCAS89/s27.bench");
	hebs_engine engine = { 0 };

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
	assert(engine.input_toggle_count > 0);
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
			hebs_logic_t a_packed = pack2(a);
			hebs_logic_t b_packed = pack2(b);
			assert(hebs_set_primary_input(&engine, &plan, 0U, a_packed) == HEBS_OK);
			assert(hebs_set_primary_input(&engine, &plan, 1U, b_packed) == HEBS_OK);
			hebs_tick(&engine, &plan);
			assert(read_signal_state(&engine, 2U) == pack2(hebs_eval_and(a_packed, b_packed)));
			assert(read_signal_state(&engine, 3U) == pack2(hebs_eval_or(a_packed, b_packed)));

		}

	}

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_signal_state(&engine, 2U) == HEBS_S0);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 1U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_signal_state(&engine, 3U) == HEBS_S1);

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
	assert(engine.signal_trays[0] == 0ULL);
	assert(((engine.signal_trays[1] >> 2U) & 0x3ULL) == (uint64_t)HEBS_S1);

}

static void test_icf_math_assertion(void)
{
	hebs_engine engine = { 0 };
	uint32_t primary_inputs[1] = { 0U };
	hebs_plan plan = { 0 };
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

	assert(engine.input_toggle_count == 5U);
	assert(engine.internal_transition_count == 0U);
	icf = calculate_icf(engine.internal_transition_count, engine.input_toggle_count);
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
	uint64_t dff_commit_mask[1] = { 0ULL };
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
	dff_exec_data[0].dst_mask = 0x3ULL << 2U;
	dff_commit_mask[0] = dff_exec_data[0].dst_mask;
	plan.dff_exec_count = 1U;
	plan.dff_exec_data = dff_exec_data;
	plan.dff_commit_mask = dff_commit_mask;

	assert(hebs_init_engine(&engine, &plan) == HEBS_OK);
	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S0) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_signal_state(&engine, 1U) == HEBS_S0);

	assert(hebs_set_primary_input(&engine, &plan, 0U, HEBS_S1) == HEBS_OK);
	hebs_tick(&engine, &plan);
	assert(read_signal_state(&engine, 1U) == HEBS_S1);
	assert(((engine.dff_state_trays[0] >> 2U) & 0x3ULL) == (uint64_t)HEBS_S1);

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
	uint64_t tri_data;
	uint64_t tri_enable;

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

	assert(hebs_gate_buf_simd(0x1234567890ABCDEFULL) == 0x1234567890ABCDEFULL);
	assert(hebs_gate_vcc_simd() == 0x5555555555555555ULL);
	assert(hebs_gate_gnd_simd() == 0x0ULL);

	tri_data = 0xAAAAAAAAAAAAAAAAULL;
	tri_enable = 0x5555555555555555ULL;
	assert(hebs_gate_tristate_simd(tri_data, tri_enable) == tri_data);

	tri_enable = 0x0ULL;
	assert(hebs_gate_tristate_simd(tri_data, tri_enable) == 0x0ULL);

}

int main(void)
{
	assert(HEBS_S1 == 1);
	assert(HEBS_WX == 7);
	test_loader_contract_from_s27();
	test_functional_gate_suite();
	test_interleaved_bit_offset_mapping();
	test_icf_math_assertion();
	test_parallel_dff_tray_commit();
	test_protocol_helper_stats();
	test_extended_primitive_suite();
	return 0;

}
