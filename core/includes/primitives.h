#ifndef HEBS_PRIMITIVES_H
#define HEBS_PRIMITIVES_H

#include "hebs_engine.h"

hebs_logic_t hebs_resolve_states(hebs_logic_t a, hebs_logic_t b);
uint64_t hebs_gate_and_simd(uint64_t tray_a, uint64_t tray_b);
uint64_t hebs_gate_or_simd(uint64_t tray_a, uint64_t tray_b);
hebs_logic_t hebs_eval_and(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_or(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_not(hebs_logic_t a);

#endif
