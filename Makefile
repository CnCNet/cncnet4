CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll launcher

dll: src/cncnet.c src/net.c src/loader.c
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | $(WINDRES) -O coff -o res/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -DINTERNET -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/cncnet.c src/net.c src/cncnet.def src/loader.c res/dll.o $(LIBS)

launcher: src/launcher.c
	sed 's/__REV__/$(REV)/g' res/launcher.rc.in | sed 's/__FILE__/cncnet-launcher/g' | sed 's/__GAME__/CnCNet Internet Launcher/g' | $(WINDRES) -O coff -o res/launcher.o
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -o cncnet-launcher.exe src/launcher.c res/launcher.o

clean:
	rm -f cncnet.dll cncnet-launcher.exe res/dll.o res/launcher.o
