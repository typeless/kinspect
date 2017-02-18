/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

extern void *get_kvaddr_from_uvaddr(struct mm_struct *mm, unsigned long uvaddr);
extern struct vm_area_struct * find_vma_of_ptr(struct mm_struct *mm, unsigned long ptr);
extern void *get_uvaddr_object(struct mm_struct *mm, unsigned long uv_start, unsigned long size, void *buf);
extern void *get_uvaddr_string(struct mm_struct *mm, unsigned long uv_start, unsigned long size, void *buf);


