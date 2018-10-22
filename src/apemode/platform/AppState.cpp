#include "AppState.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include <fstream>
#include <iostream>
#include <iomanip>

#pragma warning( push )
#pragma warning( disable: 4244 )
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#pragma warning( pop )

namespace apemode {
    class ImplementedAppState : public AppState {
    public:
        std::shared_ptr< spdlog::logger > Logger;        /* Prints to console and file */
        argh::parser                      Cmdl;          /* User parameters */
        MT::TaskScheduler                 TaskScheduler; /* Task scheduler */

        ImplementedAppState( int args, const char** argv );
        virtual ~ImplementedAppState( );
    };
} // namespace apemode

static apemode::ImplementedAppState* gState = nullptr;

apemode::AppState* apemode::AppState::Get( ) {
    return gState;
}

spdlog::logger* apemode::AppState::GetLogger( ) {
    return gState->Logger.get( );
}

argh::parser* apemode::AppState::GetArgs( ) {
    return &gState->Cmdl;
}

MT::TaskScheduler* apemode::AppState::GetTaskScheduler( ) {
    return &gState->TaskScheduler;
}

void apemode::AppState::OnMain( int args, const char** ppArgs ) {
    if ( nullptr == gState )
        gState = apemode_new ImplementedAppState( args, ppArgs );
}

void apemode::AppState::OnExit( ) {
    if ( nullptr != gState ) {
        apemode_delete( gState );
    }
}

std::string ComposeLogFile( ) {
    apemode_memory_allocation_scope;

    tm* pCurrentTime = nullptr;
    time_t currentSystemTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );

#if _WIN32
    tm currentTime;
    localtime_s( &currentTime, &currentSystemTime );
    pCurrentTime = &currentTime;
#else
    pCurrentTime = localtime( &currentSystemTime );
#endif

    std::stringstream currentTimeStrStream;
    currentTimeStrStream << std::put_time( pCurrentTime, "%F-%T-" );
    currentTimeStrStream << currentSystemTime;

    std::string curentTimeStr = currentTimeStrStream.str( );
    std::replace( curentTimeStr.begin( ), curentTimeStr.end( ), ' ', '-' );
    std::replace( curentTimeStr.begin( ), curentTimeStr.end( ), ':', '-' );

    std::string logFile = "log-";
    logFile += curentTimeStr;
    logFile += ".txt";

    return logFile;
}

std::shared_ptr< spdlog::logger > CreateLogger( spdlog::level::level_enum lvl, std::string logFile ) {
    apemode_memory_allocation_scope;

    std::vector< spdlog::sink_ptr > sinks {
#if _WIN32
        std::make_shared< spdlog::sinks::wincolor_stdout_sink_mt >( ),
        std::make_shared< spdlog::sinks::msvc_sink_mt >( ),
        std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile.c_str( ) )
#else
        std::make_shared< spdlog::sinks::stdout_sink_mt >( ),
        std::make_shared< spdlog::sinks::simple_file_sink_mt >( logFile.c_str( ) )
#endif
    };

    auto logger = spdlog::create<>( "viewer", sinks.begin( ), sinks.end( ) );
    logger->set_level( lvl );

    spdlog::set_pattern( "%v" );

    logger->info( "" );
    logger->info( "\t _    ___" );
    logger->info( "\t| |  / (_)__ _      _____  _____" );
    logger->info( "\t| | / / / _ \\ | /| / / _ \\/ ___/" );
    logger->info( "\t| |/ / /  __/ |/ |/ /  __/ /" );
    logger->info( "\t|___/_/\\___/|__/|__/\\___/_/" );
    logger->info( "" );

    spdlog::set_pattern( "%c" );
    logger->info( "" );

    spdlog::set_pattern( "[%T.%f] [%t] [%L] %v" );
    return logger;
}

apemode::ImplementedAppState::ImplementedAppState( int argc, const char** argv )
    : Logger( nullptr )
    , Cmdl( )
    , TaskScheduler( 4 ) {
    
    Logger = CreateLogger( spdlog::level::trace, ComposeLogFile( ) );
    Logger->info("Input argumets:");
    for (int i = 0; i < argc; ++i) {
        Logger->info("#{} = {}", i, argv[i]);
    }
    
    Cmdl.parse( argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION );
}

apemode::ImplementedAppState::~ImplementedAppState( ) {
    apemode_memory_allocation_scope;

    LogInfo( "ImplementedAppState: Destroying." );

    spdlog::set_pattern( "%v" );
    Logger->info( "" );
    Logger->info( "\t    _____          " );
    Logger->info( "\t   / __ /__  _____ " );
    Logger->info( "\t  / __  / / / / _ \\" );
    Logger->info( "\t / /_/ / /_/ /  __/" );
    Logger->info( "\t/_____/\\__, /\\___/ " );
    Logger->info( "\t      /____/       " );

    spdlog::set_pattern( "%c" );
    Logger->info( "" );
}
