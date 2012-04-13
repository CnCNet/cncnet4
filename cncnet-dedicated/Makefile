CC=gcc
REV=`git rev-parse --short @{0}`
CFLAGS=-O2 -g -Wall -DVERSION=\"git~$(REV)\"

all: dedicated

dedicated: src/dedicated.c src/net.c src/net.h src/log.c
	$(CC) $(CFLAGS) -o cncnet-dedicated src/dedicated.c src/net.c src/log.c

win32: src/dedicated.c src/net.c src/net.h src/log.c
	i586-mingw32msvc-gcc $(CFLAGS) -o cncnet-dedicated.exe src/dedicated.c src/net.c src/log.c -lws2_32

clean:
	rm -f cncnet-dedicated cncnet-dedicated.exe
