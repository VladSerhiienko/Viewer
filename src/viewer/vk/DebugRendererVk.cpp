#include "DebugRendererVk.h"

#include <apemode/platform/ArrayUtils.h>
#include <apemode/vk/TInfoStruct.Vulkan.h>
#include <viewer/Scene.h>

namespace apemode {
    using namespace apemodevk;
}

bool apemode::vk::DebugRenderer::RecreateResources( InitParameters* pParams ) {
    apemodevk_memory_allocation_scope;

    if ( nullptr == pParams )
        return false;

    apemodevk::GraphicsDevice* pNode = pParams->pNode;

    THandle< VkShaderModule > hVertexShaderModule;
    THandle< VkShaderModule > hFragmentShaderModule;
    {
        auto compiledVertexShaderAsset = pParams->pAssetManager->Acquire( "shaders/spv/Debug.vert.spv" );
        auto compiledVertexShader = compiledVertexShaderAsset->GetContentAsBinaryBuffer( );
        pParams->pAssetManager->Release( compiledVertexShaderAsset );
        if ( compiledVertexShader.empty( ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        auto compiledFragmentShaderAsset = pParams->pAssetManager->Acquire( "shaders/spv/Debug.frag.spv" );
        auto compiledFragmentShader = compiledFragmentShaderAsset->GetContentAsBinaryBuffer( );
        pParams->pAssetManager->Release( compiledFragmentShaderAsset );
        if ( compiledFragmentShader.empty() ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        VkShaderModuleCreateInfo vertexShaderCreateInfo;
        InitializeStruct( vertexShaderCreateInfo );
        vertexShaderCreateInfo.pCode    = reinterpret_cast< const uint32_t* >( compiledVertexShader.data( ) );
        vertexShaderCreateInfo.codeSize = compiledVertexShader.size( );

        VkShaderModuleCreateInfo fragmentShaderCreateInfo;
        InitializeStruct( fragmentShaderCreateInfo );
        fragmentShaderCreateInfo.pCode    = reinterpret_cast< const uint32_t* >( compiledFragmentShader.data( ) );
        fragmentShaderCreateInfo.codeSize = compiledFragmentShader.size( );

        if ( !hVertexShaderModule.Recreate( pNode->hLogicalDevice, vertexShaderCreateInfo ) ||
             !hFragmentShaderModule.Recreate( pNode->hLogicalDevice, fragmentShaderCreateInfo ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }
    }

    VkDescriptorSetLayoutBinding bindings[ 1 ];
    InitializeStruct( bindings );

    bindings[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[ 0 ].descriptorCount = 1;
    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = 1;
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( pNode->hLogicalDevice, descSetLayoutCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = GetArraySize( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( pNode->hLogicalDevice, pipelineLayoutCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 1 ];
    VkVertexInputBindingDescription        vertexInputBindingDescription[ 1 ];
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    colorBlendStateCreateInfo;
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState[ 1 ];
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo;
    VkPipelineViewportStateCreateInfo      viewportStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   multisampleStateCreateInfo;
    VkDynamicState                         dynamicStateEnables[ 3 ];
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

    shaderStageCreateInfo[ 0 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[ 0 ].module = hVertexShaderModule;
    shaderStageCreateInfo[ 0 ].pName  = "main";

    shaderStageCreateInfo[ 1 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[ 1 ].module = hFragmentShaderModule;
    shaderStageCreateInfo[ 1 ].pName  = "main";

    graphicsPipelineCreateInfo.stageCount = GetArraySize( shaderStageCreateInfo );
    graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

    //

    vertexInputBindingDescription[ 0 ].stride    = sizeof( PositionVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (PositionVertex*) 0 )->Position );

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
    dynamicStateEnables[ 2 ]                 = VK_DYNAMIC_STATE_LINE_WIDTH;
    dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
    dynamicStateCreateInfo.dynamicStateCount = GetArraySize( dynamicStateEnables );
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    
    #if __APPLE__
    // No support for wide lines
    dynamicStateCreateInfo.dynamicStateCount = GetArraySize( dynamicStateEnables ) - 1;
    #endif

    //

    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    depthStencilStateCreateInfo.depthTestEnable       = VK_FALSE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
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

    rasterizationStateCreateInfo.lineWidth = 1;

    if ( false == hPipelineCache.Recreate( pNode->hLogicalDevice, pipelineCacheCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    if ( false == hPipeline.Recreate( pNode->hLogicalDevice, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    //

    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    rasterizationStateCreateInfo.lineWidth = 3;

    if ( false == hLinePipelineCache.Recreate( pNode->hLogicalDevice, pipelineCacheCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    if ( false == hLinePipeline.Recreate( pNode->hLogicalDevice, hLinePipelineCache, graphicsPipelineCreateInfo ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    if (  nullptr == hCubeBuffer.Handle.pBuffer ) {

        // clang-format off
        const float vertexBufferData[] = {
            // -X side
            -1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,

            // -Z side
            -1.0f,-1.0f,-1.0f,
             1.0f, 1.0f,-1.0f,
             1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
             1.0f, 1.0f,-1.0f,

             // -Y side
             -1.0f,-1.0f,-1.0f,
             1.0f,-1.0f,-1.0f,
             1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
             1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,

            // +Y side
            -1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
             1.0f, 1.0f, 1.0f,
             1.0f, 1.0f,-1.0f,

             // +X side
             1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,

            // +Z side
            -1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
             1.0f,-1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
        };
        // clang-format on

        const uint32_t vertexBufferSize = sizeof( vertexBufferData );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.size  = vertexBufferSize;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if ( false == hCubeBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        if ( auto mappedData = hCubeBuffer.Handle.AllocationInfo.pMappedData ) {
            memcpy( mappedData, vertexBufferData, vertexBufferSize );
        }
    }

    VkPhysicalDeviceProperties adapterProps;
    vkGetPhysicalDeviceProperties( pNode->pPhysicalDevice, &adapterProps );

    for (uint32_t i = 0; i < pParams->FrameCount; ++i) {
        BufferPools[ i ].Recreate( pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false );
        DescSetPools[ i ].Recreate( pNode->hLogicalDevice, pParams->pDescPool, hDescSetLayout );
    }

    return true;
}

void apemode::vk::DebugRenderer::Reset( uint32_t frameIndex ) {
    BufferPools[ frameIndex ].Reset( );
}

bool apemode::vk::DebugRenderer::Render( const RenderCubeParameters* renderParams ) {
    apemodevk_memory_allocation_scope;

    auto frameIndex = ( renderParams->FrameIndex ) % kMaxFrameCount;

    auto suballocResult = BufferPools[ frameIndex ].TSuballocate( *renderParams->pFrameData );
    assert( VK_NULL_HANDLE != suballocResult.DescriptorBufferInfo.buffer );
    suballocResult.DescriptorBufferInfo.range = sizeof( DebugUBO );

    VkDescriptorSet descriptorSet[ 1 ]  = {nullptr};

    TDescriptorSetBindings< 1 > descSet;
    descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSet.pBinding[ 0 ].BufferInfo      = suballocResult.DescriptorBufferInfo;

    descriptorSet[ 0 ]  = DescSetPools[ frameIndex ].GetDescriptorSet( &descSet );

    vkCmdBindPipeline( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
    vkCmdBindDescriptorSets( renderParams->pCmdBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             hPipelineLayout,
                             0,
                             1,
                             descriptorSet,
                             1,
                             &suballocResult.DynamicOffset );

    VkBuffer     vertexBuffers[ 1 ] = {hCubeBuffer.Handle.pBuffer};
    VkDeviceSize vertexOffsets[ 1 ] = {0};
    vkCmdBindVertexBuffers( renderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

    VkViewport viewport;
    InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = renderParams->Dims[ 0 ] * renderParams->Scale[ 0 ];
    viewport.height   = renderParams->Dims[ 1 ] * renderParams->Scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( renderParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = (uint32_t)(renderParams->Dims[ 0 ] * renderParams->Scale[ 0 ]);
    scissor.extent.height = (uint32_t)(renderParams->Dims[ 1 ] * renderParams->Scale[ 1 ]);

    vkCmdSetScissor( renderParams->pCmdBuffer, 0, 1, &scissor );
    #if !__APPLE__
    vkCmdSetLineWidth( renderParams->pCmdBuffer, renderParams->LineWidth );
    #endif
    vkCmdDraw( renderParams->pCmdBuffer, 12 * 3, 1, 0, 0 );

    return true;
}

void PopulateNodeConnectionLines( const apemode::Scene*                   pScene,
                                  const apemode::SceneNodeTransformFrame* pTransformFrame,
                                  const uint32_t                          nodeId,
                                  apemodevk::vector< apemode::XMFLOAT3 >* pOutLines ) {
    using namespace apemode;
    if ( pScene && pTransformFrame && pOutLines ) {
        auto& nodeTransformComposite = pTransformFrame->Transforms[ nodeId ];

        XMFLOAT4X4 nodeWorldMatrix;
        XMFLOAT4X4 childWorldMatrix;

        XMStoreFloat4x4( &nodeWorldMatrix, nodeTransformComposite.WorldMatrix );
        XMFLOAT3 nodeWorldPosition{nodeWorldMatrix._41, nodeWorldMatrix._42, nodeWorldMatrix._43};

        const auto childIdRange = pScene->NodeToChildIds.equal_range( nodeId );
        for ( auto childIdIt = childIdRange.first; childIdIt != childIdRange.second; ++childIdIt ) {
            const uint32_t childId                 = childIdIt->second;
            const auto&    childTransformComposite = pTransformFrame->Transforms[ childId ];

            XMStoreFloat4x4( &childWorldMatrix, childTransformComposite.WorldMatrix );
            XMFLOAT3 childWorldPosition{childWorldMatrix._41, childWorldMatrix._42, childWorldMatrix._43};

            pOutLines->push_back( nodeWorldPosition );
            pOutLines->push_back( childWorldPosition );

            PopulateNodeConnectionLines( pScene, pTransformFrame, childId, pOutLines );
        }
    }
}

bool apemode::vk::DebugRenderer::Render( const Scene* pScene, const RenderSceneParameters* pRenderParams ) {
    if ( !pRenderParams || !pRenderParams->pTransformFrame ) {
        return true;
    }

    LineStagingBuffer.clear( );
    LineStagingBuffer.reserve( pScene->Nodes.size( ) << 1 );
    PopulateNodeConnectionLines( pScene, pRenderParams->pTransformFrame, 0, &LineStagingBuffer );

    apemodevk_memory_allocation_scope;

    auto frameIndex = ( pRenderParams->FrameIndex ) % kMaxFrameCount;

    DebugUBO linesUBO;
    linesUBO.WorldMatrix = pRenderParams->RootMatrix;
    linesUBO.ViewMatrix  = pRenderParams->ViewMatrix;
    linesUBO.ProjMatrix  = pRenderParams->ProjMatrix;
    linesUBO.Color       = pRenderParams->SceneColorOverride;

    auto linesSuballocResult = BufferPools[ frameIndex ].TSuballocate( linesUBO );
    assert( VK_NULL_HANDLE != linesSuballocResult.DescriptorBufferInfo.buffer );
    linesSuballocResult.DescriptorBufferInfo.range = sizeof( linesUBO );

    auto linesStagingSuballocResult = BufferPools[ frameIndex ].Suballocate(
        LineStagingBuffer.data( ), LineStagingBuffer.size( ) * sizeof( PositionVertex ) );
    assert( VK_NULL_HANDLE != linesStagingSuballocResult.DescriptorBufferInfo.buffer );
    linesStagingSuballocResult.DescriptorBufferInfo.range = LineStagingBuffer.size( ) * sizeof( PositionVertex );

    VkDescriptorSet descriptorSet[ 1 ] = {nullptr};

    TDescriptorSetBindings< 1 > descSet;
    descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSet.pBinding[ 0 ].BufferInfo      = linesSuballocResult.DescriptorBufferInfo;

    descriptorSet[ 0 ] = DescSetPools[ frameIndex ].GetDescriptorSet( &descSet );

    vkCmdBindPipeline( pRenderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hLinePipeline );
    vkCmdBindDescriptorSets( pRenderParams->pCmdBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             hPipelineLayout,
                             0,
                             1,
                             descriptorSet,
                             1,
                             &linesSuballocResult.DynamicOffset );

    VkBuffer     vertexBuffers[ 1 ] = {linesStagingSuballocResult.DescriptorBufferInfo.buffer};
    VkDeviceSize vertexOffsets[ 1 ] = {linesStagingSuballocResult.DynamicOffset};
    vkCmdBindVertexBuffers( pRenderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

    VkViewport viewport;
    InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pRenderParams->Dims.x * pRenderParams->Scale.x;
    viewport.height   = pRenderParams->Dims.y * pRenderParams->Scale.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( pRenderParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = viewport.width;
    scissor.extent.height = viewport.height;

    vkCmdSetScissor( pRenderParams->pCmdBuffer, 0, 1, &scissor );
    #if !__APPLE__
    vkCmdSetLineWidth( pRenderParams->pCmdBuffer, pRenderParams->LineWidth );
    #endif
    vkCmdDraw( pRenderParams->pCmdBuffer, LineStagingBuffer.size( ), 1, 0, 0 );

    return true;
}

void apemode::vk::DebugRenderer::Flush( uint32_t frameIndex ) {
    BufferPools[ frameIndex ].Flush( );
}
