#pragma once

#include <vector>
#include <string>
#include <stdint.h>

namespace apemodeos {

    struct IAsset {
        virtual ~IAsset( )                              = default;
        virtual const std::string&     GetName( ) const = 0;
        virtual const std::string&     GetId( ) const   = 0;
        virtual std::string            AsTxt( ) const   = 0;
        virtual std::vector< uint8_t > AsBin( ) const   = 0;
    };

    struct IAssetManager {
        virtual ~IAssetManager( ) { }
        virtual const IAsset * GetAsset( const std::string& InAssetName ) const = 0;
    };
}