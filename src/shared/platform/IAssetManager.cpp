
#include "IAssetManager.h"
#include <MemoryManager.h>

void apemodeos::AssetContentBuffer::Alloc( size_t size, size_t alignment ) {
    Free( );
    pData = reinterpret_cast< uint8_t* >( apemode_malloc( size, alignment ) );
    if ( pData ) {
        dataSize = size;
    }
}

void apemodeos::AssetContentBuffer::Free( ) {
    if ( pData ) {
        apemode_free( pData );
        Release( );
    }
}

apemodeos::AssetContentData apemodeos::AssetContentBuffer::Release( ) {
    apemodeos::AssetContentData assetContentData;

    assetContentData.pData    = pData;
    assetContentData.dataSize = dataSize;
    assetContentData.pFreeFn  = &AssetContentBuffer::Free;

    pData    = nullptr;
    dataSize = 0;

    return assetContentData;
}

apemodeos::AssetContentBuffer::~AssetContentBuffer( ) {
    Free( );
}

void apemodeos::AssetContentBuffer::Free( AssetContentData assetContentData ) {
    AssetContentBuffer assetContentBuffer;
    assetContentBuffer.pData    = assetContentData.pData;
    assetContentBuffer.dataSize = assetContentData.dataSize;
    assetContentBuffer.Free( );
}