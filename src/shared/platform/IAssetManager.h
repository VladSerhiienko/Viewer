#pragma once

#include <MemoryManager.h>

namespace apemodeos {

    struct AssetContentData {
        using FreeFn = void ( * )( AssetContentData );

        uint8_t* pData    = nullptr;
        size_t   dataSize = 0;
        FreeFn   pFreeFn  = nullptr;
    };

    struct AssetContentBuffer {
        uint8_t* pData    = nullptr;
        size_t   dataSize = 0;

        ~AssetContentBuffer( );

        AssetContentData Release( );
        void             Alloc( size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
        void             Free( );
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
        virtual ~IAssetManager( )                                        = default;
        virtual const IAsset* GetAsset( const char* pszAssetName ) const = 0;
    };
}