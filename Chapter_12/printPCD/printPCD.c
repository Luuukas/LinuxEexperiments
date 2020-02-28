#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/list.h>
#include<linux/sched.h>
#include<linux/sched/signal.h>

static int num = -1;
module_param(num, int, S_IRUGO);

static int print_pid(void)
{
	struct task_struct * task, * p;
	struct list_head * pos;
	int count = 0;
	printk("Hello World enter begin:\n");
	task =& init_task;
	list_for_each(pos, &task->tasks)
	{
        if(count>=num) break;
		p = list_entry(pos, struct task_struct, tasks);
		count++;
		printk("%d---------->%s\n", p->pid, p->comm);
	}
	printk("the number of process is: %d\n", count);
	return 0;
}

static int __init lkp_init(void)
{
	printk("<1>Hello, World! from the kernel space...\n");
	print_pid();
	return 0;
}

static void __exit lkp_cleanup(void)
{
	printk("<1>Good Bye, World! leaving kernel space..\n");
}

module_init(lkp_init);
module_exit(lkp_cleanup);
MODULE_LICENSE("GPL");

/*
make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` clean

make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` modules
sudo insmod printPCD.ko num=5
dmesg
sudo rmmod printPCD.ko
dmesg
*/