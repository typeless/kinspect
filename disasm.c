/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 * 
 * Ref: 
 *	1.ARM Architecture Reference Manual Chapter A3(p.109): The ARM Instruction Set
 *	1.ARM Architecture Reference Manual Chapter A4(p.151): ARM Instructions
 *	2.ARM Architecture Reference Manual Chapter A5(p.441): ARM Addressing Modes
 */

#include <linux/types.h>
#include <linux/string.h>
#include "include/disasm.h"
#include "include/sim.h"
#include "include/global.h"

#include "include/tty.h"

//#define __used __attribute__((used))
__used static const struct inst_field f_rm 		= {.name = "rm",		.size = 4,	.offset = 0};
__used static const struct inst_field f_immed_l 	= {.name = "ImmedL",		.size = 4,	.offset = 0};
__used static const struct inst_field f_immed_8 	= {.name = "immed_8",		.size = 8,	.offset = 0};
__used static const struct inst_field f_offset_8 	= {.name = "offset_8",		.size = 8,	.offset = 0};
__used static const struct inst_field f_offset_12 	= {.name = "offset_12",		.size = 12,	.offset = 0};
__used static const struct inst_field f_reglist		= {.name = "reglist",		.size = 16,	.offset = 0};
__used static const struct inst_field f_bit_4		= {.name = "bit_4",		.size = 1,	.offset = 4};
__used static const struct inst_field f_zeroed_8	= {.name = "zeroed_8",		.size = 8,	.offset = 4};
__used static const struct inst_field f_half		= {.name = "half", 		.size = 1, 	.offset = 5};
__used static const struct inst_field f_shift		= {.name = "shift", 		.size = 2, 	.offset = 5};
__used static const struct inst_field f_signed		= {.name = "signed", 		.size = 1, 	.offset = 6};
__used static const struct inst_field f_bit_7		= {.name = "bit_7",		.size = 1,	.offset = 7};
__used static const struct inst_field f_shift_imm	= {.name = "shift_imm",		.size = 5,	.offset = 7};
__used static const struct inst_field f_rotate_imm	= {.name = "rotate_imm", 	.size = 4,	.offset = 8};
__used static const struct inst_field f_rs		= {.name = "rs",		.size = 4,	.offset = 8};
__used static const struct inst_field f_immed_h 	= {.name = "ImmedH",		.size = 4,	.offset = 8};
__used static const struct inst_field f_cpnum		= {.name = "cpnum",		.size = 4,	.offset = 8};
__used static const struct inst_field f_sbz		= {.name = "SBZ",		.size = 4,	.offset = 8};

__used static const struct inst_field f_rd		= {.name = "rd", 		.size = 4, 	.offset = 12};
__used static const struct inst_field f_rn		= {.name = "rn", 		.size = 4, 	.offset = 16};


__used static const struct inst_field f_l		= {.name = "L", 		.size = 1, 	.offset = 20};
__used static const struct inst_field f_s		= {.name = "S", 		.size = 1, 	.offset = 20};
__used static const struct inst_field f_w		= {.name = "W", 		.size = 1, 	.offset = 21};
__used static const struct inst_field f_opcode		= {.name = "opcode",		.size = 4,	.offset = 21};
__used static const struct inst_field f_b		= {.name = "B", 		.size = 1, 	.offset = 22};
__used static const struct inst_field f_immorreg	= {.name = "imm/reg", 		.size = 1, 	.offset = 22};
__used static const struct inst_field f_n		= {.name = "N", 		.size = 1, 	.offset = 22};
__used static const struct inst_field f_u		= {.name = "U", 		.size = 1, 	.offset = 23};
__used static const struct inst_field f_p		= {.name = "P", 		.size = 1, 	.offset = 24};
__used static const struct inst_field f_i		= {.name = "I", 		.size = 1, 	.offset = 25};
__used static const struct inst_field f_mode		= {.name = "mode",		.size = 2,	.offset = 26};
__used static const struct inst_field f_cond 		= {.name = "cond", 		.size = 4, 	.offset = 28};





__used static const struct inst_encoding mode1_32bit_immed_enc = {
	.name = "mode1 32bit-immed",
	.pattern_mask 	= 0x0e000000,
	.pattern 	= 0x02000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_opcode, &f_s, &f_rn, &f_rd, &f_rotate_imm, &f_immed_8, NULL},
};
__used static const struct inst_encoding mode1_immed_shifts = {
	.name = "mode1 immed shifts",
	.pattern_mask	= 0x0e000008,
	.pattern	= 0x00000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_opcode, &f_s, &f_rn, &f_rd, &f_shift_imm, &f_shift, &f_rm, NULL},
};
__used static const struct inst_encoding mode1_reg_shifts = {
	.name = "mode1 reg shifts",
	.pattern_mask	= 0x0e000088,
	.pattern	= 0x00000008,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_opcode, &f_s, &f_rn, &f_rd, &f_rs, &f_shift, &f_rm, NULL},
};

__used static const struct inst_encoding mode2_immed_offset = {
	.name = "mode2 immed offset",
	.pattern_mask	= 0x0e000000,
	.pattern	= 0x04000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_b, &f_w, &f_l, &f_rn, &f_rd, &f_offset_12, NULL},
};
__used static const struct inst_encoding mode2_reg_offset = {
	.name = "mode2 reg offset",
	.pattern_mask	= 0x0e000ff0,
	.pattern	= 0x06000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_b, &f_w, &f_l, &f_rn, &f_rd, &f_zeroed_8, &f_rm, NULL},
};
__used static const struct inst_encoding mode2_scaled_reg_offset = {
	.name = "mode2 scaled reg offset",
	.pattern_mask	= 0x0e000008,
	.pattern 	= 0x06000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_b, &f_w, &f_l, &f_rn, &f_rd, &f_shift_imm, &f_shift, &f_bit_4, &f_rm, NULL},
};


__used static const struct inst_encoding mode3_immed_offset = {
	.name = "mode3 immed offset",
	.pattern_mask	= 0x0e000088,
	.pattern	= 0x00000088,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_immorreg, &f_w, &f_l, &f_rn, &f_rd, &f_immed_h, &f_bit_7, &f_signed, &f_half, &f_bit_4, &f_immed_l, NULL},
};
__used static const struct inst_encoding mode3_reg_offset = {
	.name = "mode3 reg offset",
	.pattern_mask = 0x0,
	.pattern = 0x0,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_immorreg, &f_w, &f_l, &f_rn, &f_rd, &f_sbz, &f_bit_7, &f_signed, &f_half, &f_bit_4, &f_rm, NULL},
};


__used static const struct inst_encoding mode4_reg_list = {
	.name = "mode4 load/store multiple",
	.pattern_mask	= 0x0e000000,
	.pattern	= 0x08000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_s, &f_w, &f_l, &f_rn, &f_reglist, NULL},
};

__used static const struct inst_encoding mode5_cp = {
	.name = "mode5 cp load/store",
	.pattern_mask	= 0x0e000000,
	.pattern	= 0x0c000000,
	.field_list = (const struct inst_field *[]){&f_cond, &f_mode, &f_i, &f_p, &f_u, &f_n, &f_w, &f_l, &f_rn, &f_rd, &f_cpnum, &f_offset_8, NULL},
};

struct addressing_mode {
	unsigned long mode;
	unsigned long pattern_mask;
	unsigned long pattern;
	const struct inst_encoding **encoding_list;
};
__used static const struct addressing_mode addr_mode_list[] = {
	[0] = {
		.mode 		= 3,
		.pattern_mask 	= 0x0e000088,
		.pattern	= 0x00000088,
		.encoding_list 	= (const struct inst_encoding *[]){&mode3_immed_offset, &mode3_reg_offset, NULL},
	},
	[1] = {
		.mode 		= 1,
		.pattern_mask 	= 0x0c000000,
		.pattern	= 0x00000000,
		.encoding_list 	= (const struct inst_encoding *[]){&mode1_32bit_immed_enc, &mode1_immed_shifts, &mode1_reg_shifts, NULL},
	},
	[2] = {
		.mode 		= 2,
		.pattern_mask 	= 0x0c000000,
		.pattern	= 0x04000000,
		.encoding_list 	= (const struct inst_encoding *[]){&mode2_immed_offset, &mode2_reg_offset, &mode2_scaled_reg_offset , NULL},
	},
	[3] = {
		.mode 		= 4,
		.pattern_mask 	= 0x0e000000,
		.pattern	= 0x08000000,
		.encoding_list 	= (const struct inst_encoding *[]){&mode4_reg_list, NULL},
	},
	[4] = {
		.mode 		= 5,
		.pattern_mask 	= 0x0e000088,
		.pattern	= 0x0c000088,
		.encoding_list 	= (const struct inst_encoding *[]){&mode5_cp, NULL},
	},
};
static const struct arm_inst_tab_ent arm_inst_tab[] = {
	[0] = {	
		.name = "add", 
		.mode = 1, 
		.opcode_mask = 0x01e00000, 
		.opcode = 0x00800000,
		.simulate = sim_mode1_add,
	},

	[1] = {
		.name = "ldr", 
		.mode = 2,
		.opcode_mask = 0,
		.opcode = 0,
		.simulate = sim_mode2_ldr,
	},
};


const struct addressing_mode *addressing_mode(unsigned long raw)
{
	unsigned long i;

	for (i = 0; i < sizeof(addr_mode_list)/sizeof(addr_mode_list[0]); i++) {
		const struct addressing_mode *m = &addr_mode_list[i];
		if ((raw & m->pattern_mask) == m->pattern)
			return m;
	}
	return NULL;
}

static bool encoding_match(const struct inst_encoding *enc, unsigned long raw)
{
	return (enc->pattern_mask & raw) == enc->pattern;
}
const struct inst_encoding *inst_encoding_lookup(const struct inst_encoding **enc_list, unsigned long raw)
{
	const struct inst_encoding **e;

	assert(enc_list, !=, NULL);

	for (e = enc_list; *e; e++)
		if (encoding_match(*e, raw))
			return *e;
	return NULL;
}


const struct inst_encoding *arm_inst_encoding_lookup(unsigned long raw)
{
	unsigned long i;
	const struct arm_inst_tab_ent *tab = arm_inst_tab;
	const struct addressing_mode *m;

	m = addressing_mode(raw);
	if (m == NULL)
		return NULL;

	for (i = 0; i < sizeof(arm_inst_tab)/sizeof(arm_inst_tab[0]); i++) {
		const struct inst_encoding *enc;

		if (m->mode != tab[i].mode)
			continue;

		if ((enc = inst_encoding_lookup(m->encoding_list, raw)) != NULL)
			return enc;
	}
	return NULL;
}
const struct arm_inst_tab_ent *arm_inst_lookup(unsigned long raw)
{
	unsigned long i;
	const struct arm_inst_tab_ent *tab = arm_inst_tab;
	const struct addressing_mode *m;

	m = addressing_mode(raw);
	if (m == NULL)
		return NULL;

	for (i = 0; i < sizeof(arm_inst_tab)/sizeof(arm_inst_tab[0]); i++) {
		const struct inst_encoding *enc;

		if (m->mode != tab[i].mode)
			continue;

		if ((enc = inst_encoding_lookup(m->encoding_list, raw)) != NULL)
			return &tab[i];
	}
	return NULL;
}

bool arm_inst_name_match(unsigned long raw, char *name)
{
	const struct arm_inst_tab_ent *ent;
	
	ent = arm_inst_lookup(raw);
	if (ent &&  strcmp(ent->name, name) == 0)
		return true;
	return false;
}




