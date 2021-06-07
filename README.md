# MyFileSystem
simple fuse file system

主要参考了这里[link](https://blog.csdn.net/stayneckwind2/article/details/82876330)


## RUN
+ 因为`fuse`本身涉及系统权限，打开WSL后`sudo su`切换权限后再使用

+ `mkdir /tmp/disk`，创建用于mount的目录，然后可以`cd /tmp/disk`方便调试

+ `bash init.sh`脚本可以自动编译、运行程序，fs这个程序开了debug模式，会一直运行在WSL里

+ 新开一个WSL，正常执行文件操作即可

## COMMAND
+ `touch {fileName}`新建文件
+ `echo "{text}">{fileName}`写入数据，若文件不存在自动创建
+ `cat {fileName}`读取文件中的所有数据
+ `mkdir {dirName}`新建目录
+ `mv {oldName} {newName}`进行重命名，只支持当前目录下某个文件或目录的重命名
+ `ls -l`查看目录下的所有文件，目前会显示文件的时间、大小

## TIPS
调试的时候可能因为误操作导致fuse陷入不可退出的状态，此时杀死WSL对应的线程也没用，可以用管理员模式打开Windows的终端，执行`net stop LxssManager`再执行`net start LxssManager`，重启WSL即可。