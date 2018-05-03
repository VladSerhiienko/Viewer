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

    struct FrameUniformBufferData {
        XMFLOAT4X4 WorldMatrix;
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjMatrix;
        XMFLOAT4   Color;
        XMFLOAT4   PositionOffset;
        XMFLOAT4   PositionScale;
    };

    struct SceneMeshDeviceAssetVk : apemode::SceneDeviceAsset {
        THandle< BufferComposite > hVertexBuffer;
        THandle< BufferComposite > hIndexBuffer;
        uint32_t                   VertexCount = 0;
        uint32_t                   IndexOffset = 0;
        VkIndexType                IndexType   = VK_INDEX_TYPE_UINT16;
    };

    struct SceneMaterialDeviceAssetVk : apemode::SceneDeviceAsset {
        const LoadedImage* pBaseColorLoadedImg;
        const LoadedImage* pNormalLoadedImg;
        const LoadedImage* pOcclusionLoadedImg;
        const LoadedImage* pEmissiveLoadedImg;
        const LoadedImage* pMetallicRoughnessLoadedImg;
    };

    const LoadedImage** GetLoadedImageSlotForPropertyName( SceneMaterialDeviceAssetVk* pMaterialAsset,
                                                           const char*                 pszTexturePropName ) {

        if ( strcmp( "baseColorTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pBaseColorLoadedImg;
        } else if ( strcmp( "normalTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pNormalLoadedImg;
        } else if ( strcmp( "occlusionTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pOcclusionLoadedImg;
        } else if ( strcmp( "metallicRoughnessTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pMetallicRoughnessLoadedImg;
        } else if ( strcmp( "emissiveTexture", pszTexturePropName ) == 0 ) {
            return &pMaterialAsset->pEmissiveLoadedImg;
        }

        return nullptr;
    }

    struct SceneDeviceAssetVk : apemode::SceneDeviceAsset {
        using LoadedImagePtrPair = std::pair< uint32_t, std::unique_ptr< LoadedImage > >;
        std::vector< LoadedImagePtrPair > LoadedImgs;

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

    auto pMeshesFb = pParamsBase->pSceneSrc->meshes( );
    if ( bDeviceChanged && pMeshesFb ) {

        std::vector< apemodevk::MeshBufferFillIntent > bufferFillIntents;
        bufferFillIntents.reserve( pMeshesFb->size( ) << 1 );

        uint64_t totalBytesRequired = 0;

        for ( auto & mesh : pScene->Meshes ) {
            auto pSrcMesh = FlatbuffersTVectorGetAtIndex( pParamsBase->pSceneSrc->meshes( ), mesh.SrcId );
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

    auto pMaterialsFb = pParamsBase->pSceneSrc->materials( );
    if ( bDeviceChanged && pMaterialsFb ) {

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

        for ( auto& material : pScene->Materials ) {

            /* Create material device asset if needed. */
            auto pMaterialAsset = static_cast< apemodevk::SceneMaterialDeviceAssetVk* >( material.pDeviceAsset.get( ) );
            if ( nullptr == pMaterialAsset ) {
                pMaterialAsset = apemode_new apemodevk::SceneMaterialDeviceAssetVk( );
                material.pDeviceAsset.reset( pMaterialAsset );
            }

            auto pMaterialFb          = FlatbuffersTVectorGetAtIndex( pMaterialsFb, material.SrcId );
            auto pTexturesFb          = pParamsBase->pSceneSrc->textures( );
            auto pFilesFb             = pParamsBase->pSceneSrc->files( );
            auto pTexturePropertiesFb = pMaterialFb->texture_properties( );

            assert( pMaterialFb );
            assert( pTexturesFb );
            assert( pFilesFb );
            assert( pTexturePropertiesFb );

            for ( auto pTexturePropFb : *pTexturePropertiesFb ) {

                auto pszTexturePropName      = GetCStringProperty( pParamsBase->pSceneSrc, pTexturePropFb->name_id( ) );
                auto ppLoadedImgMaterialSlot = GetLoadedImageSlotForPropertyName( pMaterialAsset, pszTexturePropName );

                if ( nullptr == ppLoadedImgMaterialSlot ) {
                    LogError( "Cannot map texture property: \"{}\"", pszTexturePropName );
                } else {
                    auto pTextureFb = FlatbuffersTVectorGetAtIndex( pTexturesFb, pTexturePropFb->value_id( ) );
                    assert( pTextureFb );

                    auto pFileFb = FlatbuffersTVectorGetAtIndex( pFilesFb, pTextureFb->file_id( ) );
                    assert( pFileFb );

                    auto pszFileName    = GetCStringProperty( pParamsBase->pSceneSrc, pFileFb->name_id( ) );
                    auto pszTextureName = GetCStringProperty( pParamsBase->pSceneSrc, pTextureFb->name_id( ) );

                    if ( auto pLoadedImg = pSceneAsset->FindLoadedImage( pFileFb->id( ) ) ) {
                        
                        LogInfo( "Assigning loaded texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );
                        ( *ppLoadedImgMaterialSlot ) = pLoadedImg;

                    } else {
                        
                        LogInfo( "Loading texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );
                        auto loadedImg = imageLoader.LoadImageFromData( pFileFb->buffer( )->data( ),
                                                                pFileFb->buffer( )->size( ),
                                                                apemodevk::ImageLoader::eImageFileFormat_PNG,
                                                                false,
                                                                true );

                        if ( loadedImg ) {
                            
                            LogInfo( "Loaded texture: \"{}\" <- {}", pszTexturePropName, pszTextureName );
                            ( *ppLoadedImgMaterialSlot ) = loadedImg.get( );
                            pSceneAsset->AddLoadedImage( pFileFb->id( ), std::move( loadedImg ) );
                            
                        } /* loadedImg */
                    }     /* pLoadedImg */

                } /* ppLoadedImgMaterialSlot */
            }     /* pTexturePropFb */
        }
    }

    return true;
}

bool apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {

    /* Scene was not provided.
     * Render parameters were not provided. */
    if ( nullptr == pScene || nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (const SceneRenderParametersVk*) pParamsBase;

    /* Device change was not handled, cannot render the scene. */
    if ( pNode != pParams->pNode ) {
        return false;
    }

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );

    VkViewport viewport;
    apemodevk::InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pParams->Dims.x * pParams->Scale.x;
    viewport.height   = pParams->Dims.y * pParams->Scale.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( pParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    apemodevk::InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = ( uint32_t )( pParams->Dims.x * pParams->Scale.x );
    scissor.extent.height = ( uint32_t )( pParams->Dims.y * pParams->Scale.y );

    vkCmdSetScissor( pParams->pCmdBuffer, 0, 1, &scissor );

    auto FrameIndex = ( pParams->FrameIndex ) % kMaxFrameCount;

    apemodevk::FrameUniformBufferData frameUniformBufferData;
    frameUniformBufferData.ProjMatrix = pParams->ProjMatrix;
    frameUniformBufferData.ViewMatrix = pParams->ViewMatrix;

    for ( auto& node : pScene->Nodes ) {
        if ( node.MeshId >= pScene->Meshes.size( ) )
            continue;

        auto& dstMesh = pScene->Meshes[ node.MeshId ];

        if ( auto pMeshAsset = (const apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset.get() ) {

            auto pSubsetIt    = pScene->Subsets.data( ) + dstMesh.BaseSubset;
            auto pSubsetItEnd = pSubsetIt + dstMesh.SubsetCount;

            for ( ; pSubsetIt != pSubsetItEnd; ++pSubsetIt ) {

                const SceneMaterial* pMaterial = nullptr;
                if ( pSubsetIt->MaterialId != uint32_t( -1 ) ) {
                    assert( pSubsetIt->MaterialId < pScene->Materials.size( ) );
                    pMaterial = &pScene->Materials[ pSubsetIt->MaterialId ];
                }

                frameUniformBufferData.Color            = pMaterial ? pMaterial->BaseColorFactor : XMFLOAT4{1, 0, 1, 1};
                frameUniformBufferData.PositionOffset.x = dstMesh.PositionOffset.x;
                frameUniformBufferData.PositionOffset.y = dstMesh.PositionOffset.y;
                frameUniformBufferData.PositionOffset.z = dstMesh.PositionOffset.z;
                frameUniformBufferData.PositionScale.x  = dstMesh.PositionScale.x;
                frameUniformBufferData.PositionScale.y  = dstMesh.PositionScale.y;
                frameUniformBufferData.PositionScale.z  = dstMesh.PositionScale.z;

                XMStoreFloat4x4( &frameUniformBufferData.WorldMatrix, pScene->WorldMatrices[ node.Id ] );

                auto suballocResult = BufferPools[ FrameIndex ].TSuballocate( frameUniformBufferData );
                assert( VK_NULL_HANDLE != suballocResult.DescriptorBufferInfo.buffer );
                suballocResult.DescriptorBufferInfo.range = sizeof( apemodevk::FrameUniformBufferData );

                apemodevk::TDescriptorSet< 1 > descriptorSet;
                descriptorSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                descriptorSet.pBinding[ 0 ].BufferInfo      = suballocResult.DescriptorBufferInfo;

                VkDescriptorSet pDescriptorSet = DescSetPools[ FrameIndex ].GetDescSet( &descriptorSet );

                vkCmdBindDescriptorSets( pParams->pCmdBuffer,             /* Cmd */
                                         VK_PIPELINE_BIND_POINT_GRAPHICS, /* BindPoint */
                                         hPipelineLayout,                 /* PipelineLayout */
                                         0,                               /* FirstSet */
                                         1,                               /* SetCount */
                                         &pDescriptorSet,                 /* Sets */
                                         1,                               /* DymamicOffsetCount */
                                         &suballocResult.DynamicOffset ); /* DymamicOffsets */

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
    }

    return true;
}

bool apemode::SceneRendererVk::Recreate( const RecreateParametersBase* pParamsBase ) {
    if ( nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (RecreateParametersVk*) pParamsBase;
    if ( nullptr == pParams->pNode )
        return false;

    apemodevk::ShaderCompilerIncludedFileSet includedFiles;

    auto compiledVertexShader = pParams->pShaderCompiler->Compile(
        "shaders/Scene.vert", nullptr, apemodevk::ShaderCompiler::eShaderType_GLSL_VertexShader, &includedFiles );

    if ( nullptr == compiledVertexShader ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    auto compiledFragmentShader = pParams->pShaderCompiler->Compile(
        "shaders/Scene.frag", nullptr, apemodevk::ShaderCompiler::eShaderType_GLSL_FragmentShader, &includedFiles );

    if ( nullptr == compiledFragmentShader ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    apemodevk::InitializeStruct( vertexShaderCreateInfo );
    vertexShaderCreateInfo.pCode    = compiledVertexShader->GetDwordPtr( );
    vertexShaderCreateInfo.codeSize = compiledVertexShader->GetByteCount( );

    VkShaderModuleCreateInfo fragmentShaderCreateInfo;
    apemodevk::InitializeStruct( fragmentShaderCreateInfo );
    fragmentShaderCreateInfo.pCode    = compiledFragmentShader->GetDwordPtr( );
    fragmentShaderCreateInfo.codeSize = compiledFragmentShader->GetByteCount( );

    apemodevk::THandle< VkShaderModule > hVertexShaderModule;
    apemodevk::THandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( *pParams->pNode, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( *pParams->pNode, fragmentShaderCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkDescriptorSetLayoutBinding bindings[ 1 ];
    apemodevk::InitializeStruct( bindings );

    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[ 0 ].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    apemodevk::InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = 1;
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( *pParams->pNode, descSetLayoutCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    if ( false == DescSets.RecreateResourcesFor( *pParams->pNode, pParams->pDescPool, descriptorSetLayouts ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    apemodevk::InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = apemodevk::GetArraySizeU( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( *pParams->pNode, pipelineLayoutCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
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

    apemodevk::InitializeStruct( graphicsPipelineCreateInfo );
    apemodevk::InitializeStruct( pipelineCacheCreateInfo );
    apemodevk::InitializeStruct( vertexInputStateCreateInfo );
    apemodevk::InitializeStruct( vertexInputAttributeDescription );
    apemodevk::InitializeStruct( vertexInputBindingDescription );
    apemodevk::InitializeStruct( inputAssemblyStateCreateInfo );
    apemodevk::InitializeStruct( rasterizationStateCreateInfo );
    apemodevk::InitializeStruct( colorBlendStateCreateInfo );
    apemodevk::InitializeStruct( colorBlendAttachmentState );
    apemodevk::InitializeStruct( depthStencilStateCreateInfo );
    apemodevk::InitializeStruct( viewportStateCreateInfo );
    apemodevk::InitializeStruct( multisampleStateCreateInfo );
    apemodevk::InitializeStruct( dynamicStateCreateInfo );
    apemodevk::InitializeStruct( shaderStageCreateInfo );

    //

    graphicsPipelineCreateInfo.layout     = hPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = pParams->pRenderPass;

    //

    shaderStageCreateInfo[ 0 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[ 0 ].module = hVertexShaderModule;
    shaderStageCreateInfo[ 0 ].pName  = "main";

    shaderStageCreateInfo[ 1 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[ 1 ].module = hFragmentShaderModule;
    shaderStageCreateInfo[ 1 ].pName  = "main";

    graphicsPipelineCreateInfo.stageCount = apemodevk::GetArraySizeU( shaderStageCreateInfo );
    graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

    //
#if 0

    vertexInputBindingDescription[ 0 ].stride    = sizeof( apemodevk::PackedVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R16G16_UNORM;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->texcoords );

#else

    vertexInputBindingDescription[ 0 ].stride    = sizeof( apemodevk::StaticVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (apemodevk::StaticVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (apemodevk::StaticVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (apemodevk::StaticVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (apemodevk::StaticVertex*) 0 )->texcoords );

#endif

    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = apemodevk::GetArraySizeU( vertexInputBindingDescription );
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = vertexInputBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = apemodevk::GetArraySizeU( vertexInputAttributeDescription );
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = vertexInputAttributeDescription;
    graphicsPipelineCreateInfo.pVertexInputState               = &vertexInputStateCreateInfo;

    //

    inputAssemblyStateCreateInfo.topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

    //

    dynamicStateEnables[ 0 ]                 = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStateEnables[ 1 ]                 = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
    dynamicStateCreateInfo.dynamicStateCount = apemodevk::GetArraySizeU( dynamicStateEnables );
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //

    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    // rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    // rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; /* CW */
    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;
    graphicsPipelineCreateInfo.pRasterizationState       = &rasterizationStateCreateInfo;

    //

    colorBlendAttachmentState[ 0 ].colorWriteMask = 0xf;
    colorBlendAttachmentState[ 0 ].blendEnable    = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount     = apemodevk::GetArraySizeU( colorBlendAttachmentState );
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
        apemodevk::platform::DebugBreak( );
        return false;
    }

    if ( false == hPipeline.Recreate( *pParams->pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        BufferPools[ i ].Recreate( pParams->pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
        DescSetPools[ i ].Recreate( *pParams->pNode, pParams->pDescPool, hDescSetLayout );
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
