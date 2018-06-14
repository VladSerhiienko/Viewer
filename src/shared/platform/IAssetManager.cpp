#include "IAssetManager.h"

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

void AssetContentBufferFree( apemodeos::AssetContentData assetContentData ) {
    apemodeos::AssetContentBuffer assetContentBuffer;
    assetContentBuffer.pData    = assetContentData.pData;
    assetContentBuffer.dataSize = assetContentData.dataSize;
    assetContentBuffer.Free( );
}

apemodeos::AssetContentData apemodeos::AssetContentBuffer::Release( ) {
    apemodeos::AssetContentData assetContentData;

    assetContentData.pData    = pData;
    assetContentData.dataSize = dataSize;
    assetContentData.pFreeFn  = &AssetContentBufferFree;

    pData    = nullptr;
    dataSize = 0;

    return assetContentData;
}

apemodeos::AssetContentBuffer::~AssetContentBuffer( ) {
    Free( );
}