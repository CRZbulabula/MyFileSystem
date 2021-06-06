make clean && make
sudo umount /tmp/disk
sudo rm /tmp/data
sudo ./fs /tmp/disk -o nonempty -d
