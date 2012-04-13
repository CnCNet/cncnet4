CC=gcc
REV=$(shell sh -c 'git rev-parse --short @{0}')
DEDICATED_CFLAGS=-O2 -g $(CFLAGS) -DVERSION=\"git~$(REV)\"
CLIENT_CFLAGS=-O0 -s -Wall -g0 -DCNCNET_REV=\"$(REV)\"

all: dedicated dll

dedicated: src/dedicated.c src/net.c src/net.h src/log.c
	$(CC) $(DEDICATED_CFLAGS) -o cncnet-dedicated src/dedicated.c src/net.c src/log.c

dedicated-win32: src/dedicated.c src/net.c src/net.h src/log.c
	i586-mingw32msvc-gcc $(DEDICATED_CFLAGS) -o cncnet-dedicated.exe src/dedicated.c src/net.c src/log.c -lws2_32

dll: src/dll.c src/net.c
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | i586-mingw32msvc-windres -O coff -o res/dll.o
	i586-mingw32msvc-gcc $(CLIENT_CFLAGS) -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/dll.c src/net.c res/dll.def res/dll.o -lws2_32

clean:
	rm -f cncnet-dedicated cncnet-dedicated.exe cncnet.dll res/*.o
