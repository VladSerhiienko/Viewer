#pragma once

#pragma warning( push )
#pragma warning( disable: 4244 )

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#pragma warning( pop )

namespace apemode {

    /**
     * @class AppStoredValue
     * @brief Contains a value created and stored by the user
     */
    class AppStoredValue {
    public:
        virtual ~AppStoredValue( ) {
        }
    };

    /**
     * @class AppState
     * @brief Contains members allowed for global access across the App classes
     */
    class AppState {
    public:
        int          Argc   = 0;
        const char** ppArgv = nullptr;

        std::shared_ptr< spdlog::logger >                          Logger;       /* Prints to console and file */
        std::unique_ptr< cxxopts::Options >                        Options;      /* User parameters */
        std::map< std::string, std::unique_ptr< AppStoredValue > > StoredValues; /* User values */

        /**
         * @return Application state instance.
         * @note Returns null before the application creation, or after its destruction.
         */
        static AppState* Get( );
        static void      OnMain( int argc, const char** argv );
        static void      OnExit( );

    private:
        AppState( int args, const char** argv );
        ~AppState( );
    };

    /**
     * @brief Passes Options instance to the functor, if it is available.
     *
     * IfHasOptions( [&] ( Options & options ) {
     *     auto opt1 = options["option-1"].as< string >( );
     *     auto opt2 = options["option-2"].as< string >( );
     *     auto opt3 = options["option-3"].as< string >( );
     * }
     **/
    template < typename TFunc >
    inline void IfHasOptions( TFunc F ) {
        if ( AppState::Get( ) && AppState::Get( )->Options )
            F( *AppState::Get( )->Options );
    }

    /**
     * @return Command line option of specified type.
     * @note Silences exceptions, returns default value.
     * auto sceneFiles = TGetOption< std::vector< std::string > >("scene", {});
     **/
    template < typename TOption >
    inline const TOption & TGetOption( const char* optionName, const TOption & defaultValue ) {

        if ( AppState::Get( ) && AppState::Get( )->Options ) {
            try {
                auto& optionDetails = AppState::Get( )->Options->operator[]( optionName );
                return 0 == optionDetails.count( ) ? defaultValue : optionDetails.as< TOption >( );
            } catch ( const std::exception& e ) {
                AppState::Get( )->Logger->error( "Failed to get option \"{}\" [{}]", optionName, e.what( ) );
            }
        }

        return defaultValue;
    }

    template < typename... Args >
    inline void LogInfo( const char* szFmt, const Args&... args ) {
        if ( auto l = AppState::Get( )->Logger.get( ) ) {
            l->info( szFmt, args... );
        }
    }

    template < typename... Args >
    inline void LogError( const char* szFmt, const Args&... args ) {
        if ( auto l = AppState::Get( )->Logger.get( ) ) {
            l->error( szFmt, args... );
        }
    }

    template < typename... Args >
    inline void LogWarn( const char* szFmt, const Args&... args ) {
        if ( auto l = AppState::Get( )->Logger.get( ) ) {
            l->warn( szFmt, args... );
        }
    }

} // namespace apemode
