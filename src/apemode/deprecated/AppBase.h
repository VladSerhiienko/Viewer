#pragma once

#include <apemode/platform/memory/MemoryManager.h>
#include <AppSurfaceBase.h>

#include <Input.h>
#include <Stopwatch.h>

namespace apemode {

    class AppBase {
        apemode::unique_ptr< AppSurfaceBase > pSurface;
        apemode::Stopwatch                    Stopwatch;
        apemode::Input                        InputState;
        apemode::InputManager                 InputManager;

        void                                          HandleWindowSizeChanged( );
        virtual apemode::unique_ptr< AppSurfaceBase > CreateAppSurface( );

    public:
        AppBase( );
        virtual ~AppBase( );

        virtual const char*     GetAppName( );
        virtual bool            Initialize( );
        AppSurfaceBase*         GetSurface( );

        virtual void OnFrameMove( );
        virtual void Update( float deltaSecs, Input const& inputState );
        virtual bool IsRunning( );
    };
}
