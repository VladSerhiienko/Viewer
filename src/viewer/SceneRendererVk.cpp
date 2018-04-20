#include <fbxvpch.h>

#include <MathInc.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <Buffer.Vulkan.h>

#include <SceneRendererVk.h>
#include <Scene.h>
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

    struct FrameUniformBuffer {
        XMFLOAT4X4 worldMatrix;
        XMFLOAT4X4 viewMatrix;
        XMFLOAT4X4 projectionMatrix;
        XMFLOAT4   color;
        XMFLOAT4   positionOffset;
        XMFLOAT4   positionScale;
    };

    struct SceneMeshDeviceAssetVk {
        THandle< BufferComposite > hBuffer;
        uint32_t                   VertexCount = 0;
        uint32_t                   IndexOffset = 0;
        VkIndexType                IndexType   = VK_INDEX_TYPE_UINT16;
        XMFLOAT4                   positionOffset;
        XMFLOAT4                   positionScale;
    };

    struct SceneMaterialDeviceAssetVk {
        std::unique_ptr< LoadedImage > pBaseColorLoadedImg;
        std::unique_ptr< LoadedImage > pNormalLoadedImg;
        std::unique_ptr< LoadedImage > pOcclusionLoadedImg;
        std::unique_ptr< LoadedImage > pEmissiveLoadedImg;
        std::unique_ptr< LoadedImage > pMetallicRoughnessLoadedImg;
    };

    /* calculate nearest power of 2 */
    uint32_t NearestPow2( uint32_t v ) {
        --v;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }

    bool AllocateStagingMemoryPool( GraphicsDevice*                pNode,
                                    apemodevk::THandle< VmaPool >& stagingMemoryPool,
                                    uint32_t                       stagingMemoryLimit,
                                    uint32_t&                      maxBlockCount,
                                    uint32_t&                      blockSize ) {

        constexpr uint32_t dummyBufferSize    = 64;               /* 64 b */
        constexpr uint32_t preferredBlockSize = 64 * 1024 * 1024; /* 64 mb */

        if ( stagingMemoryLimit < preferredBlockSize ) {
            blockSize  = NearestPow2( stagingMemoryLimit );
            maxBlockCount = 1;
        } else {
            blockSize  = preferredBlockSize;
            maxBlockCount = stagingMemoryLimit / preferredBlockSize;
        }

        assert( blockSize );
        assert( maxBlockCount );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.size  = dummyBufferSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocationCreateInfo;
        InitializeStruct( allocationCreateInfo );
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        VmaPoolCreateInfo poolCreateInfo;
        InitializeStruct( poolCreateInfo );
        poolCreateInfo.blockSize     = blockSize;
        poolCreateInfo.maxBlockCount = maxBlockCount;

        // clang-format off
        if ( VK_SUCCESS != CheckedCall( vmaFindMemoryTypeIndexForBufferInfo( pNode->hAllocator,
                                                                             &bufferCreateInfo,
                                                                             &allocationCreateInfo,
                                                                             &poolCreateInfo.memoryTypeIndex ) ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }
        // clang-format on

        if ( false != stagingMemoryPool.Recreate( pNode->hAllocator, poolCreateInfo ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

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

    if ( bDeviceChanged ) {

        /* Get queue from pool (only copying) */
        auto pQueuePool = pParams->pNode->GetQueuePool( );

        auto acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_GRAPHICS_BIT, false );
        // auto acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_GRAPHICS_BIT, false );
            // acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, false );
        }

        /* Get command buffer from pool (only copying) */
        auto pCmdBufferPool = pParams->pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, acquiredQueue.queueFamilyId );

        uint32_t blockSize;
        uint32_t maxBlockCount;

        apemodevk::THandle< VmaPool > hPool;

        if ( false != apemodevk::AllocateStagingMemoryPool( pParams->pNode, hPool, 128 * 1024 * 1024, maxBlockCount, blockSize ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        apemodevk::HostBufferPool bufferPool;
        bufferPool.Recreate( *pParams->pNode,
                             *pParams->pNode,
                             &pParams->pNode->AdapterProps.limits,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             false );

        uint32_t meshIndex = 0;
        auto & meshesFb = *pParamsBase->pSceneSrc->meshes( );

        if ( VK_SUCCESS != vkResetCommandPool( *pParams->pNode, acquiredCmdBuffer.pCmdPool, 0 ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        apemodevk::InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if ( VK_SUCCESS != vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &commandBufferBeginInfo ) ) {
            apemodevk::platform::DebugBreak( );
        }

        struct MeshResource {
            apemodevk::SceneMeshDeviceAssetVk* pMeshDeviceAsset = nullptr;
            const apemodefb::MeshFb*           pSrcMesh         = nullptr;
        };

        std::vector< MeshResource > meshResources;
        for ( auto pSrcMesh : meshesFb ) {
            /* Scene dstMesh. */
            auto& dstMesh = pScene->meshes[ meshIndex++ ];

            /* Create dstMesh device asset if needed. */
            auto pMeshDeviceAsset = (apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset;
            if ( nullptr == pMeshDeviceAsset ) {
                pMeshDeviceAsset = apemode_new apemodevk::SceneMeshDeviceAssetVk( );
                dstMesh.pDeviceAsset = pMeshDeviceAsset;
            }

            MeshResource& meshResource    = apemodevk::PushBackAndGet( meshResources );
            meshResource.pMeshDeviceAsset = pMeshDeviceAsset;
            meshResource.pSrcMesh         = pSrcMesh;
        }

        for ( auto pSrcMesh : meshesFb ) {
            /* Scene dstMesh. */
            auto& dstMesh = pScene->meshes[ meshIndex++ ];

            /* Create dstMesh device asset if needed. */
            auto pMeshDeviceAsset = (apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset;

            const uint32_t verticesByteSize = (uint32_t) pSrcMesh->vertices( )->size( );
            const uint32_t indicesByteSize  = (uint32_t) pSrcMesh->indices( )->size( );

            const uint32_t storageAlignment = (uint32_t) pNode->AdapterProps.limits.minStorageBufferOffsetAlignment;
            const uint32_t verticesStorageSize = apemodexm::AlignedOffset( verticesByteSize, storageAlignment );
            const uint32_t totalMeshSize = verticesStorageSize + indicesByteSize;

            pMeshDeviceAsset->IndexOffset = verticesStorageSize;
            if ( pSrcMesh->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ) {
                pMeshDeviceAsset->IndexType = VK_INDEX_TYPE_UINT32;
            }

            auto & offset = pSrcMesh->submeshes( )->begin( )->position_offset( );
            pMeshDeviceAsset->positionOffset.x = offset.x();
            pMeshDeviceAsset->positionOffset.y = offset.y();
            pMeshDeviceAsset->positionOffset.z = offset.z();

            auto & scale = pSrcMesh->submeshes( )->begin( )->position_scale( );
            pMeshDeviceAsset->positionScale.x = scale.x();
            pMeshDeviceAsset->positionScale.y = scale.y();
            pMeshDeviceAsset->positionScale.z = scale.z();
            pMeshDeviceAsset->VertexCount = pSrcMesh->submeshes( )->begin( )->vertex_count( );

            auto verticesSuballocResult = bufferPool.Suballocate( pSrcMesh->vertices( )->Data( ), verticesByteSize );
            auto indicesSuballocResult = bufferPool.Suballocate( pSrcMesh->indices( )->Data( ), indicesByteSize );

            static VkBufferUsageFlags eBufferUsage
                = VK_BUFFER_USAGE_TRANSFER_DST_BIT  /* Copy data from staging buffers */
                | VK_BUFFER_USAGE_INDEX_BUFFER_BIT  /* Use it as index buffer */
                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT /* Use it as vertex buffer */;

            VkBufferCreateInfo bufferCreateInfo;
            apemodevk::InitializeStruct( bufferCreateInfo );
            bufferCreateInfo.usage = eBufferUsage;
            bufferCreateInfo.size  = totalMeshSize;

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocationCreateInfo.flags = 0;

            if ( false == pMeshDeviceAsset->hBuffer.Recreate( pParams->pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
                apemodevk::platform::DebugBreak( );
            }

            VkBufferMemoryBarrier bufferMemoryBarrier[ 3 ];
            apemodevk::InitializeStruct( bufferMemoryBarrier );

            // bufferMemoryBarrier[ 0 ].size                = verticesSuballocResult.descBufferInfo.range;
            // bufferMemoryBarrier[ 0 ].offset              = verticesSuballocResult.descBufferInfo.offset;
            // bufferMemoryBarrier[ 0 ].buffer              = verticesSuballocResult.descBufferInfo.buffer;
            // bufferMemoryBarrier[ 0 ].srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
            // bufferMemoryBarrier[ 0 ].dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            // bufferMemoryBarrier[ 0 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            // bufferMemoryBarrier[ 0 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            // bufferMemoryBarrier[ 1 ].size                = indicesSuballocResult.descBufferInfo.range;
            // bufferMemoryBarrier[ 1 ].offset              = indicesSuballocResult.descBufferInfo.offset;
            // bufferMemoryBarrier[ 1 ].buffer              = indicesSuballocResult.descBufferInfo.buffer;
            // bufferMemoryBarrier[ 1 ].srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
            // bufferMemoryBarrier[ 1 ].dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            // bufferMemoryBarrier[ 1 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            // bufferMemoryBarrier[ 1 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            bufferMemoryBarrier[ 2 ].size                = bufferCreateInfo.size;
            bufferMemoryBarrier[ 2 ].offset              = 0;
            bufferMemoryBarrier[ 2 ].buffer              = pMeshDeviceAsset->hBuffer.Handle.pBuffer;
            bufferMemoryBarrier[ 2 ].srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferMemoryBarrier[ 2 ].dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            bufferMemoryBarrier[ 2 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier[ 2 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                  // VK_PIPELINE_STAGE_HOST_BIT,
                                  // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                  0,
                                  0,
                                  nullptr,
                                  1,
                                  &bufferMemoryBarrier[ 2 ],
                                  // apemode::GetArraySize( bufferMemoryBarrier ),
                                  // bufferMemoryBarrier,
                                  0,
                                  nullptr );

            VkBufferCopy bufferCopy[ 2 ];
            apemodevk::InitializeStruct( bufferCopy );

            bufferCopy[ 0 ].srcOffset = verticesSuballocResult.descBufferInfo.offset;
            bufferCopy[ 0 ].size      = verticesSuballocResult.descBufferInfo.range;

            bufferCopy[ 1 ].srcOffset = indicesSuballocResult.descBufferInfo.offset;
            bufferCopy[ 1 ].dstOffset = verticesStorageSize;
            bufferCopy[ 1 ].size      = indicesSuballocResult.descBufferInfo.range;

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                 /* Cmd */
                             verticesSuballocResult.descBufferInfo.buffer, /* Src */
                             pMeshDeviceAsset->hBuffer.Handle.pBuffer,     /* Dst */
                             1,
                             &bufferCopy[ 0 ] );

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                /* Cmd */
                             indicesSuballocResult.descBufferInfo.buffer, /* Src */
                             pMeshDeviceAsset->hBuffer.Handle.pBuffer,    /* Dst */
                             1,
                             &bufferCopy[ 1 ] );
        }

        vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

        VkSubmitInfo submitInfo;
        apemodevk::InitializeStruct( submitInfo );
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;

        bufferPool.Flush( );

        vkResetFences( *pParams->pNode, 1, &acquiredQueue.pFence );
        vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );
        vkWaitForFences( *pParams->pNode, 1, &acquiredQueue.pFence, true, UINT_MAX );

        acquiredCmdBuffer.pFence = acquiredQueue.pFence;
        pCmdBufferPool->Release( acquiredCmdBuffer );
        pQueuePool->Release( acquiredQueue );
    }

    return true;
}

bool apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {
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

    vkCmdSetScissor(pParams->pCmdBuffer, 0, 1, &scissor);

    auto FrameIndex = ( pParams->FrameIndex ) % kMaxFrameCount;

    apemodevk::FrameUniformBuffer frameData;
    frameData.projectionMatrix = pParams->ProjMatrix;
    frameData.viewMatrix       = pParams->ViewMatrix;

    for ( auto& node : pScene->nodes ) {
        if ( node.meshId >= pScene->meshes.size( ) )
            continue;

        auto& dstMesh = pScene->meshes[ node.meshId ];

        if ( auto pMeshDeviceAsset = (const apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset ) {
            for ( auto& subset : dstMesh.subsets ) {
                auto& mat = pScene->materials[ node.materialIds[ subset.materialId ] ];

                //frameData.color          = mat.baseColorFactor;
                frameData.positionOffset = pMeshDeviceAsset->positionOffset;
                frameData.positionScale  = pMeshDeviceAsset->positionScale;
                XMStoreFloat4x4( &frameData.worldMatrix, pScene->worldMatrices[ node.id ] );

                auto suballocResult = BufferPools[ FrameIndex ].TSuballocate( frameData );
                assert( VK_NULL_HANDLE != suballocResult.descBufferInfo.buffer );
                suballocResult.descBufferInfo.range = sizeof( apemodevk::FrameUniformBuffer );

                VkDescriptorSet descriptorSet[ 1 ]  = {nullptr};

                apemodevk::TDescriptorSet< 1 > descSet;
                descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                descSet.pBinding[ 0 ].BufferInfo      = suballocResult.descBufferInfo;

                descriptorSet[ 0 ]  = DescSetPools[ FrameIndex ].GetDescSet( &descSet );

                vkCmdBindDescriptorSets( pParams->pCmdBuffer,
                                         VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         hPipelineLayout,
                                         0,
                                         1,
                                         descriptorSet,
                                         1,
                                         &suballocResult.dynamicOffset );

                VkBuffer     vertexBuffers[ 1 ] = {pMeshDeviceAsset->hBuffer.Handle.pBuffer};
                VkDeviceSize vertexOffsets[ 1 ] = {0};
                vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

                vkCmdBindIndexBuffer( pParams->pCmdBuffer,
                                      pMeshDeviceAsset->hBuffer.Handle.pBuffer,
                                      pMeshDeviceAsset->IndexOffset,
                                      pMeshDeviceAsset->IndexType );

                vkCmdDrawIndexed( pParams->pCmdBuffer,
                                  subset.indexCount, /* IndexCount */
                                  1,                 /* InstanceCount */
                                  subset.baseIndex,  /* FirstIndex */
                                  0,                 /* VertexOffset */
                                  0 );               /* FirstInstance */
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

    VkPhysicalDeviceProperties adapterProps;
    vkGetPhysicalDeviceProperties( *pParams->pNode, &adapterProps );

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        BufferPools[ i ].Recreate( *pParams->pNode, *pParams->pNode, &adapterProps.limits, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
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
