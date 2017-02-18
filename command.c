/*
 * (C) 2014 Mura Li <mura_li@castech.com.tw>
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/nmi.h>
#include <linux/debug_locks.h>


#include "include/tty.h"
#include "include/global.h"

extern void show_task(struct task_struct *t, int argc, char **argv);
extern void print_ps(struct task_struct *t, int argc, char **argv);
extern void print_task_stack(struct task_struct *t, int argc, char **argv);
extern void print_data_segment(struct task_struct *t, int argc, char **argv);
extern void print_task_dynamic(struct task_struct *t, int argc, char **argv);
extern void print_task_dynsym(struct task_struct *t, int argc, char **argv);
extern void print_task_vma(struct task_struct *t, int argc, char **argv);
extern void print_task_mm(struct task_struct *t, int argc, char **argv);
extern void print_task_mem(struct task_struct *t,int argc, char **argv);
extern void print_task_regs(struct task_struct *t,int argc, char **argv);
extern void print_ps_help(void);


extern void kretprobe_init(void);
extern int kretprobe_add_by_name(char *func_name);
extern int kretprobe_del_by_name(char *func_name);
extern void kretprobe_show(void);
extern void kretprobe_clear(void);


struct command {
	int (*func)(int argc, char **argv, void *data);
	char *name;
	char *desc;
};

struct command_state {
};


static bool is_delim(char c, char *delim)
{
	char *p;
	for (p = delim; *p; p++)
		if (c == *p)
			return true;
	return false;
}
static char *find_first_delim(char *buf, char *delim)
{
	char *p;

	for (p = buf; *p; p++)
		if (is_delim(*p, delim))
			break;
	return p;
}
static char *clear_delim_str(char *buf, char *delim)
{
	char *p;

	for (p = buf; is_delim(*p, delim); p++)
		*p = '\0';
	return p;
}
static char *strtok_r(char *buf, char *delim, char **last)
{
	char *p, *start;
	
	start = buf ? buf : *last;
	if (start == NULL)
		return NULL;

	p = find_first_delim(start, delim);
	p = clear_delim_str(p, delim);
	*last = *p ? p : NULL;

	return start;
}

static char command_buf[256];



static bool pred_task_has_mm(struct task_struct *t)
{
	return t->mm != NULL;
}
static void do_all_task(void (*func)(struct task_struct *, int, char **), bool (*pred)(struct task_struct *t), int argc, char **argv)
{
	struct task_struct *g, *p;	
	
	assert(func, !=, NULL);

	read_lock(&tasklist_lock);

	do_each_thread(g, p) {
		if (pred(p))
			func(p, argc, argv);
		touch_nmi_watchdog();	
	} while_each_thread(g, p);

	touch_all_softlockup_watchdogs();

	read_unlock(&tasklist_lock);

	debug_show_all_locks();
}




static const struct command *command_lookup(const struct command *cmd, char *name)
{
	while (cmd->name) {
		if (strcmp(cmd->name, name) == 0)
			return cmd; 
		cmd++;
	}
	return NULL;
}



static int arg_int(char *buf, int *val)
{
	int count ;

	assert(buf, !=, NULL);
	assert(val, !=, NULL);

	if ((count = sscanf(buf, "%d", val)) <= 0) {
		return -1;
	}
	return 0;
}
static int cmd_quit(int argc, char **argv, void *data)
{
	return 0;
}
static int cmd_show_task(int argc, char **argv, void *data)
{
	struct task_struct *t;
	pid_t pid;
	void (*func)(struct task_struct *, int, char **);
	bool for_all;

	if (argc == 1) {
		fake_printk("Type 'ps [<PID>|*] [stack|data|dynamic|dynsym|vma|mm|mem]' for details\r\n");
		do_all_task(print_ps, pred_task_has_mm, argc-1, &argv[1]);
		goto exit;
	}

	if (strcmp(argv[1], "*") == 0) {
		for_all = true;	
	} else if (arg_int(argv[1], &pid) == 0) {
		for_all = false;
	} else {
		fake_printk("Invalid [PID] argument\r\n");
		print_ps_help();
		goto exit;
	}


	if (argc < 3) {
		fake_printk("Expect more arguments\r\n");
		goto exit;
	}

	if (strcmp(argv[2], "stack") == 0) {
		func = print_task_stack;	
	} else if (strcmp(argv[2], "data") == 0) {
		func = print_data_segment;
	} else if (strcmp(argv[2], "dynamic") == 0) {
		func = print_task_dynamic;
	} else if (strcmp(argv[2], "dynsym") == 0) {
		func = print_task_dynsym;
	} else if (strcmp(argv[2], "vma") == 0) {
		func = print_task_vma;
	} else if (strcmp(argv[2], "mm") == 0) {
		func = print_task_mm;
	} else if (strcmp(argv[2], "mem") == 0) {
		func = print_task_mem;
	} else if (strcmp(argv[2], "regs") == 0) {
		func = print_task_regs;
	} else {
		fake_printk("Invalid option\r\n");
		print_ps_help();
		goto exit;
	}

	assert(argc, >=, 3);

	if (for_all) {
		do_all_task(func, pred_task_has_mm, argc-2, &argv[3]);
	} else  {
		t = pid_task(find_vpid(pid), PIDTYPE_PID);
		func(t, argc-3, &argv[3]);
	}

exit:	return 0;
}
static int cmd_watch(int argc, char **argv, void *data)
{

	if (argc < 2)
		return -1;

	if (strcmp("add", argv[1]) == 0) {
		kretprobe_add_by_name(argv[2]);
	} else if (strcmp("del", argv[1]) == 0) {
		kretprobe_del_by_name(argv[2]);
	} else if (strcmp("show", argv[1]) == 0) {
		kretprobe_show();
	} else if (strcmp("clear", argv[1]) == 0) {
		kretprobe_clear();	
	} else {
		return -1;	
	}
	return 0;	
}

static int cmd_help(int argc, char **argv, void *data);
static const struct command main_command_list[] = {
	{.func = cmd_quit, 			.name = "quit",			.desc = "Quit"},
	{.func = cmd_help, 			.name = "help", 		.desc = "Show help docs"},
	{.func = cmd_show_task,			.name = "ps", 			.desc = "ps [[PID] [OPTION] |'help'] -- Show tasks info"},
	{.func = cmd_watch,			.name = "w",			.desc = "w [FUNCTION]"},

	{NULL, NULL, NULL},
};
static int cmd_help(int argc, char **argv, void *data)
{
	const struct command *cmd = main_command_list;

	while (cmd->name) {
		fake_printk("* %-20s --- %s\r\n", cmd->name, cmd->desc);
		cmd++;
	}
	fake_printk("* ...                  --- ...(To be implemented)\r\n");
	return 0;
}


static struct command_state command_state = {
};
static char *argv[32];

static unsigned long str_to_argv(char *buf, unsigned long size, char **argv)
{
	char *last, **p;

	p = argv;
	*p++ = strtok_r(buf, " ", &last);	

	do {
		if ((*p = strtok_r(NULL, " ", &last)) == NULL)
			break;
	} while (p++);

	return p - argv;
}
void main_loop(void)
{
	struct command_state *cs = &command_state;
	unsigned long count;
	const struct command *cmd;
	int argc;


	fake_printk("Type 'help' to get more information\r\n");

	while (1) {

		fake_printk(">>>");
		count = ntty_read_line(command_buf, sizeof(command_buf));	
		if (count <= 0)
			continue;

		argc = str_to_argv(command_buf, sizeof(command_buf), argv);

		cmd = command_lookup(main_command_list, argv[0]);	

		if (cmd == NULL) {
			fake_printk("Undefined command.\r\n");
			continue;
		}

		if (cmd->func == cmd_quit) {
			fake_printk("Exiting...\r\n");
			break;
		}

		cmd->func(argc, argv, cs);
	}
	return;
}
