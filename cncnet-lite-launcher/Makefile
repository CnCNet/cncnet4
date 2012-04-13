REV=$(shell sh -c 'git rev-parse --short @{0}')
CC=i586-mingw32msvc-gcc
CFLAGS=-pedantic -Wall -Os -s -Wall -I. -DCNCNET_VERSION=\"$(REV)\"
WINDRES=i586-mingw32msvc-windres
DLLTOOL=i586-mingw32msvc-dlltool
LIBS=-Wl,--file-alignment,512 -Wl,--gc-sections -lws2_32 -lwininet -lcomctl32

all: cncnet

cncnet.dll.xz: ../cncnet-client/cncnet.dll
	xz -C crc32 -9 -c ../cncnet-client/cncnet.dll > cncnet.dll.xz

cncnet: cncnet.dll.xz
	sed 's/__REV__/$(REV)/g' res/cncnet.rc.in | $(WINDRES) -o cncnet.rc.o
	$(CC) $(CFLAGS) -mwindows -o cncnet.exe src/main.c src/http.c src/config.c src/register.c src/base32.c xz/xz_crc32.c xz/xz_dec_lzma2.c xz/xz_dec_stream.c cncnet.rc.o $(LIBS)
	echo $(REV) > version.txt

clean:
	rm -f cncnet.exe cncnet.rc.o
