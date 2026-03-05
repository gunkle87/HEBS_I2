#ifndef HEBS_PRIMITIVES_H
#define HEBS_PRIMITIVES_H

#include "hebs_engine.h"

hebs_logic_t hebs_resolve_states(hebs_logic_t a, hebs_logic_t b);
uint64_t hebs_gate_and_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_or_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_xor_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_nand_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_nor_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_xnor_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_not_simd(uint64_t tray_a);
uint64_t hebs_gate_buf_simd(uint64_t tray_a);
uint64_t hebs_gate_weak_pull_simd(uint64_t tray_a);
uint64_t hebs_gate_strong_pull_simd(uint64_t tray_a);
uint64_t hebs_gate_vcc_simd(void);
uint64_t hebs_gate_gnd_simd(void);
uint64_t hebs_gate_tristate_simd(uint64_t tray_data, uint64_t tray_enable);
hebs_logic_t hebs_eval_and(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_or(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_xor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_nand(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_nor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_xnor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_not(hebs_logic_t a);
hebs_logic_t hebs_eval_buf(hebs_logic_t a);
hebs_logic_t hebs_eval_weak_pull(hebs_logic_t a);
hebs_logic_t hebs_eval_strong_pull(hebs_logic_t a);
hebs_logic_t hebs_eval_vcc(void);
hebs_logic_t hebs_eval_gnd(void);
hebs_logic_t hebs_eval_tristate(hebs_logic_t data, hebs_logic_t enable);

#endif
