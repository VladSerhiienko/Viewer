
#include <fstream>
#include <iostream>
#include <iomanip>

#include <AppState.h>

#pragma warning( push )
#pragma warning( disable: 4244 )
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#pragma warning( pop )

apemode::AppState* gState = nullptr;

apemode::AppState* apemode::AppState::Get( ) {
    return gState;
}

void apemode::AppState::OnMain( int args, char* ppArgs[] ) {
    if ( nullptr == gState )
        gState = new AppState( args, ppArgs );
}

void apemode::AppState::OnExit( ) {
    if ( nullptr != gState ) {
        delete gState;
        gState = nullptr;
    }
}

apemode::AppState::AppState( int argc, char* argv[] )
    : Argc( argc )
    , Argv( argv )
    , Logger( spdlog::create< spdlog::sinks::stdout_sink_mt >( "apemode/stdout" ) )
    , Options( new cxxopts::Options( argc ? argv[ 0 ] : "" ) ) {

    tm* pCurrentTime = nullptr;
    time_t currentSystemTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );

#if _WIN32
    tm currentTime;
    localtime_s(&currentTime, &currentSystemTime);
    pCurrentTime = &currentTime;
#else
    pCurrentTime = localtime( &currentSystemTime );
#endif

    std::stringstream curentTimeStrStream;
    curentTimeStrStream << std::put_time( pCurrentTime, "%F-%T-" );
    curentTimeStrStream << currentSystemTime;

    std::string curentTimeStr = curentTimeStrStream.str( );
    std::replace( curentTimeStr.begin( ), curentTimeStr.end( ), ' ', '-' );
    std::replace( curentTimeStr.begin( ), curentTimeStr.end( ), ':', '-' );

    std::string logFile = "log-";
    logFile += curentTimeStr;
    logFile += ".txt";

    std::vector< spdlog::sink_ptr > sinks{

#if _WIN32
        std::make_shared< spdlog::sinks::wincolor_stdout_sink_mt >( ),
        std::make_shared< spdlog::sinks::msvc_sink_mt >( ),
        std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile )
#else
        std::make_shared< spdlog::sinks::stdout_sink_mt >( ),
        std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile )
#endif

    };

#ifdef _DEBUG
    Logger->set_level( spdlog::level::debug );
#endif
}

apemode::AppState::~AppState( ) {
}
