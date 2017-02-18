#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>

#include <asm/ptrace.h>
#include <asm/irq_regs.h>
#include <asm/page.h>

#define MAX_KRETPROBE_SIZE 32

struct kretprobe_ext
{
	struct kretprobe kretp;
	char func_name_buf[32];
};

struct my_data {
	ktime_t entry_stamp;
};


static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct my_data *data;

	if (!current->mm)
		return 1;	/* Skip kernel threads */

	data = (struct my_data *)ri->data;
	data->entry_stamp = ktime_get();
	return 0;
}

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int retval = regs->ARM_r0; //regs_return_value(regs);
	struct my_data *data = (struct my_data *)ri->data;
	s64 delta;
	ktime_t now;

	now = ktime_get();
	delta = ktime_to_ns(ktime_sub(now, data->entry_stamp));
	printk(KERN_INFO "%d/%s %s returned %d and took %lld ns to execute\n",
			current->pid, current->comm, ri->rp->kp.symbol_name, retval, (long long)delta);
	return 0;
}

static struct kretprobe_ext kretprobe_ext_list[MAX_KRETPROBE_SIZE];


static struct kretprobe_ext *find_empty_slot(void)
{
	int i;

	for (i = 0; i < MAX_KRETPROBE_SIZE; i++) {
		if (kretprobe_ext_list[i].kretp.kp.symbol_name == NULL)
			return &kretprobe_ext_list[i];
	}
	return NULL;
}
static struct kretprobe_ext *find_kretprobe_ext(char *func_name)
{
	int i;

	for (i = 0; i < MAX_KRETPROBE_SIZE; i++) {
		if (kretprobe_ext_list[i].kretp.kp.symbol_name == NULL)
			continue;
		if (strcmp(kretprobe_ext_list[i].kretp.kp.symbol_name, func_name) == 0)	
			return &kretprobe_ext_list[i];
	}
	return NULL;
}

int kretprobe_add_by_name(char *func_name)
{
	int ret;
	struct kretprobe_ext *p;

	p = find_empty_slot();
	if (p == NULL)
		return -ENOMEM;

	strlcpy(p->func_name_buf, func_name, sizeof(p->func_name_buf));

	p->kretp.kp.symbol_name = p->func_name_buf;
	p->kretp.handler = ret_handler,
	p->kretp.entry_handler = entry_handler,
	p->kretp.data_size = sizeof(struct my_data),
	p->kretp.maxactive = 20,

	ret = register_kretprobe(&p->kretp);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n",
				ret);

		memset(p, 0, sizeof(*p));
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			p->kretp.kp.symbol_name, p->kretp.kp.addr);
	return 0;
}


void kretprobe_del(struct kretprobe_ext *p)
{
	unregister_kretprobe(&p->kretp);

	memset(p, 0, sizeof(*p));

}
int kretprobe_del_by_name(char *func_name)
{
	struct kretprobe_ext *p;

	p = find_kretprobe_ext(func_name);
	if (p == NULL) {
		printk("kretprobe '%s' is not found\n", func_name);
		return -ENOMEM;
	}

	kretprobe_del(p);	

	printk("kretprobe '%s' has been unregistered\n", func_name);
	return 0;
}

void kretprobe_show(void)
{
	int i, count;

	count = 0;
	for (i = 0; i < MAX_KRETPROBE_SIZE; i++) {
		if (kretprobe_ext_list[i].kretp.kp.symbol_name == NULL)
			continue;
		printk("%2d %s\n", count, kretprobe_ext_list[i].kretp.kp.symbol_name);
		count ++;
	}
}

void kretprobe_clear(void)
{
	int i;
	struct kretprobe_ext *p;

	for (i = 0; i < MAX_KRETPROBE_SIZE; i++) {
		p = &kretprobe_ext_list[i];
		if (p->kretp.kp.symbol_name == NULL)
			continue;
		kretprobe_del(p);
	}
}

void kretprobe_init(void)
{
	int i;
	
	for (i = 0; i < MAX_KRETPROBE_SIZE; i++) {
		struct kretprobe_ext *p = &kretprobe_ext_list[i];
		memset(p, 0, sizeof(*p));
	}
}
