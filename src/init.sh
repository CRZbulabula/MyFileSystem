cargo build --release --manifest-path ../mfs/Cargo.toml
make clean && make

if [[ "$tmpdisk" ]]; then
    sudo umount /tmp/disk
    sudo rm -f /tmp/disk/*
    sudo ./fs /tmp/disk -o nonempty -d
else
    sudo umount ./disk
    sudo rm -f ./disk/*
    sudo ./fs ./disk -o nonempty -d
fi


