/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

extern void *find_phdr_addr(Elf32_Phdr *phdr, unsigned long size, unsigned long id);
extern struct dynamic *find_dynamic_entry(struct dynamic *dyn, unsigned long size, unsigned long tag);

static inline unsigned long get_dynsym_addr(struct dynamic *dyn, unsigned long size)
{
	return (unsigned long)find_dynamic_entry(dyn, size, DT_SYMTAB)->d_un.d_ptr;
}
static inline unsigned long get_dynstr_addr(struct dynamic *dyn, unsigned size)
{
	return (unsigned long)find_dynamic_entry(dyn, size, DT_STRTAB)->d_un.d_ptr;
}
