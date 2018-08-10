#include <SceneUploaderVk.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <Buffer.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <TOneTimeCmdBufferSubmit.Vulkan.h>

#include <AppState.h>

#include <Scene.h>
#include <MathInc.h>
#include <ArrayUtils.h>
#include <cstdlib>

namespace {

/* Uploads resources from CPU to GPU with respect to staging memory limit. */
struct MeshBufferFillInfo {
    VkBuffer       pDstBuffer      = VK_NULL_HANDLE;
    const uint8_t* pSrcBufferData  = nullptr;
    uint32_t       SrcBufferSize   = 0;
    VkAccessFlags  eDstAccessFlags = 0;
};

struct MeshBufferFillInfoCmpGreaterBySize {
    int operator( )( const MeshBufferFillInfo& a, const MeshBufferFillInfo& b ) const {
        if ( a.SrcBufferSize > b.SrcBufferSize )
            return ( -1 );
        if ( a.SrcBufferSize < b.SrcBufferSize )
            return ( +1 );
        return 0;
    }
};

// Something to consider adding: An eastl sort which uses qsort underneath.
// The primary purpose of this is to have an eastl interface for sorting which
// results in very little code generation, since all instances map to the
// C qsort function.

template < typename TValue, typename TCmpOp >
int QSortCmpOp( const void* a, const void* b ) {
    return TCmpOp( )( *(const TValue*) a, *(const TValue*) b );
}

template < typename TValue, typename TCmpOp >
void QSort( TValue* pFirst, TValue* pLast ) {
    qsort( pFirst, (size_t) eastl::distance( pFirst, pLast ), sizeof( TValue ), QSortCmpOp< TValue, TCmpOp > );
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


const apemodevk::UploadedImage** GetLoadedImageSlotForPropertyName( apemode::vk::SceneUploader::MaterialDeviceAsset* pMaterialAsset,
                                                                  const char* pszTexturePropName ) {
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

bool GenerateMipMapsForPropertyName(  const char* pszTexturePropName ) {

    if ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) {
        return true;
    } else if ( strcmp( "diffuseTexture", pszTexturePropName ) == 0 ) {
        return true;
    } else if ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) {
        return true;
    } else if ( strcmp( "occlusionTexture", pszTexturePropName ) == 0 ) {
        return false;
    } else if ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) {
        return false;
    } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
        return false;
    } else if ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) {
        return true;
    }

    return false;
}

void AddLoadedImage( apemode::vk::SceneUploader::DeviceAsset*        pAsset,
                     uint32_t                                        fileId,
                     apemodevk::unique_ptr< apemodevk::UploadedImage > loadedImg ) {
    pAsset->LoadedImgs.push_back( eastl::make_pair( fileId, std::move( loadedImg ) ) );
}

const apemodevk::UploadedImage* FindLoadedImage( apemode::vk::SceneUploader::DeviceAsset* pAsset, uint32_t fileId ) {
    auto loadedImageIt = eastl::find_if( pAsset->LoadedImgs.begin( ),
                                         pAsset->LoadedImgs.end( ),
                                         [&]( apemode::vk::SceneUploader::DeviceAsset::LoadedImagePtrPair& loadedImgPair ) {
                                             return loadedImgPair.first == fileId;
                                         } );

    if ( loadedImageIt != pAsset->LoadedImgs.end( ) ) {
        return loadedImageIt->second.get();
    }

    return nullptr;
}

} // namespace

bool InitializeMesh( apemode::SceneMesh&                                 mesh,
                     const apemode::Scene*                               pScene,
                     const apemode::vk::SceneUploader::UploadParameters* pParams ) {

    assert( pParams && pParams->pSrcScene );
    auto pMeshesFb = pParams->pSrcScene->meshes( );

    /* Get source mesh object. */

    assert( pMeshesFb && pMeshesFb->size( ) > mesh.Id );
    auto pSrcMesh = pMeshesFb->Get( mesh.Id );
    assert( pSrcMesh );

    /* Get or create mesh device asset. */

    auto pMeshAsset = static_cast< apemode::vk::SceneUploader::MeshDeviceAsset* >( mesh.pDeviceAsset.get( ) );
    if ( nullptr == pMeshAsset ) {
        pMeshAsset = apemode_new apemode::vk::SceneUploader::MeshDeviceAsset( );
        mesh.pDeviceAsset.reset( pMeshAsset );
    }

    /* Assign vertex count. */

    /* Currently, there is only one submesh object supported. */
    assert( pSrcMesh->submeshes( ) && ( pSrcMesh->submeshes( )->size( ) == 1 ) );
    auto pSubmesh = pSrcMesh->submeshes( )->Get( 0 );
    pMeshAsset->VertexCount = pSubmesh->vertex_count( );

    /* Assign index type. Currently, there are only two types supported: UInt16, UInt32. */

    if ( pSrcMesh->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ) {
        pMeshAsset->IndexType = VK_INDEX_TYPE_UINT32;
    }

    /* Create vertex buffer. */

    VkBufferCreateInfo bufferCreateInfo;
    VmaAllocationCreateInfo allocationCreateInfo;

    apemodevk::InitializeStruct( bufferCreateInfo );
    apemodevk::InitializeStruct( allocationCreateInfo );

    bufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.size      = pSrcMesh->vertices( )->size( );
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if ( false == pMeshAsset->hVertexBuffer.Recreate( pParams->pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    /* Create index buffer. */

    bufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferCreateInfo.size      = pSrcMesh->indices( )->size( );
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if ( false == pMeshAsset->hIndexBuffer.Recreate( pParams->pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    return true;
}

bool UploadMeshes( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    using namespace apemodevk;
    using namespace eastl;

    assert( pScene && pParams && pParams->pSrcScene && pParams->pNode );

    auto pNode = pParams->pNode;

    auto pMeshesFb = pParams->pSrcScene->meshes( );
    assert ( pMeshesFb && pMeshesFb->size( ) );

    uint64_t totalBytesRequired = 0;

    apemode::vector< MeshBufferFillInfo > bufferFillInfos;
    bufferFillInfos.reserve( pMeshesFb->size( ) << 1 );

    for ( auto & mesh : pScene->Meshes ) {
        if ( !InitializeMesh( mesh, pScene, pParams ) ) {
            return false;
        }

        /* Get source mesh object. */

        auto pSrcMesh = pMeshesFb->Get( mesh.Id );
        assert( pSrcMesh );

        /* Get or create mesh device asset. */

        auto pMeshAsset = static_cast< apemode::vk::SceneUploader::MeshDeviceAsset* >( mesh.pDeviceAsset.get( ) );
        assert( pMeshAsset );

        MeshBufferFillInfo bufferFillInfo;

        bufferFillInfo.pDstBuffer      = pMeshAsset->hVertexBuffer.Handle.pBuffer;
        bufferFillInfo.pSrcBufferData  = pSrcMesh->vertices( )->data( );
        bufferFillInfo.SrcBufferSize   = pSrcMesh->vertices( )->size( );
        bufferFillInfo.eDstAccessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

        bufferFillInfos.push_back( bufferFillInfo );
        totalBytesRequired += pSrcMesh->vertices( )->size( );

        bufferFillInfo.pDstBuffer      = pMeshAsset->hIndexBuffer.Handle.pBuffer;
        bufferFillInfo.pSrcBufferData  = pSrcMesh->indices( )->data( );
        bufferFillInfo.SrcBufferSize   = pSrcMesh->indices( )->size( );
        bufferFillInfo.eDstAccessFlags = VK_ACCESS_INDEX_READ_BIT;

        bufferFillInfos.push_back( bufferFillInfo );
        totalBytesRequired += pSrcMesh->indices( )->size( );
    }

    { /* Sort by size in descending order. */

        MeshBufferFillInfo* pFillInfo     = bufferFillInfos.data( );
        MeshBufferFillInfo* pFillInfoLast = pFillInfo + ( bufferFillInfos.size( ) - 1 );
        QSort< MeshBufferFillInfo, MeshBufferFillInfoCmpGreaterBySize >( pFillInfo, pFillInfoLast );
    }

    const MeshBufferFillInfo* const pFillInfo    = bufferFillInfos.data( );
    const MeshBufferFillInfo* const pFillInfoEnd = pFillInfo + bufferFillInfos.size( );

    /* Pipeline Barrier: TRANSFER -> VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT or VK_ACCESS_INDEX_READ_BIT. */
    apemodevk::TOneTimeCmdBufferSubmit( pNode, 0, true, [&]( VkCommandBuffer pCmdBuffer ) {

        apemode::vector< VkBufferMemoryBarrier > bufferBarriers;
        bufferBarriers.reserve( bufferFillInfos.size( ) );

        /* Initialize buffer barriers. */
        transform( bufferFillInfos.begin( ),
                   bufferFillInfos.end( ),
                   back_inserter( bufferBarriers ),
                   []( const MeshBufferFillInfo & currFillInfo ) {
                       VkBufferMemoryBarrier bufferMemoryBarrier;
                       apemodevk::InitializeStruct( bufferMemoryBarrier );

                       bufferMemoryBarrier.size                = VK_WHOLE_SIZE;
                       bufferMemoryBarrier.buffer              = currFillInfo.pDstBuffer;
                       bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                       bufferMemoryBarrier.dstAccessMask       = currFillInfo.eDstAccessFlags;
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

    size_t const contiguousBufferSize = max< size_t >( pParams->StagingMemoryLimitHint, pFillInfo->SrcBufferSize );
    size_t const stagingMemorySizeNeeded = max< size_t >( pNode->AdapterProps.limits.maxUniformBufferRange, contiguousBufferSize );
    size_t const stagingMemorySize = min< size_t >( totalBytesRequired, stagingMemorySizeNeeded );

    /* The purpose is to get some control over the memory allocation from CPU heap
     * and make it efficient, manageable and easy to understand.
     * The idea is to allocated some reasonable amount of staging memory and reuse it.
     */
    THandle< BufferComposite > hStagingBuffer;

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.size  = stagingMemorySize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocationCreateInfo;
    InitializeStruct( stagingAllocationCreateInfo );
    stagingAllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if ( false == hStagingBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, stagingAllocationCreateInfo ) ) {
        return nullptr;
    }

    uint8_t* pMappedStagingMemory = MapStagingBuffer( pNode, hStagingBuffer );
    if ( nullptr == pMappedStagingMemory ) {
        return nullptr;
    }

    /* A limit that the code does not reach. */
    const uint64_t totalBytesAllowed = stagingMemorySize;

    /* Tracks the amount of allocated host memory (approx). */
    uint64_t totalBytesAllocated = 0;

    /* While there are elements that need to be uploaded. */
    auto pCurrFillInfo = pFillInfo;
    while ( pCurrFillInfo != pFillInfoEnd ) {

        uint8_t* pMappedStagingMemoryHead = pMappedStagingMemory;
        uint64_t stagingMemorySpaceLeft   = stagingMemorySize;

        /* While there are elements that need to be uploaded. */
        while ( pCurrFillInfo != pFillInfoEnd ) {
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

bool UploadMaterials( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {

    MT::TaskScheduler* pTaskScheduler = apemode::AppState::Get( )->GetTaskScheduler( );

    auto pMaterialsFb = pParams->pSrcScene->materials( );
    auto pTexturesFb  = pParams->pSrcScene->textures( );
    auto pFilesFb     = pParams->pSrcScene->files( );

    if ( pMaterialsFb && pTexturesFb && pFilesFb ) {

        /* Create material device asset if needed. */
        auto pSceneAsset = static_cast< apemode::vk::SceneUploader::DeviceAsset* >( pScene->pDeviceAsset.get( ) );
        if ( nullptr == pSceneAsset ) {
            pSceneAsset = apemode_new apemode::vk::SceneUploader::DeviceAsset( );
            pScene->pDeviceAsset.reset( pSceneAsset );
            assert( pSceneAsset );
        }

        apemodevk::ImageUploader imgUploader;
        apemodevk::ImageDecoder  imgDecoder;

        {
            uint8_t imageBytes[ 4 ];

            apemodevk::ImageUploader::LoadOptions    loadOptions;
            apemodevk::ImageDecoder::DecodeOptions decodeOptions;
            decodeOptions.bGenerateMipMaps = false;

            imageBytes[ 0 ] = 0;
            imageBytes[ 1 ] = 0;
            imageBytes[ 2 ] = 0;
            imageBytes[ 3 ] = 255;

            auto srcImgZeros = imgDecoder.CreateSourceImage2D( imageBytes, { 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM, decodeOptions );
            pSceneAsset->MissingTextureZeros = imgUploader.UploadImage( pParams->pNode, *srcImgZeros, loadOptions );

            imageBytes[ 0 ] = 255;
            imageBytes[ 1 ] = 255;
            imageBytes[ 2 ] = 255;
            imageBytes[ 3 ] = 255;

            auto srcImgOnes = imgDecoder.CreateSourceImage2D( imageBytes, {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, decodeOptions );
            pSceneAsset->MissingTextureOnes = imgUploader.UploadImage( pParams->pNode, *srcImgOnes, loadOptions );

            const uint32_t missingSamplerIndex = pParams->pSamplerManager->GetSamplerIndex( GetDefaultSamplerCreateInfo( 0 ) );
            assert( apemodevk::SamplerManager::IsSamplerIndexValid( missingSamplerIndex ) );
            pSceneAsset->pMissingSampler = pParams->pSamplerManager->StoredSamplers[ missingSamplerIndex ].pSampler;
        }

        for ( auto& material : pScene->Materials ) {

            /* Create material device asset if needed. */
            auto pMaterialAsset = static_cast< apemode::vk::SceneUploader::MaterialDeviceAsset* >( material.pDeviceAsset.get( ) );
            if ( nullptr == pMaterialAsset ) {
                pMaterialAsset = apemode_new apemode::vk::SceneUploader::MaterialDeviceAsset( );
                material.pDeviceAsset.reset( pMaterialAsset );
            }

            auto pMaterialFb          = pMaterialsFb->Get( material.Id );
            auto pTexturePropertiesFb = pMaterialFb->texture_properties( );

            assert( pMaterialFb );
            assert( pTexturePropertiesFb );

            auto pszMaterialName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pMaterialFb->name_id( ) );
            pMaterialAsset->pszName = pszMaterialName;

            apemode::LogError( "Loading textures for material \"{}\"", pszMaterialName );

            for ( auto pTexturePropFb : *pTexturePropertiesFb ) {

                auto pszTexturePropName      = apemode::utils::GetCStringProperty( pParams->pSrcScene, pTexturePropFb->name_id( ) );
                auto ppLoadedImgMaterialSlot = GetLoadedImageSlotForPropertyName( pMaterialAsset, pszTexturePropName );

                if ( nullptr == ppLoadedImgMaterialSlot ) {

                    apemode::LogError( "Cannot map texture property: \"{}\"", pszTexturePropName );

                } else {

                    auto pTextureFb = pTexturesFb->Get( pTexturePropFb->value_id( ) );
                    assert( pTextureFb );

                    auto pFileFb = pFilesFb->Get( pTextureFb->file_id( ) );
                    assert( pFileFb );

                    auto pszFileName    = apemode::utils::GetCStringProperty( pParams->pSrcScene, pFileFb->name_id( ) );
                    auto pszTextureName = apemode::utils::GetCStringProperty( pParams->pSrcScene, pTextureFb->name_id( ) );

                    assert( pszFileName );
                    assert( pszTextureName );

                    if ( auto pLoadedImg = FindLoadedImage( pSceneAsset, pFileFb->id( ) ) ) {

                        apemode::LogInfo( "Assigning loaded texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );
                        ( *ppLoadedImgMaterialSlot ) = pLoadedImg;

                    } else {

                        apemode::LogInfo( "Loading texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );

                        apemodevk::ImageDecoder::DecodeOptions decodeOptions;
                        decodeOptions.eFileFormat = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_PNG;
                        decodeOptions.bGenerateMipMaps = GenerateMipMapsForPropertyName( pszTexturePropName );

                        auto srcImg = imgDecoder.DecodeSourceImageFromData(
                            pFileFb->buffer( )->data( ), pFileFb->buffer( )->size( ), decodeOptions );

                        apemodevk::ImageUploader::LoadOptions loadOptions;
                        auto loadedImg = imgUploader.UploadImage( pParams->pNode, *srcImg, loadOptions );

                        if ( loadedImg ) {

                            ( *ppLoadedImgMaterialSlot ) = loadedImg.get( );
                            AddLoadedImage( pSceneAsset, pFileFb->id( ), std::move( loadedImg ) );
                        }
                    }

                } /* ppLoadedImgMaterialSlot */
            }     /* pTexturePropFb */

            if ( pMaterialAsset->pBaseColorImg ) {
                const float maxLod = float( pMaterialAsset->pBaseColorImg->ImgCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pBaseColorSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pBaseColorImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pBaseColorImg->hImg;

                if ( !pMaterialAsset->hBaseColorImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pBaseColorLoadedImg */

            if ( pMaterialAsset->pNormalImg ) {

                const float maxLod = float( pMaterialAsset->pNormalImg->ImgCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pNormalSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pNormalImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pNormalImg->hImg;

                if ( !pMaterialAsset->hNormalImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pNormalLoadedImg */

            if ( pMaterialAsset->pEmissiveImg ) {

                const float maxLod = float( pMaterialAsset->pEmissiveImg->ImgCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pEmissiveSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pEmissiveImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pEmissiveImg->hImg;
                imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;

                if ( !pMaterialAsset->hEmissiveImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pEmissiveLoadedImg */

            if ( pMaterialAsset->pMetallicRoughnessImg ) {

                const float maxLod = float( pMaterialAsset->pMetallicRoughnessImg->ImgCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pMetallicSampler  = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;
                pMaterialAsset->pRoughnessSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo metallicImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessImg->ImgViewCreateInfo;
                metallicImgViewCreateInfo.image        = pMaterialAsset->pMetallicRoughnessImg->hImg;
                metallicImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
                metallicImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ONE;
                metallicImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ONE;
                metallicImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;

                VkImageViewCreateInfo roughnessImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessImg->ImgViewCreateInfo;
                roughnessImgViewCreateInfo.image        = pMaterialAsset->pMetallicRoughnessImg->hImg;
                roughnessImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_G;
                roughnessImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ONE;
                roughnessImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ONE;
                roughnessImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;

                if ( !pMaterialAsset->hMetallicImgView.Recreate( pParams->pNode->hLogicalDevice, metallicImgViewCreateInfo ) ) {
                    return false;
                }

                if ( !pMaterialAsset->hRoughnessImgView.Recreate( pParams->pNode->hLogicalDevice, roughnessImgViewCreateInfo ) ) {
                    return false;
                }

            } /* pMetallicRoughnessLoadedImg */

            if ( pMaterialAsset->pOcclusionImg ) {

                const float maxLod = float( pMaterialAsset->pOcclusionImg->ImgCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pOcclusionSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo occlusionImgViewCreateInfo = pMaterialAsset->pOcclusionImg->ImgViewCreateInfo;

                occlusionImgViewCreateInfo.image = pMaterialAsset->pOcclusionImg->hImg;

                occlusionImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_B;
                occlusionImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
                occlusionImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
                occlusionImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ZERO;

                #if 0
                if ( pMaterialAsset->pOcclusionLoadedImg == pMaterialAsset->pMetallicRoughnessLoadedImg ) {
                    /* MetallicRoughness texture has 2 components, the third channel can used as occlusion. */
                    occlusionImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_B;
                    occlusionImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ZERO;
                } else if ( pMaterialAsset->pOcclusionLoadedImg == pMaterialAsset->pEmissiveLoadedImg ) {
                    /* Emissive texture has 3 components, the last channel can used as occlusion. */
                    occlusionImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_A;
                    occlusionImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ZERO;
                } else {
                    /* Emissive texture is a separate (dedicated) one. */
                    occlusionImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
                    occlusionImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ZERO;
                    occlusionImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ZERO;
                }
                #endif

                if ( !pMaterialAsset->hOcclusionImgView.Recreate( pParams->pNode->hLogicalDevice, occlusionImgViewCreateInfo ) ) {
                    return false;
                }

            } /* pOcclusionLoadedImg */
        }     /* pMaterialFb */
    }         /* pMaterialsFb */

    return true;
}

bool apemode::vk::SceneUploader::UploadScene( apemode::Scene* pScene, const apemode::vk::SceneUploader::UploadParameters* pParams ) {
    if ( !pParams || !pScene )
        return false;

    auto pMeshesFb = pParams->pSrcScene->meshes( );
    if ( pMeshesFb && pMeshesFb->size( ) ) {
        if ( !UploadMeshes( pScene, pParams ) ) {
            return false;
        }
    }

    auto pMaterialsFb = pParams->pSrcScene->materials( );
    if ( pMaterialsFb && pMaterialsFb->size( ) ) {
        if ( !UploadMaterials( pScene, pParams ) ) {
            return false;
        }
    }

    return true;
}
