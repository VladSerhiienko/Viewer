#include "SceneUploaderVk.h"
#include <viewer/Scene.h>

#include <apemode/vk/QueuePools.Vulkan.h>
#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/Buffer.Vulkan.h>
#include <apemode/vk/TOneTimeCmdBufferSubmit.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>

#include <apemode/platform/AppState.h>
#include <apemode/platform/MathInc.h>
#include <apemode/platform/ArrayUtils.h>

//#define APEMODEVK_NO_GOOGLE_DRACO
#ifndef APEMODEVK_NO_GOOGLE_DRACO
#include <draco/compression/decode.h>
#endif

#include <cstdlib>

namespace {

/* Uploads resources from CPU to GPU with respect to staging memory limit. */
struct BufferUploadInfo {
    VkBuffer       pDstBuffer      = VK_NULL_HANDLE;
    const void*    pSrcBufferData  = nullptr;
    VkDeviceSize   SrcBufferSize   = 0;
    VkAccessFlags  eDstAccessFlags = 0;
};

struct BufferUploadCmpOpGreaterBySizeOrByAccessFlags {
    int operator( )( const BufferUploadInfo& a, const BufferUploadInfo& b ) const {
        if ( a.SrcBufferSize == b.SrcBufferSize ) {
            if ( a.eDstAccessFlags > b.eDstAccessFlags )
                return ( -1 );
            if ( a.eDstAccessFlags < b.eDstAccessFlags )
                return ( +1 );
            return 0;
        }
    
        if ( a.SrcBufferSize > b.SrcBufferSize )
            return ( -1 );
        if ( a.SrcBufferSize < b.SrcBufferSize )
            return ( +1 );
        return 0;
    }
};

struct ImageUploadInfo {
    const char*    pszFileName      = nullptr;
    const uint8_t* pFileContents    = nullptr;
    size_t         fileContentsSize = 0;
    bool           bGenerateMipMaps = false;
    uint32_t       Id               = 0;

    apemodevk::GraphicsDevice*                        pNode = nullptr;
    apemodevk::unique_ptr< apemodevk::UploadedImage > UploadedImg;
};

void UploadImage( ImageUploadInfo* pUploadInfo ) {
    assert( pUploadInfo && pUploadInfo->pNode );

    apemodevk::ImageDecoder::DecodeOptions decodeOptions;
    decodeOptions.eFileFormat = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_Autodetect;
    decodeOptions.bGenerateMipMaps = pUploadInfo->bGenerateMipMaps;

    apemodevk::ImageDecoder imgDecoder;
    if ( auto srcImg = imgDecoder.DecodeSourceImageFromData( pUploadInfo->pFileContents, pUploadInfo->fileContentsSize, decodeOptions ) ) {
        apemodevk::ImageUploader::UploadOptions uploadOptions;
        apemodevk::ImageUploader imgUploader;
        pUploadInfo->UploadedImg = imgUploader.UploadImage( pUploadInfo->pNode, *srcImg, uploadOptions );
    }
}

//struct ImageUploadTask {
//    MT_DECLARE_TASK( ImageUploadTask, MT::StackRequirements::STANDARD, MT::TaskPriority::HIGH, MT::Color::Blue );
//
//    ImageUploadInfo* pUploadInfo = nullptr;
//
//    ImageUploadTask( ImageUploadInfo* pUploadInfo ) : pUploadInfo( pUploadInfo ) {
//    }
//
//    void Do( MT::FiberContext& ctx ) {
//
//        const uint32_t fiberIndex        = ctx.fiberIndex;
//        const uint32_t threadWorkedIndex = ctx.GetThreadContext( )->workerIndex;
//
//        apemode::Log( apemode::LogLevel::Trace,
//                      "Executing task @{}: Fb#{} <- Th#{}, "
//                      "file: \"{}\"",
//                      (void*) this,
//                      fiberIndex,
//                      threadWorkedIndex,
//                      (const char*) pUploadInfo->pszFileName );
//
//        UploadImage( pUploadInfo );
//
//        apemode::Log( apemode::LogLevel::Trace,
//                      "Done task execution @{}: Fb#{} <- Th#{}, "
//                      "file: \"{}\"",
//                      (void*) this,
//                      fiberIndex,
//                      threadWorkedIndex,
//                      (const char*) pUploadInfo->pszFileName );
//    }
//};

// Something to consider adding: An eastl sort which uses qsort underneath.
// The primary purpose of this is to have an eastl interface for sorting which
// results in very little code generation, since all instances map to the
// C qsort function.

template < typename TValue, typename TCmpOp >
int TQSortCmpOp( const void* a, const void* b ) {
    return TCmpOp( )( *(const TValue*) a, *(const TValue*) b );
}

template < typename TValue, typename TCmpOp >
void TQSort( TValue* pFirst, TValue* pLast ) {
    qsort( pFirst, (size_t) eastl::distance( pFirst, pLast ), sizeof( TValue ), TQSortCmpOp< TValue, TCmpOp > );
}

VkSamplerCreateInfo GetDefaultSamplerCreateInfo( const float maxLod ) {
    VkSamplerCreateInfo samplerCreateInfo;
    apemodevk::InitializeStruct( samplerCreateInfo );
    samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.anisotropyEnable        = true;
    samplerCreateInfo.maxAnisotropy           = 16;
    samplerCreateInfo.compareEnable           = false;
    samplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
    samplerCreateInfo.minLod                  = 0;
    samplerCreateInfo.maxLod                  = maxLod;
    samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = false;
    return samplerCreateInfo;
}

const apemodevk::UploadedImage** GetLoadedImageSlotForPropertyName(
    apemode::vk::SceneUploader::MaterialDeviceAsset* pMaterialAsset, const char* pszTexturePropName ) {

    if ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pBaseColorImg;
    } else if ( strcmp( "diffuseTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pBaseColorImg;
    } else if ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pNormalImg;
    } else if ( strcmp( "occlusionTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pOcclusionImg;
    } else if ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pMetallicRoughnessImg;
    } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pMetallicRoughnessImg;
    } else if ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pEmissiveImg;
    }

    return nullptr;
}

bool ShouldGenerateMipMapsForPropertyName(  const char* pszTexturePropName ) {
    if ( ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) ||
         ( strcmp( "diffuseTexture", pszTexturePropName ) == 0 ) ||
         ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) ||
         ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) ||
         ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) ||
         ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) ) {
        return true;
    }
    return false;
}

struct SourceSubmeshInfo {
    const apemodefb::MeshFb*    pSrcMesh    = nullptr;
    const apemodefb::SubmeshFb* pSrcSubmesh = nullptr;

    bool IsOk( ) const {
        return pSrcMesh && pSrcSubmesh;
    }
    
    bool IsIndexTypeUInt32( ) const {
        assert( IsOk( ) );
        return pSrcMesh && pSrcMesh->index_type( ) == apemodefb::EIndexTypeFb_UInt32;
    }
    
    VkIndexType GetIndexType( ) const {
        return IsIndexTypeUInt32( ) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
    }

    bool IsCompressedMesh( ) const {
        assert( IsOk( ) );
        return pSrcSubmesh && pSrcSubmesh->compression_type( ) != apemodefb::ECompressionTypeFb_None;
    }
    
    apemodefb::ECompressionTypeFb GetCompressionType( ) const {
        return pSrcSubmesh->compression_type( );
    }
    
    const apemodefb::EVertexFormatFb GetVertexFormat( ) const {
        assert( IsOk( ) );
        return pSrcSubmesh->vertex_format( );
    }
};

SourceSubmeshInfo GetSrcSubmesh( apemode::SceneMesh&                                 mesh,
                                 const apemode::Scene*                               pScene,
                                 const apemode::vk::SceneUploader::UploadParameters* pParams ) {

    assert( pParams && pParams->pSrcScene );
    if ( pParams && pParams->pSrcScene && pParams->pSrcScene->meshes( ) ) {
        auto pMeshesFb = pParams->pSrcScene->meshes( );
        assert( pMeshesFb && pMeshesFb->size( ) > mesh.Id );
        auto pSrcMesh = pMeshesFb->Get( mesh.Id );
        assert( pSrcMesh && pSrcMesh->submeshes( ) && ( pSrcMesh->submeshes( )->size( ) == 1 ) );
        auto pSrcSubmesh = pSrcMesh->submeshes( )->Get( 0 );
        assert( pSrcSubmesh );
        return {pSrcMesh, pSrcSubmesh};
    }

    return {};
}


} // namespace

namespace {
struct DecompressedMeshInfo {
    apemodevk::vector< uint8_t > DecompressedVertexBuffer = {};
    apemodevk::vector< uint8_t > DecompressedIndexBuffer  = {};
    VkDeviceSize VertexCount = 0;
    VkDeviceSize IndexCount = 0;
    VkIndexType eIndexType = VK_INDEX_TYPE_MAX_ENUM;
    
    bool IsUsed( ) const {
        return eIndexType != VK_INDEX_TYPE_MAX_ENUM && VertexCount && IndexCount &&
               !DecompressedVertexBuffer.empty() &&
               !DecompressedIndexBuffer.empty() ;
    }
};

#ifndef APEMODEVK_NO_GOOGLE_DRACO
template < typename TIndex >
void TPopulateIndices( const draco::Mesh& decodedMesh, TIndex* pDstIndices ) {
    const uint32_t faceCount = decodedMesh.num_faces( );
    for ( uint32_t i = 0; i < faceCount; ++i ) {
        const draco::Mesh::Face faceIndices = decodedMesh.face( draco::FaceIndex( i ) );
        assert( faceIndices.size( ) == 3 );
        pDstIndices[ i * 3 + 0 ] = static_cast< const TIndex >( faceIndices[ 0 ].value( ) );
        pDstIndices[ i * 3 + 1 ] = static_cast< const TIndex >( faceIndices[ 1 ].value( ) );
        pDstIndices[ i * 3 + 2 ] = static_cast< const TIndex >( faceIndices[ 2 ].value( ) );
    }
}

template <typename TVertex>
void TPopulateVertices( const draco::Mesh& decodedMesh, DecompressedMeshInfo &decompressedMeshInfo );

template <>
void TPopulateVertices< apemodefb::StaticVertexQTangentFb >( const draco::Mesh&    decodedMesh,
                                                             DecompressedMeshInfo& decompressedMeshInfo ) {
    using namespace draco;
    using namespace apemodefb;
    
    const size_t vertexCount = decodedMesh.num_points();

    decompressedMeshInfo.VertexCount = vertexCount;
    decompressedMeshInfo.IndexCount = decodedMesh.num_faces() * 3;
    
    decompressedMeshInfo.DecompressedVertexBuffer.resize( sizeof( StaticVertexQTangentFb ) * vertexCount );
    auto pDstVertices = reinterpret_cast< StaticVertexQTangentFb* >( decompressedMeshInfo.DecompressedVertexBuffer.data( ) );

    constexpr int positionAttributeIndex  = 0;
    constexpr int qtangentAttributeIndex  = 1;
    constexpr int texCoordsAttributeIndex = 2;
    assert( decodedMesh.num_attributes( ) == 3 );

    const PointAttribute* positionAttribute  = decodedMesh.attribute( positionAttributeIndex );
    const PointAttribute* qtangentAttribute  = decodedMesh.attribute( qtangentAttributeIndex );
    const PointAttribute* texCoordsAttribute = decodedMesh.attribute( texCoordsAttributeIndex );

    assert( positionAttribute->attribute_type( ) == GeometryAttribute::Type::POSITION );
    assert( qtangentAttribute->attribute_type( ) == GeometryAttribute::Type::GENERIC );
    assert( texCoordsAttribute->attribute_type( ) == GeometryAttribute::Type::TEX_COORD );

    assert( positionAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( qtangentAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( texCoordsAttribute->data_type( ) == DataType::DT_FLOAT32 );

    assert( positionAttribute->num_components( ) == 3 );
    assert( qtangentAttribute->num_components( ) == 4 );
    assert( texCoordsAttribute->num_components( ) == 2 );
    
    assert( positionAttribute->byte_stride( ) == 3 * sizeof(float) );
    assert( qtangentAttribute->byte_stride( ) == 4 * sizeof(float) );
    assert( texCoordsAttribute->byte_stride( ) == 2 * sizeof(float) );

    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        const PointIndex typedPointIndex( i );
        StaticVertexQTangentFb& dstVertex = pDstVertices[ i ];
        
        positionAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_position( ) );
        qtangentAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_qtangent( ) );
        texCoordsAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_uv( ) );
    }
}

template <>
void TPopulateVertices< apemodefb::StaticVertexFb >( const draco::Mesh&    decodedMesh,
                                                     DecompressedMeshInfo& decompressedMeshInfo ) {
    using namespace draco;
    using namespace apemodefb;
    
    const size_t pointCount = decodedMesh.num_points( );

    decompressedMeshInfo.VertexCount = pointCount;
    decompressedMeshInfo.IndexCount = decodedMesh.num_faces() * 3;
    decompressedMeshInfo.DecompressedVertexBuffer.resize( sizeof( StaticVertexFb ) * pointCount );
    auto pDstVertices = reinterpret_cast< StaticVertexFb* >( decompressedMeshInfo.DecompressedVertexBuffer.data( ) );

    constexpr int positionAttributeIndex   = 0;
    constexpr int normalAttributeIndex     = 1;
    constexpr int tangentAttributeIndex    = 2;
    constexpr int reflectionAttributeIndex = 3;
    constexpr int texCoordsAttributeIndex  = 4;
    
    assert( decodedMesh.num_attributes( ) == 5 );

    const PointAttribute* positionAttribute   = decodedMesh.attribute( positionAttributeIndex );
    const PointAttribute* normalAttribute     = decodedMesh.attribute( normalAttributeIndex );
    const PointAttribute* tangentAttribute    = decodedMesh.attribute( tangentAttributeIndex );
    const PointAttribute* reflectionAttribute = decodedMesh.attribute( reflectionAttributeIndex );
    const PointAttribute* texCoordsAttribute  = decodedMesh.attribute( texCoordsAttributeIndex );

    assert( positionAttribute->attribute_type( ) == GeometryAttribute::Type::POSITION );
    assert( normalAttribute->attribute_type( ) == GeometryAttribute::Type::NORMAL );
    assert( tangentAttribute->attribute_type( ) == GeometryAttribute::Type::NORMAL );
    assert( reflectionAttribute->attribute_type( ) == GeometryAttribute::Type::GENERIC );
    assert( texCoordsAttribute->attribute_type( ) == GeometryAttribute::Type::TEX_COORD );

    assert( positionAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( normalAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( tangentAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( reflectionAttribute->data_type( ) == DataType::DT_FLOAT32 );
    assert( texCoordsAttribute->data_type( ) == DataType::DT_FLOAT32 );

    assert( positionAttribute->num_components( ) == 3 );
    assert( normalAttribute->num_components( ) == 3 );
    assert( tangentAttribute->num_components( ) == 3 );
    assert( reflectionAttribute->num_components( ) == 1 );
    assert( texCoordsAttribute->num_components( ) == 2 );
    
    assert( positionAttribute->byte_stride( ) == 3 * sizeof(float) );
    assert( normalAttribute->byte_stride( ) == 3 * sizeof(float) );
    assert( tangentAttribute->byte_stride( ) == 3 * sizeof(float) );
    assert( reflectionAttribute->byte_stride( ) == 1 * sizeof(float) );
    assert( texCoordsAttribute->byte_stride( ) == 2 * sizeof(float) );

    for ( uint32_t i = 0; i < pointCount; ++i ) {
        const PointIndex typedPointIndex( i );
        StaticVertexFb& dstVertex = pDstVertices[ i ];
        
        positionAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_position( ) );
        normalAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_normal( ) );
        tangentAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_tangent( ) );
        reflectionAttribute->GetMappedValue( typedPointIndex, (float*)&dstVertex.mutable_tangent( ) + 3 );
        texCoordsAttribute->GetMappedValue( typedPointIndex, &dstVertex.mutable_uv( ) );
    }
}

DecompressedMeshInfo DecompressMesh( apemode::SceneMesh& mesh, const SourceSubmeshInfo& srcSubmesh ) {
    using namespace draco;
    using namespace apemodefb;
    
    if ( !srcSubmesh.IsCompressedMesh( ) ) {
        assert( false && "The mesh buffers are uncompressed." );
        return {};
    }
    
    const ECompressionTypeFb eCompression = srcSubmesh.GetCompressionType();
    switch ( eCompression ) {
    case apemodefb::ECompressionTypeFb_GoogleDraco3D:
        break;
    default:
        assert( false && "Unsupported compression type." );
        return {};
    }
    
    const EVertexFormatFb eVertexFormat = srcSubmesh.GetVertexFormat( );
    switch ( eVertexFormat ) {
    case EVertexFormatFb_Static:
    case EVertexFormatFb_StaticQTangent:
        break;
    default:
        assert( false && "Unsupported vertex type." );
        return {};
    }
    
    auto start = std::chrono::high_resolution_clock::now();

    DecoderBuffer decoderBuffer;
    decoderBuffer.Init( (const char*) srcSubmesh.pSrcMesh->vertices( )->data( ),
                        (size_t) srcSubmesh.pSrcMesh->vertices( )->size( ) );

    Decoder decoder;
    Mesh decodedMesh;

    if ( Status::OK != decoder.DecodeBufferToGeometry( &decoderBuffer, &decodedMesh ).code( ) ) {
        return {};
    }
    
    DecompressedMeshInfo decompressedMeshInfo = {};

    const size_t vertexCount = decodedMesh.num_points( );
    const size_t faceCount   = decodedMesh.num_faces( );
    const size_t indexCount  = faceCount * 3;

    if ( indexCount > std::numeric_limits< uint16_t >::max( ) ) {
        assert( indexCount <= std::numeric_limits< uint32_t >::max( ) );
        decompressedMeshInfo.eIndexType = VK_INDEX_TYPE_UINT32;
        decompressedMeshInfo.DecompressedIndexBuffer.resize( sizeof( uint32_t ) * indexCount );
        TPopulateIndices( decodedMesh, (uint32_t*) decompressedMeshInfo.DecompressedIndexBuffer.data( ) );
    } else {
        assert( indexCount <= std::numeric_limits< uint16_t >::max( ) );
        decompressedMeshInfo.eIndexType = VK_INDEX_TYPE_UINT16;
        decompressedMeshInfo.DecompressedIndexBuffer.resize( sizeof( uint16_t ) * indexCount );
        TPopulateIndices( decodedMesh, (uint16_t*) decompressedMeshInfo.DecompressedIndexBuffer.data( ) );
    }

    switch ( eVertexFormat ) {
    case EVertexFormatFb_Static:
        TPopulateVertices< StaticVertexFb >( decodedMesh, decompressedMeshInfo );
        break;
    case EVertexFormatFb_StaticQTangent:
        TPopulateVertices< StaticVertexQTangentFb >( decodedMesh, decompressedMeshInfo );
        break;
        
    default:
        assert( false );
        return {};
    }
    
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;
    apemode::LogError( "Decoding done: {} vertices, {} indices, {} seconds", vertexCount, indexCount, diff.count() );

    return decompressedMeshInfo;
}
#endif

}

struct InitializedMeshInfo {
    eastl::optional< DecompressedMeshInfo >      OptionalDecompressedMesh = {};
    BufferUploadInfo                             VertexUploadInfo         = {};
    BufferUploadInfo                             IndexUploadInfo          = {};
    VkDeviceSize                                 VertexCount              = 0;
    VkDeviceSize                                 IndexCount               = 0;
    VkIndexType                                  eIndexType               = VK_INDEX_TYPE_MAX_ENUM;
    apemode::vk::SceneUploader::MeshDeviceAsset* pMeshAsset               = nullptr;

    bool IsOk( ) const {
        return VertexUploadInfo.pDstBuffer && IndexUploadInfo.pDstBuffer &&         // Buffers
               VertexUploadInfo.SrcBufferSize && VertexUploadInfo.pSrcBufferData && // Src Vertex Data
               IndexUploadInfo.SrcBufferSize && IndexUploadInfo.pSrcBufferData &&   // Src Index Data
               ( eIndexType != VK_INDEX_TYPE_MAX_ENUM ) && pMeshAsset &&
               ( OptionalDecompressedMesh ? ( !OptionalDecompressedMesh->DecompressedVertexBuffer.empty( ) &&
                                              !OptionalDecompressedMesh->DecompressedVertexBuffer.empty( ) &&
                                              ( OptionalDecompressedMesh->eIndexType != VK_INDEX_TYPE_MAX_ENUM ) )
                                          : true );
    }
};

InitializedMeshInfo InitializeMesh( apemode::SceneMesh&                                 mesh,
                                    apemode::Scene*                                     pScene,
                                    const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    using namespace apemodevk;
    using namespace eastl;
    
    InitializedMeshInfo initializedMeshInfo;

    auto srcSubmesh = GetSrcSubmesh( mesh, pScene, pParams );
    if ( !srcSubmesh.IsOk( ) ) {
        assert( false );
        return {};
    }
    
    VkBufferCreateInfo vertexBufferCreateInfo;
    VmaAllocationCreateInfo vertexAllocationCreateInfo;
    InitializeStruct( vertexBufferCreateInfo );
    InitializeStruct( vertexAllocationCreateInfo );
    
    VkBufferCreateInfo indexBufferCreateInfo;
    VmaAllocationCreateInfo indexAllocationCreateInfo;
    InitializeStruct( indexBufferCreateInfo );
    InitializeStruct( indexAllocationCreateInfo );

    if ( srcSubmesh.IsCompressedMesh( ) ) {
        #ifndef APEMODEVK_NO_GOOGLE_DRACO
        DecompressedMeshInfo decompressedMeshInfo = DecompressMesh( mesh, srcSubmesh );

        assert( !decompressedMeshInfo.DecompressedVertexBuffer.empty( ) );
        assert( !decompressedMeshInfo.DecompressedIndexBuffer.empty( ) );
        if ( decompressedMeshInfo.DecompressedVertexBuffer.empty( ) ||
             decompressedMeshInfo.DecompressedIndexBuffer.empty( ) ) {
            assert( false );
            return {};
        }

        initializedMeshInfo.VertexUploadInfo.pSrcBufferData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
        initializedMeshInfo.VertexUploadInfo.SrcBufferSize  = decompressedMeshInfo.DecompressedVertexBuffer.size( );
        initializedMeshInfo.VertexCount                     = decompressedMeshInfo.VertexCount;

        initializedMeshInfo.IndexUploadInfo.pSrcBufferData = decompressedMeshInfo.DecompressedIndexBuffer.data( );
        initializedMeshInfo.IndexUploadInfo.SrcBufferSize  = decompressedMeshInfo.DecompressedIndexBuffer.size( );
        initializedMeshInfo.IndexCount                     = decompressedMeshInfo.IndexCount;
        initializedMeshInfo.eIndexType                     = decompressedMeshInfo.eIndexType;

        initializedMeshInfo.OptionalDecompressedMesh.emplace( eastl::move( decompressedMeshInfo ) );
        
        pScene->Subsets[ mesh.BaseSubset ].IndexCount = (uint32_t)initializedMeshInfo.IndexCount;
        assert( mesh.SubsetCount == 1 );
        
        #else
        
        assert( false );
        return {};
        
        #endif
    } else {
        initializedMeshInfo.VertexUploadInfo.pSrcBufferData = srcSubmesh.pSrcMesh->vertices( )->data( );
        initializedMeshInfo.VertexUploadInfo.SrcBufferSize  = srcSubmesh.pSrcMesh->vertices( )->size( );
        initializedMeshInfo.VertexCount                     = srcSubmesh.pSrcSubmesh->vertex_count( );

        initializedMeshInfo.IndexUploadInfo.pSrcBufferData = srcSubmesh.pSrcMesh->indices( )->data( );
        initializedMeshInfo.IndexUploadInfo.SrcBufferSize  = srcSubmesh.pSrcMesh->indices( )->size( );
        initializedMeshInfo.IndexCount                     = srcSubmesh.pSrcSubmesh->vertex_count( );
        initializedMeshInfo.eIndexType                     = srcSubmesh.GetIndexType();
    }
    
    initializedMeshInfo.VertexUploadInfo.eDstAccessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    initializedMeshInfo.IndexUploadInfo.eDstAccessFlags  = VK_ACCESS_INDEX_READ_BIT;

    vertexBufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferCreateInfo.size      = initializedMeshInfo.VertexUploadInfo.SrcBufferSize;
    vertexAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    indexBufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferCreateInfo.size      = initializedMeshInfo.IndexUploadInfo.SrcBufferSize;
    indexAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    auto pMeshAsset = static_cast< apemode::vk::SceneUploader::MeshDeviceAsset* >( mesh.pDeviceAsset.get( ) );
    if ( !pMeshAsset ) {
        pMeshAsset = apemode_new apemode::vk::SceneUploader::MeshDeviceAsset( );
        mesh.pDeviceAsset.reset( pMeshAsset );
        assert( pMeshAsset );
    }

    pMeshAsset->VertexCount = initializedMeshInfo.VertexCount;
    pMeshAsset->IndexCount  = initializedMeshInfo.IndexCount;
    pMeshAsset->eIndexType  = initializedMeshInfo.eIndexType;
    
    initializedMeshInfo.pMeshAsset = pMeshAsset;

    if ( !pMeshAsset->hVertexBuffer.Recreate( pParams->pNode->hAllocator, vertexBufferCreateInfo, vertexAllocationCreateInfo ) ||
         !pMeshAsset->hIndexBuffer.Recreate( pParams->pNode->hAllocator, indexBufferCreateInfo, indexAllocationCreateInfo ) ) {
        return {};
    }

    initializedMeshInfo.VertexUploadInfo.pDstBuffer = pMeshAsset->hVertexBuffer.Handle.pBuffer;
    initializedMeshInfo.IndexUploadInfo.pDstBuffer  = pMeshAsset->hIndexBuffer.Handle.pBuffer;

    assert( initializedMeshInfo.VertexUploadInfo.pDstBuffer );
    assert( initializedMeshInfo.IndexUploadInfo.pDstBuffer );

    return initializedMeshInfo;
}

bool UploadMeshes( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    using namespace apemodevk;
    using namespace eastl;

    assert( pScene && pParams && pParams->pSrcScene && pParams->pNode );

    auto pMeshesFb = pParams->pSrcScene->meshes( );
    if ( !pMeshesFb || !pMeshesFb->size( ) ) {
        return false;
    }

    auto pNode = pParams->pNode;
    uint64_t totalBytesRequired = 0;

    apemode::vector< InitializedMeshInfo > initializedMeshInfos;
    apemode::vector< BufferUploadInfo > bufferUploads;
    
    initializedMeshInfos.reserve( pMeshesFb->size( ) );
    bufferUploads.reserve( pMeshesFb->size( ) << 1 );

    for ( auto & mesh : pScene->Meshes ) {
        InitializedMeshInfo initializedMeshInfo = InitializeMesh( mesh, pScene, pParams );
        if ( initializedMeshInfo.IsOk() ) {
            auto& emplacedInitializedMeshInfo = initializedMeshInfos.emplace_back( eastl::move( initializedMeshInfo ) );
            bufferUploads.push_back( emplacedInitializedMeshInfo.VertexUploadInfo );
            bufferUploads.push_back( emplacedInitializedMeshInfo.IndexUploadInfo );
            totalBytesRequired += emplacedInitializedMeshInfo.VertexUploadInfo.SrcBufferSize;
            totalBytesRequired += emplacedInitializedMeshInfo.IndexUploadInfo.SrcBufferSize;
        }
    }

    { /* Sort by size in descending order. */
        BufferUploadInfo* pMeshUploadIt = bufferUploads.data( );
        BufferUploadInfo* pMeshUploadItLast = pMeshUploadIt + ( bufferUploads.size( ) - 1 );
        TQSort< BufferUploadInfo, BufferUploadCmpOpGreaterBySizeOrByAccessFlags >( pMeshUploadIt, pMeshUploadItLast );
    }

    const BufferUploadInfo* const pMeshUploadIt    = bufferUploads.data( );
    const BufferUploadInfo* const pMeshUploadItEnd = pMeshUploadIt + bufferUploads.size( );

    /* Pipeline Barrier: TRANSFER -> VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT or VK_ACCESS_INDEX_READ_BIT. */
    apemodevk::TOneTimeCmdBufferSubmit( pNode, 0, true, [&]( VkCommandBuffer pCmdBuffer ) {

        apemode::vector< VkBufferMemoryBarrier > bufferBarriers;
        bufferBarriers.reserve( bufferUploads.size( ) );

        /* Initialize buffer barriers. */
        transform( bufferUploads.begin( ),
                   bufferUploads.end( ),
                   back_inserter( bufferBarriers ),
                   []( const BufferUploadInfo & currMeshUpload ) {
                       VkBufferMemoryBarrier bufferMemoryBarrier;
                       apemodevk::InitializeStruct( bufferMemoryBarrier );

                       bufferMemoryBarrier.size                = VK_WHOLE_SIZE;
                       bufferMemoryBarrier.buffer              = currMeshUpload.pDstBuffer;
                       bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                       bufferMemoryBarrier.dstAccessMask       = currMeshUpload.eDstAccessFlags;
                       bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                       bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                       return bufferMemoryBarrier;
                   } );

        /* Stage buffers. */
        vkCmdPipelineBarrier( pCmdBuffer,                         /* Cmd */
                              VK_PIPELINE_STAGE_TRANSFER_BIT,     /* Src stage */
                              VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, /* Dst stage */
                              0,                                  /* Dependency flags */
                              0,                                  /* Memory barrier count */
                              nullptr,                            /* Memory barriers */
                              uint32_t( bufferBarriers.size( ) ), /* Buffer barrier count */
                              bufferBarriers.data( ),             /* Buffer barriers */
                              0,                                  /* Img barrier count */
                              nullptr );                          /* Img barriers */

        return true;
    } );

    const size_t contiguousBufferSize    = max< size_t >( pParams->StagingMemoryLimitHint, pMeshUploadIt->SrcBufferSize );
    const size_t stagingMemorySizeNeeded = max< size_t >( pNode->AdapterProps.limits.maxUniformBufferRange, contiguousBufferSize );
    const size_t stagingMemorySize       = min< size_t >( totalBytesRequired, stagingMemorySizeNeeded );

    /* The purpose is to get some control over the memory allocation from CPU heap
     * and make it efficient, manageable and easy to understand.
     * The idea is to allocated some reasonable amount of staging memory and reuse it.
     */
    THandle< BufferComposite > hStagingBuffer;

    VkBufferCreateInfo stagingCreateInfo;
    InitializeStruct( stagingCreateInfo );
    stagingCreateInfo.size  = stagingMemorySize;
    stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocationCreateInfo;
    InitializeStruct( stagingAllocationCreateInfo );
    stagingAllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if ( !hStagingBuffer.Recreate( pNode->hAllocator, stagingCreateInfo, stagingAllocationCreateInfo ) ) {
        return false;
    }

    uint8_t* pMappedStagingMemory = MapStagingBuffer( pNode, hStagingBuffer );
    if ( !pMappedStagingMemory ) {
        return false;
    }

    /* While there are elements that need to be uploaded. */
    auto pCurrFillInfo = pMeshUploadIt;
    while ( pCurrFillInfo != pMeshUploadItEnd ) {

        uint8_t* pMappedStagingMemoryHead = pMappedStagingMemory;
        uint64_t stagingMemorySpaceLeft   = stagingMemorySize;

        /* While there are elements that need to be uploaded. */
        while ( pCurrFillInfo != pMeshUploadItEnd ) {
            /* Check the limit. */
            if ( stagingMemorySpaceLeft < pCurrFillInfo->SrcBufferSize ) {
                /* Flush and submit. */
                stagingMemorySpaceLeft = 0;
                break;
            }

            /* Copy the src buffer data to the staging memory. */
            memcpy( pMappedStagingMemoryHead, pCurrFillInfo->pSrcBufferData, pCurrFillInfo->SrcBufferSize );

            VkDeviceSize bufferOffset = static_cast< VkDeviceSize >( pMappedStagingMemoryHead - pMappedStagingMemory );
            pMappedStagingMemoryHead += pCurrFillInfo->SrcBufferSize;
            stagingMemorySpaceLeft -= pCurrFillInfo->SrcBufferSize;

            /* Add a copy command. */
            VkBufferCopy bufferCopy;
            apemodevk::InitializeStruct( bufferCopy );
            bufferCopy.dstOffset = 0;
            bufferCopy.srcOffset = bufferOffset;
            bufferCopy.size      = pCurrFillInfo->SrcBufferSize;

            /* Get the queue pool and acquire a queue.
             * Allocate a command buffer and give it to the lambda that pushes copy commands into it.
             */
            TOneTimeCmdBufferSubmit( pNode, 0, true, [&]( VkCommandBuffer pCmdBuffer ) {

                /* Add the copy command. */
                vkCmdCopyBuffer( pCmdBuffer,                /* Cmd */
                                 hStagingBuffer,            /* Src */
                                 pCurrFillInfo->pDstBuffer, /* Dst */
                                 1,                         /* RegionCount*/
                                 &bufferCopy );             /* Regions */

                /* End command buffer.
                 * Submit and sync.
                 */
                return true;
            } ); /* TSubmitCmdBuffer */

            /* Move to the next item. */
            ++pCurrFillInfo;
        } /* while */
    }

    return true;
}

bool InitializeMaterials( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {

    /* Create material device asset if needed. */
    auto pSceneAsset = static_cast< apemode::vk::SceneUploader::DeviceAsset* >( pScene->pDeviceAsset.get( ) );
    if ( nullptr == pSceneAsset ) {
        pSceneAsset = apemode_new apemode::vk::SceneUploader::DeviceAsset( );
        pScene->pDeviceAsset.reset( pSceneAsset );
        assert( pSceneAsset );
    }

    apemodevk::ImageUploader imgUploader;
    apemodevk::ImageDecoder  imgDecoder;

    uint8_t imageBytes[ 4 ];

    apemodevk::ImageUploader::UploadOptions  loadOptions;
    apemodevk::ImageDecoder::DecodeOptions decodeOptions;

    loadOptions.bImgView = true;
    decodeOptions.bGenerateMipMaps = false;

    imageBytes[ 0 ] = 0;
    imageBytes[ 1 ] = 0;
    imageBytes[ 2 ] = 0;
    imageBytes[ 3 ] = 255;

    auto srcImgZeros = imgDecoder.CreateSourceImage2D( imageBytes, {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, decodeOptions );
    pSceneAsset->MissingTextureZeros = imgUploader.UploadImage( pParams->pNode, *srcImgZeros, loadOptions );
    if ( !pSceneAsset->MissingTextureZeros ) {
        apemode::LogError( "Failed to create texture for missing slot." );
        return false;
    }

    imageBytes[ 0 ] = 255;
    imageBytes[ 1 ] = 255;
    imageBytes[ 2 ] = 255;
    imageBytes[ 3 ] = 255;

    auto srcImgOnes = imgDecoder.CreateSourceImage2D( imageBytes, {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, decodeOptions );
    pSceneAsset->MissingTextureOnes = imgUploader.UploadImage( pParams->pNode, *srcImgOnes, loadOptions );
    if ( !pSceneAsset->MissingTextureOnes ) {
        apemode::LogError( "Failed to create texture for missing slot." );
        return false;
    }

    pSceneAsset->pMissingSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( 0 ) );
    return true;
}

bool FinalizeMaterial( apemode::vk::SceneUploader::MaterialDeviceAsset*    pMaterialAsset,
                       const apemode::vk::SceneUploader::UploadParameters* pParams ) {

    if ( pMaterialAsset->pBaseColorImg ) {
        const float maxLod = float( pMaterialAsset->pBaseColorImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pBaseColorSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pBaseColorImg->ImgViewCreateInfo;
        imgViewCreateInfo.image = pMaterialAsset->pBaseColorImg->hImg;

        if ( !pMaterialAsset->hBaseColorImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
            return false;
        }

    } /* pBaseColorImg */

    if ( pMaterialAsset->pNormalImg ) {
        const float maxLod = float( pMaterialAsset->pNormalImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pNormalSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pNormalImg->ImgViewCreateInfo;
        imgViewCreateInfo.image = pMaterialAsset->pNormalImg->hImg;

        if ( !pMaterialAsset->hNormalImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
            return false;
        }

    } /* pNormalImg */

    if ( pMaterialAsset->pEmissiveImg ) {
        const float maxLod = float( pMaterialAsset->pEmissiveImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pEmissiveSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pEmissiveImg->ImgViewCreateInfo;
        imgViewCreateInfo.image                 = pMaterialAsset->pEmissiveImg->hImg;
        imgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_ONE;

        if ( !pMaterialAsset->hEmissiveImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
            return false;
        }

    } /* pEmissiveImg */

    if ( pMaterialAsset->pMetallicRoughnessImg ) {
        const float maxLod = float( pMaterialAsset->pMetallicRoughnessImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pMetallicSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );
        pMaterialAsset->pRoughnessSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo metallicImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessImg->ImgViewCreateInfo;
        metallicImgViewCreateInfo.image                 = pMaterialAsset->pMetallicRoughnessImg->hImg;
        metallicImgViewCreateInfo.components.r          = VK_COMPONENT_SWIZZLE_R;
        metallicImgViewCreateInfo.components.g          = VK_COMPONENT_SWIZZLE_G;
        metallicImgViewCreateInfo.components.b          = VK_COMPONENT_SWIZZLE_B;
        metallicImgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_A;
//        metallicImgViewCreateInfo.components.g          = VK_COMPONENT_SWIZZLE_ONE;
//        metallicImgViewCreateInfo.components.b          = VK_COMPONENT_SWIZZLE_ONE;
//        metallicImgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_ONE;

        VkImageViewCreateInfo roughnessImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessImg->ImgViewCreateInfo;
        roughnessImgViewCreateInfo.image                 = pMaterialAsset->pMetallicRoughnessImg->hImg;
        roughnessImgViewCreateInfo.components.r          = VK_COMPONENT_SWIZZLE_R;
        roughnessImgViewCreateInfo.components.g          = VK_COMPONENT_SWIZZLE_G;
        roughnessImgViewCreateInfo.components.b          = VK_COMPONENT_SWIZZLE_B;
        roughnessImgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_A;
//        roughnessImgViewCreateInfo.components.r          = VK_COMPONENT_SWIZZLE_G;
//        roughnessImgViewCreateInfo.components.g          = VK_COMPONENT_SWIZZLE_ONE;
//        roughnessImgViewCreateInfo.components.b          = VK_COMPONENT_SWIZZLE_ONE;
//        roughnessImgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_ONE;

        if ( !pMaterialAsset->hMetallicImgView.Recreate( pParams->pNode->hLogicalDevice, metallicImgViewCreateInfo ) ) {
            return false;
        }

        if ( !pMaterialAsset->hRoughnessImgView.Recreate( pParams->pNode->hLogicalDevice, roughnessImgViewCreateInfo ) ) {
            return false;
        }

    } /* pMetallicRoughnessImg */

    if ( pMaterialAsset->pOcclusionImg ) {
        const float maxLod = float( pMaterialAsset->pOcclusionImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pOcclusionSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo occlusionImgViewCreateInfo = pMaterialAsset->pOcclusionImg->ImgViewCreateInfo;
        occlusionImgViewCreateInfo.image                 = pMaterialAsset->pOcclusionImg->hImg;
        occlusionImgViewCreateInfo.components.r          = VK_COMPONENT_SWIZZLE_B;
        occlusionImgViewCreateInfo.components.g          = VK_COMPONENT_SWIZZLE_ZERO;
        occlusionImgViewCreateInfo.components.b          = VK_COMPONENT_SWIZZLE_ZERO;
        occlusionImgViewCreateInfo.components.a          = VK_COMPONENT_SWIZZLE_ZERO;

        if ( !pMaterialAsset->hOcclusionImgView.Recreate( pParams->pNode->hLogicalDevice, occlusionImgViewCreateInfo ) ) {
            return false;
        }
    } /* pOcclusionImg */

    return true;
}

bool UploadMaterials( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    using namespace eastl;

    if ( !InitializeMaterials( pScene, pParams ) ) {
        return false;
    }

    
    auto pTaskflow =  apemode::AppState::Get( )->GetDefaultTaskflow( );
    // MT::TaskScheduler* pTaskScheduler = apemode::AppState::Get( )->GetTaskScheduler( );

    auto pMaterialsFb = pParams->pSrcScene->materials( );
    auto pTexturesFb  = pParams->pSrcScene->textures( );
    auto pFilesFb     = pParams->pSrcScene->files( );

    if ( !pMaterialsFb || !pMaterialsFb->size( ) || !pTexturesFb || !pTexturesFb->size( ) || !pFilesFb || !pFilesFb->size( ) ) {
        return true;
    }


    /* Create material device asset if needed. */
    auto pSceneAsset = static_cast< apemode::vk::SceneUploader::DeviceAsset* >( pScene->pDeviceAsset.get( ) );
    assert( pSceneAsset );

    apemode::vector_map< uint32_t, ImageUploadInfo > imgUploads;
    imgUploads.reserve( pTexturesFb->size( ) );

    for ( auto& material : pScene->Materials ) {

        auto pMaterialFb = pMaterialsFb->Get( material.Id );
        if ( !pMaterialFb )
            continue;

        auto pTexturePropertiesFb = pMaterialFb->texture_properties( );
        if ( !pTexturePropertiesFb )
            continue;

        /* Create material device asset if needed. */
        auto pMaterialAsset = static_cast< apemode::vk::SceneUploader::MaterialDeviceAsset* >( material.pDeviceAsset.get( ) );
        if ( nullptr == pMaterialAsset ) {
            pMaterialAsset = apemode_new apemode::vk::SceneUploader::MaterialDeviceAsset( );
            material.pDeviceAsset.reset( pMaterialAsset );
        }

        pMaterialAsset->pszName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pMaterialFb->name_id( ) );
        apemode::LogError( "Loading textures for material: \"{}\"", pMaterialAsset->pszName );

        for ( auto pTexturePropFb : *pTexturePropertiesFb ) {

            auto pszTexturePropName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pTexturePropFb->name_id( ) );
            auto ppLoadedImgMaterialSlot = GetLoadedImageSlotForPropertyName( pMaterialAsset, pszTexturePropName );

            if ( nullptr == ppLoadedImgMaterialSlot ) {
                apemode::LogError( "Cannot map texture property: \"{}\"", pszTexturePropName );
                continue;
            }

            const bool bGenerateMipMaps = ShouldGenerateMipMapsForPropertyName( pszTexturePropName );

            auto pTextureFb = pTexturesFb->Get( pTexturePropFb->value_id( ) );
            auto pFileFb = pFilesFb->Get( pTextureFb->file_id( ) );

            auto imgUploadIt = imgUploads.find( pFileFb->id( ) );
            if ( imgUploadIt != imgUploads.end( ) ) {
                imgUploadIt->second.bGenerateMipMaps |= bGenerateMipMaps;
                continue;
            }

            auto pszFileName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pFileFb->name_id( ) );

            if ( !pFileFb->buffer( )->size( ) ) {
                apemode::LogError( "Empty file: \"{}\"", pszFileName );
                continue;
            }

            ImageUploadInfo imgUpload;
            imgUpload.Id               = uint32_t( imgUploads.size( ) );
            imgUpload.pszFileName      = pszFileName;
            imgUpload.pFileContents    = pFileFb->buffer( )->data( );
            imgUpload.fileContentsSize = pFileFb->buffer( )->size( );
            imgUpload.bGenerateMipMaps = bGenerateMipMaps;
            imgUpload.pNode            = pParams->pNode;

            imgUploads[ pFileFb->id( ) ] = std::move( imgUpload );
            apemode::LogInfo( "Scheduled texture upload: #{} \"{}\"", pFileFb->id( ), imgUpload.pszFileName );
        } /* pTexturePropFb */
    }     /* pMaterialFb */

#if 1

    if ( !imgUploads.empty( ) ) {
//        apemode::vector< ImageUploadTask > imageUploadTasks;
//        imageUploadTasks.reserve( imgUploads.size( ) );
//        for ( auto& imgUpload : imgUploads ) {
//            imageUploadTasks.emplace_back( &imgUpload.second );
//        }
        
        for ( auto& imgUpload : imgUploads ) {
            //pTaskflow->silent_emplace([&imgUpload] () {
                
            // apemode::Log( apemode::LogLevel::Trace, "Starting loading image: \"{}\"", (const char*) imgUpload.second.pszFileName );
            UploadImage(&imgUpload.second); // });
            // apemode::Log( apemode::LogLevel::Trace, "Done loading image: \"{}\"", (const char*) imgUpload.second.pszFileName );
        }
        
        // pTaskflow->wait_for_all();
    }

#else

    for ( auto& imgUpload : imgUploads ) {
        UploadImage( &imgUpload.second );
    }

#endif

    for ( auto& material : pScene->Materials ) {

        auto pMaterialFb = pMaterialsFb->Get( material.Id );
        if ( !pMaterialFb )
            continue;

        auto pTexturePropertiesFb = pMaterialFb->texture_properties( );
        if ( !pTexturePropertiesFb )
            continue;

        auto pMaterialAsset = static_cast< apemode::vk::SceneUploader::MaterialDeviceAsset* >( material.pDeviceAsset.get( ) );
        assert( pMaterialAsset );

        for ( auto pTexturePropFb : *pTexturePropertiesFb ) {

            auto pszTexturePropName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pTexturePropFb->name_id( ) );
            auto ppLoadedImgMaterialSlot = GetLoadedImageSlotForPropertyName( pMaterialAsset, pszTexturePropName );

            if ( ppLoadedImgMaterialSlot ) {

                auto pTextureFb = pTexturesFb->Get( pTexturePropFb->value_id( ) );
                auto pFileFb = pFilesFb->Get( pTextureFb->file_id( ) );
                auto pszFileName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pFileFb->name_id( ) );
                auto imgUploadIt = imgUploads.find( pFileFb->id( ) );

                if ( imgUploadIt != imgUploads.end( ) ) {
                    apemode::LogInfo( "Assigned material texture: Img \"{}\", Slot {}", pszFileName, pszTexturePropName );
                    ( *ppLoadedImgMaterialSlot ) = imgUploads[ pFileFb->id( ) ].UploadedImg.get( );
                }
            }

        } /* pTexturePropFb */
    }     /* pMaterialFb */

    pSceneAsset->LoadedImgs.reserve( imgUploads.size( ) );
    for ( auto& imgUpload : imgUploads ) {
        pSceneAsset->LoadedImgs.emplace_back( std::move( imgUpload.second.UploadedImg ) );
    }

    apemodevk::Wipe( imgUploads );

    for ( auto& material : pScene->Materials ) {

        auto pMaterialAsset = static_cast< apemode::vk::SceneUploader::MaterialDeviceAsset* >( material.pDeviceAsset.get( ) );
        if ( !FinalizeMaterial( pMaterialAsset, pParams ) ) {
            return false;
        }

    } /* pMaterialFb */

    return true;
}

bool apemode::vk::SceneUploader::UploadScene( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    if ( !pParams || !pScene )
        return false;

    if ( !UploadMeshes( pScene, pParams ) ) {
        return false;
    }
    
    if ( !UploadMaterials( pScene, pParams ) ) {
        return false;
    }

    for ( apemode::SceneSkin& skin : pScene->Skins ) {
        DeviceAsset* deviceAsset  = static_cast< DeviceAsset* >( pScene->pDeviceAsset.get( ) );
        deviceAsset->MaxBoneCount = eastl::max( deviceAsset->MaxBoneCount, skin.LinkIds.size( ) );
    }

    return true;
}
