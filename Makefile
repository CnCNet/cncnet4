CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: lan internet server

lan: src/wsock32_lan.c src/sockets.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-lan/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP LAN patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32-lan.dll src/wsock32_lan.c src/sockets.c src/wsock32.def src/dll.o $(LIBS)

internet: src/wsock32_internet.c src/sockets.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-internet/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP Internet patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32-internet.dll src/wsock32_internet.c src/sockets.c src/config.c src/wsock32.def src/dll.o $(LIBS)

server: src/server.c src/sockets.c src/sockets.h
	gcc $(CFLAGS) -o carmanet-server src/server.c src/sockets.c

clean:
	rm -f wsock32-lan.dll wsock32-internet.dll carmanet-server src/dll.o
