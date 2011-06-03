CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall -D_WIN32_WINNT=0x0500 -I./src/
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=0

wsock32: src/wsock32.c src/sockets.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/wsock32/g' | sed 's/__GAME__/Carmageddon 95 UDP LAN patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -Wl,--enable-stdcall-fixup -shared -s -o wsock32.dll src/wsock32.c src/sockets.c src/wsock32.def src/dll.o $(LIBS)

clean:
	rm -f wsock32.dll src/dll.o
