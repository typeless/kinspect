/*
 * Here's a sample kernel module showing the use of jprobes to dump
 * the arguments of do_fork().
 *
 * For more information on theory of operation of jprobes, see
 * Documentation/kprobes.txt
 *
 * Build and insert the kernel module as done in the kprobe example.
 * You will see the trace data in /var/log/messages and on the
 * console whenever do_fork() is invoked to create a new process.
 * (Some messages may be suppressed if syslogd is configured to
 * eliminate duplicate messages.)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/sched.h>

/*
 * Jumper probe for do_fork.
 * Mirror principle enables access to arguments of the probed routine
 * from the probe handler.
 */

/* Proxy routine having the same arguments as actual do_fork() routine */
static int jdo_execve(char * filename,
	char __user *__user *argv,
	char __user *__user *envp,
	struct pt_regs * regs)
{
	struct task_struct *p = current->real_parent;
	struct task_struct *gp = p->real_parent;
	printk("[%s] (%d/%s)<-(%d/%s)<-(%d/%s)\r\n", &__func__[1], current->pid, filename, p->pid, p->comm, gp->pid, gp->comm);
	jprobe_return();
	return 0;
}
static long jdo_fork(unsigned long clone_flags, unsigned long stack_start,
	      struct pt_regs *regs, unsigned long stack_size,
	      int __user *parent_tidptr, int __user *child_tidptr)
{
	printk(KERN_INFO "jprobe: clone_flags = 0x%lx, stack_size = 0x%lx,"
			" regs = 0x%p\n",
	       clone_flags, stack_size, regs);

	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	return 0;
}
/*
static void *jdo___kmalloc(size_t size, gfp_t flags)
{
	if (size == 116)
		printk("[%s] size=%d <--%p\r\n", __func__, size, __builtin_return_address(0));
	jprobe_return();
	return 0;
}
static struct urb *jdo_usb_alloc_urb(int iso_packets, gfp_t mem_flags)
{
	printk("[%s] <--%p\r\n",  __func__, __builtin_return_address(0));
	jprobe_return();
	return 0;
}

#include <linux/usb.h>
static int jdo_usb_bulk_msg(struct usb_device *usb_dev, unsigned int pipe,
                  void *data, int len, int *actual_length, int timeout)
{
	printk("[%s] timeout=%d <--%p\r\n",  __func__, timeout, __builtin_return_address(0));
	jprobe_return();
	return 0;
}
static void jdo_usb_free_urb(struct urb *urb)
{
	printk("[%s] urb=%p refcount=%d <--%p\r\n",  __func__, urb, urb->kref.refcount, __builtin_return_address(0));
	jprobe_return();
}
static struct urb *jdo_usb_get_urb(struct urb *urb)
{
	printk("[%s] urb=%p refcount=%d <--%p\r\n",  __func__, urb, urb->kref.refcount, __builtin_return_address(0));
	jprobe_return();
	return 0;
}
static int jdo_usb_start_wait_urb(struct urb *urb, int timeout, int *actual_length)
{
	printk("[%s] urb=%p refcount=%d <--%p\r\n",  __func__, urb, urb->kref.refcount, __builtin_return_address(0));
	jprobe_return();
	return 0;
}
*/
#define __ENTRY(name, func) 			\
	{						\
		.entry			= func,		\
		.kp = {					\
			.symbol_name	= #name,	\
		},					\
	}						

static struct jprobe jprobe_list[] = {
	__ENTRY(do_execve, jdo_execve),
//	__ENTRY(__kmalloc, jdo___kmalloc),
//	__ENTRY(usb_alloc_urb, jdo_usb_alloc_urb),
//	__ENTRY(usb_bulk_msg, jdo_usb_bulk_msg),
//	__ENTRY(usb_free_urb, jdo_usb_free_urb),
//	__ENTRY(usb_get_urb, jdo_usb_get_urb),
//	__ENTRY(usb_start_wait_urb, jdo_usb_start_wait_urb),
};

static int __init jprobe_init(void)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(jprobe_list); i++) {
		ret = register_jprobe(&jprobe_list[i]);
		if (ret < 0) {
			printk(KERN_INFO "register_jprobe failed, returned %d\n", ret);
			return -1;
		}
		printk(KERN_INFO "Planted jprobe at %p, handler addr %p\n",
				jprobe_list[i].kp.addr, jprobe_list[i].entry);
	}
	return 0;
}

static void __exit jprobe_exit(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jprobe_list); i++) {
		unregister_jprobe(&jprobe_list[i]);
		printk(KERN_INFO "jprobe at %p unregistered\n", jprobe_list[i].kp.addr);
	}
}

module_init(jprobe_init)
module_exit(jprobe_exit)
MODULE_LICENSE("GPL");
