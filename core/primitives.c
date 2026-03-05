#include "hebs_engine.h"
#include "primitives.h"

/* HEBS 8-state contention resolution table. */
static const uint8_t HEBS_LUT_RESOLVE[8][8] =
{
	/*          S0       S1       W0       W1       Z        X        SX       WX */
	/* S0 */ { HEBS_S0, HEBS_SX, HEBS_S0, HEBS_S0, HEBS_S0, HEBS_SX, HEBS_SX, HEBS_S0 },
	/* S1 */ { HEBS_SX, HEBS_S1, HEBS_S1, HEBS_S1, HEBS_S1, HEBS_SX, HEBS_SX, HEBS_S1 },
	/* W0 */ { HEBS_S0, HEBS_S1, HEBS_W0, HEBS_WX, HEBS_W0, HEBS_X,  HEBS_SX, HEBS_WX },
	/* W1 */ { HEBS_S0, HEBS_S1, HEBS_WX, HEBS_W1, HEBS_W1, HEBS_X,  HEBS_SX, HEBS_WX },
	/* Z  */ { HEBS_S0, HEBS_S1, HEBS_W0, HEBS_W1, HEBS_Z,  HEBS_X,  HEBS_SX, HEBS_WX },
	/* X  */ { HEBS_SX, HEBS_SX, HEBS_X,  HEBS_X,  HEBS_X,  HEBS_X,  HEBS_SX, HEBS_X  },
	/* SX */ { HEBS_SX, HEBS_SX, HEBS_SX, HEBS_SX, HEBS_SX, HEBS_SX, HEBS_SX, HEBS_SX },
	/* WX */ { HEBS_S0, HEBS_S1, HEBS_WX, HEBS_WX, HEBS_WX, HEBS_X,  HEBS_SX, HEBS_WX }
};

hebs_logic_t hebs_resolve_states(hebs_logic_t a, hebs_logic_t b)
{
	return (hebs_logic_t)HEBS_LUT_RESOLVE[(uint8_t)a][(uint8_t)b];

}

uint64_t hebs_gate_and_simd(uint64_t tray_a, uint64_t tray_b)
{
	uint64_t a_low = tray_a & 0x5555555555555555ULL;
	uint64_t b_low = tray_b & 0x5555555555555555ULL;
	uint64_t a_high = tray_a & 0xAAAAAAAAAAAAAAAAULL;
	uint64_t b_high = tray_b & 0xAAAAAAAAAAAAAAAAULL;
	uint64_t res_low = a_low & b_low;
	uint64_t res_high = a_high | b_high;
	return res_low | res_high;

}

uint64_t hebs_gate_or_simd(uint64_t tray_a, uint64_t tray_b)
{
	uint64_t a_low = tray_a & 0x5555555555555555ULL;
	uint64_t b_low = tray_b & 0x5555555555555555ULL;
	uint64_t a_high = tray_a & 0xAAAAAAAAAAAAAAAAULL;
	uint64_t b_high = tray_b & 0xAAAAAAAAAAAAAAAAULL;
	uint64_t res_low = a_low | b_low;
	uint64_t res_high = a_high & b_high;
	return res_low | res_high;

}

static int hebs_is_binary_state(hebs_logic_t value)
{
	return (value == HEBS_S0 || value == HEBS_S1);

}

hebs_logic_t hebs_eval_and(hebs_logic_t a, hebs_logic_t b)
{
	if (hebs_is_binary_state(a) && hebs_is_binary_state(b))
	{
		return (a == HEBS_S1 && b == HEBS_S1) ? HEBS_S1 : HEBS_S0;

	}

	if (a == HEBS_S0 || b == HEBS_S0)
	{
		return HEBS_S0;

	}

	return hebs_resolve_states(a, b);

}

hebs_logic_t hebs_eval_or(hebs_logic_t a, hebs_logic_t b)
{
	if (hebs_is_binary_state(a) && hebs_is_binary_state(b))
	{
		return (a == HEBS_S1 || b == HEBS_S1) ? HEBS_S1 : HEBS_S0;

	}

	if (a == HEBS_S1 || b == HEBS_S1)
	{
		return HEBS_S1;

	}

	return hebs_resolve_states(a, b);

}

hebs_logic_t hebs_eval_not(hebs_logic_t a)
{
	if (a == HEBS_S0)
	{
		return HEBS_S1;

	}

	if (a == HEBS_S1)
	{
		return HEBS_S0;

	}

	return HEBS_X;

}
