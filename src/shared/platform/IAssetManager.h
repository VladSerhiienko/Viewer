#pragma once

#include <vector>
#include <string>
#include <stdint.h>

namespace apemodeos {

    struct AssetContentBuffer {
        uint8_t* pData    = nullptr;
        size_t   dataSize = 0;
    };

    struct IAsset {
        virtual ~IAsset( )                                           = default;
        virtual const char*        GetName( ) const                  = 0;
        virtual const char*        GetId( ) const                    = 0;
        virtual AssetContentBuffer GetContentAsTextBuffer( ) const   = 0;
        virtual AssetContentBuffer GetContentAsBinaryBuffer( ) const = 0;
        virtual uint64_t           GetCurrentVersion( ) const        = 0;
        virtual void               SetCurrentVersion( uint64_t )     = 0;
    };

    struct IAssetManager {
        virtual ~IAssetManager( ) { }
        virtual const IAsset * GetAsset( const std::string& InAssetName ) const = 0;
    };
}