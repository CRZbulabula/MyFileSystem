# MyFileSystem
simple fuse file system

主要参考了这里[link](https://blog.csdn.net/stayneckwind2/article/details/82876330)


## EXAMPLE
+ 首先`sudo mkdir /tmp/disk`，创建用于mount的目录

+ 然后`sudo bash init.sh`，fs这个程序开了debug模式，会一直运行在WSL里

+ 新开一个WSL，执行`sudo mkdir /tmp/disk/mdir`，可以创建一个目录，注意`/tmp/disk`路径是必须的，因为`/tmp/disk`是fuse mount的地方

+ 在新开的WSL执行`sudo touch /tmp/disk/mfile`，可以创建一个文件

+ 执行`sudo ls -l /tmp/disk`可以查看刚刚新建的文件

## TIPS
调试的时候可能因为误操作导致fuse陷入不可退出的状态，此时杀死WSL对应的线程也没用，可以用管理员模式打开Windows的终端，执行`net stop LxssManager`再执行`net start LxssManager`，重启WSL即可。