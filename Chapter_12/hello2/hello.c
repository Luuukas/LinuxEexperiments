/*
    如果需要修改原有内核的功能一般采用修改源代码的方法，
    如果是增加或减少功能一般采用内核模块方法更佳。

    编写内核模块需要使用内核中预先定义的宏，宏的作用是帮助内核来管理内核模块，
    由于内核模块在内核空间中运行，无法与C函数库连接，因此它的编写也受到内核编程的限制，
    如不能使用C库函数，不能使用浮点计算。内核模块编程在不同内核版本中各不相同。
    以Linux 2.6为基础讨论内核模块编程。
*/
/*
    该模块的功能是：
    在被载入内核时会向系统日志中写入“Hello, world!"；
    当该模块被卸载的时候，它也会向系统日志写入一条"Goodbye, world!"。
*/
#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("GPL");

static char *whom = "world";
static int howmany = 1;
module_param(howmany, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

static int hello_init(void){
    int i = 0;
    while(i<howmany){
        printk(KERN_ALERT "Hello, %s!\n", whom);
        ++i;
    }
    return 0;
}

static void hello_exit(void){
    printk(KERN_ALERT "Goodbye, world!\n");
}

module_init(hello_init);
module_exit(hello_exit);

/*
    一个内核模块应包括如下几部分：
    1. 头文件声明。module.h包含加载模块需要的函数和符号定义；init.h包含模块初始化和清理函数的定义。
        如果在加载时允许用户传递参数，模块还应该包含moduleparam.h头文件。
    2. 模块许可声明。从内核v2.4.10版本开始，模块必须通过MODULE_LICENSE宏声明此模块的许可证，
        否则在加载此模块时，内核会显示“kernel tainted”的警告信息。
    3. 初始化和清理函数声明。内核模块必须调用宏module_init和module_exit去注册初始化与清理函数。
*/

/*
make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` clean

make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` modules
sudo insmod hello.ko howmany=10 whom="Chong"
dmesg
sudo rmmod hello.ko
dmesg
*/