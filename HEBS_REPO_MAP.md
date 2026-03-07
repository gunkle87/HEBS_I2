1 Purpose

This file is a technical reference appendix for the engine and supporting files.
This file documents named engine elements.
This file contains no historical, governance, or bibliographic information.

2 Governance and Canonical Documents

AGENTS.md
Purpose: agent execution policy reference
Location: AGENTS.md
+
TAB_PROTOCOL.md
Purpose: test and benchmark creation and constraint protocols
Location: TAB_PROTOCOL.md
+
PROBE_PROTOCOL.md
Purpose: probe boundary and probe profile technical protocol
Location: PROBE_PROTOCOL.md
+
BENCH_METRICS.md
Purpose: benchmark metric definitions and formulas used by the runner
Location: BENCH_METRICS.md

3 Directory Structure

root
benchmarks
benchmarks/benches
benchmarks/benches/ISCAS85
benchmarks/benches/ISCAS89
benchmarks/results
benchmarks/results/archive
build
core
include
tests
tests/results

4 File Index

engine.c
Purpose: simulation tick execution, combinational execution paths, and probe snapshot export
Location: core/engine.c

loader.c
Purpose: bench netlist parsing, levelization, and execution plan construction
Location: core/loader.c

primitives.c
Purpose: scalar and tray level primitive logic evaluation functions
Location: core/primitives.c

state_manager.c
Purpose: sequential commit pipeline for DFF state propagation
Location: core/state_manager.c

hebs_engine.h
Purpose: public engine data model, enums, plans, probes, and main engine API declarations
Location: include/hebs_engine.h

hebs_probe_profile.h
Purpose: compile time probe profile selection and compatibility probe gate
Location: include/hebs_probe_profile.h

primitives.h
Purpose: primitive logic function declarations used by engine execution code
Location: include/primitives.h

state_manager.h
Purpose: sequential commit interface declaration
Location: include/state_manager.h

runner.c
Purpose: benchmark suite execution and metric derivation outside engine
Location: benchmarks/runner.c

csv_export.h
Purpose: csv ledger write helpers for benchmark runs
Location: benchmarks/csv_export.h

html_report.h
Purpose: html benchmark report generation helpers
Location: benchmarks/html_report.h

protocol_helper.h
Purpose: benchmark protocol helpers and summary helpers
Location: benchmarks/protocol_helper.h

report_types.h
Purpose: report data structures for benchmark output
Location: benchmarks/report_types.h

timing_helper.h
Purpose: timing and clock utilities for benchmark measurement
Location: benchmarks/timing_helper.h

test_runner.c
Purpose: engine validation test suite and protocol checks
Location: tests/test_runner.c

Makefile
Purpose: build targets for engine, test binary, and benchmark binaries
Location: Makefile

5 Public API Registry

hebs_load_bench
Kind: function
Use: parse a bench file and build a simulation plan
Location: core/loader.c:1293

hebs_free_plan
Kind: function
Use: free all dynamic allocations stored in a simulation plan
Location: core/loader.c:1421

hebs_init_engine
Kind: function
Use: initialize engine state from a plan and reset probes
Location: core/engine.c:716

hebs_tick
Kind: function
Use: execute one simulation tick with combinational and sequential phases
Location: core/engine.c:757

hebs_get_probes
Kind: function
Use: return a raw probe snapshot from engine state
Location: core/engine.c:784

hebs_get_state_hash
Kind: function
Use: compute hash of current engine trays and tick
Location: core/engine.c:807

hebs_set_primary_input
Kind: function
Use: apply primary input value to engine tray storage
Location: core/engine.c:833

hebs_get_primary_input
Kind: function
Use: read primary input value from engine state
Location: core/engine.c:869

hebs_get_signal_tray
Kind: function
Use: expose tray pointer for read access by tray index
Location: core/engine.c:886

hebs_resolve_states
Kind: function
Use: resolve two logic states through resolution lookup
Location: core/primitives.c:27

hebs_gate_and_simd
Kind: function
Use: evaluate and on packed tray lanes
Location: core/primitives.c:33

hebs_gate_or_simd
Kind: function
Use: evaluate or on packed tray lanes
Location: core/primitives.c:45

hebs_gate_xor_simd
Kind: function
Use: evaluate xor on packed tray lanes
Location: core/primitives.c:57

hebs_gate_nand_simd
Kind: function
Use: evaluate nand on packed tray lanes
Location: core/primitives.c:69

hebs_gate_nor_simd
Kind: function
Use: evaluate nor on packed tray lanes
Location: core/primitives.c:75

hebs_gate_xnor_simd
Kind: function
Use: evaluate xnor on packed tray lanes
Location: core/primitives.c:81

hebs_gate_not_simd
Kind: function
Use: evaluate not on packed tray lanes
Location: core/primitives.c:87

hebs_gate_buf_simd
Kind: function
Use: pass through tray lane values
Location: core/primitives.c:95

hebs_gate_weak_pull_simd
Kind: function
Use: apply weak pull behavior on packed tray lanes
Location: core/primitives.c:101

hebs_gate_weak_pull_down_simd
Kind: function
Use: apply weak pull down behavior on packed tray lanes
Location: core/primitives.c:109

hebs_gate_strong_pull_simd
Kind: function
Use: apply strong pull behavior on packed tray lanes
Location: core/primitives.c:115

hebs_gate_vcc_simd
Kind: function
Use: generate constant high tray lane value
Location: core/primitives.c:122

hebs_gate_gnd_simd
Kind: function
Use: generate constant low tray lane value
Location: core/primitives.c:128

hebs_gate_tristate_simd
Kind: function
Use: apply tristate enable mask to tray data
Location: core/primitives.c:134

hebs_eval_and
Kind: function
Use: scalar and evaluation for fallback path
Location: core/primitives.c:148

hebs_eval_or
Kind: function
Use: scalar or evaluation for fallback path
Location: core/primitives.c:166

hebs_eval_xor
Kind: function
Use: scalar xor evaluation for fallback path
Location: core/primitives.c:184

hebs_eval_nand
Kind: function
Use: scalar nand evaluation for fallback path
Location: core/primitives.c:208

hebs_eval_nor
Kind: function
Use: scalar nor evaluation for fallback path
Location: core/primitives.c:214

hebs_eval_xnor
Kind: function
Use: scalar xnor evaluation for fallback path
Location: core/primitives.c:220

hebs_eval_not
Kind: function
Use: scalar not evaluation for fallback path
Location: core/primitives.c:226

hebs_eval_buf
Kind: function
Use: scalar buffer evaluation for fallback path
Location: core/primitives.c:252

hebs_eval_weak_pull
Kind: function
Use: scalar weak pull evaluation
Location: core/primitives.c:258

hebs_eval_weak_pull_down
Kind: function
Use: scalar weak pull down evaluation
Location: core/primitives.c:280

hebs_eval_strong_pull
Kind: function
Use: scalar strong pull evaluation
Location: core/primitives.c:286

hebs_eval_vcc
Kind: function
Use: scalar constant high source
Location: core/primitives.c:307

hebs_eval_gnd
Kind: function
Use: scalar constant low source
Location: core/primitives.c:313

hebs_eval_tristate
Kind: function
Use: scalar tristate behavior evaluation
Location: core/primitives.c:319

hebs_sequential_commit
Kind: function
Use: commit staged DFF results into state trays and active trays
Location: core/state_manager.c:113

6 Type Registry

hebs_logic_e
Kind: enum
Use: logic state tag for eight logic states
Location: include/hebs_engine.h:19

hebs_logic_t
Kind: typedef
Use: logic state type used across engine APIs and trays
Location: include/hebs_engine.h:30

hebs_status_e
Kind: enum
Use: status codes returned by engine API functions
Location: include/hebs_engine.h:32

hebs_status_t
Kind: typedef
Use: status code type
Location: include/hebs_engine.h:37

hebs_gate_type_e
Kind: enum
Use: gate opcode identifiers used in plan instructions
Location: include/hebs_engine.h:39

hebs_gate_type_t
Kind: typedef
Use: gate opcode type
Location: include/hebs_engine.h:56

hebs_lep_instruction_s
Kind: struct
Use: packed logical execution plan instruction record
Location: include/hebs_engine.h:60

hebs_lep_instruction_t
Kind: typedef
Use: alias for packed logical execution plan instruction
Location: include/hebs_engine.h:69

hebs_exec_instruction_s
Kind: struct
Use: executable tray level instruction record
Location: include/hebs_engine.h:71

hebs_exec_instruction_t
Kind: typedef
Use: alias for executable tray level instruction
Location: include/hebs_engine.h:82

hebs_gate_span_s
Kind: struct
Use: contiguous span descriptor for batched gate execution
Location: include/hebs_engine.h:84

hebs_gate_span_t
Kind: typedef
Use: alias for gate span descriptor
Location: include/hebs_engine.h:93

hebs_plan_s
Kind: struct
Use: immutable simulation plan with indexed execution data
Location: include/hebs_engine.h:95

hebs_plan
Kind: typedef
Use: alias for simulation plan container
Location: include/hebs_engine.h:123

hebs_probes_s
Kind: struct
Use: raw probe snapshot layout
Location: include/hebs_engine.h:125

hebs_probes
Kind: typedef
Use: alias for raw probe snapshot type
Location: include/hebs_engine.h:136

hebs_engine_s
Kind: struct
Use: main engine context and tray storage container
Location: include/hebs_engine.h:138

hebs_engine
Kind: typedef
Use: alias for engine context type
Location: include/hebs_engine.h:158

hebs_signal_table_s
Kind: struct
Use: loader symbol table for bench net names
Location: core/loader.c:7

hebs_signal_table_t
Kind: typedef
Use: alias for loader symbol table
Location: core/loader.c:13

hebs_gate_raw_s
Kind: struct
Use: loader gate record before packed execution plan generation
Location: core/loader.c:15

hebs_gate_raw_t
Kind: typedef
Use: alias for loader gate record
Location: core/loader.c:26

hebs_comb_locality_entry_s
Kind: struct
Use: temporary locality key record for combinational packing
Location: core/loader.c:924

hebs_comb_locality_entry_t
Kind: typedef
Use: alias for combinational locality key record
Location: core/loader.c:934

7 Constants and Macros

HEBS_ENGINE_H
Kind: macro
Use: include guard for engine header
Location: include/hebs_engine.h:2

HEBS_PRIMITIVES_H
Kind: macro
Use: include guard for primitives header
Location: include/primitives.h:2

HEBS_STATE_MANAGER_H
Kind: macro
Use: include guard for state manager header
Location: include/state_manager.h:2

HEBS_PROBE_PROFILE_H
Kind: macro
Use: include guard for probe profile header
Location: include/hebs_probe_profile.h:2

HEBS_MAX_PRIMARY_INPUTS
Kind: macro
Use: maximum supported primary inputs in plan and engine
Location: include/hebs_engine.h:7

HEBS_MAX_SIGNAL_TRAYS
Kind: macro
Use: maximum tray count for fixed tray arrays
Location: include/hebs_engine.h:8

HEBS_BATCH_GATE_CHUNK
Kind: macro
Use: chunk size for batched combinational execution
Location: include/hebs_engine.h:10

HEBS_ALIGN64
Kind: macro
Use: alignment annotation for tray arrays
Location: include/hebs_engine.h:14

HEBS_COMB_GATE_TYPE_COUNT
Kind: macro
Use: gate order table length for combinational packing
Location: include/hebs_engine.h:58

HEBS_PROBE_PROFILE_PERF
Kind: macro
Use: default probe profile selector for perf mode
Location: include/hebs_probe_profile.h:9

HEBS_COMPAT_PROBES_ENABLED
Kind: macro
Use: compile time gate for compatibility probe updates
Location: include/hebs_probe_profile.h:13

HEBS_FLUSH_CHUNK_PROBES
Kind: macro
Use: flush chunk level probe counters after batched chunk execution
Location: core/engine.c:91

HEBS_PLANE_LOW_MASK
Kind: macro
Use: bit mask for low plane bits in packed trays
Location: core/primitives.c:4

HEBS_PLANE_HIGH_MASK
Kind: macro
Use: bit mask for high plane bits in packed trays
Location: core/primitives.c:5

HEBS_DFF_SCALAR_THRESHOLD
Kind: macro
Use: threshold to switch sequential commit scalar path and vectorized path
Location: core/state_manager.c:4

HEBS_LUT_RESOLVE
Kind: constant
Use: logic resolution lookup table for scalar state merge
Location: core/primitives.c:8

HEBS_COMB_GATE_ORDER
Kind: constant
Use: gate ordering table used by combinational packing
Location: core/loader.c:989

HEBS_S0
Kind: constant
Use: strong zero logic state enum value
Location: include/hebs_engine.h:21

HEBS_S1
Kind: constant
Use: strong one logic state enum value
Location: include/hebs_engine.h:22

HEBS_W0
Kind: constant
Use: weak zero logic state enum value
Location: include/hebs_engine.h:23

HEBS_W1
Kind: constant
Use: weak one logic state enum value
Location: include/hebs_engine.h:24

HEBS_Z
Kind: constant
Use: high impedance logic state enum value
Location: include/hebs_engine.h:25

HEBS_X
Kind: constant
Use: unknown logic state enum value
Location: include/hebs_engine.h:26

HEBS_SX
Kind: constant
Use: strong unknown logic state enum value
Location: include/hebs_engine.h:27

HEBS_WX
Kind: constant
Use: weak unknown logic state enum value
Location: include/hebs_engine.h:28

HEBS_OK
Kind: constant
Use: success status enum value
Location: include/hebs_engine.h:34

HEBS_ERR_LOGIC
Kind: constant
Use: logic error status enum value
Location: include/hebs_engine.h:35

HEBS_GATE_AND
Kind: constant
Use: and gate opcode enum value
Location: include/hebs_engine.h:41

HEBS_GATE_OR
Kind: constant
Use: or gate opcode enum value
Location: include/hebs_engine.h:42

HEBS_GATE_NOT
Kind: constant
Use: not gate opcode enum value
Location: include/hebs_engine.h:43

HEBS_GATE_NAND
Kind: constant
Use: nand gate opcode enum value
Location: include/hebs_engine.h:44

HEBS_GATE_NOR
Kind: constant
Use: nor gate opcode enum value
Location: include/hebs_engine.h:45

HEBS_GATE_BUF
Kind: constant
Use: buffer gate opcode enum value
Location: include/hebs_engine.h:46

HEBS_GATE_DFF
Kind: constant
Use: dff gate opcode enum value
Location: include/hebs_engine.h:47

HEBS_GATE_XOR
Kind: constant
Use: xor gate opcode enum value
Location: include/hebs_engine.h:48

HEBS_GATE_XNOR
Kind: constant
Use: xnor gate opcode enum value
Location: include/hebs_engine.h:49

HEBS_GATE_TRI
Kind: constant
Use: tristate gate opcode enum value
Location: include/hebs_engine.h:50

HEBS_GATE_VCC
Kind: constant
Use: constant high gate opcode enum value
Location: include/hebs_engine.h:51

HEBS_GATE_GND
Kind: constant
Use: constant low gate opcode enum value
Location: include/hebs_engine.h:52

HEBS_GATE_PUP
Kind: constant
Use: weak pull up gate opcode enum value
Location: include/hebs_engine.h:53

HEBS_GATE_PDN
Kind: constant
Use: weak pull down gate opcode enum value
Location: include/hebs_engine.h:54

8 Important Variables and Fields

Global variable set
Kind: variable
Use: no writable global variables are defined in core or include
Parent: none
Location: core/engine.c:1

hebs_signal_table_s.names
Kind: field
Use: dynamic array of signal name pointers in loader table
Parent: hebs_signal_table_s
Location: core/loader.c:9

hebs_signal_table_s.count
Kind: field
Use: current signal table size
Parent: hebs_signal_table_s
Location: core/loader.c:10

hebs_signal_table_s.capacity
Kind: field
Use: allocated signal table capacity
Parent: hebs_signal_table_s
Location: core/loader.c:11

hebs_gate_raw_s.type
Kind: field
Use: parsed gate type before packing
Parent: hebs_gate_raw_s
Location: core/loader.c:17

hebs_gate_raw_s.src_a
Kind: field
Use: source signal id for input A
Parent: hebs_gate_raw_s
Location: core/loader.c:18

hebs_gate_raw_s.src_b
Kind: field
Use: source signal id for input B
Parent: hebs_gate_raw_s
Location: core/loader.c:19

hebs_gate_raw_s.dst
Kind: field
Use: destination signal id
Parent: hebs_gate_raw_s
Location: core/loader.c:20

hebs_gate_raw_s.level
Kind: field
Use: computed levelization depth for gate
Parent: hebs_gate_raw_s
Location: core/loader.c:21

hebs_gate_raw_s.input_count
Kind: field
Use: gate arity for parser and planner
Parent: hebs_gate_raw_s
Location: core/loader.c:22

hebs_gate_raw_s.is_stateful
Kind: field
Use: flag for sequential gate behavior
Parent: hebs_gate_raw_s
Location: core/loader.c:23

hebs_gate_raw_s.parse_ok
Kind: field
Use: parser validity status for gate record
Parent: hebs_gate_raw_s
Location: core/loader.c:24

hebs_comb_locality_entry_s.instr_idx
Kind: field
Use: instruction index key for locality sort
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:926

hebs_comb_locality_entry_s.src_a_tray
Kind: field
Use: tray index key for source A locality
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:927

hebs_comb_locality_entry_s.src_b_tray
Kind: field
Use: tray index key for source B locality
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:928

hebs_comb_locality_entry_s.dst_tray
Kind: field
Use: tray index key for destination locality
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:929

hebs_comb_locality_entry_s.src_a_shift
Kind: field
Use: lane shift key for source A
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:930

hebs_comb_locality_entry_s.src_b_shift
Kind: field
Use: lane shift key for source B
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:931

hebs_comb_locality_entry_s.dst_shift
Kind: field
Use: lane shift key for destination
Parent: hebs_comb_locality_entry_s
Location: core/loader.c:932

hebs_lep_instruction_s.gate_type
Kind: field
Use: gate opcode for packed logical instruction
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:62

hebs_lep_instruction_s.input_count
Kind: field
Use: logical input count for instruction
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:63

hebs_lep_instruction_s.level
Kind: field
Use: level index used by packed execution ordering
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:64

hebs_lep_instruction_s.src_a_bit_offset
Kind: field
Use: bit offset for first source signal lane
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:65

hebs_lep_instruction_s.src_b_bit_offset
Kind: field
Use: bit offset for second source signal lane
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:66

hebs_lep_instruction_s.dst_bit_offset
Kind: field
Use: bit offset for destination signal lane
Parent: hebs_lep_instruction_s
Location: include/hebs_engine.h:67

hebs_exec_instruction_s.gate_type
Kind: field
Use: gate opcode for executable tray instruction
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:73

hebs_exec_instruction_s.src_a_shift
Kind: field
Use: packed lane shift for source A
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:74

hebs_exec_instruction_s.src_b_shift
Kind: field
Use: packed lane shift for source B
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:75

hebs_exec_instruction_s.dst_shift
Kind: field
Use: packed lane shift for destination
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:76

hebs_exec_instruction_s.src_a_tray
Kind: field
Use: tray index for source A
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:77

hebs_exec_instruction_s.src_b_tray
Kind: field
Use: tray index for source B
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:78

hebs_exec_instruction_s.dst_tray
Kind: field
Use: tray index for destination
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:79

hebs_exec_instruction_s.dst_mask
Kind: field
Use: lane mask for destination write
Parent: hebs_exec_instruction_s
Location: include/hebs_engine.h:80

hebs_gate_span_s.start
Kind: field
Use: start index into combinational execution arrays
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:86

hebs_gate_span_s.count
Kind: field
Use: span element count for one gate type chunk group
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:87

hebs_gate_span_s.gate_type
Kind: field
Use: gate opcode for span dispatch
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:88

hebs_gate_span_s.reserved0
Kind: field
Use: reserved alignment field
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:89

hebs_gate_span_s.reserved1
Kind: field
Use: reserved alignment field
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:90

hebs_gate_span_s.reserved2
Kind: field
Use: reserved alignment field
Parent: hebs_gate_span_s
Location: include/hebs_engine.h:91

hebs_plan_s.lep_hash
Kind: field
Use: hash of packed plan instruction content
Parent: hebs_plan_s
Location: include/hebs_engine.h:97

hebs_plan_s.level_count
Kind: field
Use: number of execution levels in plan
Parent: hebs_plan_s
Location: include/hebs_engine.h:98

hebs_plan_s.num_primary_inputs
Kind: field
Use: number of primary input signals
Parent: hebs_plan_s
Location: include/hebs_engine.h:99

hebs_plan_s.num_primary_outputs
Kind: field
Use: number of primary output signals
Parent: hebs_plan_s
Location: include/hebs_engine.h:100

hebs_plan_s.signal_count
Kind: field
Use: total signal count
Parent: hebs_plan_s
Location: include/hebs_engine.h:101

hebs_plan_s.gate_count
Kind: field
Use: total gate count
Parent: hebs_plan_s
Location: include/hebs_engine.h:102

hebs_plan_s.tray_count
Kind: field
Use: tray count derived from signal count
Parent: hebs_plan_s
Location: include/hebs_engine.h:103

hebs_plan_s.max_level
Kind: field
Use: deepest level index in plan
Parent: hebs_plan_s
Location: include/hebs_engine.h:104

hebs_plan_s.propagation_depth
Kind: field
Use: propagation depth metric from levelization
Parent: hebs_plan_s
Location: include/hebs_engine.h:105

hebs_plan_s.fanout_avg
Kind: field
Use: average fanout value computed in loader
Parent: hebs_plan_s
Location: include/hebs_engine.h:106

hebs_plan_s.fanout_max
Kind: field
Use: maximum fanout value computed in loader
Parent: hebs_plan_s
Location: include/hebs_engine.h:107

hebs_plan_s.total_fanout_edges
Kind: field
Use: total fanout edge count computed in loader
Parent: hebs_plan_s
Location: include/hebs_engine.h:108

hebs_plan_s.primary_input_ids
Kind: field
Use: signal id array for primary inputs
Parent: hebs_plan_s
Location: include/hebs_engine.h:109

hebs_plan_s.lep_data
Kind: field
Use: packed logical instruction array
Parent: hebs_plan_s
Location: include/hebs_engine.h:110

hebs_plan_s.dff_instruction_count
Kind: field
Use: number of DFF instructions in plan
Parent: hebs_plan_s
Location: include/hebs_engine.h:111

hebs_plan_s.dff_instruction_indices
Kind: field
Use: indices for DFF instructions in logical plan
Parent: hebs_plan_s
Location: include/hebs_engine.h:112

hebs_plan_s.dff_exec_count
Kind: field
Use: number of executable DFF instructions
Parent: hebs_plan_s
Location: include/hebs_engine.h:113

hebs_plan_s.dff_exec_data
Kind: field
Use: executable DFF instruction array
Parent: hebs_plan_s
Location: include/hebs_engine.h:114

hebs_plan_s.dff_commit_mask
Kind: field
Use: tray masks used for DFF commit merge
Parent: hebs_plan_s
Location: include/hebs_engine.h:115

hebs_plan_s.comb_instruction_count
Kind: field
Use: number of combinational instructions in packed execution plan
Parent: hebs_plan_s
Location: include/hebs_engine.h:116

hebs_plan_s.comb_instruction_indices
Kind: field
Use: logical instruction indices for combinational execution order
Parent: hebs_plan_s
Location: include/hebs_engine.h:117

hebs_plan_s.comb_exec_data
Kind: field
Use: executable combinational instruction array
Parent: hebs_plan_s
Location: include/hebs_engine.h:118

hebs_plan_s.comb_span_count
Kind: field
Use: number of combinational spans for batched execution
Parent: hebs_plan_s
Location: include/hebs_engine.h:119

hebs_plan_s.comb_spans
Kind: field
Use: span table used by batched combinational dispatcher
Parent: hebs_plan_s
Location: include/hebs_engine.h:120

hebs_plan_s.internal_transition_lsb_mask
Kind: field
Use: bit mask for non primary signal transition accounting support
Parent: hebs_plan_s
Location: include/hebs_engine.h:121

hebs_probes_s.input_apply
Kind: field
Use: raw count of primary input apply operations
Parent: hebs_probes_s
Location: include/hebs_engine.h:127

hebs_probes_s.input_toggle
Kind: field
Use: raw count of input toggles when compatibility probes are enabled
Parent: hebs_probes_s
Location: include/hebs_engine.h:128

hebs_probes_s.chunk_exec
Kind: field
Use: raw count of chunk or fallback execution units
Parent: hebs_probes_s
Location: include/hebs_engine.h:129

hebs_probes_s.gate_eval
Kind: field
Use: raw count of gate evaluation writes
Parent: hebs_probes_s
Location: include/hebs_engine.h:130

hebs_probes_s.state_change_commit
Kind: field
Use: raw count of committed state changes when compatibility probes are enabled
Parent: hebs_probes_s
Location: include/hebs_engine.h:131

hebs_probes_s.dff_exec
Kind: field
Use: raw count of sequential DFF execution operations
Parent: hebs_probes_s
Location: include/hebs_engine.h:132

hebs_engine_s.current_tick
Kind: field
Use: current tick index
Parent: hebs_engine_s
Location: include/hebs_engine.h:138

hebs_engine_s.tray_count
Kind: field
Use: active tray count for current plan
Parent: hebs_engine_s
Location: include/hebs_engine.h:139

hebs_engine_s.tray_plane_a
Kind: field
Use: first tray plane storage buffer
Parent: hebs_engine_s
Location: include/hebs_engine.h:140

hebs_engine_s.tray_plane_b
Kind: field
Use: second tray plane storage buffer
Parent: hebs_engine_s
Location: include/hebs_engine.h:141

hebs_engine_s.dff_state_trays
Kind: field
Use: dedicated DFF state tray storage
Parent: hebs_engine_s
Location: include/hebs_engine.h:142

hebs_engine_s.signal_trays
Kind: field
Use: pointer to active signal tray buffer
Parent: hebs_engine_s
Location: include/hebs_engine.h:143

hebs_engine_s.next_signal_trays
Kind: field
Use: pointer to next signal tray buffer
Parent: hebs_engine_s
Location: include/hebs_engine.h:144

hebs_engine_s.cycles_executed
Kind: field
Use: total executed cycle counter
Parent: hebs_engine_s
Location: include/hebs_engine.h:145

hebs_engine_s.vectors_applied
Kind: field
Use: total applied vector counter
Parent: hebs_engine_s
Location: include/hebs_engine.h:146

hebs_engine_s.probe_input_apply
Kind: field
Use: internal probe counter for input apply operations
Parent: hebs_engine_s
Location: include/hebs_engine.h:147

hebs_engine_s.probe_input_toggle
Kind: field
Use: internal probe counter for input toggles
Parent: hebs_engine_s
Location: include/hebs_engine.h:148

hebs_engine_s.probe_chunk_exec
Kind: field
Use: internal probe counter for chunk executions
Parent: hebs_engine_s
Location: include/hebs_engine.h:149

hebs_engine_s.probe_gate_eval
Kind: field
Use: internal probe counter for gate evaluations
Parent: hebs_engine_s
Location: include/hebs_engine.h:150

hebs_engine_s.probe_state_change_commit
Kind: field
Use: internal probe counter for state change commits
Parent: hebs_engine_s
Location: include/hebs_engine.h:151

hebs_engine_s.probe_dff_exec
Kind: field
Use: internal probe counter for DFF executions
Parent: hebs_engine_s
Location: include/hebs_engine.h:152

9 Referenced Inputs and Support Files

c17.bench
Kind: benchmark input
Use: ISCAS85 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS85/c17.bench

c432.bench
Kind: benchmark input
Use: ISCAS85 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS85/c432.bench

c499.bench
Kind: benchmark input
Use: ISCAS85 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS85/c499.bench

c6288.bench
Kind: benchmark input
Use: ISCAS85 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS85/c6288.bench

c880.bench
Kind: benchmark input
Use: ISCAS85 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS85/c880.bench

s27.bench
Kind: benchmark input
Use: ISCAS89 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS89/s27.bench

s298.bench
Kind: benchmark input
Use: ISCAS89 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS89/s298.bench

s382.bench
Kind: benchmark input
Use: ISCAS89 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS89/s382.bench

s526.bench
Kind: benchmark input
Use: ISCAS89 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS89/s526.bench

s820.bench
Kind: benchmark input
Use: ISCAS89 workload input used by benchmark runner
Location: benchmarks/benches/ISCAS89/s820.bench

metrics_history.csv
Kind: benchmark ledger
Use: canonical benchmark suite history rows
Location: benchmarks/results/metrics_history.csv

metrics_history_compat.csv
Kind: benchmark ledger
Use: compatibility profile benchmark history rows
Location: benchmarks/results/metrics_history_compat.csv

10 Glossary

Gate
Logical primitive that transforms input signals into an output signal.

Node
Signal storage location addressed by signal id and packed into trays.

Tray
Packed signal storage word that carries multiple two bit logic lanes.

Tick
One simulation step that runs combinational and sequential phases.

Event
State transition effect applied by gate and sequential execution paths.

Probe
Raw counter that exposes execution facts to external tools.

Plan
Packed instruction and span data generated by bench loader.

Suite
Directory group of benchmark inputs used by runner execution.

Runner
External benchmark binary that executes workloads and derives metrics.

Compatibility profile
Build profile with compatibility probes enabled.

Performance profile
Build profile with compatibility probes disabled.
