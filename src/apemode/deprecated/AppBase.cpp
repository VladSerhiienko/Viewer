
#include <AppBase.h>

#include <AppState.h>
#include <apemode/platform/memory/MemoryManager.h>

#include <SDL.h>

void apemode::AppBase::HandleWindowSizeChanged( ) {
}

apemode::AppBase::AppBase( ) {
    apemode::LogInfo( "AppBase: Created." );
}

apemode::AppBase::~AppBase( ) {
    apemode_memory_allocation_scope;

    apemode::LogInfo( "AppBase: Destroying" );

    if ( pSurface )
        pSurface->Finalize( );
}

const char* apemode::AppBase::GetAppName( ) {
    return "App";
}

bool apemode::AppBase::Initialize( ) {
    apemode_memory_allocation_scope;
    apemode::LogInfo( "AppBase: Initializing." );

    pSurface = std::move( CreateAppSurface( ) );
    SDL_assert( pSurface && "Surface is required." );

    if ( pSurface && pSurface->Initialize( 1280, 800, GetAppName( ) ) && InputManager.Initialize( ) ) {
        Stopwatch.Start( );
        Stopwatch.Update( );
        return true;
    }

    apemode::LogError( "AppBase: Initialization failed." );
    return false;
}

apemode::unique_ptr< apemode::AppSurfaceBase > apemode::AppBase::CreateAppSurface( ) {
    apemode_memory_allocation_scope;
    return apemode::unique_ptr< apemode::AppSurfaceBase >( apemode_new AppSurfaceBase( ) );
}

apemode::AppSurfaceBase* apemode::AppBase::GetSurface( ) {
    return pSurface.get( );
}

void apemode::AppBase::OnFrameMove( ) {
    apemode_memory_allocation_scope;

    SDL_assert( pSurface && "Surface is required." );

    Stopwatch.Update( );

    const float elapsedSecs = static_cast< float >( Stopwatch.GetElapsedSeconds( ) );

    InputManager.Update( InputState, elapsedSecs );

    pSurface->OnFrameMove( );
    Update( elapsedSecs, InputState );
    pSurface->OnFrameDone( );
}

void apemode::AppBase::Update( float /*deltaSecs*/, Input const& /*inputState*/ ) {
}

bool apemode::AppBase::IsRunning( ) {
    return !InputState.bIsQuitRequested &&
           !InputState.IsFirstPressed( kDigitalInput_BackButton ) &&
           !InputState.IsFirstPressed( kDigitalInput_KeyEscape );
}
