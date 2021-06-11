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

sudo rm -f ./disk/*
time dd if=/dev/zero of=./disk/data bs=4k count=2560
dd if=./disk/data bs=4k count=2560 of=/dev/null