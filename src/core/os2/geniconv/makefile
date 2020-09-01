#
# Universal iconv implementation for OS/2.
#
# OpenWatcom makefile to build a library that uses kiconv.dll / iconv2.dll /
# iconv.dll or OS/2 Uni*() API.
#
# Andrey Vasilkin, 2016.
#

LIBFILE = geniconv.lib

all: $(LIBFILE) test.exe .symbolic

CFLAGS = -i=.\;$(%WATCOM)\H\OS2;$(%WATCOM)\H -bt=os2 -q -d0 -w2

SRCS = geniconv.c os2cp.c os2iconv.c
SRCS+= sys2utf8.c

OBJS = $(SRCS:.c=.obj)

LIBS = libuls.lib libconv.lib $(LIBFILE)

test.exe: $(LIBFILE) test.obj
  @wlink op quiet system os2v2 file $* lib {$(LIBS)} name $*

$(LIBFILE): $(OBJS)
  @if exist $@ @del $@
  @for %f in ($(OBJS)) do @wlib -b $* +%f >nul

.c.obj: .AUTODEPEND
    @wcc386 $[* $(CFLAGS)

clean: .SYMBOLIC
  @if exist *.obj del *.obj
  @if exist *.err del *.err
  @if exist $(LIBFILE) del $(LIBFILE)
  @if exist test.exe del test.exe
