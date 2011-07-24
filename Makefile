CC=i586-mingw32msvc-gcc
CFLAGS=-O2 -s -Wall
WINDRES=i586-mingw32msvc-windres
LIBS=-lws2_32
REV=$(shell sh -c 'git rev-parse --short @{0}')

all: dll register

dll: src/cncnet.c src/net.c
	sed 's/__REV__/$(REV)/g' res/dll.rc.in | sed 's/__FILE__/cncnet/g' | sed 's/__GAME__/CnCNet Internet DLL/g' | $(WINDRES) -O coff -o res/dll.o
	$(CC) $(CFLAGS) -DBUILD_DLL -DINTERNET -Wl,--enable-stdcall-fixup -shared -s -o cncnet.dll src/cncnet.c src/net.c src/cncnet.def res/dll.o $(LIBS)

register: src/register.c
	sed 's/__REV__/$(REV)/g' res/register.rc.in | sed 's/__FILE__/cncnet-register/g' | sed 's/__GAME__/CnCNet Internet Installer/g' | $(WINDRES) -O coff -o res/register.o
	$(CC) $(CFLAGS) -mwindows -Wl,--enable-stdcall-fixup -s -o cncnet-register.exe src/register.c res/register.o

clean:
	rm -f cncnet.dll cncnet-register.exe res/dll.o res/register.o
