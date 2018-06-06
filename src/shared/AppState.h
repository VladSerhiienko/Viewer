#pragma once

#include <MemoryManager.h>

#pragma warning( push )
#pragma warning( disable: 4244 )

#include <argh.h>
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
        virtual ~AppStoredValue( ) = default;
    };

    /**
     * @class AppState
     * @brief Contains members allowed for global access across the App classes
     */
    class AppState {
    public:
        std::shared_ptr< spdlog::logger >                          Logger;       /* Prints to console and file */
        argh::parser                                               Cmdl;         /* User parameters */
        std::map< std::string, apemode::unique_ptr< AppStoredValue > > StoredValues; /* User values */

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
     * @return Command line option of specified type.
     * @note Silences exceptions, returns default value.
     * auto sceneFiles = TGetOption< std::vector< std::string > >("scene", {});
     **/
    template < typename TOption >
    inline TOption TGetOption( const char* const optionName, const TOption& defaultValue ) {
        if ( AppState::Get( ) && AppState::Get( )->Cmdl[ {optionName} ] ) {
            TOption option;
            if ( AppState::Get( )->Cmdl( {optionName} ) >> option )
                return option;
        }

        return defaultValue;
    }

    template < >
    inline bool TGetOption( const char* const optionName, const bool& defaultValue ) {
        if ( AppState::Get( ) && AppState::Get( )->Cmdl[ {optionName} ] ) {
            return AppState::Get( )->Cmdl[ {optionName} ];
        }

        return defaultValue;
    }

    template <>
    inline std::string TGetOption( const char* const optionName, const std::string& defaultValue ) {
        if ( AppState::Get( ) && AppState::Get( )->Cmdl( {optionName} ) ) {
            std::string option = AppState::Get( )->Cmdl( {optionName} ).str( );
            return option;
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
