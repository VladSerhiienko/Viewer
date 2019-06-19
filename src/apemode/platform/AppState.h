#pragma once

#include <apemode/platform/memory/MemoryManager.h>

#ifdef _WIN32
#ifndef MT_INSTRUMENTED_BUILD
#define MT_INSTRUMENTED_BUILD 1
#endif
#endif

//#include <MTScheduler.h>
#include <taskflow/taskflow.hpp>

#pragma warning( push )
#pragma warning( disable: 4244 )

#include <argh.h>
#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>

#pragma warning( pop )

#include <utility>

namespace apemode {

    /**
     * @class AppStoredValue
     * @brief Contains a value created and stored by the user
     */
    class AppStoredValue {
    public:
        virtual ~AppStoredValue( ) = default;
    };

    /**
     * @class AppState
     * @brief Contains members allowed for global access across the App classes
     */
    class AppState {
    public:
        virtual ~AppState( ) = default;

        /**
         * @return Application state instance.
         * @note Returns null before the application creation, or after its destruction.
         */
        static AppState* Get( );
        static void      OnMain( int argc, const char** argv );
        static void      OnExit( );

        spdlog::logger*    GetLogger( );
        argh::parser*      GetArgs( );
        tf::Taskflow*      GetDefaultTaskflow( );
    };
    
    struct AppStateExitGuard {
        ~AppStateExitGuard() {
            AppState::OnExit();
        }
    };
    

    /**
     * @return Command line option of specified type.
     * @note Silences exceptions, returns default value.
     * auto sceneFiles = TGetOption< std::vector< std::string > >("scene", {});
     **/
    template < typename TOption >
    inline TOption TGetOption( const char* const optionName, const TOption& defaultValue ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pArgs = pAppState->GetArgs( ) ) {
                if ( pArgs->operator[]( {optionName} ) ) {
                    TOption option;
                    if ( pArgs->operator( )( {optionName} ) >> option )
                        return option;
                }
            }

        return defaultValue;
    }

    template <>
    inline bool TGetOption( const char* const optionName, const bool& defaultValue ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pArgs = pAppState->GetArgs( ) ) {
                return pArgs->operator[]( {optionName} );
            }

        return defaultValue;
    }

    template <>
    inline std::string TGetOption( const char* const optionName, const std::string& defaultValue ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pArgs = pAppState->GetArgs( ) ) {
                // if ( pArgs->operator[]( {optionName} ) ) {
                    std::string option = pArgs->operator( )( {optionName} ).str( );
                    return option;
                // }
            }

        return defaultValue;
    }

    enum LogLevel {
        Trace    = 0,
        Debug    = 1,
        Info     = 2,
        Warn     = 3,
        Err      = 4,
        Critical = 5,
    };

    template < typename... Args >
    inline void Log( LogLevel eLevel, const char* szFmt, Args &&... args ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pLogger = pAppState->GetLogger( ) ) {
                pLogger->log( static_cast< spdlog::level::level_enum >( eLevel ), szFmt, std::forward< Args >( args )... );
            }
    }

    template < typename... Args >
    inline void LogInfo( const char* szFmt, Args &&... args ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pLogger = pAppState->GetLogger( ) ) {
                pLogger->info( szFmt, std::forward< Args >( args )... );
            }
    }

    template < typename... Args >
    inline void LogError( const char* szFmt, Args &&... args ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pLogger = pAppState->GetLogger( ) ) {
                pLogger->error( szFmt, std::forward< Args >( args )... );
            }
    }

    template < typename... Args >
    inline void LogWarn( const char* szFmt, Args &&... args ) {
        apemode_memory_allocation_scope;

        if ( auto pAppState = AppState::Get( ) )
            if ( auto pLogger = pAppState->GetLogger( ) ) {
                pLogger->warn( szFmt, std::forward< Args >( args )... );
            }
    }

} // namespace apemode
