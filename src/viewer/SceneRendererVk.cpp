#include <fbxvpch.h>

#include <MathInc.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <Buffer.Vulkan.h>
#include <ImageLoader.Vulkan.h>

#include <SceneRendererVk.h>
#include <Scene.h>
#include <AppState.h>
#include <ArrayUtils.h>

namespace apemodevk {

    using namespace apemodexm;

    struct StaticVertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 tangent;
        XMFLOAT2 texcoords;
    };

    struct PackedVertex {
        uint32_t position;
        uint32_t normal;
        uint32_t tangent;
        uint32_t texcoords;
    };

    struct StaticSkinnedVertex {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 tangent;
        XMFLOAT2 texcoords;
        XMFLOAT4 weights;
        XMFLOAT4 indices;
    };

    struct PackedSkinnedVertex {
        uint32_t position;
        uint32_t normal;
        uint32_t tangent;
        uint32_t weights;
        uint32_t indices;
    };

    struct ObjectUBO {
        XMFLOAT4X4 WorldMatrix;
        XMFLOAT4   PositionOffset;
        XMFLOAT4   PositionScale;
        XMFLOAT4   TexcoordOffsetScale;
    };

    struct CameraUBO {
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjMatrix;
        XMFLOAT4X4 InvViewMatrix;
        XMFLOAT4X4 InvProjMatrix;
    };

    struct MaterialUBO {
        XMFLOAT4 BaseColorFactor;
        XMFLOAT4 EmissiveFactor;
        XMFLOAT4 MetallicRoughnessFactor;
        XMUINT4 Flags;

    };

    struct LightUBO {
        XMFLOAT4 LightDirection;
        XMFLOAT4 LightColor;
    };

    struct SceneMeshDeviceAssetVk : apemode::SceneDeviceAsset {
        THandle< BufferComposite > hVertexBuffer;
        THandle< BufferComposite > hIndexBuffer;
        uint32_t                   VertexCount = 0;
        uint32_t                   IndexOffset = 0;
        VkIndexType                IndexType   = VK_INDEX_TYPE_UINT16;
    };

    struct SceneMaterialDeviceAssetVk : apemode::SceneDeviceAsset {
        const char * pszName = nullptr;

        const LoadedImage* pBaseColorLoadedImg         = nullptr;
        const LoadedImage* pNormalLoadedImg            = nullptr;
        const LoadedImage* pOcclusionLoadedImg         = nullptr;
        const LoadedImage* pEmissiveLoadedImg          = nullptr;
        const LoadedImage* pMetallicRoughnessLoadedImg = nullptr;

        VkSampler pBaseColorSampler = VK_NULL_HANDLE;
        VkSampler pNormalSampler    = VK_NULL_HANDLE;
        VkSampler pOcclusionSampler = VK_NULL_HANDLE;
        VkSampler pEmissiveSampler  = VK_NULL_HANDLE;
        VkSampler pMetallicSampler  = VK_NULL_HANDLE;
        VkSampler pRoughnessSampler = VK_NULL_HANDLE;

        THandle< VkImageView > hBaseColorImgView;
        THandle< VkImageView > hNormalImgView;
        THandle< VkImageView > hOcclusionImgView;
        THandle< VkImageView > hEmissiveImgView;
        THandle< VkImageView > hMetallicImgView;
        THandle< VkImageView > hRoughnessImgView;
    };

    const LoadedImage** GetLoadedImageSlotForPropertyName( SceneMaterialDeviceAssetVk* pMaterialAsset,
                                                           const char*                 pszTexturePropName ) {

        if ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pBaseColorLoadedImg;
        } else if ( strcmp( "diffuseTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pBaseColorLoadedImg;
        } else if ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pNormalLoadedImg;
        } else if ( strcmp( "occlusionTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pOcclusionLoadedImg;
        } else if ( strcmp( "specularGlossinessTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pMetallicRoughnessLoadedImg;
        } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pMetallicRoughnessLoadedImg;
        } else if ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pEmissiveLoadedImg;
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

    struct SceneDeviceAssetVk : apemode::SceneDeviceAsset {
        using LoadedImagePtrPair = std::pair< uint32_t, std::unique_ptr< LoadedImage > >;

        std::vector< LoadedImagePtrPair >     LoadedImgs;
        std::unique_ptr< LoadedImage >        MissingTextureZeros;
        std::unique_ptr< LoadedImage >        MissingTextureOnes;
        VkSampler                             pMissingSampler = VK_NULL_HANDLE;
        apemode::SceneMaterial                MissingMaterial;
        apemodevk::SceneMaterialDeviceAssetVk MissingMaterialAsset;

        SceneDeviceAssetVk( ) {
            InitializeStruct( MissingMaterial );
            InitializeStruct( MissingMaterialAsset );
            MissingMaterial.BaseColorFactor = XMFLOAT4( 1, 0, 1, 1 ); // Magenta
            MissingMaterial.EmissiveFactor  = XMFLOAT3( 1, 0, 1 );    // Magenta
        }

        void AddLoadedImage( uint32_t fileId, std::unique_ptr< LoadedImage > loadedImg ) {
            LoadedImgs.push_back( std::make_pair( fileId, std::move( loadedImg ) ) );
        }

        const LoadedImage* FindLoadedImage( uint32_t fileId ) {
            auto loadedImageIt =
                std::find_if( LoadedImgs.begin( ),
                              LoadedImgs.end( ),
                              [&]( LoadedImagePtrPair & loadedImgPair ) {
                                  return loadedImgPair.first == fileId;
                              } );

            if ( loadedImageIt != LoadedImgs.end( ) ) {
                return loadedImageIt->second.get();
            }

            return nullptr;
        }
    };

    /**
     * Uploads resources from CPU to GPU with respect to staging memory limit.
     */
    struct MeshBufferFillIntent {
        VkBuffer       pDstBuffer      = VK_NULL_HANDLE;
        const uint8_t* pSrcBufferData  = nullptr;
        uint32_t       SrcBufferSize   = 0;
        VkAccessFlags  eDstAccessFlags = 0;
    };

    template < typename TFillCmdBufferFunc >
    bool TSubmitCmdBuffer( GraphicsDevice*     pNode,
                           VkQueueFlags        eQueueFlags,
                           TFillCmdBufferFunc  fillCmdsFunc ) {

        /* Get queue from pool (only copying) */
        auto pQueuePool = pNode->GetQueuePool( );

        auto acquiredQueue = pQueuePool->Acquire( false, eQueueFlags, false );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = pQueuePool->Acquire( false, eQueueFlags, false );
        }

        /* Get command buffer from pool */
        auto pCmdBufferPool = pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, acquiredQueue.QueueFamilyId );

        /* Reset command pool */
        if ( VK_SUCCESS != CheckedCall( vkResetCommandPool( pNode->hLogicalDevice, acquiredCmdBuffer.pCmdPool, 0 ) ) ) {
            return false;
        }

        /* Begin recording commands to command buffer */
        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );

        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if ( VK_SUCCESS != CheckedCall( vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &commandBufferBeginInfo ) ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        fillCmdsFunc( acquiredCmdBuffer.pCmdBuffer );

        vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

        if ( VK_SUCCESS != WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence ) ) {
            return false;
        }

        VkSubmitInfo submitInfo;
        InitializeStruct( submitInfo );

        submitInfo.pCommandBuffers = &acquiredCmdBuffer.pCmdBuffer;
        submitInfo.commandBufferCount = 1;

        vkResetFences( pNode->hLogicalDevice, 1, &acquiredQueue.pFence );
        vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );

        if ( VK_SUCCESS != WaitForFence( pNode->hLogicalDevice, acquiredQueue.pFence ) ) {
            return false;
        }

        acquiredCmdBuffer.pFence = acquiredQueue.pFence;
        pCmdBufferPool->Release( acquiredCmdBuffer );
        pQueuePool->Release( acquiredQueue );

        return true;
    }

    VkSamplerCreateInfo GetDefaultSamplerCreateInfo( const float maxLod ) {
        VkSamplerCreateInfo samplerCreateInfo;
        InitializeStruct( samplerCreateInfo );
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

    bool FillCombinedImgSamplerBinding( apemodevk::DescriptorSetBase::Binding* pBinding,
                                        VkImageView                            pImgView,
                                        VkSampler                              pSampler,
                                        VkImageLayout                          eImgLayout,
                                        VkImageView                            pMissingImgView,
                                        VkSampler                              pMissingSampler ) {
        if ( pBinding ) {
            if ( pImgView && pSampler ) {
                pBinding->eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                pBinding->ImageInfo.imageLayout = eImgLayout;
                pBinding->ImageInfo.imageView   = pImgView;
                pBinding->ImageInfo.sampler     = pSampler;
                return true;
            } else if ( pMissingImgView && pMissingSampler ) {
                pBinding->eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                pBinding->ImageInfo.imageLayout = eImgLayout;
                pBinding->ImageInfo.imageView   = pMissingImgView;
                pBinding->ImageInfo.sampler     = pMissingSampler;
                return true;
            }
        }

        return false;
    }
}

bool apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParamsBase ) {
    if ( nullptr == pScene ||  nullptr == pParamsBase ) {
        return false;
    }

    bool bDeviceChanged = false;
    auto pParams = (SceneUpdateParametersVk*) pParamsBase;

    if ( pNode != pParams->pNode ) {
        pNode = pParams->pNode;
        bDeviceChanged |= true;
    }

    auto pMeshesFb = pParamsBase->pSrcScene->meshes( );
    if ( bDeviceChanged && pMeshesFb ) {

        std::vector< apemodevk::MeshBufferFillIntent > bufferFillIntents;
        bufferFillIntents.reserve( pMeshesFb->size( ) << 1 );

        uint64_t totalBytesRequired = 0;

        for ( auto & mesh : pScene->Meshes ) {
            auto pSrcMesh = FlatbuffersTVectorGetAtIndex( pParamsBase->pSrcScene->meshes( ), mesh.SrcId );
            assert( pSrcMesh );

            /* Create mesh device asset if needed. */
            auto pMeshAsset = static_cast< apemodevk::SceneMeshDeviceAssetVk* >( mesh.pDeviceAsset.get( ) );
            if ( nullptr == pMeshAsset ) {
                pMeshAsset = apemode_new apemodevk::SceneMeshDeviceAssetVk( );
                mesh.pDeviceAsset.reset( pMeshAsset );
            }

            auto pSubmesh = pSrcMesh->submeshes( )->begin( );
            pMeshAsset->VertexCount = pSubmesh->vertex_count( );

            if ( pSrcMesh->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ) {
                pMeshAsset->IndexType = VK_INDEX_TYPE_UINT32;
            }

            VkBufferCreateInfo bufferCreateInfo;
            apemodevk::InitializeStruct( bufferCreateInfo );

            VmaAllocationCreateInfo allocationCreateInfo;
            apemodevk::InitializeStruct( allocationCreateInfo );

            bufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferCreateInfo.size      = pSrcMesh->vertices( )->size( );
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if ( false == pMeshAsset->hVertexBuffer.Recreate( pParams->pNode->hAllocator,
                                                              bufferCreateInfo,
                                                              allocationCreateInfo ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }

            bufferCreateInfo.usage     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            bufferCreateInfo.size      = pSrcMesh->indices( )->size( );
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if ( false == pMeshAsset->hIndexBuffer.Recreate( pParams->pNode->hAllocator,
                                                             bufferCreateInfo,
                                                             allocationCreateInfo ) ) {
                apemodevk::platform::DebugBreak( );
                return false;
            }

            apemodevk::MeshBufferFillIntent bufferFillIntent;

            bufferFillIntent.pDstBuffer      = pMeshAsset->hVertexBuffer.Handle.pBuffer;
            bufferFillIntent.pSrcBufferData  = pSrcMesh->vertices( )->data( );
            bufferFillIntent.SrcBufferSize   = pSrcMesh->vertices( )->size( );
            bufferFillIntent.eDstAccessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

            bufferFillIntents.push_back( bufferFillIntent );

            bufferFillIntent.pDstBuffer      = pMeshAsset->hIndexBuffer.Handle.pBuffer;
            bufferFillIntent.pSrcBufferData  = pSrcMesh->indices( )->data( );
            bufferFillIntent.SrcBufferSize   = pSrcMesh->indices( )->size( );
            bufferFillIntent.eDstAccessFlags = VK_ACCESS_INDEX_READ_BIT;

            bufferFillIntents.push_back( bufferFillIntent );

            totalBytesRequired += pSrcMesh->vertices( )->size( );
            totalBytesRequired += pSrcMesh->indices( )->size( );
        }

        // clang-format off
        std::sort( bufferFillIntents.begin( ),
                   bufferFillIntents.end( ),
                   /* Sort in descending order by buffer size. */
                   []( const apemodevk::MeshBufferFillIntent& a,
                       const apemodevk::MeshBufferFillIntent& b ) {
                       return a.SrcBufferSize > b.SrcBufferSize;
                   } );
        // clang-format on

        const apemodevk::MeshBufferFillIntent* const pIntent    = bufferFillIntents.data( );
        const apemodevk::MeshBufferFillIntent* const pIntentEnd = pIntent + bufferFillIntents.size( );

        apemodevk::TSubmitCmdBuffer( pNode, VK_QUEUE_GRAPHICS_BIT, [&]( VkCommandBuffer pCmdBuffer ) {

            /* Stage buffers. */
            if ( auto pCurrIntent = pIntent )
                while ( pCurrIntent != pIntentEnd ) {

                    VkBufferMemoryBarrier bufferMemoryBarrier;
                    apemodevk::InitializeStruct( bufferMemoryBarrier );

                    bufferMemoryBarrier.size                = VK_WHOLE_SIZE;
                    bufferMemoryBarrier.buffer              = pCurrIntent->pDstBuffer;
                    bufferMemoryBarrier.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
                    bufferMemoryBarrier.dstAccessMask       = pCurrIntent->eDstAccessFlags;
                    bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                    vkCmdPipelineBarrier( pCmdBuffer,                         /* Cmd */
                                          VK_PIPELINE_STAGE_TRANSFER_BIT,     /* Src stage */
                                          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, /* Dst stage */
                                          0,                                  /* Dependency flags */
                                          0,                                  /* Memory barrier count */
                                          nullptr,                            /* Memory barriers */
                                          1,                                  /* Buffer barrier count */
                                          &bufferMemoryBarrier,               /* Buffer barriers */
                                          0,                                  /* Img barrier count */
                                          nullptr );                          /* Img barriers */

                    ++pCurrIntent;
                }
        } );

        /* Host storage pool */
        apemodevk::HostBufferPool bufferPool;
        bufferPool.Recreate( pParams->pNode, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false );

#define APEMODE_SCENERENDERERVK_PORTIONED_UPLOADING
#ifdef APEMODE_SCENERENDERERVK_PORTIONED_UPLOADING

        /* This piece of code is a bit complicated, so I added comments.
         * The purpose is to get some control over the memory allocation from CPU heap
         * and make it efficient, manageable and easy to understand.
         * The idea is to allocated some reasonable amount of staging memory in the first iteration,
         * and than reuse it in the next iterations.
         */
        if ( auto pCurrIntent = pIntent ) {

            /* Lets start from 1/4 from total required.
             * Do not allocate less than 64 kb.
             * Do not allocate more than 64 mb.
             */
            uint64_t heuristicTotalBytesAllowed = totalBytesRequired >> 2;
            heuristicTotalBytesAllowed = std::max< uint64_t >( pNode->AdapterProps.limits.maxUniformBufferRange, heuristicTotalBytesAllowed );
            heuristicTotalBytesAllowed = std::min< uint64_t >( 64 * 1024 * 1024, heuristicTotalBytesAllowed );

            /* A limit that the code is trying NOT to reach. */
            const uint64_t totalBytesAllowed = heuristicTotalBytesAllowed;

            /* Tracks the amount of allocated host memory (approx). */
            uint64_t totalBytesAllocated = 0;

            /* While there are elements that need to be uploaded. */
            while ( pCurrIntent != pIntentEnd ) {

                /* Get the queue pool and acquire a queue.
                 * Allocate a command buffer and give it to the lambda that pushes copy commands into it.
                 */
                apemodevk::TSubmitCmdBuffer( pNode, VK_QUEUE_GRAPHICS_BIT, [&]( VkCommandBuffer pCmdBuffer ) {

                    /* While there are elements that need to be uploaded. */
                    while ( pCurrIntent != pIntentEnd ) {

                        /* Allocate some host storage and copy the src buffer data to it. */
                        auto uploadBuffer = bufferPool.Suballocate( pCurrIntent->pSrcBufferData, pCurrIntent->SrcBufferSize );

                        /* A copy command. */
                        VkBufferCopy bufferCopy;
                        apemodevk::InitializeStruct( bufferCopy );

                        bufferCopy.srcOffset = uploadBuffer.DescriptorBufferInfo.offset;
                        bufferCopy.size      = uploadBuffer.DescriptorBufferInfo.range;

                        /* Add the copy command. */
                        vkCmdCopyBuffer( pCmdBuffer,                               /* Cmd */
                                         uploadBuffer.DescriptorBufferInfo.buffer, /* Src */
                                         pCurrIntent->pDstBuffer,                  /* Dst */
                                         1,                                        /* Region count*/
                                         &bufferCopy );                            /* Regions */

                        /* Move to the next item. */
                        ++pCurrIntent;

                        totalBytesAllocated += pCurrIntent->SrcBufferSize;
                        if ( totalBytesAllocated >= totalBytesAllowed ) {

                            /* The limit is reached. Flush and submit. */
                            break;
                        }
                    }  /* while */

                    /* Make memory visible if needed. */
                    bufferPool.Flush( );
                    totalBytesAllocated = 0;

                    /* End command buffer.
                     * Submit and sync.
                     */

                } ); /* TSubmitCmdBuffer */
            }
        }

#else /* ! APEMODE_SCENERENDERERVK_PORTIONED_UPLOADING */

        /* This piece of code is easier to understand.
         * It allocates all staging buffers, fills it with src data and copies to the GPU buffers.
         */
        apemodevk::TSubmitCmdBuffer( pNode, VK_QUEUE_GRAPHICS_BIT, [&]( VkCommandBuffer pCmdBuffer ) {

            /* Fill buffers. */
            if ( auto pCurrIntent = pIntent )
                while ( pCurrIntent != pIntentEnd ) {
                    auto uploadBuffer = bufferPool.Suballocate( pCurrIntent->pSrcBufferData, pCurrIntent->SrcBufferSize );

                    VkBufferCopy bufferCopy;
                    apemodevk::InitializeStruct( bufferCopy );

                    bufferCopy.srcOffset = uploadBuffer.DescriptorBufferInfo.offset;
                    bufferCopy.size      = uploadBuffer.DescriptorBufferInfo.range;

                    vkCmdCopyBuffer( pCmdBuffer,                               /* Cmd */
                                     uploadBuffer.DescriptorBufferInfo.buffer, /* Src */
                                     pCurrIntent->pDstBuffer,                  /* Dst */
                                     1,                                        /* Region count*/
                                     &bufferCopy );                            /* Regions */

                    ++pCurrIntent;
                }

            bufferPool.Flush( );
        } );

#endif /* APEMODE_SCENERENDERERVK_PORTIONED_UPLOADING */

    }

    auto pMaterialsFb = pParamsBase->pSrcScene->materials( );
    auto pTexturesFb  = pParamsBase->pSrcScene->textures( );
    auto pFilesFb     = pParamsBase->pSrcScene->files( );

    if ( bDeviceChanged && pMaterialsFb && pTexturesFb && pFilesFb ) {

        /* Create material device asset if needed. */
        auto pSceneAsset = static_cast< apemodevk::SceneDeviceAssetVk* >( pScene->pDeviceAsset.get( ) );
        if ( nullptr == pSceneAsset ) {
            pSceneAsset = apemode_new apemodevk::SceneDeviceAssetVk( );
            pScene->pDeviceAsset.reset( pSceneAsset );
            assert( pSceneAsset );
        }

        apemodevk::ImageLoader imageLoader;
        if ( false == imageLoader.Recreate( pParams->pNode, nullptr ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        {
            uint8_t imageBytes[ 4 ];
            apemodevk::ImageLoader::LoadOptions loadOptions;

            loadOptions.eFileFormat      = apemodevk::ImageLoader::eImageFileFormat_PNG;
            loadOptions.bAwaitLoading    = true;
            loadOptions.bImgView         = true;
            loadOptions.bGenerateMipMaps = false;

            imageBytes[ 0 ] = 0;
            imageBytes[ 1 ] = 0;
            imageBytes[ 2 ] = 0;
            imageBytes[ 3 ] = 255;

            pSceneAsset->MissingTextureZeros = imageLoader.LoadImageFromRawImgRGBA8( imageBytes, 1, 1, loadOptions );

            imageBytes[ 0 ] = 255;
            imageBytes[ 1 ] = 255;
            imageBytes[ 2 ] = 255;
            imageBytes[ 3 ] = 255;

            pSceneAsset->MissingTextureOnes = imageLoader.LoadImageFromRawImgRGBA8( imageBytes, 1, 1, loadOptions );

            const uint32_t missingSamplerIndex = pParams->pSamplerManager->GetSamplerIndex( apemodevk::GetDefaultSamplerCreateInfo( 0 ) );
            assert( apemodevk::SamplerManager::IsSamplerIndexValid( missingSamplerIndex ) );
            pSceneAsset->pMissingSampler = pParams->pSamplerManager->StoredSamplers[ missingSamplerIndex ].pSampler;
        }

        for ( auto& material : pScene->Materials ) {

            /* Create material device asset if needed. */
            auto pMaterialAsset = static_cast< apemodevk::SceneMaterialDeviceAssetVk* >( material.pDeviceAsset.get( ) );
            if ( nullptr == pMaterialAsset ) {
                pMaterialAsset = apemode_new apemodevk::SceneMaterialDeviceAssetVk( );
                material.pDeviceAsset.reset( pMaterialAsset );
            }

            auto pMaterialFb          = FlatbuffersTVectorGetAtIndex( pMaterialsFb, material.SrcId );
            auto pTexturePropertiesFb = pMaterialFb->texture_properties( );

            assert( pMaterialFb );
            assert( pTexturePropertiesFb );

            auto pszMaterialName = GetCStringProperty( pParamsBase->pSrcScene, pMaterialFb->name_id( ) );
            pMaterialAsset->pszName = pszMaterialName;

            LogError( "Loading textures for material \"{}\"", pszMaterialName );

            for ( auto pTexturePropFb : *pTexturePropertiesFb ) {

                auto pszTexturePropName      = GetCStringProperty( pParamsBase->pSrcScene, pTexturePropFb->name_id( ) );
                auto ppLoadedImgMaterialSlot = GetLoadedImageSlotForPropertyName( pMaterialAsset, pszTexturePropName );

                if ( nullptr == ppLoadedImgMaterialSlot ) {

                    LogError( "Cannot map texture property: \"{}\"", pszTexturePropName );

                } else {

                    auto pTextureFb = FlatbuffersTVectorGetAtIndex( pTexturesFb, pTexturePropFb->value_id( ) );
                    assert( pTextureFb );

                    auto pFileFb = FlatbuffersTVectorGetAtIndex( pFilesFb, pTextureFb->file_id( ) );
                    assert( pFileFb );

                    auto pszFileName    = GetCStringProperty( pParamsBase->pSrcScene, pFileFb->name_id( ) );
                    auto pszTextureName = GetCStringProperty( pParamsBase->pSrcScene, pTextureFb->name_id( ) );

                    assert( pszFileName );
                    assert( pszTextureName );

                    if ( auto pLoadedImg = pSceneAsset->FindLoadedImage( pFileFb->id( ) ) ) {

                        LogInfo( "Assigning loaded texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );
                        ( *ppLoadedImgMaterialSlot ) = pLoadedImg;

                    } else {

                        LogInfo( "Loading texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );

                        apemodevk::ImageLoader::LoadOptions loadOptions;
                        loadOptions.eFileFormat    = apemodevk::ImageLoader::eImageFileFormat_PNG;
                        loadOptions.bAwaitLoading    = true;
                        loadOptions.bImgView         = false;
                        loadOptions.bGenerateMipMaps = apemodevk::GenerateMipMapsForPropertyName( pszTexturePropName );

                        auto loadedImg = imageLoader.LoadImageFromFileData( pFileFb->buffer( )->data( ),
                                                                        pFileFb->buffer( )->size( ),
                                                                        loadOptions );

                        if ( loadedImg ) {

                            ( *ppLoadedImgMaterialSlot ) = loadedImg.get( );
                            pSceneAsset->AddLoadedImage( pFileFb->id( ), std::move( loadedImg ) );

                        }
                    }

                } /* ppLoadedImgMaterialSlot */
            }     /* pTexturePropFb */

            if ( pMaterialAsset->pBaseColorLoadedImg ) {
                const float maxLod = float( pMaterialAsset->pBaseColorLoadedImg->ImageCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = apemodevk::GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pBaseColorSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pBaseColorLoadedImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pBaseColorLoadedImg->hImg;

                if ( !pMaterialAsset->hBaseColorImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pBaseColorLoadedImg */

            if ( pMaterialAsset->pNormalLoadedImg ) {

                const float maxLod = float( pMaterialAsset->pNormalLoadedImg->ImageCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = apemodevk::GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pNormalSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pNormalLoadedImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pNormalLoadedImg->hImg;

                if ( !pMaterialAsset->hNormalImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pNormalLoadedImg */

            if ( pMaterialAsset->pEmissiveLoadedImg ) {

                const float maxLod = float( pMaterialAsset->pEmissiveLoadedImg->ImageCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = apemodevk::GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pEmissiveSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo imgViewCreateInfo = pMaterialAsset->pEmissiveLoadedImg->ImgViewCreateInfo;
                imgViewCreateInfo.image = pMaterialAsset->pEmissiveLoadedImg->hImg;
                imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;

                if ( !pMaterialAsset->hEmissiveImgView.Recreate( pParams->pNode->hLogicalDevice, imgViewCreateInfo ) ) {
                    return false;
                }

            } /* pEmissiveLoadedImg */

            if ( pMaterialAsset->pMetallicRoughnessLoadedImg ) {

                const float maxLod = float( pMaterialAsset->pMetallicRoughnessLoadedImg->ImageCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = apemodevk::GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pMetallicSampler  = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;
                pMaterialAsset->pRoughnessSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo metallicImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessLoadedImg->ImgViewCreateInfo;
                metallicImgViewCreateInfo.image        = pMaterialAsset->pMetallicRoughnessLoadedImg->hImg;
                metallicImgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
                metallicImgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_ONE;
                metallicImgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_ONE;
                metallicImgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_ONE;

                VkImageViewCreateInfo roughnessImgViewCreateInfo = pMaterialAsset->pMetallicRoughnessLoadedImg->ImgViewCreateInfo;
                roughnessImgViewCreateInfo.image        = pMaterialAsset->pMetallicRoughnessLoadedImg->hImg;
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

            if ( pMaterialAsset->pOcclusionLoadedImg ) {

                const float maxLod = float( pMaterialAsset->pOcclusionLoadedImg->ImageCreateInfo.mipLevels );

                VkSamplerCreateInfo samplerCreateInfo = apemodevk::GetDefaultSamplerCreateInfo( maxLod );
                const uint32_t samplerIndex = pParams->pSamplerManager->GetSamplerIndex( samplerCreateInfo );
                assert( apemodevk::SamplerManager::IsSamplerIndexValid( samplerIndex ) );
                pMaterialAsset->pOcclusionSampler = pParams->pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;

                VkImageViewCreateInfo occlusionImgViewCreateInfo = pMaterialAsset->pOcclusionLoadedImg->ImgViewCreateInfo;
                occlusionImgViewCreateInfo.image                 = pMaterialAsset->pOcclusionLoadedImg->hImg;

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

                if ( !pMaterialAsset->hOcclusionImgView.Recreate( pParams->pNode->hLogicalDevice, occlusionImgViewCreateInfo ) ) {
                    return false;
                }

            } /* pOcclusionLoadedImg */
        }     /* pMaterialFb */
    }         /* pMaterialsFb */

    return true;
}

bool apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {
    using namespace apemodevk;

    /* Scene was not provided.
     * Render parameters were not provided. */
    if ( nullptr == pScene || nullptr == pParamsBase ) {
        return false;
    }

    auto pSceneAsset = static_cast< const SceneDeviceAssetVk* >( pScene->pDeviceAsset.get( ) );
    auto pParams     = static_cast< const SceneRenderParametersVk* >( pParamsBase );

    /* Device change was not handled, cannot render the scene. */
    if ( pNode != pParams->pNode ) {
        return false;
    }

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );

    VkViewport viewport;
    InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pParams->Dims.x * pParams->Scale.x;
    viewport.height   = pParams->Dims.y * pParams->Scale.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( pParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = ( uint32_t )( pParams->Dims.x * pParams->Scale.x );
    scissor.extent.height = ( uint32_t )( pParams->Dims.y * pParams->Scale.y );

    vkCmdSetScissor( pParams->pCmdBuffer, 0, 1, &scissor );

    const uint32_t frameIndex = pParams->FrameIndex % kMaxFrameCount;

    VkDescriptorSet ppDescriptorSets[ 2 ] = {nullptr};
    uint32_t pDynamicOffsets[ 4 ] = {0};

    CameraUBO cameraData;
    cameraData.ViewMatrix            = pParams->ViewMatrix;
    cameraData.ProjMatrix            = pParams->ProjMatrix;
    cameraData.InvViewMatrix         = pParams->InvViewMatrix;
    cameraData.InvProjMatrix         = pParams->InvProjMatrix;

    LightUBO lightData;
    lightData.LightDirection = XMFLOAT4( 0, -1, 0, 1 );
    lightData.LightColor     = XMFLOAT4( 1, 0, 0, 1 );

    auto cameraDataUploadBufferRange = BufferPools[ frameIndex ].TSuballocate( cameraData );
    assert( VK_NULL_HANDLE != cameraDataUploadBufferRange.DescriptorBufferInfo.buffer );
    cameraDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( CameraUBO );

    auto lightDataUploadBufferRange = BufferPools[ frameIndex ].TSuballocate( cameraData );
    assert( VK_NULL_HANDLE != lightDataUploadBufferRange.DescriptorBufferInfo.buffer );
    lightDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( LightUBO );

    TDescriptorSet< 4 > descriptorSetForPass;

    descriptorSetForPass.pBinding[ 0 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 0 */
    descriptorSetForPass.pBinding[ 0 ].BufferInfo            = cameraDataUploadBufferRange.DescriptorBufferInfo;

    descriptorSetForPass.pBinding[ 1 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 1 */
    descriptorSetForPass.pBinding[ 1 ].BufferInfo            = lightDataUploadBufferRange.DescriptorBufferInfo;

    descriptorSetForPass.pBinding[ 2 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; /* 2 */
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.imageLayout = pParams->RadianceMap.eImgLayout;
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.imageView   = pParams->RadianceMap.pImgView;
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.sampler     = pParams->RadianceMap.pSampler;

    descriptorSetForPass.pBinding[ 3 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; /* 3 */
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.imageLayout = pParams->IrradianceMap.eImgLayout;
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.imageView   = pParams->IrradianceMap.pImgView;
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.sampler     = pParams->IrradianceMap.pSampler;

    ppDescriptorSets[ kDescriptorSetForPass ] = DescriptorSetPools[ frameIndex ][ kDescriptorSetForPass ].GetDescSet( &descriptorSetForPass );

    pDynamicOffsets[ 0 ] = cameraDataUploadBufferRange.DynamicOffset;
    pDynamicOffsets[ 1 ] = lightDataUploadBufferRange.DynamicOffset;

    for ( auto& node : pScene->Nodes ) {
        if ( node.MeshId >= pScene->Meshes.size( ) )
            continue;

        auto& mesh = pScene->Meshes[ node.MeshId ];

        auto pMeshAsset = (const SceneMeshDeviceAssetVk*) mesh.pDeviceAsset.get( );
        assert( pMeshAsset );

        ObjectUBO objectData;
        objectData.PositionOffset.x      = mesh.PositionOffset.x;
        objectData.PositionOffset.y      = mesh.PositionOffset.y;
        objectData.PositionOffset.z      = mesh.PositionOffset.z;
        objectData.PositionScale.x       = mesh.PositionScale.x;
        objectData.PositionScale.y       = mesh.PositionScale.y;
        objectData.PositionScale.z       = mesh.PositionScale.z;
        objectData.TexcoordOffsetScale.x = mesh.TexcoordOffset.x;
        objectData.TexcoordOffsetScale.y = mesh.TexcoordOffset.y;
        objectData.TexcoordOffsetScale.z = mesh.TexcoordScale.x;
        objectData.TexcoordOffsetScale.w = mesh.TexcoordScale.y;
        XMStoreFloat4x4( &objectData.WorldMatrix, pScene->WorldMatrices[ node.Id ] );

        auto pSubsetIt    = pScene->Subsets.data( ) + mesh.BaseSubset;
        auto pSubsetItEnd = pSubsetIt + mesh.SubsetCount;

        for ( ; pSubsetIt != pSubsetItEnd; ++pSubsetIt ) {

            const SceneMaterial*              pMaterial      = nullptr;
            const SceneMaterialDeviceAssetVk* pMaterialAsset = nullptr;

            if ( pSubsetIt->MaterialId != uint32_t( -1 ) ) {
                assert( pSubsetIt->MaterialId < pScene->Materials.size( ) );
                pMaterial      = &pScene->Materials[ pSubsetIt->MaterialId ];
                pMaterialAsset = static_cast< SceneMaterialDeviceAssetVk const* >( pMaterial->pDeviceAsset.get( ) );
            } else {
                pMaterial        = &pSceneAsset->MissingMaterial;
                pMaterialAsset   = &pSceneAsset->MissingMaterialAsset;
            }

            uint32_t flags = 0;
            flags |= pMaterialAsset->hBaseColorImgView ? 1 << 0 : 0;
            flags |= pMaterialAsset->hNormalImgView ? 1 << 1 : 0;
            flags |= pMaterialAsset->hEmissiveImgView ? 1 << 2 : 0;
            flags |= pMaterialAsset->hMetallicImgView ? 1 << 3 : 0;
            flags |= pMaterialAsset->hRoughnessImgView ? 1 << 4 : 0;
            flags |= pMaterialAsset->hOcclusionImgView ? 1 << 5 : 0;

            MaterialUBO materialData;
            materialData.BaseColorFactor.x         = pMaterial->BaseColorFactor.x;
            materialData.BaseColorFactor.y         = pMaterial->BaseColorFactor.y;
            materialData.BaseColorFactor.z         = pMaterial->BaseColorFactor.z;
            materialData.BaseColorFactor.w         = pMaterial->BaseColorFactor.w;
            materialData.EmissiveFactor.x          = pMaterial->EmissiveFactor.x;
            materialData.EmissiveFactor.y          = pMaterial->EmissiveFactor.y;
            materialData.EmissiveFactor.z          = pMaterial->EmissiveFactor.z;
            materialData.MetallicRoughnessFactor.x = pMaterial->MetallicFactor;
            materialData.MetallicRoughnessFactor.y = pMaterial->RoughnessFactor;
            materialData.Flags.x                   = flags;

            auto objectDataUploadBufferRange = BufferPools[ frameIndex ].TSuballocate( objectData );
            assert( VK_NULL_HANDLE != objectDataUploadBufferRange.DescriptorBufferInfo.buffer );
            objectDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( ObjectUBO );

            auto materialDataUploadBufferRange = BufferPools[ frameIndex ].TSuballocate( materialData );
            assert( VK_NULL_HANDLE != materialDataUploadBufferRange.DescriptorBufferInfo.buffer );
            materialDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( MaterialUBO );

            TDescriptorSet< 8 > descriptorSetForObject( eTDescriptorSetNoInit );

            descriptorSetForObject.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorSetForObject.pBinding[ 0 ].BufferInfo      = objectDataUploadBufferRange.DescriptorBufferInfo;
            descriptorSetForObject.pBinding[ 0 ].DstBinding      = 0;

            descriptorSetForObject.pBinding[ 1 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorSetForObject.pBinding[ 1 ].BufferInfo      = materialDataUploadBufferRange.DescriptorBufferInfo;
            descriptorSetForObject.pBinding[ 1 ].DstBinding      = 1;

            uint32_t objectSetBindingCount = 2;

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 2;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hBaseColorImgView,
                                                                    pMaterialAsset->pBaseColorSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 3;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hNormalImgView,
                                                                    pMaterialAsset->pNormalSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 4;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hEmissiveImgView,
                                                                    pMaterialAsset->pEmissiveSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 5;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hMetallicImgView,
                                                                    pMaterialAsset->pMetallicSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 6;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hRoughnessImgView,
                                                                    pMaterialAsset->pRoughnessSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 7;
            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
                                                                    pMaterialAsset->hOcclusionImgView,
                                                                    pMaterialAsset->pOcclusionSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.BindingCount = objectSetBindingCount;

            ppDescriptorSets[ kDescriptorSetForObj ] = DescriptorSetPools[ frameIndex ][ kDescriptorSetForObj ].GetDescSet( &descriptorSetForObject );

            pDynamicOffsets[ 2 ] = objectDataUploadBufferRange.DynamicOffset;
            pDynamicOffsets[ 3 ] = materialDataUploadBufferRange.DynamicOffset;

            vkCmdBindDescriptorSets( pParams->pCmdBuffer,              /* Cmd */
                                     VK_PIPELINE_BIND_POINT_GRAPHICS,  /* BindPoint */
                                     hPipelineLayout,                  /* PipelineLayout */
                                     0,                                /* FirstSet */
                                     GetArraySize( ppDescriptorSets ), /* SetCount */
                                     ppDescriptorSets,                 /* Sets */
                                     GetArraySize( pDynamicOffsets ),  /* DymamicOffsetCount */
                                     pDynamicOffsets );                /* DymamicOffsets */

            VkBuffer     ppVertexBuffers[ 1 ] = {pMeshAsset->hVertexBuffer.Handle.pBuffer};
            VkDeviceSize pVertexOffsets[ 1 ]  = {0};

            vkCmdBindVertexBuffers( pParams->pCmdBuffer, /* Cmd */
                                    0,                   /* FirstBinding */
                                    1,                   /* BindingCount */
                                    ppVertexBuffers,     /* Buffers */
                                    pVertexOffsets );    /* Offsets */

            vkCmdBindIndexBuffer( pParams->pCmdBuffer,                     /* Cmd */
                                  pMeshAsset->hIndexBuffer.Handle.pBuffer, /* IndexBuffer */
                                  pMeshAsset->IndexOffset,                 /* Offset */
                                  pMeshAsset->IndexType );                 /* UInt16/Uint32 */

            vkCmdDrawIndexed( pParams->pCmdBuffer,   /* Cmd */
                              pSubsetIt->IndexCount, /* IndexCount */
                              1,                     /* InstanceCount */
                              pSubsetIt->BaseIndex,  /* FirstIndex */
                              0,                     /* VertexOffset */
                              0 );                   /* FirstInstance */
        }
    }

    return true;
}

bool apemode::SceneRendererVk::Recreate( const RecreateParametersBase* pParamsBase ) {
    using namespace apemodevk;

    if ( nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (RecreateParametersVk*) pParamsBase;
    if ( nullptr == pParams->pNode )
        return false;

    ShaderCompilerIncludedFileSet includedFiles;

    auto compiledVertexShader = pParams->pShaderCompiler->Compile(
        "shaders/Scene.vert", nullptr, ShaderCompiler::eShaderType_GLSL_VertexShader, &includedFiles );

    if ( nullptr == compiledVertexShader ) {
        platform::DebugBreak( );
        return false;
    }

    auto compiledFragmentShader = pParams->pShaderCompiler->Compile(
        "shaders/Scene.frag", nullptr, ShaderCompiler::eShaderType_GLSL_FragmentShader, &includedFiles );

    if ( nullptr == compiledFragmentShader ) {
        platform::DebugBreak( );
        return false;
    }

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    InitializeStruct( vertexShaderCreateInfo );
    vertexShaderCreateInfo.pCode    = compiledVertexShader->GetDwordPtr( );
    vertexShaderCreateInfo.codeSize = compiledVertexShader->GetByteCount( );

    VkShaderModuleCreateInfo fragmentShaderCreateInfo;
    InitializeStruct( fragmentShaderCreateInfo );
    fragmentShaderCreateInfo.pCode    = compiledFragmentShader->GetDwordPtr( );
    fragmentShaderCreateInfo.codeSize = compiledFragmentShader->GetByteCount( );

    THandle< VkShaderModule > hVertexShaderModule;
    THandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( *pParams->pNode, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( *pParams->pNode, fragmentShaderCreateInfo ) ) {
        platform::DebugBreak( );
        return false;
    }

    //
    // Set 0 (Pass)
    //
    // layout( std140, set = 0, binding = 0 ) uniform CameraUBO;
    // layout( std140, set = 0, binding = 1 ) uniform LightUBO;
    // layout( set = 0, binding = 2 ) uniform samplerCube SkyboxCubeMap;
    // layout( set = 0, binding = 3 ) uniform samplerCube IrradianceCubeMap;

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindingsForPass[ 4 ];
    InitializeStruct( descriptorSetLayoutBindingsForPass );

    descriptorSetLayoutBindingsForPass[ 0 ].binding         = 0;
    descriptorSetLayoutBindingsForPass[ 0 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForPass[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForPass[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindingsForPass[ 1 ].binding         = 1;
    descriptorSetLayoutBindingsForPass[ 1 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForPass[ 1 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForPass[ 1 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindingsForPass[ 2 ].binding         = 2;
    descriptorSetLayoutBindingsForPass[ 2 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForPass[ 2 ].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindingsForPass[ 2 ].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindingsForPass[ 3 ].binding         = 3;
    descriptorSetLayoutBindingsForPass[ 3 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForPass[ 3 ].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindingsForPass[ 3 ].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    //
    // Set 1 (Object)
    //
    // layout( std140, set = 1, binding = 0 ) uniform ObjectUBO;
    // layout( std140, set = 1, binding = 1 ) uniform MaterialUBO;
    // layout( set = 1, binding = 2 ) uniform sampler2D BaseColorMap;
    // layout( set = 1, binding = 3 ) uniform sampler2D NormalMap;
    // layout( set = 1, binding = 4 ) uniform sampler2D OcclusionMap;
    // layout( set = 1, binding = 5 ) uniform sampler2D EmissiveMap;
    // layout( set = 1, binding = 6 ) uniform sampler2D MetallicMap;
    // layout( set = 1, binding = 7 ) uniform sampler2D RoughnessMap;

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindingsForObj[ 8 ];
    InitializeStruct( descriptorSetLayoutBindingsForObj );

    descriptorSetLayoutBindingsForObj[ 0 ].binding         = 0;
    descriptorSetLayoutBindingsForObj[ 0 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForObj[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForObj[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    descriptorSetLayoutBindingsForObj[ 1 ].binding         = 1;
    descriptorSetLayoutBindingsForObj[ 1 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForObj[ 1 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForObj[ 1 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    for ( uint32_t i = 2; i < GetArraySize( descriptorSetLayoutBindingsForObj ); ++i ) {
        descriptorSetLayoutBindingsForObj[ i ].binding         = i;
        descriptorSetLayoutBindingsForObj[ i ].descriptorCount = 1;
        descriptorSetLayoutBindingsForObj[ i ].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetLayoutBindingsForObj[ i ].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfos[ kDescriptorSetCount ];
    InitializeStruct( descriptorSetLayoutCreateInfos );

    VkDescriptorSetLayout ppDescriptorSetLayouts[ kDescriptorSetCount ] = {nullptr};

    descriptorSetLayoutCreateInfos[ 0 ].bindingCount = apemode::GetArraySize( descriptorSetLayoutBindingsForPass );
    descriptorSetLayoutCreateInfos[ 0 ].pBindings    = descriptorSetLayoutBindingsForPass;

    descriptorSetLayoutCreateInfos[ 1 ].bindingCount = apemode::GetArraySize( descriptorSetLayoutBindingsForObj );
    descriptorSetLayoutCreateInfos[ 1 ].pBindings    = descriptorSetLayoutBindingsForObj;

    for ( uint32_t i = 0; i < kDescriptorSetCount; ++i ) {
        THandle< VkDescriptorSetLayout >& hDescriptorSetLayout = hDescriptorSetLayouts[ i ];
        if ( !hDescriptorSetLayout.Recreate( *pParams->pNode, descriptorSetLayoutCreateInfos[ i ] ) ) {
            return false;
        }

        // []
        // kDescriptorSetForPass
        // kDescriptorSetForObj
        ppDescriptorSetLayouts[ i ] = hDescriptorSetLayout;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = GetArraySize( ppDescriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = ppDescriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( *pParams->pNode, pipelineLayoutCreateInfo ) ) {
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 4 ];
    VkVertexInputBindingDescription        vertexInputBindingDescription[ 1 ];
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    colorBlendStateCreateInfo;
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState[ 1 ];
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo;
    VkPipelineViewportStateCreateInfo      viewportStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   multisampleStateCreateInfo;
    VkDynamicState                         dynamicStateEnables[ 2 ];
    VkPipelineDynamicStateCreateInfo       dynamicStateCreateInfo ;
    VkPipelineShaderStageCreateInfo        shaderStageCreateInfo[ 2 ];

    InitializeStruct( graphicsPipelineCreateInfo );
    InitializeStruct( pipelineCacheCreateInfo );
    InitializeStruct( vertexInputStateCreateInfo );
    InitializeStruct( vertexInputAttributeDescription );
    InitializeStruct( vertexInputBindingDescription );
    InitializeStruct( inputAssemblyStateCreateInfo );
    InitializeStruct( rasterizationStateCreateInfo );
    InitializeStruct( colorBlendStateCreateInfo );
    InitializeStruct( colorBlendAttachmentState );
    InitializeStruct( depthStencilStateCreateInfo );
    InitializeStruct( viewportStateCreateInfo );
    InitializeStruct( multisampleStateCreateInfo );
    InitializeStruct( dynamicStateCreateInfo );
    InitializeStruct( shaderStageCreateInfo );

    //

    graphicsPipelineCreateInfo.layout     = hPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = pParams->pRenderPass;

    //

    shaderStageCreateInfo[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[ 0 ].module = hVertexShaderModule;
    shaderStageCreateInfo[ 0 ].pName  = "main";

    shaderStageCreateInfo[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[ 1 ].module = hFragmentShaderModule;
    shaderStageCreateInfo[ 1 ].pName  = "main";

    graphicsPipelineCreateInfo.stageCount = GetArraySize( shaderStageCreateInfo );
    graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

    //
#if 0

    vertexInputBindingDescription[ 0 ].stride    = sizeof( PackedVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R16G16_UNORM;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->texcoords );

#else

    vertexInputBindingDescription[ 0 ].stride    = sizeof( StaticVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->texcoords );

#endif

    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = GetArraySize( vertexInputBindingDescription );
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = vertexInputBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = GetArraySize( vertexInputAttributeDescription );
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = vertexInputAttributeDescription;
    graphicsPipelineCreateInfo.pVertexInputState               = &vertexInputStateCreateInfo;

    //

    inputAssemblyStateCreateInfo.topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

    //

    dynamicStateEnables[ 0 ]                 = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStateEnables[ 1 ]                 = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
    dynamicStateCreateInfo.dynamicStateCount = GetArraySize( dynamicStateEnables );
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //

    // rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    // rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; /* CW */
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;
    graphicsPipelineCreateInfo.pRasterizationState       = &rasterizationStateCreateInfo;

    //

    colorBlendAttachmentState[ 0 ].colorWriteMask = 0xf;
    colorBlendAttachmentState[ 0 ].blendEnable    = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount     = GetArraySize( colorBlendAttachmentState );
    colorBlendStateCreateInfo.pAttachments        = colorBlendAttachmentState;
    graphicsPipelineCreateInfo.pColorBlendState   = &colorBlendStateCreateInfo;

    //

    depthStencilStateCreateInfo.depthTestEnable       = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.front                 = depthStencilStateCreateInfo.back;
    graphicsPipelineCreateInfo.pDepthStencilState     = &depthStencilStateCreateInfo;

    //

    viewportStateCreateInfo.scissorCount      = 1;
    viewportStateCreateInfo.viewportCount     = 1;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

    //

    multisampleStateCreateInfo.pSampleMask          = NULL;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    graphicsPipelineCreateInfo.pMultisampleState    = &multisampleStateCreateInfo;

    //

    if ( false == hPipelineCache.Recreate( *pParams->pNode, pipelineCacheCreateInfo ) ) {
        platform::DebugBreak( );
        return false;
    }

    if ( false == hPipeline.Recreate( *pParams->pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        platform::DebugBreak( );
        return false;
    }

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        BufferPools[ i ].Recreate( pParams->pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
        DescriptorSetPools[ i ][ kDescriptorSetForPass ].Recreate( *pParams->pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForPass ] );
        DescriptorSetPools[ i ][ kDescriptorSetForObj ].Recreate( *pParams->pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForObj ] );
    }

    return true;
}

bool apemode::SceneRendererVk::Reset( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        BufferPools[ FrameIndex ].Reset( );
    }

    return true;
}

bool apemode::SceneRendererVk::Flush( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        BufferPools[ FrameIndex ].Flush( );
    }

    return true;
}
