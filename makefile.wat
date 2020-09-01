#
#  This is an OpenWatcom makefile to build SDL2 for OS/2
#
#  Andrey Vasilkin (Digi), 2016.
#

DLLPATH = ..\..\dll
LIBPATH = ..\..\lib
LIBNAME = sdl2
VERSION = 2.0.4
DESCRIPTION = Simple DirectMedia Layer 2

DLLFILE = $(DLLPATH)\$(LIBNAME).dll
LIBFILE = $(LIBPATH)\$(LIBNAME).lib
LNKFILE = $(LIBNAME).lnk

INCPATH = $(%WATCOM)\H\os2;$(%WATCOM)\H;
INCPATH +=.\include;.\src;.\src\audio;.\src\audio\disk;.\src\events;
INCPATH +=.\src\joystick;.\src\thread;.\src\render;
INCPATH +=.\src\timer;.\src\video;\src\haptic;.\src\joystick;
INCPATH +=.\src\render\software;
INCPATH +=.\src\timer\os2;.\src\video\dummy;\src\haptic\dummy;
INCPATH +=.\src\joystick\dummy;.\src\loadso\os2;.\src\filesystem\os2;
INCPATH +=.\src\thread\os2;.\src\core\os2;.\src\core\os2\geniconv;

SRCS = SDL.c SDL_assert.c SDL_error.c SDL_hints.c SDL_log.c SDL_audio.c &
       SDL_audiocvt.c SDL_audiodev.c SDL_audiotypecvt.c SDL_mixer.c SDL_wave.c &
       SDL_cpuinfo.c SDL_clipboardevents.c SDL_dropevents.c SDL_events.c &
       SDL_gesture.c SDL_keyboard.c SDL_mouse.c SDL_quit.c SDL_touch.c &
       SDL_windowevents.c SDL_rwops.c SDL_haptic.c SDL_gamecontroller.c &
       SDL_power.c SDL_joystick.c SDL_d3dmath.c SDL_render.c SDL_yuv_mmx.c &
       SDL_yuv_sw.c SDL_blendfillrect.c SDL_blendline.c SDL_blendpoint.c &
       SDL_drawline.c SDL_drawpoint.c SDL_render_sw.c SDL_rotate.c SDL_getenv.c &
       SDL_iconv.c SDL_malloc.c SDL_qsort.c SDL_stdlib.c SDL_string.c &
       SDL_thread.c SDL_timer.c SDL_blit.c SDL_blit_0.c SDL_blit_1.c &
       SDL_blit_A.c SDL_blit_auto.c SDL_blit_copy.c SDL_blit_N.c &
       SDL_blit_slow.c SDL_bmp.c SDL_clipboard.c SDL_egl.c SDL_fillrect.c &
       SDL_pixels.c SDL_rect.c SDL_RLEaccel.c SDL_shape.c SDL_stretch.c &
       SDL_surface.c SDL_video.c SDL_atomic.c SDL_spinlock.c
SRCS+= SDL_os2audio.c SDL_syshaptic.c SDL_sysjoystick.c SDL_sysloadso.c &
       SDL_sysfilesystem.c SDL_syscond.c SDL_sysmutex.c SDL_syssem.c &
       SDL_systhread.c SDL_systls.c SDL_systimer.c SDL_syspower.c &
       SDL_os2util.c SDL_os2video.c SDL_os2dive.c SDL_os2vman.c &
       SDL_os2mouse.c SDL_os2messagebox.c
SRCS+= SDL_os2.c geniconv.c os2cp.c os2iconv.c sys2utf8.c


OBJS = $(SRCS:.c=.obj)

LIBS = mmpm2.lib libuls.lib libconv.lib

CFLAGS = -bt=os2 -i=$(INCPATH) -d0 -q -bm -5s -fp5 -fpi87 -sg -oteanbmier
CFLAGS+= -bd -wx -ei -fo=.\obj\
CFLAGS+= -dBUILD_SDL
#
# Debug options:
#   -dOS2DEBUG           - debug messages from OS/2 related code to stdout,
#   -dOS2DEBUG=SDLOUTPUT - debug messages from OS/2 code via SDL_LogDebug().
#

.extensions:
.extensions: .lib .dll .obj .c .asm

.c: .\src;.\src\core\os2;.\src\core\os2\geniconv;.\src\audio;.\src\cpuinfo;.\src\events;.\src\file;.\src\haptic;.\src\joystick;.\src\power;.\src\render;.\src\render\software;.\src\stdlib;.\src\thread;.\src\timer;.\src\video;.\src\audio\os2;.\src\haptic\dummy;.\src\joystick\dummy;.\src\loadso\os2;.\src\filesystem\os2;.\src\thread\os2;.\src\thread\generic;.\src\timer\os2;.\src\video\os2;.\src\power;.\src\power\os2;.\src\atomic;.\src\test;
.obj: .\obj

all: $(DLLFILE) $(LIBFILE) .symbolic

$(DLLFILE): $(LNKFILE) mkdir_obj info_comiling $(OBJS)
    @echo * Linking: $@
    @wlink @$(LNKFILE)

$(LIBFILE): $(DLLFILE)
    @echo * Creating LIB file: $@
    @wlib -b -n -q $* $(DLLFILE)

.c.obj: .AUTODEPEND
    @wcc386 $[* $(CFLAGS)

SDL_cpuinfo.obj: SDL_cpuinfo.c
  @wcc386 $(CFLAGS) -wcd=200 $<

SDL_rwops.obj: SDL_rwops.c
  @wcc386 $(CFLAGS) -wcd=136 $<

SDL_blendfillrect.obj: SDL_blendfillrect.c
  @wcc386 $(CFLAGS) -wcd=200 $<

SDL_blendline.obj: SDL_blendline.c
  @wcc386 $(CFLAGS) -wcd=200 $<

SDL_blendpoint.obj: SDL_blendpoint.c
  @wcc386 $(CFLAGS) -wcd=200 $<

SDL_RLEaccel.obj: SDL_RLEaccel.c
  @wcc386 $(CFLAGS) -wcd=201 $<

$(LNKFILE):
#
#   About LIBPATH: OpenWatcom's library os2386.lib does not contain entries
#   Gre32Entry1..Gre32Entry10. We need os2386.lib from IBM OS/2 developer's
#   toolkit. %LIB% should contain path item like xxxx\OS2TK45\lib (you already
#   have it if toolkit was installed).
#
    @echo * Creating linker file: $@
    @%create $@
    @%append $@ SYSTEM os2v2_dll INITINSTANCE TERMINSTANCE
    @%append $@ NAME $(DLLPATH)\$(LIBNAME)
    @for %i in ($(OBJS)) do @%append $@ FILE obj\%i
    @%append $@ LIBPATH $(%LIB);$(LIBPATH)
    @for %i in ($(LIBS)) do @%append $@ LIB %i
    @%append $@ OPTION QUIET
    @%append $@ OPTION MAP=$^&.map
!ifdef %osdir
    @$(%osdir)\KLIBC\BIN\date +"OPTION DESCRIPTION '@$#libsdl org:$(VERSION)$#@$#$#1$#$# %F               $(%HOSTNAME)::::::@@$(DESCRIPTION)'" >>$^@
!else
    @%append $@ OPTION DESCRIPTION '@$#libsdl org:$(VERSION)$#@$(DESCRIPTION)'
!endif
    @%append $@ OPTION QUIET
    @%append $@ OPTION ELIMINATE
    @%append $@ OPTION MANYAUTODATA
    @%append $@ OPTION OSNAME='OS/2 and eComStation'
    @%append $@ OPTION SHOWDEAD

clean: .SYMBOLIC
    @ echo * Clean: $(LIBNAME)
    @if exist .\obj\*.obj @del .\obj\*.obj
    @if exist .\obj @rd .\obj
    @if exist *.map @del *.map
    @if exist *.err @del *.err
    @if exist $(LNKFILE) @del $(LNKFILE)
    @if exist $(DLLFILE) @del $(DLLFILE)
    @if exist $(LIBFILE) @del $(LIBFILE)

mkdir_obj: .symbolic
    @if not exist obj @mkdir obj

info_comiling: .symbolic
    @echo * Compiling...
