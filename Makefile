CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall -D_DEBUG
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll launcher

dll: src/cncnet.c src/net.c src/loader.c
	sed 's/__REV__/$(REV)/g' src/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet patch/g' | $(WINDRES) -O coff -o src/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -DINTERNET -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/cncnet.c src/net.c src/cncnet.def src/loader.c src/dll.o $(LIBS)

launcher: src/launcher.c
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -o cncnet-launcher.exe src/launcher.c

clean:
	rm -f cncnet.dll cncnet-launcher.exe src/dll.o
