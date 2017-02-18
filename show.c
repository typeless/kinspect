/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */
#include  <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/elf.h>


#include "include/vm.h"
#include "include/elf.h"
#include "include/tty.h"
#include "include/disasm.h"
#include "include/sim.h"
#include "include/global.h"

#define printk fake_printk

static void 
print_int(unsigned int *buf, int count)
{
	int i;
	printk("[stack]\r\n");

	for (i = 0; i < count; i++) {
		printk("(%03x)[%p] %x\r\n", count - i-1, &buf[i], buf[i]);
	}
	printk("\r\n");
}
static void 
print_pte(pte_t *e, unsigned long size)
{
	unsigned long i;

	for (i = 0; i < size; i++) {
		printk("        %08lx->%08lx\r\n", i << 12, e[i]);
	}
}
static void 
print_second_level_descriptor(unsigned long vaddr, unsigned long desc)
{
	switch (desc & 0x03) {
	case 0x00:
		//printk("..[%08lx] -> %08lx IGN\r\n", vaddr, desc);
		break;
	case 0x01:
		printk("..[%08lx] -> %08lx base:%08lx CB:%lx,%lx Large(64k)\r\n", vaddr, desc, desc & ~((1<<16)-1), (desc>>3)&1 , (desc>>2)&1);
		break;
	case 0x02:
		printk("..[%08lx] -> %08lx base:%08lx CB:%lx,%lx Small(4k)\r\n", vaddr, desc, desc & ~((1<<12)-1), (desc>>3)&1 , (desc>>2)&1);
		break;
	case 0x03:
		printk("..[%08lx] -> %08lx base:%08lx CB:%lx,%lx Tiny(1k)\r\n", vaddr, desc, desc & ~((1<<10)-1), (desc>>3)&1 , (desc>>2)&1);
		break;
	}	
}
static void 
print_second_level_descriptor_table(unsigned long *tbl, unsigned long size, unsigned long vaddr)
{
	unsigned long i;

	for (i = 0; i < size; i++)
		print_second_level_descriptor(vaddr | (i<<12), tbl[i]);
}
static void 
print_first_level_descriptor(unsigned long vaddr, unsigned long desc)
{
	switch (desc & 0x03) {
	case 0x00:
		//printk("[%08lx] %08lx [ignored]\r\n", vaddr, desc);
		break;
	case 0x01:
		printk("[%08lx] %08lx [coarse page table] base:%08lx P=%lx Domain=%01lx SBZ=%lx NS=%lx SBZ=%lx\r\n", vaddr, desc, desc & ~((1<<10) -1), (desc>>9) & 1, (desc>>5) & 0xf, (desc>>4)&1, (desc>>3)&1, (desc>>2)&1  );
		print_second_level_descriptor_table((unsigned long *)__va(desc & ~((1<<10) -1)), 256, vaddr);
		break;
	case 0x02:
		switch ((desc >> 18) & 1) {
		case 0:
			printk("[%08lx] %08lx [section(1M)] base:%08lx\r\n", vaddr, desc, desc & ~((1<<20) -1));
			break;
		case 1:
			printk("[%08lx] %08lx [super section(16M)] base:%08lx\r\n", vaddr, desc, desc & ~((1<<24) -1));
			break;
		}
		break;
	case 0x03:
		printk("[%08lx] %08lx [Reserved]\r\n", vaddr, desc);
		break;
	}
}
static void 
print_first_level_descriptor_table(unsigned long *pgd, unsigned long size)
{
	unsigned long i;
	
	printk("---------------------------------First Level Descriptor Table-------------------------------------\r\n");
	for (i = 0; i < size; i++)
		print_first_level_descriptor(i << 20, pgd[i]);
}
static void 
print_pgd(pgd_t *p, unsigned long size)
{
	unsigned long i;
	struct {unsigned long pgd[2];} *pgd = (void *)p;

	for (i = 0; i < size; i++) {
		printk("%08lx->[%08lx %08lx] bit[18]:(%ld/%ld)\r\n", i << 21, pgd[i].pgd[0], pgd[i].pgd[1], (pgd[i].pgd[0] >> 18) & 1, (pgd[i].pgd[1] >> 18) & 1);
	}
}

static void
print_hex_map(void *addr, unsigned long size)
{
	unsigned long i;
	
	for (i = 0; i < size; i++) {
		printk("%02X", ((unsigned char *)addr)[i]);

		if (((i+1)% 16) == 0)
			printk("\r\n");
		else if (((i+1)% 4) == 0)
			printk("  ");
		else
			printk(" ");	
	}
}
void print_task_mem(struct task_struct *t, int argc, char **argv)
{
	unsigned long addr;
	void *kvaddr;
	if (argc < 1) {
		fake_printk("Expect address: ps [PID] mem [ADDRESS]\r\n");
		return;
	}

	if (sscanf(argv[0], "%lx", &addr) <= 0) {
		fake_printk("Invalid address\r\n");
		return;
	}

	kvaddr = get_kvaddr_from_uvaddr(t->mm, addr);
	if (kvaddr == NULL) {
		fake_printk("Invalid address, no mapping\r\n");
		return;
	}
	print_hex_map(kvaddr, 0x100);
}
void print_dynamic(struct dynamic *dyn, unsigned long size)
{
	unsigned long i;

	for (i = 0; i < size / sizeof(Elf32_Dyn); i++) {
		if (dyn[i].d_tag == DT_NULL)
			break;
		printk("[%5ld] tag:%08lx, d_val:%08lx d_ptr:%08lx\r\n", i, (unsigned long)dyn[i].d_tag, (unsigned long)dyn[i].d_un.d_val, (unsigned long)dyn[i].d_un.d_ptr);
	}
}
void print_elf(struct mm_struct *mm)
{
	struct vm_area_struct *vma;

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		Elf32_Ehdr *ehdr;
		Elf32_Phdr *phdr;
		
		ehdr = get_kvaddr_from_uvaddr(mm, vma->vm_start);
		if (ehdr == NULL)
			continue;
		
		if (memcmp(ehdr, "\x7f" "ELF", 4) != 0)
			continue;

		phdr = find_phdr_addr(((void *)ehdr) + ehdr->e_phoff, ehdr->e_phnum, PT_DYNAMIC);
		if (phdr == NULL) {
			printk("could not find the segment\r\n");
			continue;
		}			

printk("---------------ehdr=%p, phdr=%p phdr->p_offset:%08lx e_phoff:%08x vm_file:%s\r\n", ehdr, phdr, (unsigned long)phdr->p_offset, ehdr->e_phoff, vma->vm_file ? vma->vm_file->f_path.dentry->d_iname : NULL);
		print_dynamic(((void *)ehdr) + phdr->p_offset, phdr->p_memsz);
		
	}
}

static void print_vma(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	void *kvaddr;

	printk("-------------- vma -------------\r\n");
	printk("mm->map_count=%d\r\n", mm->map_count);
	for (vma = mm->mmap; vma != NULL; vma = vma->vm_next) {
		unsigned long vm_size = vma->vm_end - vma->vm_start;
		printk("..[%08lx, %08lx] flags:%08lx:%c%c%c%c prot:%08lx path:%s\r\n", 
				vma->vm_start, 
				vma->vm_end, 
				vma->vm_flags,
				vma->vm_flags & VM_READ ? 'r' : '-',	
				vma->vm_flags & VM_WRITE ? 'w' : '-',	
				vma->vm_flags & VM_EXEC ? 'x' : '-',	
				vma->vm_flags & VM_MAYSHARE ? 's' : 'p',	
				vma->vm_page_prot,
				vma->vm_file ? vma->vm_file->f_path.dentry->d_iname : NULL);
		kvaddr = get_kvaddr_from_uvaddr(mm, vma->vm_start);


		if (kvaddr != NULL && (vma->vm_flags & VM_READ))
			print_hex_map(get_kvaddr_from_uvaddr(mm, vma->vm_start), vm_size > 0x40 ? 0x40 : vm_size);
		else
			printk("[UNABLE TO SHOW]\r\n");

	}

	printk("PAGE_NONE:%lx PAGE_SHARED:%lx PAGE_SHARED_EXEC:%lx PAGE_COPY:%lx PAGE_COPY_EXEC:%lx PAGE_READOLY:%lx PAGE_READONLY_EXEC:%lx PAGE_KERNEL:%lx PAGE_KERNEL_EXEC:%lx\r\n",
		PAGE_NONE, PAGE_SHARED, PAGE_SHARED_EXEC, PAGE_COPY, PAGE_COPY_EXEC, PAGE_READONLY, PAGE_READONLY_EXEC, PAGE_KERNEL, PAGE_KERNEL_EXEC);
}
void print_task_vma(struct task_struct *t, int argc, char **argv)
{
	if (t->mm)
		print_vma(t->mm);
}
static void print_mm_struct(struct mm_struct *mm)
{
	printk("-------------- mm struct -------------\r\n");
	if (mm)
		print_vma(mm);

	printk("pgd:%p\r\nstart_code:%08lx end_code:%08lx\r\nstart_data:%08lx end_data:%08lx\r\nstart_brk:%08lx brk:%08lx start_stack:%08lx\r\n",
			mm->pgd, mm->start_code, mm->end_code, mm->start_data, mm->end_data, mm->start_brk, mm->brk, mm->start_stack);
}
void print_task_mm(struct task_struct *t, int argc, char **argv)
{
	if (t->mm)
		print_mm_struct(t->mm);
}
void print_data_segment(struct task_struct *t)
{
	unsigned long /*i,*/ start, size;
	unsigned char *p;

	start = t->mm->start_data;
	size = t->mm->end_data - t->mm->start_data;

	printk("%s/%d data segment [%08lx-%08lx]\r\n", t->comm, t->pid, t->mm->start_data, t->mm->end_data);

	p = get_kvaddr_from_uvaddr(t->mm, start);
	if (p == NULL) {
		printk("null 0\r\n");
	}

/*
	for (i = 0; i < size; i++) {
		printk("%02X", *p);	
		printk((i+1) % 16 == 0 ? "\r\n" : " ");

		p = (start & ((1<<12) -1)) ? p+1 : get_kvaddr_from_uvaddr(t->mm, start + i);
		if (p == NULL)	
			break;
	}
*/
	print_dynamic((void *)p + 0x0c, size);
}

void print_task_dynamic(struct task_struct *t, int argc, char **argv)
{
	unsigned long start, size;
	unsigned char *p;

	if (t->mm == NULL) {
		fake_printk("Not a userspace task\r\n");
		return;
	}

	start = t->mm->start_data;
	size = t->mm->end_data - t->mm->start_data;

	p = get_kvaddr_from_uvaddr(t->mm, start);
	assert(p, !=, NULL);

	print_dynamic((void *)p + 0x0c, size);	
}

/*
struct print_dynsym_state {
	struct mm_struct *mm;
	unsigned long uv_dynstr;
};
*/
struct pred_state {
	struct mm_struct *mm;
	unsigned long data;
};

static bool check_if_indirect_plt_call(struct mm_struct *mm, unsigned long uvaddr);
void print_dynsym(struct pred_state *ps, struct elf32_sym *sym)
{
	char buf[32];
	char *dynstr = get_uvaddr_string(ps->mm, ps->data+sym->st_name, sizeof(buf), buf);
	unsigned long *got_kvaddr = NULL, got_uvaddr, got_entry = 0;
/*
	printk("-*:%08lx size:%05x info:%02x other:%02x Ndx:%03x name:%05x[%s]\r\n",
			sym->st_value,
			sym->st_size,
			sym->st_info,
			sym->st_other,
			sym->st_shndx,
			sym->st_name,
			dynstr);
*/
	unsigned long *ent_kvaddr;

	ent_kvaddr = get_kvaddr_from_uvaddr(ps->mm, sym->st_value);

	if (ent_kvaddr != NULL && check_if_indirect_plt_call(ps->mm, sym->st_value)) {
		got_uvaddr = calculate_pc_from_plt_entry(ent_kvaddr, sym->st_value + 8);
		got_kvaddr = get_kvaddr_from_uvaddr(ps->mm, got_uvaddr);	
		assert(got_kvaddr, !=, NULL);
		got_entry = *got_kvaddr;
	} else {
		got_entry = -1;
	}

	fake_printk("-*:%08lx size:%05x info:%02x other:%02x Ndx:%03x name:%05x [%08lx%c %08lx%c %08lx%c] ->%08lx [%s]\r\n",
			sym->st_value,
			sym->st_size,
			sym->st_info,
			sym->st_other,
			sym->st_shndx,
			sym->st_name,
			ent_kvaddr ? ent_kvaddr[0] : 0,
//			ent_kvaddr ? (arm_inst_lookup(ent_kvaddr[0]) ? '*' : ' ') : ' ',
			ent_kvaddr ? (arm_inst_name_match(ent_kvaddr[0], "add") ? '*' : ' ') : ' ',
			ent_kvaddr ? ent_kvaddr[1] : 0,
//			ent_kvaddr ? (arm_inst_lookup(ent_kvaddr[1]) ? '*' : ' ') : ' ',
			ent_kvaddr ? (arm_inst_name_match(ent_kvaddr[1], "add") ? '*' : ' ') : ' ',
			ent_kvaddr ? ent_kvaddr[2] : 0,
//			ent_kvaddr ? (arm_inst_lookup(ent_kvaddr[2]) ? '*' : ' ') : ' ',
			ent_kvaddr ? (arm_inst_name_match(ent_kvaddr[2], "ldr") ? '*' : ' ') : ' ',
			got_entry,
			dynstr);

}
static bool pred_print_dynsym_all(struct elf32_sym *sym, struct pred_state *ps)
{
	print_dynsym(ps, sym);
	return false;
}
static bool pred_uvaddr_in_range(struct elf32_sym *sym, struct pred_state *ps)
{
	unsigned long start, end;

	assert(ps, !=, NULL);

	start = sym->st_value;
	end = sym->st_value + sym->st_size;
	return ps->data >= start && ps->data < end;
}






static void print_arm_inst(unsigned long inst)
{
	const struct inst_encoding *enc;
	const struct inst_field **f;
	
	enc = arm_inst_encoding_lookup(inst);
	assert(enc, !=, NULL);

	fake_printk("<%08lx> ", inst);
	for (f = &enc->field_list[0]; f[0]; f++)
	{
		fake_printk("%s:%x", f[0]->name, get_inst_field(f[0], inst));
		fake_printk("%s", (f[0]->offset) ? " " : "\r\n");
	}

}

static bool check_if_indirect_plt_call(struct mm_struct *mm, unsigned long uvaddr)
{
	unsigned long *plt_ent_kvaddr;
	
	plt_ent_kvaddr = get_kvaddr_from_uvaddr(mm, uvaddr);
	if (plt_ent_kvaddr == NULL)
		return false;

	if (!arm_inst_name_match(plt_ent_kvaddr[0], "add"))
		return false;
	if (!arm_inst_name_match(plt_ent_kvaddr[1], "add"))
		return false;
	if (!arm_inst_name_match(plt_ent_kvaddr[2], "ldr"))
		return false;
	return true;	
}
static unsigned long get_pltgot_entry_uvaddr_from_plt_entry(struct mm_struct *mm, unsigned long uvaddr)
{
	unsigned long *plt_ent;
	unsigned long *got_kvaddr = NULL, got_uvaddr, got_entry = 0;

	plt_ent = get_kvaddr_from_uvaddr(mm, uvaddr);
	assert(plt_ent, !=, NULL);

	got_uvaddr = calculate_pc_from_plt_entry(plt_ent, uvaddr + 8);
	got_kvaddr = get_kvaddr_from_uvaddr(mm, got_uvaddr);	
	assert(got_kvaddr, !=, NULL);
	got_entry = *got_kvaddr;

	return got_entry;	
}


static bool pred_uvaddr_in_range_with_indirect_plt_call(struct elf32_sym *sym, struct pred_state *ps)
{
	unsigned long obj_start, obj_end, uvaddr;

	uvaddr = ps->data;

	if (!check_if_indirect_plt_call(ps->mm, sym->st_value))
		return false;
	obj_start = get_pltgot_entry_uvaddr_from_plt_entry(ps->mm, sym->st_value);	
	obj_end = obj_start + sym->st_size;
	return uvaddr >= obj_start && uvaddr < obj_end;
}




static unsigned long find_dynsym_object(
		struct mm_struct *mm, 
		unsigned long start,
		unsigned long end,
		unsigned long size,
		bool (pred)(struct elf32_sym *, struct pred_state *),
		struct pred_state *ps) 
{
	unsigned long uvaddr;

	assert(size, !=, 0);
	assert(end, >=, size);
	assert(end, >=, start);

	for (uvaddr = start; uvaddr < end; uvaddr+=size) {
		struct elf32_sym buf;
		struct elf32_sym *kv_dynsym = get_uvaddr_object(mm, uvaddr, sizeof(struct elf32_sym), &buf);
		if (pred(kv_dynsym, ps))
			return uvaddr;
	}
	return 0;
}


static char *find_symbol_name_basic(
		struct mm_struct *mm, 
		bool (pred)(struct elf32_sym *, struct pred_state *),
		struct pred_state *data)
{
	struct dynamic *dyn;
	struct elf32_sym *sym;
	char *dynstr;
	unsigned long size;
	unsigned long uv_dynsym, uv_dynstr, uv_dest; 

	size = mm->end_data - mm->start_data;
	dyn = (struct dynamic *)(get_kvaddr_from_uvaddr(mm, mm->start_data) + 0x0c);

	uv_dynsym = get_dynsym_addr(dyn, size);
	uv_dynstr = get_dynstr_addr(dyn, size);

	assert(uv_dynstr, >, uv_dynsym);

	uv_dest = find_dynsym_object(mm, uv_dynsym, uv_dynstr, sizeof(struct elf32_sym), pred, data); 
	if (uv_dest == 0)
		return NULL;

	sym = get_kvaddr_from_uvaddr(mm, uv_dest);
	dynstr = get_kvaddr_from_uvaddr(mm, uv_dynstr);	
		
	return dynstr + sym->st_name;
} 
static char *find_symbol_name(struct mm_struct *mm, unsigned long uvaddr)
{
	char *ret;
	struct pred_state ps;

	ps = (struct pred_state ){.mm = mm, .data = uvaddr};

	if ((ret = find_symbol_name_basic(mm, pred_uvaddr_in_range, &ps)))
		return ret;
	return find_symbol_name_basic(mm, pred_uvaddr_in_range_with_indirect_plt_call, &ps);
}
static unsigned long find_symbol_value(struct mm_struct *mm, unsigned long uvaddr)
{
	struct dynamic *dyn;
	struct elf32_sym *sym;
	unsigned long size;
	unsigned long uv_dynsym, uv_dynstr, uv_dest; 
	struct pred_state ps;

	size = mm->end_data - mm->start_data;
	dyn = (struct dynamic *)(get_kvaddr_from_uvaddr(mm, mm->start_data) + 0x0c);

	uv_dynsym = get_dynsym_addr(dyn, size);
	uv_dynstr = get_dynstr_addr(dyn, size);

	assert(uv_dynstr, >, uv_dynsym);

	ps = (struct pred_state){.mm = mm, .data = uvaddr};
	uv_dest = find_dynsym_object(mm, uv_dynsym, uv_dynstr, sizeof(struct elf32_sym), pred_uvaddr_in_range, &ps); 
	if (uv_dest == 0)
		return 0;

	sym = get_kvaddr_from_uvaddr(mm, uv_dest);
	return sym->st_value;
}


void print_task_dynsym(struct task_struct *t, int argc, char **argv)
{
	struct mm_struct *mm = t->mm;
	struct dynamic *dyn;
	char *dynstr;
	unsigned long size;
	unsigned long uv_dynsym, uv_dynstr, uv_dest; 
	struct pred_state ps;

	size = mm->end_data - mm->start_data;
	dyn = (struct dynamic *)(get_kvaddr_from_uvaddr(mm, mm->start_data) + 0x0c);

	uv_dynsym = get_dynsym_addr(dyn, size);
	uv_dynstr = get_dynstr_addr(dyn, size);

	dynstr = get_kvaddr_from_uvaddr(mm, uv_dynstr);

	assert(uv_dynstr, >, uv_dynsym);
	assert(dynstr, !=, NULL);

	ps = (struct pred_state){.mm = mm, .data = uv_dynstr};

	uv_dest = find_dynsym_object(
			mm, 
			uv_dynsym, 
			uv_dynstr, 
			sizeof(struct elf32_sym), 
			pred_print_dynsym_all, 
			&ps); 

	fake_printk("Total: %ld\r\n", (uv_dynstr-uv_dynsym)/sizeof(struct elf32_sym));
}

static void print_task_kvaddr_info(struct task_struct *t, unsigned long *kvaddr, unsigned long val)
{
	struct vm_area_struct *vma;
	char *tag;

	vma = find_vma_of_ptr(t->mm, val);

	if (vma == NULL)
		tag = NULL;
	else if (t->mm->start_stack >= vma->vm_start && t->mm->start_stack < vma->vm_end)
		tag = "<stack>";
	else if (t->stack_start >= vma->vm_start && t->stack_start < vma->vm_end)
		tag = "<threadstack>";
	else
		tag = (vma && vma->vm_file) ? vma->vm_file->f_path.dentry->d_iname : NULL;


	printk("[%s%p] %08lx", (kvaddr ? "" : "lr"), kvaddr, val);

	if (vma) {
		printk(" ->[%08lx-%08lx] flags=%08lx prot=%08lx [%s:%s]\r\n", 
				vma->vm_start, 
				vma->vm_end, 
				vma->vm_flags, 
				vma->vm_page_prot,
				tag,
				find_symbol_name(t->mm, val));
	} else {
		printk("\r\n");
	}
}

void print_task_regs(struct task_struct *t, int argc, char **argv)
{
	struct pt_regs *regs = (struct pt_regs *)((unsigned char *)t->stack + 0x2000 - sizeof(struct pt_regs) -8);

	printk("-------------------- Task %s/%d---------------------\r\n", t->comm, t->pid);
	printk("r0:%08lx r1:%08lx r2:%08lx r3:%08lx r4:%08lx r5:%08lx r6:%08lx r7:%08lx\r\n"
			"r8:%08lx r9:%08lx sl:%08lx fp:%08lx ip:%08lx sp:%08lx lr:%08lx pc:%08lx cpsr:%08lx orig_r0=%08lx\r\n", 
			regs->ARM_r0, regs->ARM_r1, regs->ARM_r2, regs->ARM_r3, regs->ARM_r4, regs->ARM_r5, regs->ARM_r6, regs->ARM_r7, regs->ARM_r8, regs->ARM_r9, regs->ARM_r10, regs->ARM_fp, regs->ARM_ip, regs->ARM_sp, regs->ARM_lr, regs->ARM_pc, regs->ARM_cpsr, regs->ARM_ORIG_r0);
}
void print_task_stack(struct task_struct *t, int argc, char **argv)
{
	struct pt_regs *regs = (struct pt_regs *)((unsigned char *)t->stack + 0x2000 - sizeof(struct pt_regs) -8);
	unsigned long i, *sp_kvaddr, *sb_kvaddr;

	sp_kvaddr = get_kvaddr_from_uvaddr(t->mm, regs->ARM_sp);
	sb_kvaddr = get_kvaddr_from_uvaddr(t->mm, t->stack_start);
	
	printk("stack_start=%08lx start_task=%08lx start_data=%08lx end_data=%08lx\r\n", t->stack_start, t->mm->start_stack, t->mm->start_data, t->mm->end_data);
	printk("sp_kvaddr=%p sb_kvaddr=%p depth=%d\r\n", sp_kvaddr, sb_kvaddr, sb_kvaddr-sp_kvaddr);


	print_task_kvaddr_info(t, NULL, regs->ARM_lr);

	for (i = 0; &sp_kvaddr[i] < sb_kvaddr; i++) {
		print_task_kvaddr_info(t, &sp_kvaddr[i], sp_kvaddr[i]);
	}
}

void print_ps(struct task_struct *t)
{
	fake_printk("%c%5u %s\r\n", t == current ? '*' : ' ', t->pid, t->comm); 
}

void  show_task(struct task_struct *t, int argc, char **argv)
{
//	struct pt_regs *regs = get_irq_regs();
//	struct mm_struct *init_mm = init_task.active_mm; //((struct mm_struct *)0xc000b834);
//	struct mm_struct *mm = ((struct mm_struct *)t->active_mm);
//	struct vm_area_struct *vma;
//	struct pt_regs *regs = (struct pt_regs *)((unsigned char *)t->stack + 0x2000 - sizeof(struct pt_regs) -8);
//	struct cpu_context_save *c = &((struct thread_info *)t->stack)->cpu_context;


//	print_elf(t->mm);
//	print_task_stack(t);
//	print_vma(t->mm);

//	printk("sizeof(Elf32_Phdr)=%x\r\n", sizeof(Elf32_Phdr));
	//vma = find_vma_of_elf_header(t->mm);

//	printk("<%p> %s/%d <- %d stack=%p\r\n", 
//			t,
//			t->comm, 
//			t->pid, 
//			t->real_parent->pid,
//			t->stack
//	      );

//	printk("r4:%08x r5:%08x r6:%08x r7:%08x r8:%08x r9:%08x sl:%08x fp:%08x sp:%08x pc:%08x\r\n", 
//			c->r4, c->r5, c->r6, c->r7, c->r8, c->r9, c->sl, c->fp, c->sp, c->pc);


//	printk("mem_map:%p max_mapnr=%ld\r\n", mem_map, max_mapnr);

//	printk("mm->stack_start:%08lx stack_vm:%08lx\r\n", t->mm->start_stack, t->mm->stack_vm);

//	printk("pgd:%p\r\n", t->mm->pgd);

//	printk("active_mm->pgd:%p start_code:%08lx end_code:%08lx\r\n", mm->pgd, mm->start_code, mm->end_code);

//	printk("init_task.pid:%d init_task.comm:%s\r\n", init_task.pid, init_task.comm);

//	print_mm_struct(init_task.active_mm);

//	print_pgd((void *)(0xc0004000), 2048);
//	printk("===================================================================================\r\n");
//	print_first_level_descriptor_table((unsigned long *)(0xc0004000), 4096);
//	printk("===================================================================================\r\n");
//	print_first_level_descriptor_table((unsigned long *)t->mm->pgd, 4096);


//	printk("mm->start_stack:%08lx stack_vm:%08lx\r\n", t->mm->start_stack, t->mm->stack_vm);

//	print_pgd(t->mm->pgd, 2048);



//	print_int((unsigned int  *)t->stack + 2048 - 128, 128);
//	print_int((unsigned int *)c->sp, (0x2000 -(c->sp & 0x1fff)) >> 2 );
	printk("\r\n---------------------------------------------------------------------------------\r\n");
}
void print_ps_help(void)
{
	fake_printk("----------------------------------------------\r\n"
			"  stack    -- show stack information\r\n"
			"  mm       -- show mm_struct brief\r\n"
			"  vma      -- show vm area information\r\n"
			"  dynsym   -- show .dynsym entries\r\n"
			"  dynamic  -- show .dynamic entries\r\n"
			"  mem      -- 'ps [PID] mem [ADDRESS]' show hexdump at the address\r\n");
}
