#pragma once

#ifdef _WIN32

#ifndef STRICT
#define STRICT
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef SDL_VIDEO_DRIVER_WINDOWS
#define SDL_VIDEO_DRIVER_WINDOWS 1
#endif

#include <Windows.h>

#elif __APPLE__

#ifndef SDL_VIDEO_DRIVER_COCOA
#define SDL_VIDEO_DRIVER_COCOA 1
#endif

#elif __linux__

#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#ifndef SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_X11 1
#endif

#else
#error "Unsupported platform"
#endif

#include <SDL.h>
#include <SDL_syswm.h>

namespace apemode {
namespace platform {
namespace sdl2 {

/* Wraps SDL2 window.
 */
class AppSurface {
public:
    AppSurface( );
    virtual ~AppSurface( );

    bool Initialize( uint32_t width, uint32_t height, const char* name );
    void Finalize( );

    uint32_t GetWidth( ) const;
    uint32_t GetHeight( ) const;
    void*    GetWindowHandle( );

public:
    SDL_Window* pSdlWindow   = nullptr;

#ifdef SDL_VIDEO_DRIVER_X11
    Display* pDisplayX11 = nullptr;
    Window   pWindowX11;
#elif SDL_VIDEO_DRIVER_WINDOWS
    HWND      hWnd      = NULL;
    HINSTANCE hInstance = NULL;
#elif SDL_VIDEO_DRIVER_COCOA
    void* pView = nullptr;
#endif
};

} // namespace sdl2
} // namespace platform
} // namespace apemode
