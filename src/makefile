CC=gcc -std=c99
CFLAGS=-O2 -c
INCLUDES=-I ../libsrc/zlib \
         -I ../libsrc/libjpeg \
         -I ../libsrc/libpng
LD=g++
LDFLAGS=-std=c99 -o
LIBS=-L../lib -lpng -ljpeg -lz -lm

TARGETPATH=..
TARGET=imgconv

all: $(TARGETPATH) $(TARGETPATH)/$(TARGET)

$(TARGETPATH):
	@mkdir -p $@

$(TARGETPATH)/$(TARGET): main.c imgconv.h
	./build.sh
	$(CC) $(CFLAGS) main.c $(INCLUDES)
	$(LD) $(LDFLAGS) $@ *.o $(LIBS)

clean:
	rm -r *.o *~
