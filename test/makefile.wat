BINPATH = .

TARGETS = testsprite2.exe testatomic.exe testdisplayinfo.exe testdraw2.exe &
          testdrawchessboard.exe testdropfile.exe testerror.exe testfile.exe &
          testfilesystem.exe testgamecontroller.exe testgesture.exe &
          testhittesting.exe testhotplug.exe testiconv.exe testime.exe &
          testintersections.exe testjoystick.exe testkeys.exe testloadso.exe &
          testlock.exe testmessage.exe testoverlay2.exe testplatform.exe &
          testpower.exe testrelative.exe testrendercopyex.exe &
          testrendertarget.exe testrumble.exe testscale.exe testsem.exe &
          testshader.exe testshape.exe testsprite2.exe testspriteminimal.exe &
          teststreaming.exe testthread.exe testtimer.exe testver.exe &
          testviewport.exe testwm2.exe torturethread.exe checkkeys.exe &
          testnative.exe loopwave.exe loopwavequeue.exe testautomation.exe

# Common sources from ..\src\test

CSRCS = SDL_test_assert.c SDL_test_common.c SDL_test_compare.c &
        SDL_test_crc32.c SDL_test_font.c SDL_test_fuzzer.c SDL_test_harness.c &
        SDL_test_imageBlit.c SDL_test_imageBlitBlend.c SDL_test_imageFace.c &
        SDL_test_imagePrimitives.c SDL_test_imagePrimitivesBlend.c &
        SDL_test_log.c SDL_test_md5.c SDL_test_random.c

# Test automation sources.

TASRCS = testautomation.c testautomation_audio.c testautomation_clipboard.c &
         testautomation_events.c testautomation_hints.c &
         testautomation_keyboard.c testautomation_main.c &
         testautomation_mouse.c testautomation_pixels.c &
         testautomation_platform.c testautomation_rect.c &
         testautomation_render.c testautomation_rwops.c &
         testautomation_sdltest.c testautomation_stdlib.c &
         testautomation_surface.c testautomation_syswm.c &
         testautomation_timer.c testautomation_video.c

OBJS = $(TARGETS:.exe=.obj)
COBJS = $(CSRCS:.c=.obj)
TAOBJS = $(TASRCS:.c=.obj)

all: $(COBJS) $(TARGETS) show_info

INCPATH = $(%WATCOM)\H\os2;$(%WATCOM)\H;..\include
INCPATH +=..\src\core\os2
INCPATH +=;..\..\..\h;..\..\..\h\SDL2

CFLAGS_DEF = -i=$(INCPATH) -bt=os2 -d0 -q -bm -5s -fp5 -fpi87 -sg -oteanbmier
CFLAGS_EXE = $(CFLAGS_DEF)
CFLAGS = $(CFLAGS_EXE) -ei
CFLAGS+= -dBUILD_SDL -dHAVE_SDL_TTF

LIBPATH = ..\..\..\lib
LIBS = sdl2.lib

.c: ..\src\test
.obj: .\obj

testautomation.exe: $(TAOBJS)
  @rem echo * Link: $@
  @wlink SYS os2v2 libpath $(LIBPATH) lib {$(LIBS)} op q op el file {$< $COBJS} name $*

testnative.exe: testnative.obj testnativeos2.obj
  @rem echo * Link: $@
  @wlink SYS os2v2 libpath $(LIBPATH) lib {$(LIBS)} op q op el file {$< $COBJS} name $*

testime.exe: testime.obj
  @rem echo * Link: $@
  @wlink SYS os2v2 libpath $(LIBPATH) lib {$(LIBS) sdl2ttf.lib} op q op el file {$< $COBJS} name $*

.obj.exe:
  @rem echo * Link: $@
  @wlink SYS os2v2 libpath $(LIBPATH) lib {$(LIBS)} op q op el file {$< $COBJS} name $*

.c.obj:
  @rem echo * Compilation: $<
  @wcc386 $(CFLAGS) -wcd=107 $<

show_info: .SYMBOLIC
  @echo * Tests compiled in $(BINPATH)

clean: .SYMBOLIC
  @echo * Clean tests in $(BINPATH)
  @for %i in ($(OBJS)) do @if exist %i @del %i
  @for %i in ($(COBJS)) do @if exist %i @del %i
  @for %i in ($(TAOBJS)) do @if exist %i @del %i
  @for %i in ($(TARGETS:.exe=.err)) do @if exist %i @del %i
  @for %i in ($(TARGETS)) do @if exist %i @del %i
