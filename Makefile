CC=gcc
REV=$(shell sh -c 'git rev-parse --short @{0}')
DLL_CFLAGS=-std=c99 -pedantic -Wall -O3 -s -Wall
CLIENT_CFLAGS=-std=c99 -pedantic -Wall -Os -s -Wall -I. -Ires -DCNCNET_VERSION=\"$(REV)\"

all: cncnet.dll cncnet.exe

cncnet.dll: src/wsock32.c src/thipx32.c res/dll.def
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | i586-mingw32msvc-windres -O coff -o res/dll.o
	i586-mingw32msvc-gcc $(DLL_CFLAGS) -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/wsock32.c src/thipx32.c res/dll.def res/dll.o -lws2_32

cncnet.exe: res/cncnet.rc.in src/client.c src/connect.c src/http.c
	sed 's/__REV__/$(REV)/g' res/cncnet.rc.in | i586-mingw32msvc-windres -o res/cncnet.rc.o
	i586-mingw32msvc-gcc $(CLIENT_CFLAGS) -mwindows -o cncnet.exe src/client.c src/connect.c src/http.c res/cncnet.rc.o -Wl,--file-alignment,512 -Wl,--gc-sections -lws2_32 -lwininet -lcomctl32
	echo $(REV) > version.txt

clean:
	rm -f cncnet.exe res/*.o
