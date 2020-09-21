#ifndef _SDL_os2_h
#define _SDL_os2_h

#include "SDL_log.h"
#include "SDL_stdinc.h"
#include "../src/core/os2/geniconv/geniconv.h"

#if defined(__WATCOMC__)
#if OS2DEBUG==SDLOUTPUT

# define debug(category, s,...) SDL_LogDebug( category, \
                                    __func__"(): "##s, ##__VA_ARGS__ )

#elif defined(OS2DEBUG)

# define debug(category, s,...) printf( __func__"(): category: %i "##s"\n", category, ##__VA_ARGS__ )

#else

# define debug(s,...)

#endif // OS2DEBUG

// StrUTF8New() - geniconv\sys2utf8.c.
#define OS2_SysToUTF8(S) StrUTF8New( 1, S, SDL_strlen( S ) + 1 )
#define OS2_UTF8ToSys(S) StrUTF8New( 0, (char *)(S), SDL_strlen( S ) + 1 )

// SDL_OS2Quit() will be called from SDL_QuitSubSystem().
void SDL_OS2Quit();

#else
#if OS2DEBUG==SDLOUTPUT
# define debug(category, s,...) SDL_LogDebug( category, \
                                       "%s(): "s, __func__, ##__VA_ARGS__)
#elif defined(OS2DEBUG)
# define debug(category, s,...) printf("%s(): category: %i message: "s"\n", \
                                       __func__, category, ##__VA_ARGS__)
#else
# define debug(s,...)
#endif // OS2DEBUG
#endif

#define OS2_SysToUTF8(S) SDL_iconv_string("UTF-8", "", (char *)(S), SDL_strlen(S)+1)
#define OS2_UTF8ToSys(S) SDL_iconv_string("", "UTF-8", (char *)(S), SDL_strlen(S)+1)

#endif // _SDL_os2_h
