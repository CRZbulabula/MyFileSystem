/mfs目录下为rust代码
/src目录下为c代码实现的fuse文件系统


运行文件系统前，需要先安装rust
curl https://sh.rustup.rs -sSf | sh
随后命令行显示
1) Proceed with installation (default)
2) Customize installation
3) Cancel installation
>
输入1然后回车即可

在/src目录下建立空的子目录disk用于mount，运行 sudo bash init.sh即可运行文件系统，
此时文件系统在debug模式下，
在/src新打开一个命令行，cd ./disk，切换到mount目录中，即可测试baseline功能


运行测试前需要先安装fio
sudo apt-get install fio
在/src目录下sudo bash bench.sh进行io性能测试