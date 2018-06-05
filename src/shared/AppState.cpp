
#include <fstream>
#include <iostream>
#include <iomanip>

#include <AppState.h>
#include <MemoryManager.h>


#pragma warning( push )
#pragma warning( disable: 4244 )
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#pragma warning( pop )

apemode::AppState* gState = nullptr;

apemode::AppState* apemode::AppState::Get( ) {
    return gState;
}

void apemode::AppState::OnMain( int args, const char** ppArgs ) {
    if ( nullptr == gState )
        gState = apemode_new AppState( args, ppArgs );
}

void apemode::AppState::OnExit( ) {
    if ( nullptr != gState ) {
        apemode_delete gState;
        gState = nullptr;
    }
}

std::string ComposeLogFile( ) {

    tm* pCurrentTime = nullptr;
    time_t currentSystemTime = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now( ) );

#if _WIN32
    tm currentTime;
    localtime_s( &currentTime, &currentSystemTime );
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

    return logFile;
}

std::shared_ptr< spdlog::logger > CreateLogger( spdlog::level::level_enum lvl, std::string logFile ) {

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

    spdlog::set_pattern( "[%T.%f] [%L] %v" );
    return logger;
}

apemode::AppState::AppState( int argc, const char** argv )
    : Logger( CreateLogger( spdlog::level::trace, ComposeLogFile( ) ) )
    , Cmdl( argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION ) {
}

apemode::AppState::~AppState( ) {
}
