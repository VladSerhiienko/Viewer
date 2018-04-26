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
        XMFLOAT4   PositionOffset;
        XMFLOAT4   PositionScale;
    };

    struct SceneMeshDeviceAssetVk {
        THandle< BufferComposite > hVertexBuffer;
        THandle< BufferComposite > hIndexBuffer;
        uint32_t                   VertexCount = 0;
        uint32_t                   IndexOffset = 0;
        VkIndexType                IndexType   = VK_INDEX_TYPE_UINT16;
        XMFLOAT4                   PositionOffset;
        XMFLOAT4                   PositionScale;
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

    bool AllocateStagingMemoryPool( GraphicsDevice*     pNode,
                                    THandle< VmaPool >& stagingMemoryPool,
                                    uint32_t            stagingMemoryLimit,
                                    uint32_t&           maxBlockCount,
                                    uint32_t&           blockSize ) {
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
            platform::DebugBreak( );
            return false;
        }
        // clang-format on

        if ( false == stagingMemoryPool.Recreate( pNode->hAllocator, poolCreateInfo ) ) {
            platform::DebugBreak( );
            return false;
        }

        return true;
    }

    struct MeshResource {
        SceneMeshDeviceAssetVk*  pDstAsset = nullptr;
        const apemodefb::MeshFb* pSrcMesh  = nullptr;
    };

    /**
     * Uploads resources from CPU to GPU with respect to staging memory limit.
     */
    MeshResource* UploadResources( GraphicsDevice*     pNode,
                                   THandle< VmaPool >& pool,
                                   uint32_t            blockSize,
                                   MeshResource*       pResourceIt,
                                   MeshResource*       pResourcesEndIt ) {
        return nullptr;
    }

    /**
     * Uploads resources from CPU to GPU with respect to staging memory limit.
     */
    struct BufferFillIntent {
        VkBuffer       pDstBuffer     = VK_NULL_HANDLE;
        const uint8_t* pSrcBufferData = nullptr;
        size_t         SrcBufferSize  = 0;
    };
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

        // uint32_t blockSize;
        // uint32_t maxBlockCount;
        // apemodevk::THandle< VmaPool > hPool;
        // if ( false == apemodevk::AllocateStagingMemoryPool( pParams->pNode, hPool, 128 * 1024 * 1024, maxBlockCount, blockSize ) ) {
        //     apemodevk::platform::DebugBreak( );
        //     return false;
        // }

        apemodevk::HostBufferPool bufferPool;
        bufferPool.Recreate( pParams->pNode,
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

        std::vector< apemodevk::MeshResource >     meshResources;
        std::vector< apemodevk::BufferFillIntent > bufferFillIntents;

        for ( auto pSrcMesh : meshesFb ) {
            /* Scene dstMesh. */
            auto& dstMesh = pScene->meshes[ meshIndex++ ];

            /* Create mesh device asset if needed. */
            auto pMeshAsset = (apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset;
            if ( nullptr == pMeshAsset ) {
                pMeshAsset = apemode_new apemodevk::SceneMeshDeviceAssetVk( );
                dstMesh.pDeviceAsset = pMeshAsset;
            }

            auto pSubmesh = pSrcMesh->submeshes( )->begin( );

            pMeshAsset->VertexCount = pSubmesh->vertex_count( );

            auto offset                  = pSubmesh->position_offset( );
            pMeshAsset->PositionOffset.x = offset.x( );
            pMeshAsset->PositionOffset.y = offset.y( );
            pMeshAsset->PositionOffset.z = offset.z( );

            auto scale                  = pSubmesh->position_scale( );
            pMeshAsset->PositionScale.x = scale.x( );
            pMeshAsset->PositionScale.y = scale.y( );
            pMeshAsset->PositionScale.z = scale.z( );

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

            apemodevk::MeshResource meshResource;

            meshResource.pDstAsset = pMeshAsset;
            meshResource.pSrcMesh  = pSrcMesh;

            meshResources.push_back( meshResource );

            apemodevk::BufferFillIntent bufferFillIntent;

            bufferFillIntent.pDstBuffer     = pMeshAsset->hVertexBuffer.Handle.pBuffer;
            bufferFillIntent.pSrcBufferData = pSrcMesh->vertices( )->data( );
            bufferFillIntent.SrcBufferSize  = pSrcMesh->vertices( )->size( );

            bufferFillIntents.push_back( bufferFillIntent );
            
            bufferFillIntent.pDstBuffer     = pMeshAsset->hIndexBuffer.Handle.pBuffer;
            bufferFillIntent.pSrcBufferData = pSrcMesh->indices( )->data( );
            bufferFillIntent.SrcBufferSize  = pSrcMesh->indices( )->size( );

            bufferFillIntents.push_back( bufferFillIntent );
        }

        std::sort( bufferFillIntents.begin( ),
                   bufferFillIntents.end( ),
                   []( const apemodevk::BufferFillIntent& a,
                       const apemodevk::BufferFillIntent& b ) {
                       return a.SrcBufferSize > b.SrcBufferSize;
                   } );

        meshIndex = 0;
        for ( auto pSrcMesh : meshesFb ) {
            /* Scene dstMesh. */
            auto& dstMesh = pScene->meshes[ meshIndex++ ];

            /* Create dstMesh device asset if needed. */
            auto pMeshAsset = (apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset;

            const uint32_t verticesByteSize = (uint32_t) pSrcMesh->vertices( )->size( );
            const uint32_t indicesByteSize  = (uint32_t) pSrcMesh->indices( )->size( );

            auto verticesSuballocResult = bufferPool.Suballocate( pSrcMesh->vertices( )->Data( ), verticesByteSize );
            auto indicesSuballocResult  = bufferPool.Suballocate( pSrcMesh->indices( )->Data( ), indicesByteSize );

            VkBufferCopy bufferCopy;
            apemodevk::InitializeStruct( bufferCopy );

            bufferCopy.srcOffset = verticesSuballocResult.descBufferInfo.offset;
            bufferCopy.size      = verticesSuballocResult.descBufferInfo.range;

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                 /* Cmd */
                             verticesSuballocResult.descBufferInfo.buffer, /* Src */
                             pMeshAsset->hVertexBuffer.Handle.pBuffer,     /* Dst */
                             1,
                             &bufferCopy );

            bufferCopy.srcOffset = indicesSuballocResult.descBufferInfo.offset;
            bufferCopy.size      = indicesSuballocResult.descBufferInfo.range;

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                /* Cmd */
                             indicesSuballocResult.descBufferInfo.buffer, /* Src */
                             pMeshAsset->hIndexBuffer.Handle.pBuffer,     /* Dst */
                             1,
                             &bufferCopy );
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

        if ( auto pMeshAsset = (const apemodevk::SceneMeshDeviceAssetVk*) dstMesh.pDeviceAsset ) {
            for ( auto& subset : dstMesh.subsets ) {
                auto& mat = pScene->materials[ node.materialIds[ subset.materialId ] ];

                //frameData.color          = mat.baseColorFactor;
                frameData.PositionOffset = pMeshAsset->PositionOffset;
                frameData.PositionScale  = pMeshAsset->PositionScale;
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

                VkBuffer     vertexBuffers[ 1 ] = {pMeshAsset->hVertexBuffer.Handle.pBuffer};
                VkDeviceSize vertexOffsets[ 1 ] = {0};
                vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

                vkCmdBindIndexBuffer( pParams->pCmdBuffer,
                                      pMeshAsset->hIndexBuffer.Handle.pBuffer,
                                      pMeshAsset->IndexOffset,
                                      pMeshAsset->IndexType );

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
