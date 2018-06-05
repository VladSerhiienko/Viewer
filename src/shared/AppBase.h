#pragma once

#include <AppBase.h>
#include <AppSurfaceBase.h>

#include <Input.h>
#include <Stopwatch.h>

namespace apemode {

    class AppBase {
        apemode::AppSurfaceBase* pSurface = nullptr;
        apemode::Stopwatch       Stopwatch;
        apemode::Input           InputState;
        apemode::InputManager    InputManager;

        void                    HandleWindowSizeChanged( );
        virtual AppSurfaceBase* CreateAppSurface( );

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
