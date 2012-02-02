CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall -g0
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll

dll: src/cncnet.c src/net.c
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | $(WINDRES) -O coff -o res/dll.o
	$(CC) $(CFLAGS) -DCNCNET_REV=\"$(REV)\" -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/cncnet.c src/net.c src/cncnet.def res/dll.o $(LIBS)

clean:
	rm -f cncnet.dll res/*.o
