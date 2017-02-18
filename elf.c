/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

#include <linux/kernel.h>
#include <linux/elf.h>

#include "include/global.h"

void *find_phdr_addr(struct elf32_phdr *phdr, unsigned long size, unsigned long id)
{
	unsigned long i;

	for (i = 0; i < size; i++) {
		if (phdr[i].p_type == id)
			return &phdr[i];
	}
	return NULL;		
}

struct dynamic *find_dynamic_entry(struct dynamic *dyn, unsigned long size, unsigned long tag)
{
	unsigned long i;

	assert(dyn, !=, NULL);
	
	for (i = 0; i < size/sizeof(struct dynamic); i++) {
		if (dyn[i].d_tag == tag)
			return &dyn[i];
		if (dyn[i].d_tag == DT_NULL)
			return NULL;
	}
	return NULL;
}

