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
        return &pMaterialAsset->pMetallicRoughnessOcclusionImg;
    // } else if ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) {
    //    return &pMaterialAsset->pMetallicRoughnessOcclusionImg;
    } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
        return &pMaterialAsset->pMetallicRoughnessOcclusionImg;
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
         // ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) ||
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
    apemodevk::vector< uint8_t > RenderableVertexBuffer = {};
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

namespace {
}

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
void TDecompressVertices( const draco::Mesh&    decodedMesh,
                        DecompressedMeshInfo& decompressedMeshInfo,
                        const size_t vertexCount,
                        const size_t vertexStride );

template <>
void TDecompressVertices< apemodefb::DecompressedVertexFb >( const draco::Mesh&    decodedMesh,
                                                             DecompressedMeshInfo& decompressedMeshInfo,
                                                             const size_t          vertexCount,
                                                             const size_t          vertexStride ) {
    using namespace draco;
    using namespace apemodefb;

    constexpr int positionAttributeIndex        = 0;
    constexpr int uvAttributeIndex              = 1;
    constexpr int normalAttributeIndex          = 2;
    constexpr int tangentAttributeIndex         = 3;
    constexpr int colorAttributeIndex           = 4;
    constexpr int reflectionIndexAttributeIndex = 5;

    const PointAttribute* positionAttribute        = decodedMesh.attribute( positionAttributeIndex );
    const PointAttribute* uvAttribute              = decodedMesh.attribute( uvAttributeIndex );
    const PointAttribute* normalAttribute          = decodedMesh.attribute( normalAttributeIndex );
    const PointAttribute* tangentAttribute         = decodedMesh.attribute( tangentAttributeIndex );
    const PointAttribute* colorAttribute           = decodedMesh.attribute( colorAttributeIndex );
    const PointAttribute* reflectionIndexAttribute = decodedMesh.attribute( reflectionIndexAttributeIndex );

    auto vertexData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto             dstVertex = reinterpret_cast< apemodefb::DecompressedVertexFb* >( vertexData + i * vertexStride );
        const PointIndex typedPointIndex( i );

        positionAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_position( ) );
        uvAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_uv( ) );
        normalAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_normal( ) );
        tangentAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_tangent( ) );
        colorAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_color( ) );

        uint32_t reflectionIndex = 0;
        reflectionIndexAttribute->GetMappedValue( typedPointIndex, &reflectionIndex );
        dstVertex->mutate_reflection_index_packed( reflectionIndex );
    }
}

template <>
void TDecompressVertices< apemodefb::DecompressedSkinnedVertexFb >( const draco::Mesh&    decodedMesh,
                                                                    DecompressedMeshInfo& decompressedMeshInfo,
                                                                    const size_t          vertexCount,
                                                                    const size_t          vertexStride ) {
    TDecompressVertices< apemodefb::DecompressedVertexFb >( decodedMesh, decompressedMeshInfo, vertexCount, vertexStride );

    using namespace draco;
    using namespace apemodefb;

    constexpr int jointIndicesAttributeIndex = 6;
    constexpr int jointWeightsAttributeIndex = 7;

    const PointAttribute* jointIndicesAttribute = decodedMesh.attribute( jointIndicesAttributeIndex );
    const PointAttribute* jointWeightsAttribute = decodedMesh.attribute( jointWeightsAttributeIndex );

    auto vertexData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto dstVertex = reinterpret_cast< apemodefb::DecompressedSkinnedVertexFb* >( vertexData + i * vertexStride );
        const PointIndex typedPointIndex( i );

        uint32_t jointIndices = 0;
        jointIndicesAttribute->GetMappedValue( typedPointIndex, &jointIndices );
        dstVertex->mutate_joint_indices( jointIndices );
        jointWeightsAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_joint_weights( ) );
    }
}

template <>
void TDecompressVertices< apemodefb::DecompressedFatSkinnedVertexFb >( const draco::Mesh&    decodedMesh,
                                                                       DecompressedMeshInfo& decompressedMeshInfo,
                                                                       const size_t          vertexCount,
                                                                       const size_t          vertexStride ) {
    TDecompressVertices< apemodefb::DecompressedSkinnedVertexFb >(
        decodedMesh, decompressedMeshInfo, vertexCount, vertexStride );

    using namespace draco;
    using namespace apemodefb;

    constexpr int extraJointIndicesAttributeIndex = 8;
    constexpr int extraJointWeightsAttributeIndex = 9;

    const PointAttribute* extraJointIndicesAttribute = decodedMesh.attribute( extraJointIndicesAttributeIndex );
    const PointAttribute* extraJointWeightsAttribute = decodedMesh.attribute( extraJointWeightsAttributeIndex );

    auto vertexData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto dstVertex = reinterpret_cast< apemodefb::DecompressedFatSkinnedVertexFb* >( vertexData + i * vertexStride );
        const PointIndex typedPointIndex( i );

        uint32_t jointIndices = 0;
        extraJointIndicesAttribute->GetMappedValue( typedPointIndex, &jointIndices );
        dstVertex->mutate_extra_joint_indices( jointIndices );
        extraJointWeightsAttribute->GetMappedValue( typedPointIndex, &dstVertex->mutable_extra_joint_weights( ) );
    }
}

apemodefb::QuatFb GetQTangent( const apemodefb::Vec3Fb normalFb, const apemodefb::Vec3Fb tangentFb, float reflection ) {
    assert( reflection != 0.0 );
    using namespace apemode;
    XMFLOAT4 qd;

    XMVECTOR n = XMLoadFloat3( (const XMFLOAT3*) &normalFb );
    XMVECTOR t = XMLoadFloat3( (const XMFLOAT3*) &tangentFb );
    XMVECTOR b = XMVector3Cross( n, t );
    b          = XMVector3Normalize( b );

    XMMATRIX tangentFrame( n, t, b, g_XMIdentityR3 );
    XMVECTOR q = XMQuaternionRotationMatrix( tangentFrame );
    q          = XMQuaternionNormalize( q );

    // https://bitbucket.org/sinbad/ogre/src/18ebdbed2edc61d30927869c7fb0cf3ae5697f0a/OgreMain/src/OgreSubMesh2.cpp?at=v2-1&fileviewer=file-view-default#OgreSubMesh2.cpp-988
    if ( XMVectorGetW( q ) < 0.0f ) {
        q = XMVectorNegate( q );
    }

    // TODO: Should be in use for packing it into ints
    //       because the "-" sign will be lost.
    const float bias = 1.0f / 32767.0f; // ( pow( 2.0, 15.0 ) - 1.0 );
    if ( XMVectorGetW( q ) < bias ) {
        const float normFactor = sqrtf( 1.0f - bias * bias );

        XMStoreFloat4( &qd, q );
        qd.w = normFactor;
        qd.x *= normFactor;
        qd.y *= normFactor;
        qd.z *= normFactor;

        q = XMLoadFloat4( &qd );
    }

    if ( reflection < 0.0 ) {
        q = XMVectorNegate( q );
    }

    XMStoreFloat4( &qd, q );

    apemodefb::QuatFb qq;
    qq.mutate_nx( qd.x );
    qq.mutate_ny( qd.y );
    qq.mutate_nz( qd.z );
    qq.mutate_s( qd.w );

    return qq;
}

template < typename TVertex >
void TConvertVertices( DecompressedMeshInfo& decompressedMeshInfo,
                       const size_t          vertexCount,
                       const size_t          decompressedStride,
                       const size_t          renderableStride );

template <>
void TConvertVertices< apemodefb::DecompressedVertexFb >( DecompressedMeshInfo& decompressedMeshInfo,
                                                          const size_t          vertexCount,
                                                          const size_t          decompressedStride,
                                                          const size_t          renderableStride ) {
    const auto decompressedData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    auto renderableData = decompressedMeshInfo.RenderableVertexBuffer.data( );

    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto srcVertex = reinterpret_cast< const apemodefb::DecompressedVertexFb* >( decompressedData + i * decompressedStride );
        auto dstVertex = reinterpret_cast< apemodefb::DefaultVertexFb* >( renderableData + i * renderableStride );

        union {
            struct {
                uint8_t reflection : 4;
                uint8_t index : 4;
            };

            uint8_t reflection_index;
        } reflection_index_unpacker;

        reflection_index_unpacker.reflection_index = uint8_t( srcVertex->reflection_index_packed( ) );

        union {
            struct {
                uint8_t i;
                uint8_t r;
                uint8_t g;
                uint8_t b;
            };

            uint32_t irgb;
        } irgb_packer;

        irgb_packer.i = reflection_index_unpacker.index;
        irgb_packer.r = uint8_t( srcVertex->color( ).x( ) * 255 );
        irgb_packer.g = uint8_t( srcVertex->color( ).y( ) * 255 );
        irgb_packer.b = uint8_t( srcVertex->color( ).z( ) * 255 );

        const float             reflection = reflection_index_unpacker.reflection_index ? 1 : -1;
        const apemodefb::QuatFb qtangent   = GetQTangent( srcVertex->normal( ), srcVertex->tangent( ), reflection );

        dstVertex->mutable_position( ) = srcVertex->position( );
        dstVertex->mutable_uv( )       = srcVertex->uv( );
        dstVertex->mutable_qtangent( ) = qtangent;
        dstVertex->mutate_index_color_RGB( irgb_packer.irgb );
        dstVertex->mutate_color_alpha( srcVertex->color( ).w( ) );
    }
}

template <>
void TConvertVertices< apemodefb::DecompressedSkinnedVertexFb >( DecompressedMeshInfo& decompressedMeshInfo,
                                                                 const size_t          vertexCount,
                                                                 const size_t          decompressedStride,
                                                                 const size_t          renderableStride ) {
    TConvertVertices< apemodefb::DecompressedVertexFb >(
        decompressedMeshInfo, vertexCount, decompressedStride, renderableStride );

    auto renderableData   = decompressedMeshInfo.RenderableVertexBuffer.data( );
    const auto decompressedData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto srcVertex = reinterpret_cast< const apemodefb::DecompressedSkinnedVertexFb* >( decompressedData + i * decompressedStride );
        auto dstVertex = reinterpret_cast< apemodefb::SkinnedVertexFb* >( renderableData + i * renderableStride );

        auto indices = srcVertex->joint_indices( );
        auto weights = srcVertex->joint_weights( );

        union {
            struct {
                uint8_t _0;
                uint8_t _1;
                uint8_t _2;
                uint8_t _3;
            };

            uint32_t _0123;
        } joint_index_unpacker;
        joint_index_unpacker._0123 = indices;
        
        if (weights.x( ) >= 0.99f) {
            weights.mutate_x( weights.x( ) * 0.5f + float( joint_index_unpacker._0 ) );
            weights.mutate_y( weights.x( ) );
            weights.mutate_z( 0 ); // weights.y( ) + float( joint_index_unpacker._1 ));
            weights.mutate_w( 0 ); // weights.z( ) + float( joint_index_unpacker._2 ) );
        } else {
            weights.mutate_x( weights.x( ) + float( joint_index_unpacker._0 ) );
            weights.mutate_y( weights.y( ) + float( joint_index_unpacker._1 ) );
            weights.mutate_z( weights.z( ) + float( joint_index_unpacker._2 ) );
            weights.mutate_w( weights.w( ) + float( joint_index_unpacker._3 ) );
        }

        dstVertex->mutable_joint_indices_weights( ) = weights;
    }
}

template <>
void TConvertVertices< apemodefb::DecompressedFatSkinnedVertexFb >( DecompressedMeshInfo& decompressedMeshInfo,
                                                                    const size_t          vertexCount,
                                                                    const size_t          decompressedStride,
                                                                    const size_t          renderableStride ) {
    TConvertVertices< apemodefb::DecompressedSkinnedVertexFb >(
        decompressedMeshInfo, vertexCount, decompressedStride, renderableStride );

    const auto decompressedData = decompressedMeshInfo.DecompressedVertexBuffer.data( );
    auto renderableData = decompressedMeshInfo.RenderableVertexBuffer.data( );

    for ( uint32_t i = 0; i < vertexCount; ++i ) {
        auto srcVertex = reinterpret_cast< const apemodefb::DecompressedFatSkinnedVertexFb* >( decompressedData + i * decompressedStride );
        auto dstVertex = reinterpret_cast< apemodefb::FatSkinnedVertexFb* >( renderableData + i * renderableStride );

        auto indices = srcVertex->extra_joint_indices( );
        auto weights = srcVertex->extra_joint_weights( );

        union {
            struct {
                uint8_t _0;
                uint8_t _1;
                uint8_t _2;
                uint8_t _3;
            };

            uint32_t _0123;
        } joint_index_unpacker;
        joint_index_unpacker._0123 = indices;

        weights.mutate_x( weights.x( ) + float( joint_index_unpacker._0 ) );
        weights.mutate_y( weights.y( ) + float( joint_index_unpacker._1 ) );
        weights.mutate_z( weights.z( ) + float( joint_index_unpacker._2 ) );
        weights.mutate_w( weights.w( ) + float( joint_index_unpacker._3 ) );

        dstVertex->mutable_extra_joint_indices_weights( ) = weights;
    }
}

DecompressedMeshInfo DecompressMesh( apemode::SceneMesh& mesh, const SourceSubmeshInfo& srcSubmesh ) {
    using namespace draco;
    using namespace apemodefb;
    
    if ( !srcSubmesh.IsCompressedMesh( ) ) {
        assert( false && "The mesh buffers are uncompressed." );
        return {};
    }
    
    const ECompressionTypeFb eCompression = srcSubmesh.GetCompressionType( );
    switch ( eCompression ) {
    case apemodefb::ECompressionTypeFb_GoogleDraco3D:
        break;
    default:
        assert( false && "Unsupported compression type." );
        return {};
    }

    const EVertexFormatFb eVertexFormat = srcSubmesh.GetVertexFormat( );
    switch ( eVertexFormat ) {
    case apemodefb::EVertexFormatFb_Decompressed:
    case apemodefb::EVertexFormatFb_DecompressedSkinned:
    case apemodefb::EVertexFormatFb_DecompressedFatSkinned:
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

    const Status decoderStatus = decoder.DecodeBufferToGeometry( &decoderBuffer, &decodedMesh );
    if ( Status::OK != decoderStatus.code( ) ) {
        assert( false );
        return {};
    }

    DecompressedMeshInfo decompressedMeshInfo{};

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

    size_t decompressedStride = 0;
    size_t renderableStride   = 0;

    switch ( eVertexFormat ) {
        case apemodefb::EVertexFormatFb_Decompressed:
            decompressedStride = sizeof( apemodefb::DecompressedVertexFb );
            renderableStride   = sizeof( apemodefb::DefaultVertexFb );
            break;
        case apemodefb::EVertexFormatFb_DecompressedSkinned:
            decompressedStride = sizeof( apemodefb::DecompressedSkinnedVertexFb );
            renderableStride   = sizeof( apemodefb::SkinnedVertexFb );
            break;
        case apemodefb::EVertexFormatFb_DecompressedFatSkinned:
            decompressedStride = sizeof( apemodefb::DecompressedFatSkinnedVertexFb );
            renderableStride   = sizeof( apemodefb::FatSkinnedVertexFb );
            break;
        default:
            assert( false && "Unsupported vertex type." );
            return {};
    }

    assert( decompressedStride && renderableStride && "Unassigned vertex stride value." );
    decompressedMeshInfo.DecompressedVertexBuffer.resize( vertexCount * decompressedStride );
    decompressedMeshInfo.RenderableVertexBuffer.resize( vertexCount * renderableStride );
    decompressedMeshInfo.VertexCount = vertexCount;
    decompressedMeshInfo.IndexCount  = indexCount;

    switch ( eVertexFormat ) {
        case apemodefb::EVertexFormatFb_Decompressed:
            TDecompressVertices< apemodefb::DecompressedVertexFb >(
                decodedMesh, decompressedMeshInfo, vertexCount, decompressedStride );
            TConvertVertices< apemodefb::DecompressedVertexFb >(
                decompressedMeshInfo, vertexCount, decompressedStride, renderableStride );
            break;
        case apemodefb::EVertexFormatFb_DecompressedSkinned:
            TDecompressVertices< apemodefb::DecompressedSkinnedVertexFb >(
                decodedMesh, decompressedMeshInfo, vertexCount, decompressedStride );
            TConvertVertices< apemodefb::DecompressedSkinnedVertexFb >(
                decompressedMeshInfo, vertexCount, decompressedStride, renderableStride );
            break;
        case apemodefb::EVertexFormatFb_DecompressedFatSkinned:
            TDecompressVertices< apemodefb::DecompressedFatSkinnedVertexFb >(
                decodedMesh, decompressedMeshInfo, vertexCount, decompressedStride );
            TConvertVertices< apemodefb::DecompressedFatSkinnedVertexFb >(
                decompressedMeshInfo, vertexCount, decompressedStride, renderableStride );
            break;
        default:
            assert( false && "Unsupported vertex type." );
            return {};
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-start;
    apemode::LogInfo( "Decoding done: {} vertices, {} indices, {} seconds",
                       vertexCount, indexCount, diff.count() );

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
        assert( !decompressedMeshInfo.RenderableVertexBuffer.empty( ) );
        
        if ( decompressedMeshInfo.DecompressedVertexBuffer.empty( ) ||
             decompressedMeshInfo.DecompressedIndexBuffer.empty( ) ||
             decompressedMeshInfo.RenderableVertexBuffer.empty( ) ) {
            assert( false );
            return {};
        }

        initializedMeshInfo.VertexUploadInfo.pSrcBufferData = decompressedMeshInfo.RenderableVertexBuffer.data( );
        initializedMeshInfo.VertexUploadInfo.SrcBufferSize  = decompressedMeshInfo.RenderableVertexBuffer.size( );
        initializedMeshInfo.VertexCount                     = decompressedMeshInfo.VertexCount;

        initializedMeshInfo.IndexUploadInfo.pSrcBufferData = decompressedMeshInfo.DecompressedIndexBuffer.data( );
        initializedMeshInfo.IndexUploadInfo.SrcBufferSize  = decompressedMeshInfo.DecompressedIndexBuffer.size( );
        initializedMeshInfo.IndexCount                     = decompressedMeshInfo.IndexCount;
        initializedMeshInfo.eIndexType                     = decompressedMeshInfo.eIndexType;

        initializedMeshInfo.OptionalDecompressedMesh.emplace( eastl::move( decompressedMeshInfo ) );
        
        pScene->Subsets[ mesh.BaseSubset ].IndexCount = (uint32_t)initializedMeshInfo.IndexCount;
        assert( mesh.SubsetCount == 1 );
        
        switch ( srcSubmesh.GetVertexFormat( ) ) {
        case apemodefb::EVertexFormatFb_Decompressed:
            mesh.eVertexType = apemode::detail::eVertexType_Default;
            break;
        case apemodefb::EVertexFormatFb_DecompressedSkinned:
            mesh.eVertexType = apemode::detail::eVertexType_Skinned;
            break;
        case apemodefb::EVertexFormatFb_DecompressedFatSkinned:
            mesh.eVertexType = apemode::detail::eVertexType_FatSkinned;
            break;
        default:
            assert( false && "Unsupported vertex type." );
            return {};
        }
        
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
        
        switch ( srcSubmesh.GetVertexFormat( ) ) {
        case apemodefb::EVertexFormatFb_Default:
            mesh.eVertexType = apemode::detail::eVertexType_Default;
            break;
        case apemodefb::EVertexFormatFb_Skinned:
            mesh.eVertexType = apemode::detail::eVertexType_Skinned;
            break;
        case apemodefb::EVertexFormatFb_FatSkinned:
            mesh.eVertexType = apemode::detail::eVertexType_FatSkinned;
            break;
        default:
            assert( false && "Unsupported vertex type." );
            return {};
        }
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

    for ( auto& mesh : pScene->Meshes ) {
        InitializedMeshInfo initializedMeshInfo = InitializeMesh( mesh, pScene, pParams );
        if ( initializedMeshInfo.IsOk( ) ) {
            auto& emplacedInitializedMeshInfo = initializedMeshInfos.emplace_back( eastl::move( initializedMeshInfo ) );
            bufferUploads.push_back( emplacedInitializedMeshInfo.VertexUploadInfo );
            bufferUploads.push_back( emplacedInitializedMeshInfo.IndexUploadInfo );
            totalBytesRequired += emplacedInitializedMeshInfo.VertexUploadInfo.SrcBufferSize;
            totalBytesRequired += emplacedInitializedMeshInfo.IndexUploadInfo.SrcBufferSize;
        }
    }

    { /* Sort by size in descending order. */
        BufferUploadInfo* pMeshUploadIt     = bufferUploads.data( );
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
        assert(false);
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
        assert(false);
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
    
    if ( pMaterialAsset->pMetallicRoughnessOcclusionImg ) {
        const float maxLod = float( pMaterialAsset->pMetallicRoughnessOcclusionImg->ImgCreateInfo.mipLevels );
        pMaterialAsset->pMetallicRoughnessOcclusionSampler = pParams->pSamplerManager->GetSampler( GetDefaultSamplerCreateInfo( maxLod ) );

        VkImageViewCreateInfo metallicRoughnessOcclusionImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessOcclusionImg->ImgViewCreateInfo;
        metallicRoughnessOcclusionImgViewCreateInfo.image                 = pMaterialAsset->pMetallicRoughnessOcclusionImg->hImg;
        
        if ( !pMaterialAsset->hMetallicRoughnessOcclusionImgView.Recreate( pParams->pNode->hLogicalDevice, metallicRoughnessOcclusionImgViewCreateInfo ) ) {
            assert(false);
            return false;
        }
    }

    return true;
}

bool UploadMaterials( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    using namespace eastl;

    if ( !InitializeMaterials( pScene, pParams ) ) {
        return false;
    }

    
    // auto pTaskflow =  apemode::AppState::Get( )->GetDefaultTaskflow( );
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
