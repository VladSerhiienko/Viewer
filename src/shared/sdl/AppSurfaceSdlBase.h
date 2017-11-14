#pragma once

#include <AppSurfaceBase.h>

#ifdef __unix__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>
#ifndef SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_X11 1
#endif
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
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

#ifdef X_PROTOCOL
        Display* pDisplayX11 = nullptr;
        Window   pWindowX11;
#endif

#ifdef _WINDOWS_
        HWND      hWnd      = NULL;
        HINSTANCE hInstance = NULL;
#endif
    };
}