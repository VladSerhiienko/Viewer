#pragma once

#pragma warning( push )
#pragma warning( disable: 4244 )

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#pragma warning( pop )

namespace apemode {

    /**
     * @class AppState
     * @brief Contains members allowed for global access across the App classes
     * @note All the subsystems, that can potentially can be used as separate modules
     *       must avoid the usage of these members.
     */
    class AppState {
    public:
        int    Argc = 0;
        char** Argv = nullptr;

        std::shared_ptr< spdlog::logger >   Logger;  /* Prints to console and file, on Windows also prints to Output Window */
        std::unique_ptr< cxxopts::Options > Options; /* User parameters */

        /**
         * @return Application state instance.
         * @note Returns null before the application creation, or after its destruction.
         */
        static AppState* Get( );
        static void      OnMain( int argc, char* argv[] );
        static void      OnExit( );

    private:
        AppState( int args, char* ppArgs[] );
        ~AppState( );
    };

    template < typename TFunc >
    inline void IfHasOptions( TFunc F ) {
        if ( AppState::Get( ) && AppState::Get( )->Options )
            F( *AppState::Get( )->Options );
    }

    template < typename TOption >
    inline const TOption & TGetOption( const char* optionName, const TOption & defaultValue ) {

        if ( AppState::Get( ) && AppState::Get( )->Options ) {

            auto& optionDetails = AppState::Get( )->Options->operator[]( optionName );
            return 0 == optionDetails.count( ) ? defaultValue : optionDetails.as< TOption >( );
        }

        return defaultValue;
    }
} // namespace apemode
