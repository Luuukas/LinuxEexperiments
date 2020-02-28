#include "fsheader.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include <time.h>
using namespace std;
SUPERBLOCK superblock;
std::fstream disk;
std::fstream user;
int usernum = 1;
int road[20];    // 当前操作路径 e.g. road[0]/road[1]/road[3]/.../road[num-1]
int num = 1;      // 当前操作路径层数
char curname[14]; // 当前操作目录的目录名
char auser[6];
char agroup[6];
char apwd[6];
int ialloc(void)
{
    if (superblock.fiptr > 0)
    {
        int temp = superblock.fistack[80 - superblock.fiptr]; // 当前可用
        superblock.fistack[80 - superblock.fiptr] = -1;
        superblock.fiptr--;
        return temp;
    }
    return -1;
}

void ifree(int index)
{
    disk.open("disk.txt", ios::in | ios::out); // 清空节点
    if (!disk)
    {
        cerr << "unable to open disk.txt." << endl;
        exit(1);
    }
    disk.seekp(514 + 64 * index + 2 * (index / 8));
    disk << setw(64) << ' ';
    disk.close();
    for (int i = 80 - superblock.fiptr; i < 80; i++)
    { // 节点号找到合适位置插入空闲节点栈
        if (superblock.fistack[i] < index)
        { // 若小于它，前移一位
            superblock.fistack[i - 1] = superblock.fistack[i];
        }
        else
        { // 放在第1个大于它的节点号前面
            superblock.fistack[i - 1] = index;
            break;
        }
    }
    superblock.fiptr++;
}

void readinode(int index, INODE &inode)
{
    disk.open("disk.txt", ios::in | ios::out);
    disk.seekg(514 + 64 * index + 2 * (index / 8));

    disk >> inode.fsize;        // 文件大小
    disk >> inode.fbnum;        // 文件盘块数
    for (int i = 0; i < 4; i++) // 4个直接盘块号
        disk >> inode.addr[i];
    disk >> inode.addr1; // 一个一次间址
    disk >> inode.addr2; // 一个二次间址
    disk >> inode.owner; // 文件所有者
    disk >> inode.group; // 文件所属组
    disk >> inode.mode;  // 文件类别及存储权限
    disk >> inode.ctime; // 最近修改时间

    disk.close();
}

void writeinode(INODE inode, int index)
{
    disk.open("disk.txt", ios::in | ios::out);
    disk.seekp(514 + 64 * index + 2 * (index / 8));

    disk << setw(6) << inode.fsize; // 文件大小
    disk << setw(6) << inode.fbnum; // 文件盘块数
    for (int i = 0; i < 4; i++)     // 4个直接盘块号
        disk << setw(3) << inode.addr[i];
    disk << setw(3) << inode.addr1;  // 一个一次间址
    disk << setw(3) << inode.addr2;  // 一个二次间址
    disk << setw(6) << inode.owner;  // 文件所有者
    disk << setw(6) << inode.group;  // 文件所属组
    disk << setw(12) << inode.mode;  // 文件列别及存储权限
    disk << setw(10) << inode.ctime; // 最近修改时间

    disk.close();
}

int balloc(void)
{
    int temp = superblock.fbstack[10 - superblock.fbptr];
    if (superblock.fbptr == 1)
    { // 栈到底了
        // 是最后记录盘块号0（保留作为栈底，分配不成功）
        if (temp == 0)
        {
            return -1;
        }
        superblock.fbstack[10 - superblock.fbptr] = -1;
        superblock.fbptr = 0;

        // 盘块内容读入栈
        int id, num = 0;
        disk.open("disk.txt", ios::in | ios::out);
        // 先计算盘块内容个数num（最多10），最后盘块可能不到10个
        disk.seekg(514 * temp);
        // 因为当前这个将要分配的块是这组的最后一块，里面记录了下一组空闲块的每个块id
        for (int i = 0; i < 10; i++)
        {
            disk >> id;
            num++;
            if (id == 0)
                break;
        }
        disk.seekg(514 * temp); // 盘块内容读入栈
        // 当前这组空闲块全部分配出去了，利用这组的最后一块所记录的下一组块的信息，换一组空闲块进来待分配
        for (int j = 10 - num; j < 10; j++)
        {
            disk >> id;
            superblock.fbstack[j] = id;
        }
        superblock.fbptr = num;
        disk.close();

        disk.open("disk.txt", ios::in | ios::out); // 清空回收盘块
        disk.seekp(514 * temp);
        disk << setw(512) << ' ';
        disk.close();
        // 盘块使用掉
        return temp;
    }
    else
    { // 不是记录盘块
        superblock.fbstack[10 - superblock.fbptr] = -1;
        superblock.fbptr--;
        return temp;
    }
}

void bfree(int index)
{
    disk.open("disk.txt", ios::in | ios::out); // 清空回收盘块
    disk.seekp(514 * index);
    disk << setw(512) << ' ';
    disk.close();

    if (superblock.fbptr == 10)
    { // 栈已满，栈中内容计入回收盘，栈清空
        // 原有的10个空闲块组成一组空闲块，把当前归还的一块看作另一组空闲块（只有一块）的最后一块，
        // 把原有的10个空闲块的信息记录在该归还块上，空闲块栈中仅记录归还块
        disk.open("disk.txt", ios::in | ios::out);
        disk.seekp(514 * index);
        for (int i = 0; i < 10; i++)
        {
            disk << setw(3) << superblock.fbstack[i];
            superblock.fbstack[i] = -1;
        }
        disk.close();
        superblock.fbptr = 0;
    }
    // 回收盘块压栈
    superblock.fbstack[10 - superblock.fbptr - 1] = index;
    superblock.fbptr++;
}

void readsuper(void)
{
    // 读超级块到主存
    disk.open("disk.txt", ios::in | ios::out);

    int i;
    for (i = 0; i < 80; i++)
    { // 读空闲节点号栈
        disk >> superblock.fistack[i];
    }
    disk >> superblock.fiptr; // 空闲节点号栈指针
    for (i = 0; i < 10; i++)
    { // 读空闲盘块号栈
        disk >> superblock.fbstack[i];
    }
    disk >> superblock.fbptr; // 空闲盘块号栈指针
    disk >> superblock.inum;  // 空闲节点号总数
    disk >> superblock.bnum;  // 空闲盘块号总数

    disk.close();
}

void writesuper(void)
{
    // 主存写回超级块
    disk.open("disk.txt", ios::in | ios::out);

    int i;
    for (int i = 0; i < 80; i++)
    { // 写空闲节点号栈
        disk << setw(3) << superblock.fistack[i];
    }
    disk << setw(3) << superblock.fiptr; // 空闲节点号栈指针
    for (i = 0; i < 10; i++)
    { // 写空闲盘块号栈
        disk << setw(3) << superblock.fbstack[i];
    }
    disk << setw(3) << superblock.fbptr; // 空闲盘块号栈指针
    disk << setw(3) << superblock.inum;  // 空闲节点号总数
    disk << setw(3) << superblock.bnum;  // 空闲盘块号总数

    disk.close();
}

void readdir(INODE inode, int index, DIR &dir)
{
    disk.open("disk.txt", ios::in | ios::out);

    disk.seekg(514 * inode.addr[0] + 36 * index);
    disk >> dir.fname;
    disk >> dir.index;
    disk >> dir.parfname;
    disk >> dir.parindex;

    disk.seekp(514 * inode.addr[0] + 36 * index);
    disk << setw(15) << ' ';
    disk << setw(3) << ' ';
    disk << setw(15) << ' ';
    disk << setw(3) << ' ';

    disk.close();
}

void writedir(INODE inode, DIR dir, int index)
{
    disk.open("disk.txt", ios::in | ios::out);

    disk.seekp(514 * inode.addr[0] + 36 * index);
    disk << setw(15) << dir.fname;
    disk << setw(3) << dir.index;
    disk << setw(15) << dir.parfname;
    disk << setw(3) << dir.parindex;

    disk.close();
}

void _strtime(char *buf)
{
    time_t curtime = time(0);
    tm *ptm = localtime(&curtime);
    sprintf(buf, "%02d%02d%02d%02d", ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min);
}

void mk(char *filename, char *content)
{
    INODE inode, inode2;             // inode : 当前目录的， inode2 : 新建的文件的
    readinode(road[num - 1], inode); // 把当前节点 road[num-1] 内容读入索引节点
    if (havewpower(inode))
    { // 判断权限
        if (512 - inode.fsize < 36)
        { // 是否目录项已达到最多14个
            cout << "当前目录已满，创建子目录失败！" << endl;
        }
        else
        {
            int i, index2;
            if (havesame(filename, inode, i, index2))
            { // 有无重名存在
                cout << "该名已存在，创建失败！" << endl;
            }
            else
            { // 可以创建目录
                int size = strlen(content) + 1;
                if (size > 2048)
                {
                    cout << "文件太大，创建失败！" << endl;
                }
                else
                {
                    int bnum = (size - 1) / 512 + 1; // 计算盘块数(1~4)
                    int bid[4];
                    int iid = ialloc(); // 申请节点
                    if (iid != -1)
                    {
                        bool success = true;
                        for (int i = 0; i < bnum; i++)
                        { // 申请盘块
                            bid[i] = balloc();
                            if (bid[i] == -1)
                            {
                                cout << "盘块不够，创建数据文件失败！" << endl;
                                success = false;
                                ifree(iid); // 已申请的节点和盘块均释放掉
                                for (int j = i - 1; j >= 0; j--)
                                {
                                    bfree(bid[j]);
                                }
                                break;
                            }
                        }
                        if (success)
                        { // 当前目录盘块的修改
                            /*
                                当前目录对应一个文件，该文件的数据区中是一个个目录项。
                                现新建一个文件，把文件名，索引节点号等信息写入当前目录对应的文件，作为一个目录项
                            */
                            disk.open("disk.txt", ios::in | ios::out);
                            disk.seekp(514 * inode.addr[0] + inode.fsize); // 写目录名
                            disk << setw(15) << filename;                  // 写节点
                            disk << setw(3) << iid;
                            disk << setw(15) << curname;
                            disk << setw(3) << road[num - 1];
                            disk.close();
                            // 当前目录节点的修改
                            inode.fsize += 36;
                            char tmpbuf[9];
                            _strtime(tmpbuf);
                            strcpy(inode.ctime, tmpbuf);
                            // 新建目录节点的初始化
                            inode2.fsize = size; // 文件大小
                            inode2.fbnum = bnum; // 文件盘块数
                            int i;
                            for (i = 0; i < 4; i++)
                            { // 4个直接盘块号
                                if (i < bnum)
                                    inode2.addr[i] = bid[i];
                                else
                                    inode2.addr[i] = 0;
                            }
                            inode2.addr1 = 0;
                            inode2.addr2 = 0;
                            strcpy(inode2.owner, auser);  // 文件所有者
                            strcpy(inode2.group, agroup); // 文件所属组
                            // 文件类别及存储权限（默认最高）
                            strcpy(inode2.mode, "-rwxrwxrwx");
                            _strtime(tmpbuf);
                            strcpy(inode2.ctime, tmpbuf); // 最近修改时间
                            writeinode(inode2, iid);
                            // 新建文件盘块的初始化（内容写入），最后盘块不一定满
                            char temp;
                            disk.open("disk.txt", ios::in | ios::out);
                            disk.seekp(514 * bid[0]);
                            for (i = 0; i < size; i++)
                            {
                                temp = content[i];
                                disk << temp;
                                if (i % 512 == 511)
                                {
                                    disk << '\n';
                                }
                            }
                            disk.close();
                            cout << "文件已成功创建" << endl;
                        }
                    }
                    else
                        cout << "节点已用完，创建数据文件失败！" << endl;
                }
            }
        }
    }
    else
    {
        cout << "你没有权限" << endl;
    }
    writeinode(inode, road[num - 1]); // 把 inode 写入指定节点
}

void rm(char *filename)
{
    INODE inode, inode2;
    DIR dir;
    readinode(road[num - 1], inode); // 当前节点写入节点对象
    if (havewpower(inode))
    {                  // 判断权限
        int i, index2; // i为待删除目录目录项下标；index2为目录项中的待删子目录的节点号
        if (havesame(filename, inode, i, index2))
        {                              // 存在该子目录名
            readinode(index2, inode2); // 删除子目录的节点写入节点对象
            if (havewpower(inode2))
            { // 判断权限
                if (inode2.mode[0] == '-')
                { // 判断是数据文件海而非目录文件
                    // 回收盘块和节点
                    for (int ii = 0; ii < inode2.fbnum; ii++)
                    {
                        bfree(inode2.addr[ii]);
                    }
                    ifree(index2);
                    // 对当前目录盘块的修改，inode.addr[0]为当前盘块号，i为待删子目录的目录项下标
                    disk.open("disk.txt", ios::in | ios::out);
                    disk.seekp(514 * inode.addr[0] + 36 * i); // 清空当前盘块第i个子目录的目录项内容
                    disk << setw(36) << ' ';
                    disk.close();

                    for (int j = i + 1; j < (inode.fsize / 36); j++)
                    {                           // 后面的目录项前移一项
                        readdir(inode, j, dir); // inode 指向盘块读入
                        writedir(inode, dir, j - 1);
                    }

                    // 对当前目录节点的修改
                    inode.fsize -= 36;
                    char tmpbuf[9];
                    _strtime(tmpbuf);
                    strcpy(inode.ctime, tmpbuf);
                    cout << "文件已成功删除" << endl;
                }
                else
                {
                    cout << "目录文件应用rmdir命令删除" << endl;
                }
            }
            else
            {
                cout << "你没有权限" << endl;
            }
        }
        else
        {
            cout << "目录中不存在该子目录" << endl;
        }
    }
    else
    {
        cout << "你没有权限" << endl;
    }
}

void cp(char *string){
    // 把指定目录下的指定文件复制到当前目录下
    bool getit = false;    // 记录指定文件是否找到
    char content[2048];    // 保存内容
    char fname[14];    // 保存文件名

    char tcurname[14];    // 保存当秦路径
    int troad[20];
    int tnum = num;
    strcpy(tcurname, curname);
    for(int i=0;i<num;i++){
        troad[i] = road[i];
    }
    if(find(string)){    // 如果找到目标（当前路径会跟着改变）
        INODE inode;
        readinode(road[num-1], inode);
        if(inode.mode[0]=='-'){    // 确定是数据文件
            getit = true;
            // 文件名复制到fname变量，盘块内容复制到字符串 content[2048]
            strcpy(fname, curname);
            char temp = ' ';
            disk.open("disk.txt", ios::in | ios::out);
            int i, j;
            for(i=0,j=0;i<inode.fsize;i++,j++){
                disk.seekp(514*inode.addr[0]+i);
                disk >> temp;
                content[j] = temp;
                if(i%512==511){    // 跳过'\n'
                    i = i+2;
                }
            }
            content[j+1] = '\0';
            disk.close();
        }else{
            cout << "不能复制非数据文件" << endl;
        }
    }else{
        cout << "不能根据路径找到相关目录" << endl;
    }
    strcpy(curname, tcurname);    // 路径还原
    num = tnum;
    for(int ii=0;ii<tnum;ii++){
        road[ii] = troad[ii];
    }
    if(getit){
        mk(fname, content);
        cout << "文件复制完毕" << endl;
    }
}

void cat(char *filename){
    // 显示文件内容（当前目录下指定数据文件）
    INODE inode, inode2;
    readinode(road[num-1], inode);    // 当前节点写入节点对象
    int i, index2;    // i为子目录待显目录项下标，index2为目录项中的待显目录的节点号
    if(havesame(filename, inode, i, index2)){
        readinode(index2, inode);    // 要显示的数据文件的节点写入节点对象
        if(inode2.mode[0]=='-'){
            cout << "文件内容为：" << endl;
            char content[512];
            disk.open("disk.txt", ios::in | ios::out);
            for(int i=0;i<inode2.fbnum;i++){    // 遍历盘块并输出盘块内容
                disk.seekg(inode2.addr[i]*514);
                disk >> content;
                cout << content;
            }
            disk.close();
        }else{
            cout << "显示失败（目标是目录文件，cat是用来显示数据文件内容的）" << endl;
        }
    }else{
        cout << "目录中不存在该数据文件" << endl;
    }
}

bool havesame(char *dirname, INODE inode, int &i, int &index2){
    bool have = false;
    char name[14];
    disk.open("disk.txt", ios::in | ios::out);
    
    for(i=0;i<(inode.fsize/36);i++){    // 遍历所有的目录项
        disk.seekg(514*inode.addr[0]+36*i);
        disk >> name;
        if(!strcmp(dirname, name)){
            have = true;
            disk >> index2;
            break;
        }
    }

    disk.close();
    return have;
}

bool find(char *string){
    int ptr = 0;
    char name[14]="";
    INODE inode;
    // 读根目录
    for(int i=0;string[ptr]!='/';ptr++,i++){
        if(i==15) return 0;    // 超过命名最大长度
        name[i] = string[ptr];
    }
    if(!strcmp(name, "root")){    // 第1个应该是root
        strcpy(curname, "root");    // 初始化当前路径
        road[0] = 0;
        num = 1;
        for(;;){
            readinode(road[num-1], inode);    // 当前节点读入
            ptr++;
            char name[14] = "";
            for(int i=0;(string[ptr]!='/')&&(string[ptr]!='\0');ptr++,i++){
                // 从 string 读入一个名字
                if(i==15) return 0;    // 超过命名最大长度
                name[i] = string[ptr];
            }
            int ii, index2;    // 当前目录查找该目录项
            if(havesame(name, inode, ii, index2)){
                char tname[14];
                // 路径下一步
                disk.open("disk.txt", ios::in | ios::out);
                disk.seekg(514*inode.addr[0]+36*ii);
                disk >> tname;
                disk.close();

                strcpy(curname, tname);
                road[num] = index2;
                num++;
                // 判断字符串结尾
                if(string[ptr]=='\0'){
                    return 1;
                }
            }else{
                return 0;
            }
        }
    }else{
        return 0;
    }
}

void mkdir(char *dirname){
    // 创建目录（规定目录文件只占一个盘块）当前目录下创建
    INODE inode, inode2;
    readinode(road[num-1], inode);    // 把当前节点road[num-1]的内容读入索引节点
    if(havewpower(inode)){    // 判断权限
        if(512-inode.fsize<36){    // 是否目录项已达到最多14个
            cout << "当前目录已满，创建子目录失败！" << endl;
        }else{
            int i, index2;
            if(havesame(dirname, inode, i, index2)){    // 有无重名存在
                cout << "该目录已存在，创建失败！" << endl;
            }else{    // 可以创建目录
                int iid = ialloc();    // 申请节点
                if(iid!=-1){
                    int bid = balloc();    // 申请盘块
                    if(bid!=-1){
                        // 当前目录盘块的修改
                        disk.open("disk.txt", ios::in | ios::out);
                        disk.seekp(514*inode.addr[0]+inode.fsize);
                        disk << setw(15) << dirname;    // 写目录名
                        disk << setw(3) << iid;    // 写节点
                        disk << setw(15) << curname;
                        disk << setw(3) << road[num-1];
                        disk.close();
                        // 当前目录节点的修改
                        inode.fsize += 36;
                        char tmpbuf[9];
                        _strtime(tmpbuf);
                        strcpy(inode.ctime, tmpbuf);
                        // 新建目录节点的初始化
                        inode2.fsize = 0;    // 文件大小
                        inode2.fbnum = 1;    // 文件盘块数
                        inode2.addr[0] = bid;
                        for(int iii=1;iii<4;iii++)    // 指向0#表示没有指向
                            inode2.addr[iii] = 0;
                        inode2.addr1 = 0;    // 一次间址
                        inode2.addr2 = 0;    // 两次间址
                        strcpy(inode2.owner, auser);    // 文件所有者
                        strcpy(inode.group, agroup);    // 文件所属组
                        strcpy(inode2.mode, "drwxrwxrwx");    // 文件类别及存储权限（默认最高）
                        _strtime(tmpbuf);
                        strcpy(inode2.ctime, tmpbuf);    // 最近修改时间
                        writeinode(inode2, iid);
                        cout << "目录已成功创建" << endl;
                    }else{
                        ifree(iid);    // 释放刚申请的节点
                        cout << "盘块已用完，创建子目录失败！" << endl;
                    }
                }else{
                    cout << "节点已用完，创建子目录失败！" << endl;
                }
            }
        }
    }else{
        cout << "你没权限创建" << endl;
    }
    writeinode(inode, road[num-1]);    // 把 inode 写入指定节点
}

int t = 0;
/*
    在删除某个子目录时，目录是有足够权限删除的，但是目录里存在无权删除的文件，这个函数好像有bug
*/
void rmdir(char *dirname, int index){
    // 删除目录，当前目录下删除，参数为（子目录名，当前节点）
    t++;
    INODE inode, inode2;
    DIR dir;
    readinode(index, inode);    // 当前节点写入节点对象
    if(havewpower(inode)){    // 判断权限
        int i, index2;    // i为待删子目录目录项下标，index2为目录项中的待删子目录的节点
        if(havesame(dirname, inode, i, index2)){    // 存在该子目录名
            readinode(index2, inode2);    // 待删子目录的节点写入节点对象
            if(havewpower(inode2)){    // 判断权限
                if(inode2.mode[0]=='d'){    // 判断要删除的是目录文件而非数据文件
                    if(inode2.fsize!=0){    // 判断待删子目录有无子目录
                        char yes = 'y';
                        if(t==1){
                            cout << "该目录非空，如果删除的话，将失去目录下所有文件，要继续吗？(y/n)";
                            cin >> yes;
                        }
                        if(yes=='y'||yes=='Y'){
                            // 遍历待删子目录（inode2）所有子目录，递归将其删除
                            char name[14];
                            int index3;
                            INODE inode3;
                            for(int i=0;i<(inode2.fsize/36);i++){    // 遍历所有的目录项
                                disk.open("disk.txt", ios::in | ios::out);
                                disk.seekg(inode2.addr[0]*514+36*i);
                                disk >> name;
                                disk >> index3;
                                disk.close();
                                readinode(index3, inode3);
                                if(inode3.mode[0]=='d'){    // 是目录文件
                                    rmdir(name, index2);
                                }else{    // 是数据文件
                                    rm(name);
                                }
                            }
                            rmdir(dirname, index);    // 子目录空了后再删除自己
                        }else{
                            cout << "目录删除失败" << endl;
                        }
                    }else{    // 删除一个空目录
                        bfree(inode2.addr[0]);
                        ifree(index2);
                        // 对当前目录盘块的修改，inode.addr[0]为当前目录盘号，i为待删子目录目录项下标
                        disk.open("disk.txt",ios::in|ios::out);
                        disk.seekp(514*inode.addr[0]+36*i);    // 清空当前盘块第i个子目录的目录项记录内容
                        disk.close();
                        for(int j=i+1;j<(inode.fsize/36);j++){    // 后面的目录项全部前移一项
                            readdir(inode, j, dir);    // inode 指向盘块读入
                            writedir(inode, dir, j-1);
                        }
                    }
                    cout << "目录成功删除" << endl;
                    // 对当前目录节点的修改
                    inode.fsize -= 36;
                    char tmpbuf[9];
                    _strtime(tmpbuf);
                    strcpy(inode.ctime, tmpbuf);
                }else{
                    cout << "数据文件应用rm命令删除" << endl;
                }
            }else{
                cout << "你没有权限" << endl;
            }
        }else{
            cout << "目录中不存在该子目录" << endl;
        }
    }else{
        cout << "你没有权限" << endl;
    }
    writeinode(inode, index);
    t--;
}

void ls(void){
    // 显示当前节点的所有子目录
    INODE inode, inode2;
    readinode(road[num-1], inode);
    char name[14];
    int index;
    for(int i=0;i<(inode.fsize/36);i++){    // 遍历所有的目录项
        disk.open("disk.txt", ios::in | ios::out);
        disk.seekg(inode.addr[0]*514+36*i);
        disk >> name;
        disk >> index;
        disk.close();
        readinode(index, inode2);
        cout << setw(15) << name;
        cout << setw(6) << inode2.fsize;
        cout << setw(6) << inode2.owner;
        cout << setw(6) << inode2.group;
        cout << setw(12) << inode2.mode;
        cout << setw(10) << inode2.ctime;
        cout << endl;
    }
    cout << "显示完毕" << endl;
}

void cd(char *string){
    if(!strcmp(string, ".")){    // 切换到当前目录
        cout << "已切换到当前目录" << endl;
        return;
    }
    if(!strcmp(string, "/")){    // 切换到根目录
        strcpy(curname, "root");
        road[0] = 0;
        num = 1;
        cout << "已切换到根目录" << endl;
        return;
    }
    if(!strcmp(string, "..")){    // 切换到父目录
        if(strcmp(curname, "root")){    // 当前不是根目录才可切换
            INODE inode;
            readinode(road[num-2], inode);    // 父目录节点号
            char name[14];
            disk.open("disk.txt", ios::in | ios::out);
            disk.seekg(inode.addr[0]*514+18);
            disk >> name;
            disk.close();
            strcpy(curname,name);
            num--;
            cout << "已切换到父目录" << endl;
            return;
        }
        cout << "当前已是根目录" << endl;
        return;
    }
    char *per = strchr(string, (int)'/');
    if(per==NULL){    // 没有"/"的是子目录名，切换到某一子目录
        INODE inode, inode2;
        int i, index2;
        readinode(road[num-1], inode);
        char name[14];
        if(havesame(string, inode, i, index2)){
            readinode(index2, inode2);
            if(inode2.mode[0]=='d'){
                disk.open("disk.txt", ios::in | ios::out);
                disk.seekg(514*inode.addr[0]+36*i);
                disk >> name;
                disk.close();
                strcpy(curname, name);
                road[num] = index2;
                num++;
                cout << "已切换到子目录" << endl;
                return;
            }
            cout << "不能根据路径找到相关的目录，因为 " << string << " 是数据文件" << endl;
        }else{
            cout << "该子目录不存在，不能根据路径找到相关目录" << endl;
        }
    }else{    // 根据指定路径切换目录
        char tcurname[14];    // 保存当前路径
        int troad[20];
        int tnum = num;
        strcpy(tcurname, curname);
        for(int i=0;i<num;i++){
            troad[i] = road[i];
        }
        if(find(string)){    // 如果找到目标（当前路径随之更改）
            INODE inode;
            readinode(road[num-1], inode);
            if(inode.mode[0]=='d'){    // 确定为目录文件
                cout << "已切换到该目录" << endl;
                return;
            }
        }
        cout << "不能根据路径找到相关目录" << endl;    // 找不到目标，还原
        strcpy(curname, tcurname);
        num = tnum;
        for(int i=0;i<tnum;i++){
            road[i] = troad[i];
        }
    }
}

bool login(void){
    // 登录
    char auser2[6];    // 存放用户组内的信息
    char apwd2[6];    // 存放用户组内的信息
    cout << "-> your name: ";
    cin >> auser;
    cout << "-> your password: ";
    cin >> apwd;
    bool have = false;
    user.open("user.txt", ios::in);
    for(int n=0;n<usernum;n++){
        // 用户名为 18n+0~18n+5，密码为 18n+6~18n+11，用户组为 18n+12~18n+17
        user.seekg(18*n);

        user >> auser2 >> apwd2;
        int a = strcmp(auser, auser2);
        int b = strcmp(apwd, apwd2);
        if((!a)&&(!b)){
            have = true;
            user >> agroup;
            break;
        }
    }
    user.close();
    if(have==true) return 1;
    return 0;
}

void changepassword(void){
    char auser2[6];
    char apwd2[6];
    cout << "请输入原有密码：";
    cin >> apwd2;
    if(!strcmp(apwd, apwd2)){
        user.open("user.txt", ios::in | ios::out);
        int n;
        for(n=0;n<usernum;n++){
            user.seekg(18*n);
            user >> auser2;
            if(!strcmp(auser, auser2)){
                user >> agroup;
                break;
            }
        }
        cout << "请设置新的密码：";
        cin >> apwd2;
        user.seekp(18*n+6);
        user << setw(6) << apwd2;
        cout << "密码修改成功" << endl;
        user.close();
    }else{
        cout << "输入错误" << endl;
    }
}

bool havewpower(INODE inode){
    // 判断当前用户对指定的节点有无写权限
     if(!strcmp(auser, inode.owner)){    // 是文件所有者
        if(inode.mode[2]=='w') return true;
        return false;
     }else{
         if(!strcmp(agroup, inode.group)){    // 在组内
            if(inode.mode[5]=='w') return true;
            return false;
         }else{    // 其他用户
             if(inode.mode[8]=='w') return true;
             return false;
         }
     }
}

void chmod(char *name){
    INODE inode, inode2;
    readinode(road[num-1], inode);    // 当前节点写入节点对象
    int i, index2;    // i为目录项下标，index2为目录项中节点号

    if(havesame(name, inode, i, index2)){
        readinode(index2, inode2);
        if(havewpower(inode2)){
            char amode[3];
            cout << "1 表示所有者，4 表示组内，7 表示其他用户" << endl;
            cout << "a 表示-wx模式，b 表示r-x模式，c 表示rwx模式" << endl;
            cout << "请输入修改方案（例如4c）： ";
            cin >> amode;
            if(amode[0]=='1'||amode[0]=='4'||amode[0]=='7'){
                switch (amode[1])
                {
                case 'a':
                    inode2.mode[(int)amode[0]-48] = '-';
                    inode2.mode[(int)amode[0]-48+1] = 'w';
                    break;
                case 'b':
                    inode2.mode[(int)amode[0]-48]='r';
                    inode2.mode[(int)amode[0]-48+1]='-';
                    break;
                case 'c':
                    inode2.mode[(int)amode[0]-48]='r';
                    inode2.mode[(int)amode[0]-48+1]='w';
                    break;
                default:
                    cout << "输入不合法" << endl;
                    return;
                }
                cout << "修改完毕" << endl;
            }else{
                cout << "输入不合法" << endl;
                return;
            }
        }else{
            cout << "你无权修改该子目录或文件" << endl;
            return;
        }
    }else{
        cout << "不存在该子目录或文件" << endl;
        return;
    }
    char tmpbuf[9];
    _strtime(tmpbuf);
    strcpy(inode.ctime, tmpbuf);
    strcpy(inode2.ctime, tmpbuf);
    writeinode(inode, road[num-1]);
    writeinode(inode2,index2);
}

void chown(char *name){
    INODE inode, inode2;
    readinode(road[num-1], inode);    // 当前节点写入节点对象
    int i, index2;    // i为目录项下标，index2为目录项中节点号
    if(havewpower(inode)){
        if(havesame(name, inode, i, index2)){
            readinode(index2, inode2);
            if(havewpower(inode2)){
                char owner2[6];
                char auser2[6];
                char group2[6];
                cout << "请输入改后的文件所有者：";
                cin >> owner2;
                bool is = false;    // 判断输入的是否为合法用户名
                user.open("user.txt", ios::in);
                for(int n=0;n<usernum;n++){
                    user.seekg(18*n);
                    user >> auser2;
                    if(!strcmp(owner2,auser2)){
                        is = true;
                        user.seekg(18*n+12);
                        user >> group2;
                        break;
                    }
                }
                user.close();
                if(is){
                    strcpy(inode2.owner, owner2);
                    strcpy(inode2.group, group2);
                    cout << "修改成功";
                    // 修改当前节点和子节点
                    char tmpbuf[9];
                    _strtime(tmpbuf);
                    strcpy(inode.ctime, tmpbuf);
                    strcpy(inode2.ctime, tmpbuf);
                    writeinode(inode, road[num-1]);
                    writeinode(inode2, index2);
                }else{
                    cout << "不存在该用户，修改失败" << endl;
                }
            }else{
                cout << "你没有权限" << endl;
            }
        }else{
            cout << "不存在该子目录或文件" << endl;
        }
    }else{
        cout << "你没有权限" << endl;
    }
}

void chgrp(char *name){
    // 改变文件所属组
    INODE inode, inode2;
    readinode(road[num-1], inode);    // 当前节点写入节点对象
    int i, index2;    // i为目录项下标，index2为目录项中节点号

    if(havewpower(inode)){
        if(havesame(name, inode, i, index2)){
            readinode(index2, inode2);
            if(havewpower(inode2)){
                char group2[6];
                char agroup2[6];
                cout << "请输入改后的文件所属组：";
                cin >> group2;
                bool is = false;    // 判断输入的是否为合法组名
                user.open("user.txt", ios::in);
                for(int n=0;n<usernum;n++){
                    user.seekg(18*n+12);
                    user >> agroup2;
                    if(!strcmp(group2, agroup2)){
                        is = true;
                        break;
                    }
                }
                user.close();
                if(is){
                    strcpy(inode2.group, group2);
                    cout << "修改成功";
                    char tmpbuf[9];
                    _strtime(tmpbuf);
                    strcpy(inode.ctime, tmpbuf);
                    strcpy(inode2.ctime, tmpbuf);
                    writeinode(inode, road[num-1]);
                    writeinode(inode2, index2);
                }else{
                    cout << "不存在该组，修改失败" << endl;
                }
            }else{
                cout << "你没有权限" << endl;
            }
        }else{
            cout << "不存在该子目录或文件" << endl;
        }
    }else{
        cout << "你没有权限" << endl;
    }
}

void chnam(char *name){
    INODE inode, inode2;
    readinode(road[num-1], inode);
    int i, index2;
    if(havewpower(inode)){
        if(havesame(name, inode, i, index2)){
            readinode(index2, inode2);
            if(havewpower(inode2)){
                char name2[14];
                cout << "请输入更改后的文件名：";
                cin >> name2;
                // 这里没有判断重命名后是否重名
                disk.open("disk.txt", ios::in | ios::out);
                disk.seekp(514*inode.addr[0]+36*i);
                disk << setw(15) << name2;
                disk.close();
                // 如果是目录文件，其下的所有目录项都要改parfname[14]
                if(inode2.mode[0]=='d'){
                    disk.open("disk.txt", ios::in | ios::out);
                    for(int i=0;i<(inode2.fsize/36);i++){    // 遍历所有的目录项
                        disk.seekg(514*inode2.addr[0]+36*i+18);
                        disk << setw(15) << name2;
                    }
                    disk.close();
                }
                cout << "更改成功" << endl;
                char tmpbuf[9];
                _strtime(tmpbuf);
                strcpy(inode.ctime, tmpbuf);
                strcpy(inode2.ctime, tmpbuf);
                writeinode(inode, road[num-1]);
                writeinode(inode2, index2);
            }else{
                cout << "你没有权限" << endl;
            }
        }else{
            cout << "不存在该子目录或文件" << endl;
        }
    }else{
        cout << "你没有权限" << endl;
    }
}

void printroad(void){
    cout << "root";
    INODE inode;
    int nextindex;
    char name[14];
    for(int i=0;i+1<num;i++){
        readinode(road[i], inode);
        disk.open("disk.txt", ios::in | ios::out);
        for(int j=0;j<(inode.fsize/36);j++){    // 遍历所有的目录项
            disk.seekg(514*inode.addr[0]+36*j);
            disk >> name;
            disk >> nextindex;
            if(nextindex==road[i+1]){
                cout << '/';
                cout << name;
                break;
            }
        }
        disk.close();
    }
}

void initial(){
    // 用户组初始化
    user.open("user.txt", ios::in | ios::out | ios::trunc);
    
    user << setw(6) << "adm";
    user << setw(6) << "132";
    user << setw(6) << "adm";
    
    user << setw(6) << "cnj";
    user << setw(6) << "123";
    user << setw(6) << "adm";

    user << setw(6) << "jtq";
    user << setw(6) << "123";
    user << setw(6) << "guest";

    user.close();

    // disk 初始化
    disk.open("disk.txt", ios::in | ios::out | ios::trunc);
    if(!disk){
        cout << "can't use the disk" << endl;
        exit(1);
    }
    int i;
    for(i=0;i<100;i++){    // 先全部填“空”
        disk << setw(512) << ' ';
        disk << "\n";
    }

    // 超级块初始化 0#盘块
    disk.seekp(0);    // 空闲节点号栈 80*3=240 B (0~239)
    disk << setw(3) << -1;    // 0#的节点已使用，就赋-1(0~2)
    for(i=1;i<=79;i++){
        disk << setw(3) << i;
    }
    disk << setw(3) << 79;    // 空闲节点栈指针==当前节点数(240~242)
    for(i=0;i<10;i++){    // 空闲盘块号栈10*3=30 B (243~270) 11#盘块已使用末尾盘块为 21#
        disk << setw(3) << i+12;
    }
    disk << setw(3) << 10;    // 空闲节点栈指针==当前栈中盘块数(271~273)
    disk << setw(3) << 80;    // 空闲节点总数(274~276)
    disk << setw(3) << 89;    // 空闲盘块总数(277~279)
    // 空闲节点栈互斥访问标志（尚不用）
    // 空闲盘块栈访问标志（尚不用）
    
    // 根目录文件节点的初始化，是 #1 盘块开始的
    disk.seekp(514);
    disk << setw(6) << 0;    // 文件大小
    disk << setw(6) << 1;    // 文件盘块数
    disk << setw(3) << 11;
    disk << setw(3) << 0;    // 指向 0# 表示没有指向
    disk << setw(3) << 0;
    disk << setw(3) << 0;
    disk << setw(3) << 0;    // 一个一次间址
    disk << setw(3) << 0;    // 一个两次间址
    disk << setw(6) << "adm";    // 文件所有者，根目录由超级用户所有
    disk << setw(6) << "adm";    // 文件所属组
    disk << setw(11) << "drwxrwxrwx";    // 文件类别及存储权限
    char tmpbuf[9];
    _strtime(tmpbuf);
    disk << setw(9) << tmpbuf;
    // 根目录文件初始化，如果尚无子目录，不用初始化
    // 空闲盘块初始化，只用初始化记录盘块 i# 成组连接最后记录盘块最后记录号后要压个 0
    for(i=21;i<100;i++){
        if(i%10==1){
            disk.seekp(514*i);    // 定位记录盘块
            for(int j=0;j<10;j++){
                int temp = i+j+1;
                if(temp<100){
                    disk << setw(3) << temp;
                }
            }
        }
    }
    disk << setw(3) << 0;    // 最后记录盘块最后记录号后要压个0
    disk.close();
}

void getcommand(){
    char commond[10];
    for(;;){    // 接受并解释命令
        bool have = false;    // 记录命令接收否
        cout << '\n';
        printroad();
        cout << ">";
        cin >> commond;
        if(!strcmp(commond, "cd")){
            have = true;
            char string[100];
            cin >> string;
            cd(string);
        }
        if(!strcmp(commond, "mksome")){    // 构建基本文件结构
            have = true;
            cout << "bin" << endl;  mkdir("bin");
            cout << "dev" << endl;  mkdir("dev");
            cout << "lib" << endl;  mkdir("lib");
            cout << "etc" << endl;  mkdir("etc");
            cout << "usr" << endl;  mkdir("usr");
            cout << "tmp" << endl;  mkdir("tmp");
        }
        if(!strcmp(commond, "mkdir")){
            have = true;
            char dirname[14];
            cin >> dirname;
            mkdir(dirname);
        }
        if(!strcmp(commond, "rmdir")){
            have = true;
            char dirname[14];
            cin >> dirname;
            rmdir(dirname, road[num-1]);
        }
        if(!strcmp(commond, "mk")){
            have=true;
            char filename[14];
            cin >> filename;
            char content[2048];
            cout << "请输入文件内容（1～2048位）：" << endl;
            cin >> content;
            mk(filename, content);
        }
        if(!strcmp(commond, "cp")){
            have=true;
            char string[100];
            cin >> string;
            cp(string);
        }
        if(!strcmp(commond, "rm")){
            have = true;
            char filename[14];
            cin >> filename;
            rm(filename);
        }
        if(!strcmp(commond, "cat")){
            have = true;
            char filename[14];
            cin >> filename;
            cat(filename);
        }
        if(!strcmp(commond, "chmod")){
            have = true;
            char name[14];
            cin >> name;
            chmod(name);
        }
        if(!strcmp(commond, "chown")){
            have = true;
            char name[14];
            cin >> name;
            chown(name);
        }
        if(!strcmp(commond, "chgrp")){
            have = true;
            char name[14];
            cin >> name;
            chgrp(name);
        }
        if(!strcmp(commond, "chnam")){
            have = true;
            char name[14];
            cin >> name;
            chnam(name);
        }
        if(!strcmp(commond, "pwd")){
            have = true;
            cout << "您当前的目录为： " << curname;
        }
        if(!strcmp(commond, "ls")){
            have = true;
            ls();
        }
        if(!strcmp(commond, "login")){
            have = true;
            cout << "账号 " << auser << " 已注销" << endl;
            while(!login()) cout << "wrong!" << endl;
            cout << "login success" << endl;
            cout << "*******Welcom " << auser << "*******" << endl;
        }
        if(!strcmp(commond, "passwd")){
            have=true;
            changepassword();
        }
        if(!strcmp(commond, "reset")){
            have = true;
            initial();
            cout << "系统已重置" << endl;
        }
        if(!strcmp(commond, "exit")){
            have = true;
            return;
        }
        if(have==false){
            cout << commond << " is not a legal command!" << endl;
        }
    }
}
