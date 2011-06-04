CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall -D_WIN32_WINNT=0x0500 -I./src/
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: internet

lan: src/wsock32_lan.c src/sockets.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-lan/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP LAN patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32-lan.dll src/wsock32_lan.c src/sockets.c src/wsock32.def src/dll.o $(LIBS)

internet: src/wsock32_internet.c src/sockets.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32-internet/g' | sed 's/__GAME__/Carmageddon & Carmageddon 2 UDP Internet patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32-internet.dll src/wsock32_internet.c src/sockets.c src/config.c src/wsock32.def src/dll.o $(LIBS)

clean:
	rm -f wsock32.dll src/dll.o
