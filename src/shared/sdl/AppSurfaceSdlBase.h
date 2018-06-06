#pragma once

#include "AppSurfaceBase.h"

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
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#ifndef SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_X11 1
#endif

#else
#error "Unsupported platform"
#endif

#include <SDL.h>
#include <SDL_syswm.h>

namespace apemode {
    class AppSurfaceSettings;

    /**
     * Contains handle to window and graphics context.
     **/
    class AppSurfaceSdlBase : public AppSurfaceBase {
    public:
        AppSurfaceSdlBase( );
        virtual ~AppSurfaceSdlBase( );

        virtual bool Initialize( uint32_t width, uint32_t height, const char* name ) override;
        virtual void Finalize( ) override;

        virtual uint32_t GetWidth( ) const override;
        virtual uint32_t GetHeight( ) const override;
        virtual void*    GetWindowHandle( ) override;

        uint32_t    LastWidth  = 0;
        uint32_t    LastHeight = 0;
        SDL_Window* pSdlWindow = nullptr;

#ifdef SDL_VIDEO_DRIVER_X11
        Display* pDisplayX11 = nullptr;
        Window   pWindowX11;
#elif SDL_VIDEO_DRIVER_WINDOWS
        HWND      hWnd      = NULL;
        HINSTANCE hInstance = NULL;
#elif SDL_VIDEO_DRIVER_COCOA
        void* pNSWindow = nullptr;
#endif
    };
}
