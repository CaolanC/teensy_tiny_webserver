PREFIX ?= /usr/local
LIB_DIR = $(PREFIX)/lib
INCLUDE_DIR = $(PREFIX)/include

I_FILES = $(wildcard src/**.c)
O_FILES = $(I_FILES:.c=.o)

all:
	gcc -Isrc -Iinclude src/*.c examples/main.c -o ./build/example
	./build/example
