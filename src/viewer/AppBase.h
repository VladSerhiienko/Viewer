#pragma once

namespace apemode {
    class Input;
    class AppSurfaceBase;

    class AppBaseContent;
    class AppBase {
        friend class AppBaseContent;
        AppBaseContent* pAppContent;

        void HandleWindowSizeChanged( );

    public:
        AppBase( );
        virtual ~AppBase( );

        virtual bool Initialize( int Args, char* ppArgs[] );
        virtual AppSurfaceBase* CreateAppSurface( );
        AppSurfaceBase*         GetSurface( );

        virtual void OnFrameMove( );
        virtual void Update( float DeltaSecs, Input const& InputState );
        virtual bool IsRunning( );
    };
}
