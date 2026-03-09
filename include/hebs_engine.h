#ifndef HEBS_ENGINE_H
#define HEBS_ENGINE_H

#include <stdint.h>
#include "hebs_types.h"

#define HEBS_MAX_PRIMARY_INPUTS 4096
#define HEBS_MAX_SIGNAL_TRAYS 4096
#define HEBS_MAX_SIGNALS (HEBS_MAX_SIGNAL_TRAYS * 32U)
#ifndef HEBS_BATCH_GATE_CHUNK
#define HEBS_BATCH_GATE_CHUNK 64U
#endif

#if defined(_MSC_VER)
#define HEBS_ALIGN64 __declspec(align(64))
#else
#define HEBS_ALIGN64 __attribute__((aligned(64)))
#endif

typedef enum hebs_logic_e
{
	HEBS_S0 = 0,
	HEBS_S1 = 1,
	HEBS_W0 = 2,
	HEBS_W1 = 3,
	HEBS_Z = 4,
	HEBS_X = 5,
	HEBS_SX = 6,
	HEBS_WX = 7

} hebs_logic_t;

typedef enum hebs_status_e
{
	HEBS_OK = 0,
	HEBS_ERR_LOGIC = 1

} hebs_status_t;

typedef enum hebs_gate_type_e
{
	HEBS_GATE_AND = 0,
	HEBS_GATE_OR = 1,
	HEBS_GATE_NOT = 2,
	HEBS_GATE_NAND = 3,
	HEBS_GATE_NOR = 4,
	HEBS_GATE_BUF = 5,
	HEBS_GATE_DFF = 6,
	HEBS_GATE_XOR = 7,
	HEBS_GATE_XNOR = 8,
	HEBS_GATE_TRI = 9,
	HEBS_GATE_VCC = 10,
	HEBS_GATE_GND = 11,
	HEBS_GATE_PUP = 12,
	HEBS_GATE_PDN = 13

} hebs_gate_type_t;

#define HEBS_COMB_GATE_TYPE_COUNT 13U

typedef struct hebs_lep_instruction_s
{
	uint8_t gate_type;
	uint8_t input_count;
	uint16_t level;
	uint32_t src_a_bit_offset;
	uint32_t src_b_bit_offset;
	uint32_t dst_bit_offset;

} hebs_lep_instruction_t;

typedef struct hebs_exec_instruction_s
{
	uint8_t gate_type;
	uint8_t src_a_shift;
	/* Single-input DFF exec records mirror src_a into src_b. */
	uint8_t src_b_shift;
	uint8_t dst_shift;
	uint32_t src_a_tray;
	uint32_t src_b_tray;
	uint32_t dst_tray;
	uint64_t dst_mask;

} hebs_exec_instruction_t;

typedef struct hebs_gate_span_s
{
	uint32_t start;
	uint32_t count;
	uint8_t gate_type;
	uint8_t reserved0;
	uint8_t reserved1;
	uint8_t reserved2;

} hebs_gate_span_t;

typedef struct hebs_plan_s
{
	uint64_t lep_hash;
	uint32_t level_count;
	uint32_t num_primary_inputs;
	uint32_t num_primary_outputs;
	uint32_t signal_count;
	uint32_t gate_count;
	uint32_t tray_count;
	uint32_t max_level;
	uint32_t propagation_depth;
	double fanout_avg;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	uint32_t* primary_input_ids;
	hebs_lep_instruction_t* lep_data;
	uint32_t dff_instruction_count;
	uint32_t* dff_instruction_indices;
	uint32_t dff_exec_count;
	hebs_exec_instruction_t* dff_exec_data;
	uint64_t* dff_commit_mask;
	uint32_t comb_instruction_count;
	uint32_t* comb_instruction_indices;
	hebs_exec_instruction_t* comb_exec_data;
	uint32_t comb_span_count;
	hebs_gate_span_t* comb_spans;
	uint64_t* internal_transition_lsb_mask;

} hebs_plan;

typedef struct hebs_plan_metadata_s
{
	uint64_t plan_hash;
	uint32_t level_count;
	uint32_t num_primary_inputs;
	uint32_t num_primary_outputs;
	uint32_t signal_count;
	uint32_t gate_count;
	uint32_t tray_count;
	uint32_t propagation_depth;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	double fanout_avg;
	uint32_t dff_exec_count;
	uint32_t comb_instruction_count;

} hebs_plan_metadata;

typedef struct hebs_run_status_s
{
	hebs_status_t last_status;
	uint64_t current_tick;
	uint64_t cycles_executed;
	uint64_t vectors_applied;
	uint32_t tray_count;
	uint8_t test_probes_enabled;

} hebs_run_status;

typedef struct hebs_engine_s
{
	uint64_t current_tick;
	uint32_t tray_count;
	uint32_t net_count;
	HEBS_ALIGN64 uint64_t tray_plane_a[HEBS_MAX_SIGNAL_TRAYS];
	HEBS_ALIGN64 uint64_t tray_plane_b[HEBS_MAX_SIGNAL_TRAYS];
	HEBS_ALIGN64 uint64_t dff_state_trays[HEBS_MAX_SIGNAL_TRAYS];
	HEBS_ALIGN64 uint8_t net_physical[HEBS_MAX_SIGNALS];
	HEBS_ALIGN64 uint8_t pending_state[HEBS_MAX_SIGNALS];
	uint32_t dirty_net_ids[HEBS_MAX_SIGNALS];
	uint8_t dirty_net_flags[HEBS_MAX_SIGNALS];
	uint32_t dirty_count;
	uint64_t* signal_trays;
	uint64_t* next_signal_trays;
	uint64_t cycles_executed;
	uint64_t vectors_applied;
	hebs_status_t last_status;
	uint64_t probe_input_apply;
	uint64_t probe_input_toggle;
	uint64_t probe_chunk_exec;
	uint64_t probe_gate_eval;
	uint64_t probe_state_change_commit;
	uint64_t probe_dff_exec;

} hebs_engine;

hebs_plan* hebs_load_bench(const char* file_path);
void hebs_free_plan(hebs_plan* plan);

hebs_status_t hebs_init_engine(hebs_engine* ctx, hebs_plan* plan);
void hebs_tick(hebs_engine* ctx, hebs_plan* plan);
hebs_probes hebs_get_probes(const hebs_engine* ctx);
int hebs_get_run_status(const hebs_engine* ctx, hebs_run_status* out_status);
uint64_t hebs_get_state_hash(hebs_engine* ctx);
uint32_t hebs_get_final_crc32(const hebs_engine* ctx);
uint64_t hebs_get_plan_hash(const hebs_plan* plan);
int hebs_get_plan_metadata(const hebs_plan* plan, hebs_plan_metadata* out_metadata);
hebs_status_t hebs_set_primary_input(hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index, hebs_logic_t value);
uint8_t hebs_get_primary_input_state(const hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index);
hebs_logic_t hebs_get_primary_input(const hebs_engine* ctx, const hebs_plan* plan, uint32_t input_index);
const uint64_t* hebs_get_signal_tray(const hebs_engine* ctx, uint32_t tray_index);

#endif
