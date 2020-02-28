#ifndef FSHEADER
#define FSHEADER

/*
基本思路：
    在现有文件系统中创建一个文件，并将其模拟成一个物理磁盘，并在该文件上实现模拟磁盘块及文件系统的管理。
    在此基础上，设计并实现一组文件操作函数调用接口，支持对模拟文件系统的访问。
物理磁盘块设计：
    卷盘块数等于100块，每个磁盘块512字节，磁盘块之间用'\n'隔开，总共是514B。
    0#表示超级块，1#~10#放索引节点，每个索引节点占64B，共80个索引节点。
    初始化时存在根目录root，占用0#节点和11#盘块。
空闲磁盘块管理：
    采用成组链接法管理。每组10块，12#~99#分为9组，每组的最后一个磁盘块里存放下一组的磁盘号信息。
    最后一组只有8块，加上0作为结束标志。在超级块中用一个一维数组（10个元素）作为空闲磁盘块栈，放入第一组盘块。
空闲索引节点：
    采用混合索引式文件结构。索引节点结构中文件物理地址包括6项，即4个直接块号，一个一次间址，一个两次间址。其中一次间址和两次间址中一个磁盘块中存放16个磁盘号。
    在超级块中也用一维数组（80）个元素作为空闲节点栈。与空闲磁盘管理块管理不同的是，这里不采用成组链接法，这一维数组中存放所有节点编号，
    而且一直保持同一大小秩序。根目录占0#索引节点，由于根目录不会删改，一直占0#索引节点。
*/

struct SUPERBLOCK{    // 超级块
    int fistack[80];    // 空闲节点号栈
    int fiptr;    // 空闲节点栈指针
    int fbstack[10];    // 空闲盘块号栈
    int fbptr;    // 空闲盘块栈指针
    int inum;    // 空闲节点总数
    int bnum;    // 空闲盘块总数
};

struct INODE{    // 节点(64B)已保证每两个数据之间由空格隔开
    int fsize;    // 文件大小
    int fbnum;    // 文件盘块数
    int addr[4];    // 4个直接盘块号(0~512*4-1)
    int addr1;    // 一个一次间址
    int addr2;    // 一个二次间址
    char owner[6];    // 文件所有者
    char group[6];    // 文件所属组
    char mode[11];    // 文件类别及存储权限
    char ctime[9];    // 最近修改时间
};

struct DIR{    // 目录项(36B)
    char fname[14];    // 文件名(当前目录)
    int index;    // 节点号
    char parfname[14];    // 父目录名
    int parindex;    // 父目录节点号
};

/*
    申请一个节点，返回节点号，否则返回-1。
    返回的是空闲节点号栈中最小的节点号，节点用完时返回-1，申请失败。
*/
int ialloc(void);

/*
    指定一个节点号，回收一个节点。
    先清空节点，然后插入栈中合适位置（必须保持节点号的有序性）
*/
void ifree(int index);

/*
    读指定节点的索引节点信息（节点号为index，读指针应定位到514+64*index+2*(index/8)），
    把索引节点信息保存到变量inode中，便于对同一节点进行大量操作
*/
void readinode(int index, INODE &inode);

/*
    把 INODE inode 写回指定的节点
*/
void writeinode(INODE inode, int index);

/*
    成组链接法
    申请一个盘块，返回盘块号，否则返回-1
*/
int balloc(void);

/*
    指定一个盘块号，回收一个盘块
*/
void bfree(int index);

/*
    读超级块到主存 SUPERBLOCK superblock
*/
void readsuper(void);

/*
    主存 SUPERBLOCK superblock；写回超级块
*/
void writesuper(void);

/*
    读指定目录项（索引节点的盘块，下标index）进临时对象
*/
void readdir(INODE inode, int index, DIR &dir);

/*
    目录项对象 DIR dir 写到指定目录项
*/
void writedir(INODE inode, DIR dir, int index);

/*
    当前目录下创建一个数据文件（规定目录文件只占1~4个盘块）。
    第1个参数表示文件名，第2个参数表示文件内容，
    可以在文件副本中使用这个函数。
*/
void mk(char *dirname, char *content);

/*
    当前目录下删除指定数据文件
*/
void rm(char *filename);

/*
    给定一个路径，找到该文件并复制到当前目录下
*/
void cp(char *string);

/*
    显示当前目录下指定数据文件的内容
*/
void cat(char *filename);

/*
    判断对象inode指向的目录文件盘块中有无该名（dirname）的目录项存在，
    有则返回1，无则返回0。
    同时，若有该目录项，则按引用调用的i为待删子目录目录项下标，
    index2为目录项中代删子目录的节点号。
*/
bool havesame(char *dirname, INODE inode, int &i, int &index2);

/*
    根据路径找到指定文件或目录，路径至少有一个“/”以root开头，不以“/”结尾。
    需要注意，使用此函数时当前路径跟着改掉了，所以使用前必须保存当前路径。
    万一找不到目标，可以还原为当前路径。
*/
bool find(char *string);

/*
    当前目录下创建目录。规定目录文件只占一个盘块。为了降低难度，已设置目录文件只占一个盘块
*/
void mkdir(char *dirname);

/*
    当前目录下删除目录。将要删除的目录可能非空。有两种策略：一是规定只能删除空目录；
    二是递归地将非空目录的所有子目录删除，然后再删除自己。第一种实现校简单，这里使用了第二种策略。
    所以参数为（子目录名，当前节点）。如果使用第一种策略，参数只要子目录名即可
*/
void rmdir(char *dirname, int index);

/*
    显示当前节点的所有子目录
*/
void ls(void);

/*
    改变当前目录。有4中处理过程：
    string="."时，表示切换到当前目录；
    string=".."时，表示切换到父目录；
    string="/"时，表示切换到根目录；
    string为一路径时，则调用 bool find(char *string) 切换到指定目录
*/
void cd(char *string);

/*
    登录信息，要求输入用户信息，并判断是否合法
*/
bool login(void);

/*
    改变当前用户的密码
*/
void changepassword(void);

/*
    判断当前用户对指定的节点有无写权限
*/
bool havewpower(INODE inode);

/*
    改变当前目录下指定文件的文件权限
*/
void chmod(char *name);

/*
    改变当前目录下指定文件的所有者（如所有者在另一个组，那么组也要改）
*/
void chown(char *name);

/*
    改变当前目录下指定文件的所属组
*/
void chgrp(char *name);

/*
    改变当前目录下指定文件的文件名
*/
void chnam(char *name);

/*
    根据节点路径，输出路径
*/
void printroad(void);

/*
    用户组的初始化
    最多 用户名5位，密码5位，用户组5位
*/
void initial();

/*
    命令解析函数
*/
void getcommand();
#endif