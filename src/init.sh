cargo build --release --manifest-path ../mfs/Cargo.toml
make clean && make
sudo umount /tmp/disk
sudo rm -f /tmp/disk/*
sudo ./fs /tmp/disk -o nonempty -d
