/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

#include "include/disasm.h"
#include "include/sim.h"
#include "include/tty.h"
#include "include/global.h"

typedef void (*simfunc_t)(struct sim_state *, unsigned long);

static unsigned long rotate_left(unsigned long x, unsigned long y)
{
	return (x >> y) | (x << (32-y));
}
void sim_mode1_add(struct sim_state *s, unsigned long raw)
{
	union addressing_mode_1 inst;

	inst.raw = raw;
	s->reg[inst.imm32.f_rd] = s->reg[inst.imm32.f_rn] + rotate_left(inst.imm32.f_immed_8, inst.imm32.f_rotate_imm << 1);
//	fake_printk("rd=%x rn=%x imm8=%x rot=%x\r\n", inst.imm32.f_rd, inst.imm32.f_rn, inst.imm32.f_immed_8, inst.imm32.f_rotate_imm);
}
void sim_mode2_ldr(struct sim_state *s, unsigned long raw)
{
	union addressing_mode_2 inst;
	unsigned long uvaddr;

	inst.raw = raw;

	uvaddr = s->reg[inst.imm_offset.f_rn] + inst.imm_offset.f_offset;

	s->reg[inst.imm_offset.f_rd] = uvaddr;
//	fake_printk("rd=%x rn=%x offset=%x\r\n", inst.imm_offset.f_rd, inst.imm_offset.f_rn, inst.imm_offset.f_offset);
}

int simple_sim(struct sim_state *s, unsigned long *insts, unsigned long count)
{
	unsigned long i;
	const struct arm_inst_tab_ent *ent;

	for (i = 0; i < count; i++) {
		unsigned long inst = insts[i];
		ent = arm_inst_lookup(inst);
		if (ent == NULL)
			return -1;
		assert(ent, !=, NULL);
		assert(ent->simulate, !=, NULL);

		((simfunc_t)ent->simulate)(s, inst);
	
	}
	return 0;
}
unsigned long calculate_pc_from_plt_entry(unsigned long *pltent, unsigned long pc)
{
	struct sim_state sim;	

	sim.reg[15] = pc;
	simple_sim(&sim, pltent, 3);
	return sim.reg[15];
}
