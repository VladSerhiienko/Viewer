

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
    apemode_memory_allocation_scope;

    AppStateScope appStateScope( argc, (const char**) argv );
    apemode::unique_ptr< apemode::AppBase > app( CreateApp( ) );

    if ( app && app->Initialize( ) ) {
        apemode_memory_allocation_scope;

        while ( app->IsRunning( ) )
            app->OnFrameMove( );
    }

    return 0;
}