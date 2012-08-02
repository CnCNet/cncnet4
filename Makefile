CC=gcc
REV=$(shell sh -c 'git rev-parse --short @{0}')
DEDICATED_CFLAGS=-O2 -g $(CFLAGS) -DVERSION=\"git~$(REV)\"
DLL_CFLAGS=-O0 -s -Wall -g0 -DCNCNET_REV=\"$(REV)\"
CLIENT_CFLAGS=-pedantic -Wall -Os -s -Wall -I. -Ires -DCNCNET_VERSION=\"$(REV)\"

all: cncnet-dedicated cncnet.dll cncnet.exe

cncnet-dedicated: src/dedicated.c src/net.c src/net.h src/log.c
	$(CC) $(DEDICATED_CFLAGS) -o cncnet-dedicated src/dedicated.c src/net.c src/log.c

cncnet-dedicated.exe: src/dedicated.c src/net.c src/net.h src/log.c
	i586-mingw32msvc-gcc $(DEDICATED_CFLAGS) -o cncnet-dedicated.exe src/dedicated.c src/net.c src/log.c -lws2_32

cncnet.dll: src/dll.c src/net.c res/dll.def
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | i586-mingw32msvc-windres -O coff -o res/dll.o
	i586-mingw32msvc-gcc $(DLL_CFLAGS) -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/dll.c src/net.c res/dll.def res/dll.o -lws2_32

cncnet.dll.xz: cncnet.dll
	xz -C crc32 -9 -c cncnet.dll > cncnet.dll.xz

rules.ini.xz: rules.ini
	xz -C crc32 -9 -c rules.ini > rules.ini.xz

cncnet.exe: rules.ini.xz cncnet.dll.xz res/cncnet.rc.in src/extract.c src/client.c src/wait.c src/update.c src/download.c src/connect.c src/settings.c src/test.c src/http.c src/config.c src/net.c
	sed 's/__REV__/$(REV)/g' res/cncnet.rc.in | i586-mingw32msvc-windres -o res/cncnet.rc.o
	i586-mingw32msvc-gcc $(CLIENT_CFLAGS) -mwindows -o cncnet.exe src/extract.c src/client.c src/wait.c src/update.c src/download.c src/connect.c src/settings.c src/test.c src/http.c src/config.c src/net.c xz/xz_crc32.c xz/xz_dec_lzma2.c xz/xz_dec_stream.c res/cncnet.rc.o -Wl,--file-alignment,512 -Wl,--gc-sections -lws2_32 -lwininet -lcomctl32
	echo $(REV) > version.txt

clean:
	rm -f cncnet-dedicated cncnet-dedicated.exe cncnet.dll cncnet.dll.xz cncnet.exe res/*.o
