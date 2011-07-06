CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: lan internet launcher server

lan: src/wsock32.c src/wsock32_lan.c src/net.c src/loader.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-lan/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP LAN patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32-lan.dll src/wsock32.c src/wsock32_lan.c src/net.c src/wsock32.def src/loader.c src/dll.o $(LIBS)

internet: src/wsock32.c src/wsock32_internet.c src/net.c src/loader.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-internet/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP Internet patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -DINTERNET -Wl,--enable-stdcall-fixup -shared -s -o wsock32-internet.dll src/wsock32.c src/wsock32_internet.c src/net.c src/config.c src/wsock32.def src/loader.c src/dll.o $(LIBS)

launcher: src/launcher.c
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -o carmanet-launcher.exe src/launcher.c

server: src/server.c src/net.c src/net.h
	gcc $(CFLAGS) -o carmanet-server src/server.c src/net.c

clean:
	rm -f wsock32-lan.dll wsock32-internet.dll carmanet-launcher.exe carmanet-server src/dll.o
