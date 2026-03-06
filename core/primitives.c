#include "hebs_engine.h"
#include "primitives.h"

#define HEBS_PLANE_LOW_MASK  0x5555555555555555ULL
#define HEBS_PLANE_HIGH_MASK 0xAAAAAAAAAAAAAAAAULL

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

static uint64_t hebs_pack_planes(uint64_t low_plane, uint64_t high_plane)
{
	return (low_plane & HEBS_PLANE_LOW_MASK) | (high_plane & HEBS_PLANE_HIGH_MASK);

}

hebs_logic_t hebs_resolve_states(hebs_logic_t a, hebs_logic_t b)
{
	return (hebs_logic_t)HEBS_LUT_RESOLVE[(uint8_t)a][(uint8_t)b];

}

uint64_t hebs_gate_and_simd(uint64_t tray_a, uint64_t tray_b)
{
	uint64_t a_low = tray_a & HEBS_PLANE_LOW_MASK;
	uint64_t b_low = tray_b & HEBS_PLANE_LOW_MASK;
	uint64_t a_high = tray_a & HEBS_PLANE_HIGH_MASK;
	uint64_t b_high = tray_b & HEBS_PLANE_HIGH_MASK;
	uint64_t res_low = a_low & b_low;
	uint64_t res_high = a_high | b_high;
	return hebs_pack_planes(res_low, res_high);

}

uint64_t hebs_gate_or_simd(uint64_t tray_a, uint64_t tray_b)
{
	uint64_t a_low = tray_a & HEBS_PLANE_LOW_MASK;
	uint64_t b_low = tray_b & HEBS_PLANE_LOW_MASK;
	uint64_t a_high = tray_a & HEBS_PLANE_HIGH_MASK;
	uint64_t b_high = tray_b & HEBS_PLANE_HIGH_MASK;
	uint64_t res_low = a_low | b_low;
	uint64_t res_high = a_high & b_high;
	return hebs_pack_planes(res_low, res_high);

}

uint64_t hebs_gate_xor_simd(uint64_t tray_a, uint64_t tray_b)
{
	uint64_t a_low = tray_a & HEBS_PLANE_LOW_MASK;
	uint64_t b_low = tray_b & HEBS_PLANE_LOW_MASK;
	uint64_t a_high = tray_a & HEBS_PLANE_HIGH_MASK;
	uint64_t b_high = tray_b & HEBS_PLANE_HIGH_MASK;
	uint64_t res_low = a_low ^ b_low;
	uint64_t res_high = a_high | b_high;
	return hebs_pack_planes(res_low, res_high);

}

uint64_t hebs_gate_nand_simd(uint64_t tray_a, uint64_t tray_b)
{
	return hebs_gate_not_simd(hebs_gate_and_simd(tray_a, tray_b));

}

uint64_t hebs_gate_nor_simd(uint64_t tray_a, uint64_t tray_b)
{
	return hebs_gate_not_simd(hebs_gate_or_simd(tray_a, tray_b));

}

uint64_t hebs_gate_xnor_simd(uint64_t tray_a, uint64_t tray_b)
{
	return hebs_gate_not_simd(hebs_gate_xor_simd(tray_a, tray_b));

}

uint64_t hebs_gate_not_simd(uint64_t tray_a)
{
	uint64_t res_low = (~tray_a) & HEBS_PLANE_LOW_MASK;
	uint64_t res_high = tray_a & HEBS_PLANE_HIGH_MASK;
	return hebs_pack_planes(res_low, res_high);

}

uint64_t hebs_gate_buf_simd(uint64_t tray_a)
{
	return tray_a;

}

uint64_t hebs_gate_weak_pull_simd(uint64_t tray_a)
{
	uint64_t res_low = (tray_a & HEBS_PLANE_LOW_MASK) | HEBS_PLANE_LOW_MASK;
	uint64_t res_high = tray_a & HEBS_PLANE_HIGH_MASK;
	return hebs_pack_planes(res_low, res_high);

}

uint64_t hebs_gate_weak_pull_down_simd(uint64_t tray_a)
{
	return hebs_gate_not_simd(hebs_gate_weak_pull_simd(hebs_gate_not_simd(tray_a)));

}

uint64_t hebs_gate_strong_pull_simd(uint64_t tray_a)
{
	(void)tray_a;
	return HEBS_PLANE_LOW_MASK;

}

uint64_t hebs_gate_vcc_simd(void)
{
	return HEBS_PLANE_LOW_MASK;

}

uint64_t hebs_gate_gnd_simd(void)
{
	return 0ULL;

}

uint64_t hebs_gate_tristate_simd(uint64_t tray_data, uint64_t tray_enable)
{
	uint64_t enable_low = tray_enable & HEBS_PLANE_LOW_MASK;
	uint64_t enable_pair_mask = enable_low | (enable_low << 1U);
	return tray_data & enable_pair_mask;

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

hebs_logic_t hebs_eval_xor(hebs_logic_t a, hebs_logic_t b)
{
	if (hebs_is_binary_state(a) && hebs_is_binary_state(b))
	{
		return (a != b) ? HEBS_S1 : HEBS_S0;

	}

	if (a == HEBS_SX || b == HEBS_SX)
	{
		return HEBS_SX;

	}

	if (a == HEBS_X || b == HEBS_X || a == HEBS_Z || b == HEBS_Z)
	{
		return HEBS_X;

	}

	return hebs_resolve_states(a, b);

}

hebs_logic_t hebs_eval_nand(hebs_logic_t a, hebs_logic_t b)
{
	return hebs_eval_not(hebs_eval_and(a, b));

}

hebs_logic_t hebs_eval_nor(hebs_logic_t a, hebs_logic_t b)
{
	return hebs_eval_not(hebs_eval_or(a, b));

}

hebs_logic_t hebs_eval_xnor(hebs_logic_t a, hebs_logic_t b)
{
	return hebs_eval_not(hebs_eval_xor(a, b));

}

hebs_logic_t hebs_eval_not(hebs_logic_t a)
{
	switch (a)
	{
		case HEBS_S0:
			return HEBS_S1;
		case HEBS_S1:
			return HEBS_S0;
		case HEBS_W0:
			return HEBS_W1;
		case HEBS_W1:
			return HEBS_W0;
		case HEBS_Z:
			return HEBS_Z;
		case HEBS_WX:
			return HEBS_WX;
		case HEBS_SX:
			return HEBS_SX;
		case HEBS_X:
		default:
			return HEBS_X;

	}

}

hebs_logic_t hebs_eval_buf(hebs_logic_t a)
{
	return a;

}

hebs_logic_t hebs_eval_weak_pull(hebs_logic_t a)
{
	switch (a)
	{
		case HEBS_S0:
		case HEBS_W0:
			return HEBS_W0;
		case HEBS_S1:
		case HEBS_W1:
		case HEBS_Z:
			return HEBS_W1;
		case HEBS_SX:
			return HEBS_SX;
		case HEBS_WX:
		case HEBS_X:
		default:
			return HEBS_WX;

	}

}

hebs_logic_t hebs_eval_weak_pull_down(hebs_logic_t a)
{
	return hebs_eval_not(hebs_eval_weak_pull(hebs_eval_not(a)));

}

hebs_logic_t hebs_eval_strong_pull(hebs_logic_t a)
{
	switch (a)
	{
		case HEBS_S0:
		case HEBS_W0:
			return HEBS_S0;
		case HEBS_S1:
		case HEBS_W1:
		case HEBS_Z:
			return HEBS_S1;
		case HEBS_SX:
		case HEBS_WX:
		case HEBS_X:
		default:
			return HEBS_SX;

	}

}

hebs_logic_t hebs_eval_vcc(void)
{
	return HEBS_S1;

}

hebs_logic_t hebs_eval_gnd(void)
{
	return HEBS_S0;

}

hebs_logic_t hebs_eval_tristate(hebs_logic_t data, hebs_logic_t enable)
{
	if (enable == HEBS_S1 || enable == HEBS_W1)
	{
		return data;

	}

	if (enable == HEBS_S0 || enable == HEBS_W0)
	{
		return HEBS_Z;

	}

	if (enable == HEBS_Z)
	{
		return HEBS_Z;

	}

	if (enable == HEBS_SX)
	{
		return HEBS_SX;

	}

	return HEBS_X;

}
