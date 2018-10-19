
#include "SkyboxRendererVk.h"

#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/QueuePools.Vulkan.h>

#include <apemode/platform/AppState.h>
#include <apemode/platform/ArrayUtils.h>

using namespace apemode;
using namespace DirectX;

struct SkyboxUBO {
    XMFLOAT4X4 ProjBias;
    XMFLOAT4X4 InvView;
    XMFLOAT4X4 InvProj;
    XMFLOAT4   Params0;
    XMFLOAT4   Params1;
};

struct SkyboxVertex {
    float position[ 3 ];
    float texcoords[ 2 ];
};

bool apemode::vk::SkyboxRenderer::Recreate( RecreateParameters* pParams ) {
    using namespace apemodevk;

    apemodevk_memory_allocation_scope;
    if ( nullptr == pParams->pNode ) {
        return false;
    }

    pNode = pParams->pNode;

    THandle< VkShaderModule > hVertexShaderModule;
    THandle< VkShaderModule > hFragmentShaderModule;
    {
        auto compiledVertexShaderAsset = pParams->pAssetManager->Acquire( "shaders/.spv/Skybox.vert.spv" );
        auto compiledVertexShader = compiledVertexShaderAsset->GetContentAsBinaryBuffer( );
        pParams->pAssetManager->Release( compiledVertexShaderAsset );
        if ( compiledVertexShader.empty( ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        auto compiledFragmentShaderAsset = pParams->pAssetManager->Acquire( "shaders/.spv/Skybox.frag.spv" );
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

    VkDescriptorSetLayoutBinding bindings[ 2 ];
    InitializeStruct( bindings );

    bindings[ 0 ].binding         = 0;
    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[ 0 ].descriptorCount = 1;

    bindings[ 1 ].binding         = 1;
    bindings[ 1 ].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[ 1 ].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[ 1 ].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = utils::GetArraySizeU( bindings );
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( *pParams->pNode, descSetLayoutCreateInfo ) ) {
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = utils::GetArraySizeU( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( *pParams->pNode, pipelineLayoutCreateInfo ) ) {
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 2 ];
    VkVertexInputBindingDescription        vertexInputBindingDescription[ 1 ];
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    colorBlendStateCreateInfo;
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState[ 1 ];
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo;
    VkPipelineViewportStateCreateInfo      viewportStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   multisampleStateCreateInfo;
    VkDynamicState                         dynamicStateEnables[ 2 ];
    VkPipelineDynamicStateCreateInfo       dynamicStateCreateInfo;
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

    graphicsPipelineCreateInfo.stageCount = utils::GetArraySizeU( shaderStageCreateInfo );
    graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;

    //

    inputAssemblyStateCreateInfo.topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

    //

    dynamicStateEnables[ 0 ]                 = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStateEnables[ 1 ]                 = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
    dynamicStateCreateInfo.dynamicStateCount = utils::GetArraySizeU( dynamicStateEnables );
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //

    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;
    graphicsPipelineCreateInfo.pRasterizationState       = &rasterizationStateCreateInfo;

    //

    colorBlendAttachmentState[ 0 ].colorWriteMask = 0xf;
    colorBlendAttachmentState[ 0 ].blendEnable    = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount     = utils::GetArraySizeU( colorBlendAttachmentState );
    colorBlendStateCreateInfo.pAttachments        = colorBlendAttachmentState;
    graphicsPipelineCreateInfo.pColorBlendState   = &colorBlendStateCreateInfo;

    //

    depthStencilStateCreateInfo.depthTestEnable       = VK_FALSE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_NEVER;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_NEVER;
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
        return false;
    }

    if ( false == hPipeline.Recreate( *pParams->pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        return false;
    }

    Frames.resize( pParams->FrameCount );

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        Frames[ i ].BufferPool.Recreate( pParams->pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );

        if ( false == Frames[ i ].DescriptorSetPool.Recreate( *pParams->pNode, pParams->pDescPool, hDescSetLayout ) ) {
            return false;
        }
    }

    return true;
}

void apemode::vk::SkyboxRenderer::Reset( uint32_t FrameIndex ) {
    apemodevk_memory_allocation_scope;
    Frames[ FrameIndex ].BufferPool.Reset( );
}

bool apemode::vk::SkyboxRenderer::Render( Skybox* pSkybox, RenderParameters* pParams ) {
    using namespace apemodevk;
    apemodevk_memory_allocation_scope;

    const uint32_t FrameIndex = ( pParams->FrameIndex ) % kMaxFrameCount;

    SkyboxUBO skyboxUBO;
    skyboxUBO.InvView   = pParams->InvViewMatrix;
    skyboxUBO.InvProj   = pParams->InvProjMatrix;
    skyboxUBO.ProjBias  = pParams->ProjBiasMatrix;
    skyboxUBO.Params0.x = 0;                               /* u_lerpFactor */
    skyboxUBO.Params0.y = pSkybox->Exposure;               /* u_exposure0 */
    skyboxUBO.Params0.z = float( pSkybox->Dimension );     /* u_textureCubeDim0 */
    skyboxUBO.Params0.w = float( pSkybox->LevelOfDetail ); /* u_textureCubeLod0 */

    auto suballocResult = Frames[ FrameIndex ].BufferPool.TSuballocate( skyboxUBO );
    assert( VK_NULL_HANDLE != suballocResult.DescriptorBufferInfo.buffer );
    suballocResult.DescriptorBufferInfo.range = sizeof( SkyboxUBO );

    VkDescriptorImageInfo skyboxImageInfo;
    InitializeStruct( skyboxImageInfo );
    skyboxImageInfo.imageLayout = pSkybox->eImgLayout;
    skyboxImageInfo.imageView   = pSkybox->pImgView;
    skyboxImageInfo.sampler     = pSkybox->pSampler;

    VkDescriptorSet descriptorSet[ 1 ]  = {nullptr};
    uint32_t        dynamicOffsets[ 1 ] = {0};

    TDescriptorSetBindings< 2 > descSet;
    descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSet.pBinding[ 0 ].BufferInfo      = suballocResult.DescriptorBufferInfo;
    descSet.pBinding[ 1 ].eDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descSet.pBinding[ 1 ].ImageInfo       = skyboxImageInfo;

    descriptorSet[ 0 ]  = Frames[ FrameIndex ].DescriptorSetPool.GetDescriptorSet( &descSet );
    dynamicOffsets[ 0 ] = suballocResult.DynamicOffset;

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
    vkCmdBindDescriptorSets( pParams->pCmdBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             hPipelineLayout,
                             0,
                             utils::GetArraySizeU( descriptorSet ),
                             descriptorSet,
                             utils::GetArraySizeU( dynamicOffsets ),
                             dynamicOffsets );

//#if 1
//    VkBuffer     vertexBuffers[ 1 ] = {hVertexBuffer};
//    VkDeviceSize vertexOffsets[ 1 ] = {0};
//    vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );
//#endif

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
    vkCmdDraw( pParams->pCmdBuffer, 3, 1, 0, 0 );

    return true;
}

void apemode::vk::SkyboxRenderer::Flush( uint32_t FrameIndex ) {
    Frames[ FrameIndex ].BufferPool.Flush( );
}
