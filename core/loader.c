#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "hebs_engine.h"

typedef struct hebs_signal_table_s
{
	char** names;
	uint32_t count;
	uint32_t capacity;

} hebs_signal_table_t;

typedef struct hebs_gate_raw_s
{
	hebs_gate_type_t type;
	uint32_t src_a;
	uint32_t src_b;
	uint32_t dst;
	uint32_t level;
	uint8_t input_count;
	uint8_t is_stateful;

} hebs_gate_raw_t;

static uint64_t hebs_hash_mix(uint64_t hash, uint64_t value)
{
	hash ^= value;
	hash *= 1099511628211ULL;
	return hash;

}

static char* hebs_strdup_local(const char* text)
{
	size_t len;
	char* copy;

	if (!text)
	{
		return NULL;

	}

	len = strlen(text);
	copy = (char*)malloc(len + 1U);
	if (!copy)
	{
		return NULL;

	}

	memcpy(copy, text, len + 1U);
	return copy;

}

static int hebs_reserve_bytes(void** ptr, uint32_t needed, uint32_t* capacity, size_t elem_size)
{
	uint32_t new_capacity;
	void* resized;

	if (needed <= *capacity)
	{
		return 1;

	}

	new_capacity = (*capacity == 0U) ? 16U : (*capacity * 2U);
	while (new_capacity < needed)
	{
		new_capacity *= 2U;

	}

	resized = realloc(*ptr, (size_t)new_capacity * elem_size);
	if (!resized)
	{
		return 0;

	}

	*ptr = resized;
	*capacity = new_capacity;
	return 1;

}

static char* hebs_trim(char* text)
{
	char* end;

	if (!text)
	{
		return text;

	}

	while (*text && isspace((unsigned char)*text))
	{
		++text;

	}

	if (*text == '\0')
	{
		return text;

	}

	end = text + strlen(text) - 1;
	while (end > text && isspace((unsigned char)*end))
	{
		*end = '\0';
		--end;

	}

	return text;

}

static void hebs_strip_comment(char* line)
{
	char* hash;

	if (!line)
	{
		return;

	}

	hash = strchr(line, '#');
	if (hash)
	{
		*hash = '\0';

	}

}

static uint32_t hebs_signal_get_or_add(hebs_signal_table_t* table, const char* name, int* ok)
{
	uint32_t idx;
	char* copy;

	if (!table || !name || !ok)
	{
		if (ok)
		{
			*ok = 0;

		}
		return 0;

	}

	for (idx = 0; idx < table->count; ++idx)
	{
		if (strcmp(table->names[idx], name) == 0)
		{
			*ok = 1;
			return idx;

		}

	}

	if (!hebs_reserve_bytes((void**)&table->names, table->count + 1U, &table->capacity, sizeof(char*)))
	{
		*ok = 0;
		return 0;

	}

	copy = hebs_strdup_local(name);
	if (!copy)
	{
		*ok = 0;
		return 0;

	}

	table->names[table->count] = copy;
	*ok = 1;
	return table->count++;

}

static int hebs_append_primary_input(uint32_t** ids, uint32_t* count, uint32_t* capacity, uint32_t value)
{
	if (!hebs_reserve_bytes((void**)ids, *count + 1U, capacity, sizeof(uint32_t)))
	{
		return 0;

	}

	(*ids)[*count] = value;
	++(*count);
	return 1;

}

static int hebs_append_unique_id(uint32_t** ids, uint32_t* count, uint32_t* capacity, uint32_t value)
{
	uint32_t idx;

	for (idx = 0U; idx < *count; ++idx)
	{
		if ((*ids)[idx] == value)
		{
			return 1;

		}

	}

	if (!hebs_reserve_bytes((void**)ids, *count + 1U, capacity, sizeof(uint32_t)))
	{
		return 0;

	}

	(*ids)[*count] = value;
	++(*count);
	return 1;

}

static int hebs_append_gate(hebs_gate_raw_t** gates, uint32_t* count, uint32_t* capacity, const hebs_gate_raw_t* gate)
{
	if (!hebs_reserve_bytes((void**)gates, *count + 1U, capacity, sizeof(hebs_gate_raw_t)))
	{
		return 0;

	}

	(*gates)[*count] = *gate;
	++(*count);
	return 1;

}

static int hebs_extract_name_in_parens(const char* text, char* out, size_t out_size)
{
	const char* open;
	const char* close;
	size_t len;

	if (!text || !out || out_size == 0U)
	{
		return 0;

	}

	open = strchr(text, '(');
	close = strrchr(text, ')');
	if (!open || !close || close <= open + 1)
	{
		return 0;

	}

	len = (size_t)(close - (open + 1));
	if (len >= out_size)
	{
		len = out_size - 1U;

	}

	memcpy(out, open + 1, len);
	out[len] = '\0';
	return 1;

}

static hebs_gate_type_t hebs_parse_gate_type(const char* op, uint8_t* input_count, uint8_t* is_stateful)
{
	*input_count = 2U;
	*is_stateful = 0U;

	if (strcmp(op, "AND") == 0)
	{
		return HEBS_GATE_AND;

	}

	if (strcmp(op, "OR") == 0)
	{
		return HEBS_GATE_OR;

	}

	if (strcmp(op, "NOT") == 0)
	{
		*input_count = 1U;
		return HEBS_GATE_NOT;

	}

	if (strcmp(op, "NAND") == 0)
	{
		return HEBS_GATE_NAND;

	}

	if (strcmp(op, "NOR") == 0)
	{
		return HEBS_GATE_NOR;

	}

	if (strcmp(op, "DFF") == 0)
	{
		*input_count = 1U;
		*is_stateful = 1U;
		return HEBS_GATE_DFF;

	}

	if (strcmp(op, "BUF") == 0)
	{
		*input_count = 1U;
		return HEBS_GATE_BUF;

	}

	*input_count = 1U;
	return HEBS_GATE_BUF;

}

static int hebs_parse_gate_line(
	char* line,
	hebs_signal_table_t* signals,
	hebs_gate_raw_t** gates,
	uint32_t* gate_count,
	uint32_t* gate_capacity)
{
	char* eq;
	char* lhs;
	char* rhs;
	char* open;
	char* close;
	char op[32];
	char src_a_name[128];
	char src_b_name[128];
	char rhs_args[256];
	char* comma;
	uint32_t dst_id;
	uint32_t src_a_id;
	uint32_t src_b_id;
	hebs_gate_raw_t gate;
	int ok;
	uint8_t input_count;
	uint8_t is_stateful;

	eq = strchr(line, '=');
	if (!eq)
	{
		return 1;

	}

	*eq = '\0';
	lhs = hebs_trim(line);
	rhs = hebs_trim(eq + 1);
	if (*lhs == '\0' || *rhs == '\0')
	{
		return 0;

	}

	open = strchr(rhs, '(');
	close = strrchr(rhs, ')');
	if (!open || !close || close <= open)
	{
		return 0;

	}

	if ((size_t)(open - rhs) >= sizeof(op))
	{
		return 0;

	}

	memcpy(op, rhs, (size_t)(open - rhs));
	op[open - rhs] = '\0';
	{
		char* op_trimmed = hebs_trim(op);
		memmove(op, op_trimmed, strlen(op_trimmed) + 1U);
	}

	if ((size_t)(close - (open + 1)) >= sizeof(rhs_args))
	{
		return 0;

	}

	memcpy(rhs_args, open + 1, (size_t)(close - (open + 1)));
	rhs_args[close - (open + 1)] = '\0';
	comma = strchr(rhs_args, ',');

	memset(&gate, 0, sizeof(gate));
	gate.type = hebs_parse_gate_type(op, &input_count, &is_stateful);
	gate.input_count = input_count;
	gate.is_stateful = is_stateful;

	dst_id = hebs_signal_get_or_add(signals, lhs, &ok);
	if (!ok)
	{
		return 0;

	}

	if (comma)
	{
		char* trimmed_a;
		char* trimmed_b;
		size_t len_a;
		size_t len_b;

		*comma = '\0';
		trimmed_a = hebs_trim(rhs_args);
		trimmed_b = hebs_trim(comma + 1);
		len_a = strlen(trimmed_a);
		len_b = strlen(trimmed_b);
		if (len_a >= sizeof(src_a_name) || len_b >= sizeof(src_b_name))
		{
			return 0;

		}

		memcpy(src_a_name, trimmed_a, len_a + 1U);
		memcpy(src_b_name, trimmed_b, len_b + 1U);
	}
	else
	{
		char* trimmed_a = hebs_trim(rhs_args);
		size_t len_a = strlen(trimmed_a);
		if (len_a >= sizeof(src_a_name))
		{
			return 0;

		}

		memcpy(src_a_name, trimmed_a, len_a + 1U);
		src_b_name[0] = '\0';
	}

	src_a_id = hebs_signal_get_or_add(signals, src_a_name, &ok);
	if (!ok)
	{
		return 0;

	}

	src_b_id = src_a_id;
	if (gate.input_count > 1U)
	{
		src_b_id = hebs_signal_get_or_add(signals, src_b_name, &ok);
		if (!ok)
		{
			return 0;

		}

	}

	gate.dst = dst_id;
	gate.src_a = src_a_id;
	gate.src_b = src_b_id;
	return hebs_append_gate(gates, gate_count, gate_capacity, &gate);

}

static int hebs_load_and_parse(
	const char* file_path,
	hebs_signal_table_t* signals,
	uint32_t** primary_inputs,
	uint32_t* primary_input_count,
	uint32_t* primary_input_capacity,
	uint32_t** primary_outputs,
	uint32_t* primary_output_count,
	uint32_t* primary_output_capacity,
	hebs_gate_raw_t** gates,
	uint32_t* gate_count,
	uint32_t* gate_capacity)
{
	FILE* bench_file;
	char line[512];

	bench_file = fopen(file_path, "r");
	if (!bench_file)
	{
		return 0;

	}

	while (fgets(line, (int)sizeof(line), bench_file))
	{
		char name[128];
		char* trimmed;
		int ok;
		uint32_t signal_id;

		hebs_strip_comment(line);
		trimmed = hebs_trim(line);
		if (*trimmed == '\0')
		{
			continue;

		}

		if (strncmp(trimmed, "INPUT(", 6) == 0)
		{
			if (!hebs_extract_name_in_parens(trimmed, name, sizeof(name)))
			{
				fclose(bench_file);
				return 0;

			}

			signal_id = hebs_signal_get_or_add(signals, name, &ok);
			if (!ok || !hebs_append_primary_input(primary_inputs, primary_input_count, primary_input_capacity, signal_id))
			{
				fclose(bench_file);
				return 0;

			}

			continue;

		}

		if (strncmp(trimmed, "OUTPUT(", 7) == 0)
		{
			if (!hebs_extract_name_in_parens(trimmed, name, sizeof(name)))
			{
				fclose(bench_file);
				return 0;

			}

			signal_id = hebs_signal_get_or_add(signals, name, &ok);
			if (!ok)
			{
				fclose(bench_file);
				return 0;

			}

			if (!hebs_append_unique_id(primary_outputs, primary_output_count, primary_output_capacity, signal_id))
			{
				fclose(bench_file);
				return 0;

			}

			continue;

		}

		if (!hebs_parse_gate_line(trimmed, signals, gates, gate_count, gate_capacity))
		{
			fclose(bench_file);
			return 0;

		}

	}

	fclose(bench_file);
	return 1;

}

static uint32_t hebs_levelize_and_pack(hebs_plan* plan, hebs_gate_raw_t* gates, uint32_t gate_count, uint32_t signal_count)
{
	uint32_t* net_levels;
	uint32_t* fanout_counts;
	uint32_t pass;
	uint32_t gate_idx;
	uint32_t write_idx;
	uint32_t max_level;
	uint32_t fanout_max;
	uint32_t total_fanout_edges;
	int changed;

	net_levels = (uint32_t*)calloc(signal_count, sizeof(uint32_t));
	fanout_counts = (uint32_t*)calloc(signal_count, sizeof(uint32_t));
	if (!net_levels || !fanout_counts)
	{
		free(net_levels);
		free(fanout_counts);
		return 0;

	}

	fanout_max = 0U;
	total_fanout_edges = 0U;
	for (gate_idx = 0; gate_idx < gate_count; ++gate_idx)
	{
		++fanout_counts[gates[gate_idx].src_a];
		++total_fanout_edges;
		if (fanout_counts[gates[gate_idx].src_a] > fanout_max)
		{
			fanout_max = fanout_counts[gates[gate_idx].src_a];

		}

		if (gates[gate_idx].input_count > 1U)
		{
			++fanout_counts[gates[gate_idx].src_b];
			++total_fanout_edges;
			if (fanout_counts[gates[gate_idx].src_b] > fanout_max)
			{
				fanout_max = fanout_counts[gates[gate_idx].src_b];

			}

		}

	}

	for (pass = 0; pass < gate_count + 1U; ++pass)
	{
		changed = 0;

		for (gate_idx = 0; gate_idx < gate_count; ++gate_idx)
		{
			hebs_gate_raw_t* gate = &gates[gate_idx];
			uint32_t src_a_level = net_levels[gate->src_a];
			uint32_t src_b_level = (gate->input_count > 1U) ? net_levels[gate->src_b] : src_a_level;
			uint32_t gate_level;
			uint32_t dst_level;

			if (gate->is_stateful)
			{
				gate_level = 0U;
				dst_level = 0U;
			}
			else
			{
				gate_level = (src_a_level > src_b_level ? src_a_level : src_b_level) + 1U;
				dst_level = gate_level;
			}

			gate->level = gate_level;
			if (dst_level > net_levels[gate->dst])
			{
				net_levels[gate->dst] = dst_level;
				changed = 1;

			}

		}

		if (!changed)
		{
			break;

		}

	}

	max_level = 0U;
	for (gate_idx = 0; gate_idx < gate_count; ++gate_idx)
	{
		if (gates[gate_idx].level > max_level)
		{
			max_level = gates[gate_idx].level;

		}

	}

	plan->lep_data = (hebs_lep_instruction_t*)calloc(gate_count, sizeof(hebs_lep_instruction_t));
	if (!plan->lep_data)
	{
		free(net_levels);
		return 0;

	}

	write_idx = 0U;
	for (pass = 0; pass <= max_level; ++pass)
	{
		for (gate_idx = 0; gate_idx < gate_count; ++gate_idx)
		{
			if (gates[gate_idx].level == pass)
			{
				hebs_lep_instruction_t* instr = &plan->lep_data[write_idx++];
				instr->gate_type = (uint8_t)gates[gate_idx].type;
				instr->input_count = gates[gate_idx].input_count;
				instr->level = (uint16_t)gates[gate_idx].level;
				instr->src_a_bit_offset = gates[gate_idx].src_a * 2U;
				instr->src_b_bit_offset = gates[gate_idx].src_b * 2U;
				instr->dst_bit_offset = gates[gate_idx].dst * 2U;

			}

		}

	}

	plan->gate_count = write_idx;
	plan->max_level = max_level;
	plan->level_count = max_level + 1U;
	plan->propagation_depth = max_level;
	plan->fanout_avg = (signal_count > 0U) ? ((double)total_fanout_edges / (double)signal_count) : 0.0;
	plan->fanout_max = fanout_max;
	plan->total_fanout_edges = total_fanout_edges;
	free(net_levels);
	free(fanout_counts);
	return 1;

}

static int hebs_index_dff_instructions(hebs_plan* plan)
{
	uint32_t idx;
	uint32_t count;
	uint32_t write_idx;

	if (!plan || !plan->lep_data)
	{
		return 0;

	}

	count = 0U;
	for (idx = 0U; idx < plan->gate_count; ++idx)
	{
		if ((hebs_gate_type_t)plan->lep_data[idx].gate_type == HEBS_GATE_DFF)
		{
			++count;

		}

	}

	plan->dff_instruction_count = count;
	plan->dff_instruction_indices = NULL;
	if (count == 0U)
	{
		return 1;

	}

	plan->dff_instruction_indices = (uint32_t*)calloc(count, sizeof(uint32_t));
	if (!plan->dff_instruction_indices)
	{
		plan->dff_instruction_count = 0U;
		return 0;

	}

	write_idx = 0U;
	for (idx = 0U; idx < plan->gate_count; ++idx)
	{
		if ((hebs_gate_type_t)plan->lep_data[idx].gate_type == HEBS_GATE_DFF)
		{
			plan->dff_instruction_indices[write_idx++] = idx;

		}

	}

	return 1;

}

static int hebs_build_dff_execution_plan(hebs_plan* plan)
{
	uint32_t idx;

	if (!plan)
	{
		return 0;

	}

	plan->dff_exec_count = plan->dff_instruction_count;
	plan->dff_exec_data = NULL;
	plan->dff_commit_mask = NULL;
	if (plan->dff_instruction_count == 0U)
	{
		return 1;

	}

	plan->dff_exec_data = (hebs_exec_instruction_t*)calloc(plan->dff_instruction_count, sizeof(hebs_exec_instruction_t));
	plan->dff_commit_mask = (uint64_t*)calloc(plan->tray_count, sizeof(uint64_t));
	if (!plan->dff_exec_data || !plan->dff_commit_mask)
	{
		free(plan->dff_exec_data);
		free(plan->dff_commit_mask);
		plan->dff_exec_data = NULL;
		plan->dff_commit_mask = NULL;
		plan->dff_exec_count = 0U;
		return 0;

	}

	for (idx = 0U; idx < plan->dff_instruction_count; ++idx)
	{
		const uint32_t dff_idx = plan->dff_instruction_indices[idx];
		const hebs_lep_instruction_t* instr = &plan->lep_data[dff_idx];
		hebs_exec_instruction_t* exec_instr = &plan->dff_exec_data[idx];

		exec_instr->gate_type = (uint8_t)HEBS_GATE_DFF;
		exec_instr->src_a_shift = (uint8_t)(instr->src_a_bit_offset % 64U);
		exec_instr->src_b_shift = 0U;
		exec_instr->dst_shift = (uint8_t)(instr->dst_bit_offset % 64U);
		exec_instr->src_a_tray = instr->src_a_bit_offset / 64U;
		exec_instr->src_b_tray = 0U;
		exec_instr->dst_tray = instr->dst_bit_offset / 64U;
		exec_instr->dst_mask = 0x3ULL << exec_instr->dst_shift;
		if (exec_instr->dst_tray < plan->tray_count)
		{
			plan->dff_commit_mask[exec_instr->dst_tray] |= exec_instr->dst_mask;

		}

	}

	return 1;

}

typedef struct hebs_comb_locality_entry_s
{
	uint32_t instr_idx;
	uint32_t src_a_tray;
	uint32_t src_b_tray;
	uint32_t dst_tray;
	uint8_t src_a_shift;
	uint8_t src_b_shift;
	uint8_t dst_shift;

} hebs_comb_locality_entry_t;

static int hebs_compare_comb_locality_entries(const void* lhs, const void* rhs)
{
	const hebs_comb_locality_entry_t* a = (const hebs_comb_locality_entry_t*)lhs;
	const hebs_comb_locality_entry_t* b = (const hebs_comb_locality_entry_t*)rhs;

	if (a->src_a_tray != b->src_a_tray)
	{
		return (a->src_a_tray < b->src_a_tray) ? -1 : 1;

	}

	if (a->src_b_tray != b->src_b_tray)
	{
		return (a->src_b_tray < b->src_b_tray) ? -1 : 1;

	}

	if (a->dst_tray != b->dst_tray)
	{
		return (a->dst_tray < b->dst_tray) ? -1 : 1;

	}

	if (a->src_a_shift != b->src_a_shift)
	{
		return (a->src_a_shift < b->src_a_shift) ? -1 : 1;

	}

	if (a->src_b_shift != b->src_b_shift)
	{
		return (a->src_b_shift < b->src_b_shift) ? -1 : 1;

	}

	if (a->dst_shift != b->dst_shift)
	{
		return (a->dst_shift < b->dst_shift) ? -1 : 1;

	}

	if (a->instr_idx != b->instr_idx)
	{
		return (a->instr_idx < b->instr_idx) ? -1 : 1;

	}

	return 0;

}

static int hebs_build_comb_execution_plan(hebs_plan* plan)
{
	static const uint8_t HEBS_COMB_GATE_ORDER[HEBS_COMB_GATE_TYPE_COUNT] =
	{
		(uint8_t)HEBS_GATE_AND,
		(uint8_t)HEBS_GATE_OR,
		(uint8_t)HEBS_GATE_NOT,
		(uint8_t)HEBS_GATE_NAND,
		(uint8_t)HEBS_GATE_NOR,
		(uint8_t)HEBS_GATE_BUF
	};
	uint32_t level;
	uint32_t order_idx;
	uint32_t instr_idx;
	uint32_t comb_count;
	uint32_t span_count;
	uint32_t write_idx;
	uint32_t span_idx;
	hebs_comb_locality_entry_t* local_entries;

	if (!plan || !plan->lep_data)
	{
		return 0;

	}

	comb_count = 0U;
	span_count = 0U;
	for (level = 0U; level <= plan->max_level; ++level)
	{
		for (order_idx = 0U; order_idx < HEBS_COMB_GATE_TYPE_COUNT; ++order_idx)
		{
			uint32_t local_count = 0U;
			for (instr_idx = 0U; instr_idx < plan->gate_count; ++instr_idx)
			{
				const hebs_lep_instruction_t* instr = &plan->lep_data[instr_idx];
				if (instr->level == level && instr->gate_type == HEBS_COMB_GATE_ORDER[order_idx])
				{
					++local_count;

				}

			}

			if (local_count > 0U)
			{
				comb_count += local_count;
				span_count += (local_count + (HEBS_BATCH_GATE_CHUNK - 1U)) / HEBS_BATCH_GATE_CHUNK;

			}

		}

	}

	plan->comb_instruction_count = comb_count;
	plan->comb_span_count = span_count;
	plan->comb_instruction_indices = NULL;
	plan->comb_exec_data = NULL;
	plan->comb_spans = NULL;
	local_entries = NULL;
	if (comb_count == 0U || span_count == 0U)
	{
		return 1;

	}

	plan->comb_instruction_indices = (uint32_t*)calloc(comb_count, sizeof(uint32_t));
	plan->comb_exec_data = (hebs_exec_instruction_t*)calloc(comb_count, sizeof(hebs_exec_instruction_t));
	plan->comb_spans = (hebs_gate_span_t*)calloc(span_count, sizeof(hebs_gate_span_t));
	if (!plan->comb_instruction_indices || !plan->comb_exec_data || !plan->comb_spans)
	{
		free(plan->comb_instruction_indices);
		free(plan->comb_exec_data);
		free(plan->comb_spans);
		plan->comb_instruction_indices = NULL;
		plan->comb_exec_data = NULL;
		plan->comb_spans = NULL;
		plan->comb_instruction_count = 0U;
		plan->comb_span_count = 0U;
		return 0;

	}

	local_entries = (hebs_comb_locality_entry_t*)malloc((size_t)plan->gate_count * sizeof(hebs_comb_locality_entry_t));
	if (!local_entries)
	{
		free(plan->comb_instruction_indices);
		free(plan->comb_exec_data);
		free(plan->comb_spans);
		plan->comb_instruction_indices = NULL;
		plan->comb_exec_data = NULL;
		plan->comb_spans = NULL;
		plan->comb_instruction_count = 0U;
		plan->comb_span_count = 0U;
		return 0;

	}

	write_idx = 0U;
	span_idx = 0U;
	for (level = 0U; level <= plan->max_level; ++level)
	{
		for (order_idx = 0U; order_idx < HEBS_COMB_GATE_TYPE_COUNT; ++order_idx)
		{
			const uint8_t gate_type = HEBS_COMB_GATE_ORDER[order_idx];
			const uint32_t span_start = write_idx;
			uint32_t local_count = 0U;

			for (instr_idx = 0U; instr_idx < plan->gate_count; ++instr_idx)
			{
				const hebs_lep_instruction_t* instr = &plan->lep_data[instr_idx];
				if (instr->level != level || instr->gate_type != gate_type)
				{
					continue;

				}

				local_entries[local_count].instr_idx = instr_idx;
				local_entries[local_count].src_a_shift = (uint8_t)(instr->src_a_bit_offset % 64U);
				local_entries[local_count].src_b_shift = (uint8_t)(instr->src_b_bit_offset % 64U);
				local_entries[local_count].dst_shift = (uint8_t)(instr->dst_bit_offset % 64U);
				local_entries[local_count].src_a_tray = instr->src_a_bit_offset / 64U;
				local_entries[local_count].src_b_tray = instr->src_b_bit_offset / 64U;
				local_entries[local_count].dst_tray = instr->dst_bit_offset / 64U;
				++local_count;

			}

			if (local_count > 0U)
			{
				uint32_t local_idx;
				qsort(local_entries, local_count, sizeof(local_entries[0]), hebs_compare_comb_locality_entries);

				for (local_idx = 0U; local_idx < local_count; ++local_idx)
				{
					hebs_exec_instruction_t* exec_instr = &plan->comb_exec_data[write_idx];
					const hebs_comb_locality_entry_t* local = &local_entries[local_idx];
					plan->comb_instruction_indices[write_idx] = local->instr_idx;
					exec_instr->gate_type = gate_type;
					exec_instr->src_a_shift = local->src_a_shift;
					exec_instr->src_b_shift = local->src_b_shift;
					exec_instr->dst_shift = local->dst_shift;
					exec_instr->src_a_tray = local->src_a_tray;
					exec_instr->src_b_tray = local->src_b_tray;
					exec_instr->dst_tray = local->dst_tray;
					exec_instr->dst_mask = 0x3ULL << exec_instr->dst_shift;
					++write_idx;

				}

				uint32_t chunk_start = span_start;
				uint32_t remaining = local_count;
				while (remaining > 0U)
				{
					hebs_gate_span_t* span = &plan->comb_spans[span_idx++];
					const uint32_t chunk_count = (remaining > HEBS_BATCH_GATE_CHUNK) ? HEBS_BATCH_GATE_CHUNK : remaining;
					span->start = chunk_start;
					span->count = chunk_count;
					span->gate_type = gate_type;
					chunk_start += chunk_count;
					remaining -= chunk_count;

				}

			}

		}

	}

	free(local_entries);
	return 1;

}

static int hebs_build_internal_transition_mask(hebs_plan* plan)
{
	uint8_t* is_primary_input;
	uint32_t idx;

	if (!plan)
	{
		return 0;

	}

	plan->internal_transition_lsb_mask = NULL;
	if (plan->tray_count == 0U || plan->signal_count == 0U)
	{
		return 1;

	}

	plan->internal_transition_lsb_mask = (uint64_t*)calloc(plan->tray_count, sizeof(uint64_t));
	if (!plan->internal_transition_lsb_mask)
	{
		return 0;

	}

	is_primary_input = (uint8_t*)calloc(plan->signal_count, sizeof(uint8_t));
	if (!is_primary_input)
	{
		free(plan->internal_transition_lsb_mask);
		plan->internal_transition_lsb_mask = NULL;
		return 0;

	}

	for (idx = 0U; idx < plan->num_primary_inputs; ++idx)
	{
		uint32_t signal_id = plan->primary_input_ids[idx];
		if (signal_id < plan->signal_count)
		{
			is_primary_input[signal_id] = 1U;

		}

	}

	for (idx = 0U; idx < plan->signal_count; ++idx)
	{
		uint32_t bit_offset;
		uint32_t tray_index;
		uint32_t bit_position;

		if (is_primary_input[idx])
		{
			continue;

		}

		bit_offset = idx * 2U;
		tray_index = bit_offset / 64U;
		bit_position = bit_offset % 64U;
		if (tray_index < plan->tray_count)
		{
			plan->internal_transition_lsb_mask[tray_index] |= (1ULL << bit_position);

		}

	}

	free(is_primary_input);
	return 1;

}

static uint64_t hebs_calculate_lep_hash(const hebs_plan* plan)
{
	uint64_t hash;
	uint32_t idx;

	hash = 1469598103934665603ULL;
	hash = hebs_hash_mix(hash, plan->signal_count);
	hash = hebs_hash_mix(hash, plan->gate_count);
	hash = hebs_hash_mix(hash, plan->num_primary_inputs);
	hash = hebs_hash_mix(hash, plan->level_count);

	for (idx = 0; idx < plan->gate_count; ++idx)
	{
		const hebs_lep_instruction_t* instr = &plan->lep_data[idx];
		hash = hebs_hash_mix(hash, instr->gate_type);
		hash = hebs_hash_mix(hash, instr->input_count);
		hash = hebs_hash_mix(hash, instr->level);
		hash = hebs_hash_mix(hash, instr->src_a_bit_offset);
		hash = hebs_hash_mix(hash, instr->src_b_bit_offset);
		hash = hebs_hash_mix(hash, instr->dst_bit_offset);

	}

	return hash;

}

static void hebs_free_signal_table(hebs_signal_table_t* table)
{
	uint32_t idx;

	if (!table)
	{
		return;

	}

	for (idx = 0; idx < table->count; ++idx)
	{
		free(table->names[idx]);

	}

	free(table->names);
	table->names = NULL;
	table->count = 0;
	table->capacity = 0;

}

hebs_plan* hebs_load_bench(const char* file_path)
{
	hebs_plan* plan;
	hebs_signal_table_t signals;
	uint32_t* primary_inputs;
	uint32_t primary_input_count;
	uint32_t primary_input_capacity;
	uint32_t* primary_outputs;
	uint32_t primary_output_count;
	uint32_t primary_output_capacity;
	hebs_gate_raw_t* gates;
	uint32_t gate_count;
	uint32_t gate_capacity;

	if (!file_path)
	{
		return NULL;

	}

	memset(&signals, 0, sizeof(signals));
	primary_inputs = NULL;
	primary_input_count = 0U;
	primary_input_capacity = 0U;
	primary_outputs = NULL;
	primary_output_count = 0U;
	primary_output_capacity = 0U;
	gates = NULL;
	gate_count = 0U;
	gate_capacity = 0U;

	if (!hebs_load_and_parse(
		file_path,
		&signals,
		&primary_inputs,
		&primary_input_count,
		&primary_input_capacity,
		&primary_outputs,
		&primary_output_count,
		&primary_output_capacity,
		&gates,
		&gate_count,
		&gate_capacity))
	{
		hebs_free_signal_table(&signals);
		free(primary_inputs);
		free(primary_outputs);
		free(gates);
		return NULL;

	}

	plan = (hebs_plan*)calloc(1U, sizeof(hebs_plan));
	if (!plan)
	{
		hebs_free_signal_table(&signals);
		free(primary_inputs);
		free(primary_outputs);
		free(gates);
		return NULL;

	}

	plan->signal_count = signals.count;
	plan->num_primary_inputs = primary_input_count;
	plan->num_primary_outputs = primary_output_count;
	plan->tray_count = (signals.count + 31U) / 32U;
	plan->gate_count = 0U;
	plan->primary_input_ids = primary_inputs;

	if (!hebs_levelize_and_pack(plan, gates, gate_count, signals.count))
	{
		hebs_free_signal_table(&signals);
		free(primary_outputs);
		free(gates);
		hebs_free_plan(plan);
		return NULL;

	}

	if (!hebs_index_dff_instructions(plan))
	{
		hebs_free_signal_table(&signals);
		free(primary_outputs);
		free(gates);
		hebs_free_plan(plan);
		return NULL;

	}

	if (!hebs_build_dff_execution_plan(plan))
	{
		hebs_free_signal_table(&signals);
		free(primary_outputs);
		free(gates);
		hebs_free_plan(plan);
		return NULL;

	}

	if (!hebs_build_comb_execution_plan(plan))
	{
		hebs_free_signal_table(&signals);
		free(primary_outputs);
		free(gates);
		hebs_free_plan(plan);
		return NULL;

	}

	if (!hebs_build_internal_transition_mask(plan))
	{
		hebs_free_signal_table(&signals);
		free(primary_outputs);
		free(gates);
		hebs_free_plan(plan);
		return NULL;

	}

	plan->lep_hash = hebs_calculate_lep_hash(plan);
	hebs_free_signal_table(&signals);
	free(primary_outputs);
	free(gates);
	return plan;

}

void hebs_free_plan(hebs_plan* plan)
{
	if (!plan)
	{
		return;

	}

	free(plan->primary_input_ids);
	free(plan->lep_data);
	free(plan->dff_instruction_indices);
	free(plan->dff_exec_data);
	free(plan->dff_commit_mask);
	free(plan->comb_instruction_indices);
	free(plan->comb_exec_data);
	free(plan->comb_spans);
	free(plan->internal_transition_lsb_mask);
	free(plan);

}
