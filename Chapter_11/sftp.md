# 小型远程访问FTP服务系统

## 访问命令格式

```bash
# ftp://客户名:密码@FTP服务器IP或域名:FTP命令端口/路径/文件名
ftp://user:passwd@ftp.cs.nju.edu.cn:2008/soft/list.txt
```

## 传输模式

* ASCII传输模式
* 二进制传输模式

## 工作模式

* PORT模式

  ​	FTP客户端首先与FTP服务器的TCP 21端口建立连接，客户端需要接收数据的时候，在这个连接通道上发送PORT命令。PORT命令包含客户端接收数据的端口。在传送数据的时候，服务器端通过自己的TCP 20端口连接至客户端的指定端口并发送数据。

* PASV模式

  ​	FTP服务器受到PASV命令后，随机打开一个高端端口（端口号大于1024），并通知客户端在这个端口上传送数据请求。客户端将通过此端口建立到FTP服务器的连接。随后，FTP服务器将通过该连接进行数据传送。

## 基本功能

1. 基于套接字的客户/服务器通信模式
2. 远程登录功能
3. 并发执行及管理功能
4. 活动客户计数功能
5. 文件管理功能

## 实现要点

* FTP服务器21端口一直等待连接，连接成功则用别的端口创建线程提供服务。
* 客户计数需要用互斥锁保护计数器。
* 注意区分监听连接的socket和真正用于传输数据的socket，不要用混了。

## 运行方式

```bash
# 编译FTP服务器
gcc sFTPserver.c sFTPserver.h sFTPcnt.c sFTPcnt.h sFTPfns.c sFTPfns.h sFTPfile.c sFTPfile.h sFTPpthread.c sFTPpthread.h -o Server -lpthread
# 运行FTP服务器
./Server
# 编译FTP客户端
gcc sFTPclient.c sFTPclient.h sFTPfns.c sFTPfns.h sFTPfile.c sFTPfile.h sFTPpthread.c sFTPpthread.h -o Client -lpthread
# 运行FTP客户端
./Client
```

## TODO

* 客户端自动提取登录信息
* 服务器端在启动服务线程后，主线程可一边等待输入命令，如查看当前在线人数
* 登录成功后，用于传输数据的端口仅允许特定IP请求连接
* 分配随机端口（目前用于传输数据的端口固定了8081、8082）
* 写入文件无效的BUG
* 实现其他文件操作（目前仅支持上传和下载）
* 读写文件操作，当前仅支持1024 bytes的内容
* 目前connect相关的操作并不是封装在函数内的