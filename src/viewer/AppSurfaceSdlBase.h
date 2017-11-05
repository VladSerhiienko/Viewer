#pragma once

#include <AppSurfaceBase.h>

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

        uint32_t    LastWidth;
        uint32_t    LastHeight;
        SDL_Window* pSdlWindow;

#ifdef X_PROTOCOL
        Display* pDisplayX11;
        Window   pWindowX11;
#endif

#ifdef _WINDOWS_
        HWND      hWnd;
        HINSTANCE hInstance;
#endif
    };
}