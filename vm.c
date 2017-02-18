/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 *
 * Ref:
 *	1.ARM Architecture Reference Manual Chapter B4(p.701): Virtual Memory System Architecture
 *	 - B4.7 Hardware page table translation
 */

#include <linux/kernel.h>
#include <linux/mm.h>


#include "include/global.h"
#include "include/tty.h"


static unsigned long 
decode_desc1_paddr(unsigned long desc)
{
	unsigned long mask;

	switch (desc & 0x03) {
	case 0x00:
		mask = 0;
		break;
	case 0x01:
		mask = ~((1<<10)-1);
		break;
	case 0x02:
		switch ((desc >> 18) & 1) {
		case 0:
			mask = ~((1<<20)-1);	
		case 1:
			mask =  ~((1<<24)-1);	
		default:
			mask = 0;
		}
		break;
	case 0x03:
		mask = 0;
		break;
	default:
		mask = 0;
	}

	if (mask == 0) 
		return -1;
	else 
		return desc & mask;
}
static unsigned long 
decode_desc2_paddr(unsigned long desc)
{
	unsigned long mask;
	
	switch (desc & 0x03) {
	case 0x00:
		mask = 0;
		break;
	case 0x01:
		mask = ~((1 << 16) -1);
		break;
	case 0x02:
		mask = ~((1 << 12) -1);
		break;
	case 0x03:
		mask = ~((1 << 10) -1);
		break;
	default:
		mask = 0;
	}

//	if (mask == 0)
//		printk("[desc1 can't be decoded]\r\n");
		
	if (mask == 0) 
		return -1;
	else
		return desc & mask;
}
//static unsigned long vaddr_to_paddr(unsigned long *pgd, unsigned long vpfn); // alternative interface
static unsigned long 
lookup_paddr_from_vaddr(unsigned long *pgd, unsigned long *vaddr)
{
	unsigned long utab2_paddr, *utab2_vaddr, desc1, desc2, idx1, idx2, mapped_paddr;

	idx1 = (unsigned long)vaddr >> 20;
	idx2 = ((unsigned long)vaddr >> 12) & 0xff; /* this may depend on the page size */

	desc1 = pgd[idx1];
	utab2_paddr = decode_desc1_paddr(desc1);	
	if (utab2_paddr == -1)
		return -1;
	utab2_vaddr = __va((unsigned long *)utab2_paddr);
	desc2 = utab2_vaddr[idx2];
	mapped_paddr = decode_desc2_paddr(desc2) | ((unsigned long)vaddr & ((1<<12)-1));
	if (mapped_paddr == -1)
		return -1;
	return mapped_paddr;
}
/*
static unsigned long 
find_page_paddr_from_tab2(unsigned long *tab2, unsigned long paddr)
{
	unsigned long i, mapped_paddr;

	for (i = 0; i < 256; i++) {
		mapped_paddr = decode_desc2_paddr(tab2[i]);
		if (mapped_paddr == paddr)
			return mapped_paddr;
	}
	return -1;
}
static unsigned long 
find_page_paddr_from_tab1(unsigned long *tab1, unsigned long paddr)
{
	unsigned long i, mapped_paddr, tab2_paddr, *tab2_vaddr;
	
	for (i = 0; i < 4096; i++) {
		tab2_paddr = decode_desc1_paddr(tab1[i]);
		tab2_vaddr = __va(tab2_paddr);
		mapped_paddr = find_page_paddr_from_tab2(tab2_vaddr, paddr);
		if (mapped_paddr == paddr)
			return mapped_paddr;
	}
	return -1;
}
*/
/*
static unsigned long 
find_vma(struct mm_struct *mm, unsigned long ptr)
{
	struct vm_area_struct *vma;

	for (vma = mm->mmap; vma; vma = container_of(vma->vm_next, struct vm_area_struct, vm_next)) {
		if (ptr > vma->vm_start && ptr < vma->vm_end)
			printk("*\r");
		printk("[%08lx]->[%08lx]\r\n", vma->vm_start, vma->vm_end);
	}
}
*/
void *
get_kvaddr_from_uvaddr(struct mm_struct *mm, unsigned long uvaddr)
{
	unsigned long paddr;
	void *kvaddr;

	paddr = lookup_paddr_from_vaddr((unsigned long *)mm->pgd, (unsigned long *)uvaddr);
	if (paddr == -1)
		return NULL;
	kvaddr = __va(paddr);
	return kvaddr;
}
struct vm_area_struct *
find_vma_of_ptr(struct mm_struct *mm, unsigned long ptr)
{
	struct vm_area_struct *vma;

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (ptr >=  vma->vm_start && ptr < vma->vm_end)
			break;
	}
	return vma;
}

void *get_uvaddr_object(struct mm_struct *mm, unsigned long uv_start, unsigned long size, void *buf)
{
	unsigned long uvaddr;
	void *dest;

	assert(buf, !=, NULL);

	uvaddr = uv_start;

	if ((uvaddr ^ (uvaddr+size)) & PAGE_MASK) {
		unsigned long second_half_size = (uvaddr + size) & ~PAGE_MASK;
		unsigned long first_half_size = size - second_half_size;
		void *kvaddr;

		assert(first_half_size+second_half_size, <=, size);
		assert(second_half_size, <, size);
		assert(first_half_size, <, size);
		assert((uvaddr+first_half_size) & ~PAGE_MASK, ==, 0);

		kvaddr = get_kvaddr_from_uvaddr(mm, uvaddr);
		assert(kvaddr, !=, NULL);
		memcpy(buf, kvaddr, first_half_size);

		kvaddr = get_kvaddr_from_uvaddr(mm, uvaddr + first_half_size);
		assert(kvaddr, !=, NULL);
		memcpy(buf + first_half_size, kvaddr, second_half_size);

		dest = buf;
//		fake_printk("***---------------------------------------------------------------------------\r\n");
	} else {
		dest = get_kvaddr_from_uvaddr(mm, uvaddr);
	}

//	fake_printk("[%08lx] ", uvaddr);
	assert(dest , !=, NULL);

//	fake_printk("%05d(%p) ", i, dest);
	return dest;
}

void *get_uvaddr_string(struct mm_struct *mm, unsigned long uv_start, unsigned long size, void *buf)
{
	unsigned long uvaddr;
	void *dest;

	assert(buf, !=, NULL);

	uvaddr = uv_start;

	if ((uvaddr ^ (uvaddr+size)) & PAGE_MASK) {
		unsigned long second_half_size;// = (uvaddr + size) & ~PAGE_MASK;
		unsigned long max_first_half_size = PAGE_SIZE - (uvaddr & ~PAGE_MASK);
		unsigned long first_half_size;
		void *kvaddr;

		assert(max_first_half_size, <, size);
		assert((uvaddr+max_first_half_size) & ~PAGE_MASK, ==, 0);

		kvaddr = get_kvaddr_from_uvaddr(mm, uvaddr);
		first_half_size = strnlen(kvaddr, max_first_half_size);

		assert(kvaddr, !=, NULL);

		memcpy(buf, kvaddr, first_half_size);

		kvaddr = get_kvaddr_from_uvaddr(mm, uvaddr + first_half_size);
		second_half_size = strnlen(kvaddr, PAGE_SIZE);
		
		assert(first_half_size+second_half_size, <=, size);
		assert(kvaddr, !=, NULL);

		memcpy(buf + first_half_size, kvaddr, second_half_size);

		dest = buf;
	} else {
		dest = get_kvaddr_from_uvaddr(mm, uvaddr);
	}

	assert(dest , !=, NULL);

	return dest;
}

