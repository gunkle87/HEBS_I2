#ifndef HEBS_PRIMITIVES_H
#define HEBS_PRIMITIVES_H

#include "hebs_engine.h"

hebs_logic_t hebs_resolve_states(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_and(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_or(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_xor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_nand(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_nor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_xnor(hebs_logic_t a, hebs_logic_t b);
hebs_logic_t hebs_eval_not(hebs_logic_t a);
hebs_logic_t hebs_eval_buf(hebs_logic_t a);
hebs_logic_t hebs_eval_weak_pull(hebs_logic_t a);
hebs_logic_t hebs_eval_weak_pull_down(hebs_logic_t a);
hebs_logic_t hebs_eval_strong_pull(hebs_logic_t a);
hebs_logic_t hebs_eval_vcc(void);
hebs_logic_t hebs_eval_gnd(void);
hebs_logic_t hebs_eval_tristate(hebs_logic_t data, hebs_logic_t enable);

#endif
