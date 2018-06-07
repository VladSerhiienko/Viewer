
#include <AppBase.h>

#include <AppState.h>
#include <MemoryManager.h>

#include <SDL.h>

void apemode::AppBase::HandleWindowSizeChanged( ) {
}

apemode::AppBase::AppBase( ) : pSurface( nullptr ) {
    apemode::LogInfo( "AppBase/Create" );
}

apemode::AppBase::~AppBase( ) {
    apemode::LogInfo( "AppBase/Destroy" );

    if ( pSurface )
        pSurface->Finalize( );
}

const char* apemode::AppBase::GetAppName( ) {
    return "App";
}

bool apemode::AppBase::Initialize( ) {
    apemode::LogInfo( "AppBase/Initialize" );

    pSurface = CreateAppSurface( );
    SDL_assert( pSurface && "Surface is required." );

    if ( pSurface && pSurface->Initialize( 1280, 800, GetAppName( ) ) && InputManager.Initialize( ) ) {
        Stopwatch.Start( );
        Stopwatch.Update( );
        return true;
    }

    apemode::LogError( "AppBase/Initialize: Failed" );
    return false;
}

apemode::AppSurfaceBase* apemode::AppBase::CreateAppSurface( ) {
    return apemode_new AppSurfaceBase( );
}

apemode::AppSurfaceBase* apemode::AppBase::GetSurface( ) {
    return pSurface;
}

void apemode::AppBase::OnFrameMove( ) {
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

bool apemode::AppBase::IsRunning ()
{
    return !InputState.bIsQuitRequested
        && !InputState.IsFirstPressed (kDigitalInput_BackButton)
        && !InputState.IsFirstPressed (kDigitalInput_KeyEscape);
}
