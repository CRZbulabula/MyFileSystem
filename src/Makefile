CC := gcc
CFLAGS := -g -Wall -D_FILE_OFFSET_BITS=64 -lpthread
OBJS := $(patsubst %.c, %.o, $(wildcard *.c))
STATIC_LIB := ../mfs/target/release/libmfs.a

LIBS := -lfuse -ldl -lpthread -lm

TARGET := fs

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(STATIC_LIB) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)