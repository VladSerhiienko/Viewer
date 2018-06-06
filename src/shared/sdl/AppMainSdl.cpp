#include <fbxvpch.h>

#include <AppBase.h>
#include <AppState.h>
#include <SDL.h>
#include <SDL_main.h>

extern "C" apemode::AppBase* CreateApp( );

struct AppStateScope {
    AppStateScope( int args, const char** ppArgs ) {
        apemode::AppState::OnMain( args, ppArgs );
    }
    ~AppStateScope( ) {
        apemode::AppState::OnExit( );
    }
};

#ifdef main
#undef main
#endif

int main( int argc, char** argv ) {
    AppStateScope appStateScope( argc, (const char**) argv );

    apemode::unique_ptr< apemode::AppBase > pAppImpl( CreateApp( ) );

    if ( pAppImpl && pAppImpl->Initialize( ) ) {
        while ( pAppImpl->IsRunning( ) )
            pAppImpl->OnFrameMove( );
    }

    return 0;
}