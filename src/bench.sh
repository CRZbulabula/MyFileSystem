cargo build --release --manifest-path ../mfs/Cargo.toml
make clean && make

if [[ "$tmpdisk" ]]; then
    sudo umount /tmp/disk
    sudo rm -f /tmp/disk/*
    sudo ./fs /tmp/disk
else
    sudo umount ./disk
    sudo rm -f ./disk/*
    sudo ./fs ./disk
fi

# sudo rm -f ./disk/*
# time dd if=/dev/zero of=./disk/data bs=4k count=2560
# dd if=./disk/data bs=4k count=2560 of=/dev/null

sudo rm -f ./disk/*
sudo fio --randrepeat=1 --ioengine=libaio --direct=1 --gtod_reduce=1 --name=test --filename=./disk/data --bs=4k --iodepth=64 --size=10M --readwrite=randrw --rwmixread=75
sudo rm -f ./disk/*
sudo fio --randrepeat=1 --ioengine=libaio --direct=1 --gtod_reduce=1 --name=test --filename=./disk/data --bs=4k --iodepth=64 --size=10M --numjobs=2 --readwrite=randrw --rwmixread=75
sudo rm -f ./disk/*
sudo fio --randrepeat=1 --ioengine=libaio --direct=1 --gtod_reduce=1 --name=test --filename=./disk/data --bs=4k --iodepth=64 --size=10M --numjobs=4 --readwrite=randrw --rwmixread=75
sudo rm -f ./disk/*
sudo fio --randrepeat=1 --ioengine=libaio --direct=1 --gtod_reduce=1 --name=test --filename=./disk/data --bs=4k --iodepth=64 --size=10M --numjobs=8 --readwrite=randrw --rwmixread=75