/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

#include <linux/types.h>

#define __packed __attribute__((packed))

union addressing_mode_1 {
	unsigned long raw;
	struct {
		unsigned f_immed_8 	: 8;
		unsigned f_rotate_imm 	: 4;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_s 		: 1;
		unsigned f_opcode 	: 4;
		unsigned f_i 		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} imm32;
	struct {
		unsigned f_rm 		: 4;
		unsigned f_sbz_4 	: 1;
		unsigned f_shift 	: 2;
		unsigned f_shift_imm 	: 5;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_s 		: 1;
		unsigned f_opcode 	: 4;
		unsigned f_i 		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} imm_shift;
	struct {
		unsigned f_rm 		: 4;
		unsigned f_sbo_4 	: 1;
		unsigned f_shift 	: 2;
		unsigned f_sbz_7 	: 1;
		unsigned f_rs 		: 4;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_s 		: 1;
		unsigned f_opcode 	: 4;
		unsigned f_i 		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} reg_shift;
} __packed;

union addressing_mode_2 {
	unsigned long raw;
	struct {
		unsigned f_offset 	: 12;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_b 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_i		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} imm_offset;
	struct {
		unsigned f_rm 		: 4;
		unsigned f_sbz_4_11 	: 8;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_b 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_i		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} reg_offset;
	struct {
		unsigned f_rm 		: 4;
		unsigned f_sbz_4 	: 4;
		unsigned f_shift 	: 2;
		unsigned f_shift_imm 	: 5;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_b 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_i		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} scaled_reg_offset;
} __packed;


union addressing_mode_3 {
	unsigned long raw;
	struct {
		unsigned f_shift 	: 12;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_b 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_i 		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} imm_offset;
	struct {
		unsigned f_rm 		: 4;
		unsigned f_sbo_4 	: 1;
		unsigned f_half 	: 1;
		unsigned f_signed 	: 1;
		unsigned f_sbo_7 	: 1;
		unsigned f_sbz_8_11 	: 4;
		unsigned f_rd 		: 4;
		unsigned f_rn		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_b 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_i 		: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} reg_offset;

} __packed;

union addressing_mode_4 {
	unsigned long raw;
	struct {
		unsigned f_reglist 	: 16;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_s 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_sbz_25 	: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} b;
} __packed;

union addressing_mode_5 {
	unsigned long raw;
	struct {
		unsigned f_offset_8 	: 8;
		unsigned f_cpnum 	: 4;
		unsigned f_rd 		: 4;
		unsigned f_rn 		: 4;
		unsigned f_l 		: 1;
		unsigned f_w 		: 1;
		unsigned f_n 		: 1;
		unsigned f_u 		: 1;
		unsigned f_p 		: 1;
		unsigned f_sbz_25 	: 1;
		unsigned f_mode 	: 2;
		unsigned f_cond 	: 4;
	} b;
} __packed;



struct inst_field {
	char 				*name;
	unsigned long 			offset;
	unsigned long 			size;
};
struct inst_encoding {
	char 				*name;
	unsigned long 			pattern_mask;
	unsigned long 			pattern;
	const struct inst_field 	**field_list;
};
struct arm_inst_tab_ent {
	char 				*name;
	unsigned long 			mode;
	unsigned long			opcode_mask;
	unsigned long 			opcode;	
	void				*simulate;
};

static inline unsigned long get_inst_field(const struct inst_field *f, unsigned long raw)
{
	return (raw >> f->offset) & ((1 << f->size) -1);
}



extern const struct inst_encoding *arm_inst_encoding_lookup(unsigned long raw);
extern const struct arm_inst_tab_ent *arm_inst_lookup(unsigned long raw);
extern bool arm_inst_name_match(unsigned long raw, char *name);
