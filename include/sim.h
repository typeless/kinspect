/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */


struct sim_state {
	unsigned long reg[16];
};

extern void sim_mode1_add(struct sim_state *s, unsigned long raw);
extern void sim_mode2_ldr(struct sim_state *s, unsigned long raw);


extern int simple_sim(struct sim_state *s, unsigned long *insts, unsigned long count);
extern unsigned long calculate_pc_from_plt_entry(unsigned long *pltent, unsigned long pc);
