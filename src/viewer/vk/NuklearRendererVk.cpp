#include <viewer/vk/NuklearRendererVk.h>
#include <apemode/platform/ArrayUtils.h>
#include <apemode/platform/AppState.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>

namespace apemode {
    using namespace apemodevk;
}

struct NuklearPC {
    float OffsetScale[ 4 ];
};

apemode::vk::NuklearRenderer::~NuklearRenderer( ) {
    apemode::LogInfo( "NuklearRenderer: Destroying." );
    Shutdown( );
}

void apemode::vk::NuklearRenderer::DeviceDestroy( ) {
    apemode_memory_allocation_scope;

    apemode::LogInfo( "NuklearRenderer: Destroying device resources." );

    for ( auto& frame : Frames ) {
        frame.hVertexBuffer.Destroy( );
        frame.hIndexBuffer.Destroy( );
    }

    hPipeline.Destroy( );
    hPipelineCache.Destroy( );
    hDescSetLayout.Destroy( );
    hPipelineLayout.Destroy( );

    NuklearRendererBase::DeviceDestroy( );
}

bool apemode::vk::NuklearRenderer::Render( RenderParametersBase* pRenderParamsBase ) {
    apemode_memory_allocation_scope;

    auto pRenderParams = (RenderParameters*) pRenderParamsBase;
    const uint32_t frameIndex = pRenderParams->FrameIndex;
    auto& frame = Frames[ frameIndex ];

    if ( frame.hVertexBuffer.Handle.AllocationInfo.size < pRenderParamsBase->MaxVertexBufferSize ) {
        frame.hVertexBuffer.Destroy( );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );

        bufferCreateInfo.size        = pRenderParamsBase->MaxVertexBufferSize;
        bufferCreateInfo.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if ( !frame.hVertexBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
            return false;
        }
    }

    if ( frame.hIndexBuffer.Handle.AllocationInfo.size < pRenderParamsBase->MaxElementBufferSize ) {
        frame.hIndexBuffer.Destroy( );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );

        bufferCreateInfo.size        = pRenderParamsBase->MaxElementBufferSize;
        bufferCreateInfo.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        if ( !frame.hIndexBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
            return false;
        }
    }

    /* Convert from command queue into draw list and draw to screen */
    void* vertices = nullptr;
    void* elements = nullptr;

    /* Load vertices/elements directly into vertex/element buffer */
    vertices = frame.hVertexBuffer.Handle.AllocationInfo.pMappedData;
    elements = frame.hIndexBuffer.Handle.AllocationInfo.pMappedData;

    /* Fill convert configuration */
    struct nk_convert_config config;
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, pos )},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, uv )},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF( Vertex, col )},
        {NK_VERTEX_LAYOUT_END}};

    memset( &config, 0, sizeof( config ) );
    config.vertex_layout        = vertex_layout;
    config.vertex_size          = sizeof( Vertex );
    config.vertex_alignment     = NK_ALIGNOF( Vertex );
    config.null                 = NullTexture;
    config.circle_segment_count = 22;
    config.curve_segment_count  = 22;
    config.arc_segment_count    = 22;
    config.global_alpha         = 1.0f;
    config.shape_AA             = pRenderParamsBase->eAntiAliasing;
    config.line_AA              = pRenderParamsBase->eAntiAliasing;

    /* Setup buffers to load vertices and elements */
    nk_buffer vbuf, ebuf;
    nk_buffer_init_fixed( &vbuf, vertices, (nk_size) pRenderParamsBase->MaxVertexBufferSize );
    nk_buffer_init_fixed( &ebuf, elements, (nk_size) pRenderParamsBase->MaxElementBufferSize );
    nk_convert( &Context, &RenderCmds, &vbuf, &ebuf, &config );

    /* Bind pipeline and descriptor sets */
    {
        TDescriptorSetBindings< 1 > descriptorSet;
        descriptorSet.pBinding[ 0 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSet.pBinding[ 0 ].ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorSet.pBinding[ 0 ].ImageInfo.imageView   = FontUploadedImg->hImgView;
        descriptorSet.pBinding[ 0 ].ImageInfo.sampler     = hFontSampler;

        VkDescriptorSet pDescriptorSet[ 1 ] = {frame.DescSetPool.GetDescriptorSet( &descriptorSet )};
        vkCmdBindPipeline( pRenderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
        vkCmdBindDescriptorSets( pRenderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipelineLayout, 0, 1, pDescriptorSet, 0, NULL );
    }

    /* Bind Vertex And Index Buffer */
    {
        VkBuffer     vertexBuffers[ 1 ] = {frame.hVertexBuffer.Handle.pBuffer};
        VkDeviceSize vertexOffsets[ 1 ] = {0};
        vkCmdBindVertexBuffers( pRenderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );
        vkCmdBindIndexBuffer( pRenderParams->pCmdBuffer, frame.hIndexBuffer.Handle.pBuffer, 0, VK_INDEX_TYPE_UINT16 );
    }

    VkViewport viewport;
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pRenderParamsBase->Dims[ 0 ] * pRenderParamsBase->Scale[ 0 ];
    viewport.height   = pRenderParamsBase->Dims[ 1 ] * pRenderParamsBase->Scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( pRenderParams->pCmdBuffer, 0, 1, &viewport );

    /* Setup scale and translation */
    {
        NuklearPC nuklearPC;
        nuklearPC.OffsetScale[ 0 ] = -1;                                  /* Translation X */
        nuklearPC.OffsetScale[ 1 ] = -1;                                  /* Translation Y */
        nuklearPC.OffsetScale[ 2 ] = 2.0f / pRenderParamsBase->Dims[ 0 ]; /* Scaling X */
        nuklearPC.OffsetScale[ 3 ] = 2.0f / pRenderParamsBase->Dims[ 1 ]; /* Scaling Y */

        vkCmdPushConstants( pRenderParams->pCmdBuffer, hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( nuklearPC ), &nuklearPC );
    }

    const nk_draw_command* cmd        = nullptr;
    uint32_t               offset     = 0;
    uint32_t               vtx_offset = 0;

    /* Iterate over and execute each draw command */
    nk_draw_foreach( cmd, &Context, &RenderCmds ) {
        if ( !cmd->elem_count )
            continue;

        VkRect2D scissor;

        scissor.offset.x      = (  int32_t )( cmd->clip_rect.x * pRenderParamsBase->Scale[ 0 ] );
        scissor.offset.y      = (  int32_t )( cmd->clip_rect.y * pRenderParamsBase->Scale[ 1 ] );
        scissor.extent.width  = ( uint32_t )( cmd->clip_rect.w * pRenderParamsBase->Scale[ 0 ] );
        scissor.extent.height = ( uint32_t )( cmd->clip_rect.h * pRenderParamsBase->Scale[ 1 ] );

        scissor.offset.x      = std::max< int32_t >( 0, scissor.offset.x );
        scissor.offset.y      = std::max< int32_t >( 0, scissor.offset.y );
        scissor.extent.width  = std::min< uint32_t >( (uint32_t) viewport.width, scissor.extent.width );
        scissor.extent.height = std::min< uint32_t >( (uint32_t) viewport.height, scissor.extent.height );

        vkCmdSetScissor( pRenderParams->pCmdBuffer, 0, 1, &scissor );
        vkCmdDrawIndexed( pRenderParams->pCmdBuffer, cmd->elem_count, 1, offset, vtx_offset, 0 );

        offset += cmd->elem_count;
    }

    return true;
}

bool apemode::vk::NuklearRenderer::DeviceCreate( InitParametersBase* pInitParamsBase ) {
    apemode_memory_allocation_scope;

    auto pParams = (InitParameters*) pInitParamsBase;
    if ( nullptr == pParams ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    pNode = pParams->pNode;
    Frames.resize(pParams->FrameCount);

    THandle< VkShaderModule > hVertexShaderModule;
    THandle< VkShaderModule > hFragmentShaderModule;
    {
        auto compiledVertexShaderAsset = pParams->pAssetManager->Acquire( "shaders/spv/NuklearUI.vert.spv" );
        auto compiledVertexShader = compiledVertexShaderAsset->GetContentAsBinaryBuffer( );
        pParams->pAssetManager->Release( compiledVertexShaderAsset );
        if ( compiledVertexShader.empty( ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        auto compiledFragmentShaderAsset = pParams->pAssetManager->Acquire( "shaders/spv/NuklearUI.frag.spv" );
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

    if ( pNode ) {
        VkSamplerCreateInfo fontSamplerCreateInfo;
        InitializeStruct( fontSamplerCreateInfo );
        fontSamplerCreateInfo.magFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo.minFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        fontSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.minLod       = -1000;
        fontSamplerCreateInfo.maxLod       = +1000;

        hFontSampler = pParams->pSamplerManager->GetSampler( fontSamplerCreateInfo );
        if ( !hFontSampler ) {
            return false;
        }

        VkDescriptorSetLayoutBinding bindings[ 1 ];
        InitializeStruct( bindings );

        bindings[ 0 ].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[ 0 ].descriptorCount    = 1;
        bindings[ 0 ].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[ 0 ].pImmutableSamplers = &hFontSampler;

        VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
        InitializeStruct( descSetLayoutCreateInfo );
        descSetLayoutCreateInfo.bindingCount = 1;
        descSetLayoutCreateInfo.pBindings    = bindings;

        if ( false == hDescSetLayout.Recreate( pNode->hLogicalDevice, descSetLayoutCreateInfo ) ) {
            return false;
        }

        for ( auto& frame : Frames ) {
            if ( !frame.DescSetPool.Recreate( *pParams->pNode, pParams->pDescPool, hDescSetLayout ) ) {
                return false;
            }

            if ( frame.hVertexBuffer.IsNull( ) ) {
                VkBufferCreateInfo bufferCreateInfo;
                InitializeStruct( bufferCreateInfo );

                bufferCreateInfo.size        = pParams->pNode->AdapterProps.limits.maxUniformBufferRange;
                bufferCreateInfo.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VmaAllocationCreateInfo allocationCreateInfo = {};
                allocationCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;
                allocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                if ( !frame.hVertexBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
                    return false;
                }
            }

            if ( frame.hIndexBuffer.IsNull( ) ) {
                VkBufferCreateInfo bufferCreateInfo;
                InitializeStruct( bufferCreateInfo );

                bufferCreateInfo.size        = pParams->pNode->AdapterProps.limits.maxUniformBufferRange;
                bufferCreateInfo.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                VmaAllocationCreateInfo allocationCreateInfo = {};
                allocationCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;
                allocationCreateInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                if ( !frame.hIndexBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, allocationCreateInfo ) ) {
                    return false;
                }
            }
        }

        VkPushConstantRange pushConstant;
        InitializeStruct( pushConstant );
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.size       = sizeof( NuklearPC );

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        InitializeStruct( pipelineLayoutCreateInfo );
        pipelineLayoutCreateInfo.setLayoutCount         = 1;
        pipelineLayoutCreateInfo.pSetLayouts            = hDescSetLayout.GetAddressOf( );
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

        if ( false == hPipelineLayout.Recreate( pNode->hLogicalDevice, pipelineLayoutCreateInfo ) ) {
            return false;
        }
    }

    VkPipelineShaderStageCreateInfo pipelineStages[ 2 ];
    InitializeStruct( pipelineStages );

    pipelineStages[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineStages[ 0 ].module = hVertexShaderModule;
    pipelineStages[ 0 ].pName  = "main";

    pipelineStages[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineStages[ 1 ].module = hFragmentShaderModule;
    pipelineStages[ 1 ].pName  = "main";

    VkVertexInputBindingDescription bindingDescs[ 1 ];
    InitializeStruct( bindingDescs );
    bindingDescs[ 0 ].stride    = sizeof( Vertex );
    bindingDescs[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttributeDesc[ 3 ];
    InitializeStruct( inputAttributeDesc );

    inputAttributeDesc[ 0 ].location = 0;
    inputAttributeDesc[ 0 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 0 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 0 ].offset   = ( size_t )( &( (Vertex*) 0 )->pos );

    inputAttributeDesc[ 1 ].location = 1;
    inputAttributeDesc[ 1 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 1 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 1 ].offset   = ( size_t )( &( (Vertex*) 0 )->uv );

    inputAttributeDesc[ 2 ].location = 2;
    inputAttributeDesc[ 2 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 2 ].format   = VK_FORMAT_R8G8B8A8_UNORM;
    inputAttributeDesc[ 2 ].offset   = ( size_t )( &( (Vertex*) 0 )->col );

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputState;
    InitializeStruct( pipelineVertexInputState );
    pipelineVertexInputState.vertexBindingDescriptionCount   = GetArraySize( bindingDescs );
    pipelineVertexInputState.pVertexBindingDescriptions      = bindingDescs;
    pipelineVertexInputState.vertexAttributeDescriptionCount = GetArraySize( inputAttributeDesc );
    pipelineVertexInputState.pVertexAttributeDescriptions    = inputAttributeDesc;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyState;
    InitializeStruct( pipelineInputAssemblyState );
    pipelineInputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo pipelineViewportState;
    InitializeStruct( pipelineViewportState );
    pipelineViewportState.viewportCount = 1;
    pipelineViewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationState;
    InitializeStruct( pipelineRasterizationState );
    pipelineRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationState.cullMode    = VK_CULL_MODE_NONE;
    pipelineRasterizationState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationState.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleState;
    InitializeStruct( pipelineMultisampleState );
    pipelineMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
    InitializeStruct( pipelineColorBlendAttachmentState );
    pipelineColorBlendAttachmentState.blendEnable         = VK_TRUE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT
                                                          | VK_COLOR_COMPONENT_G_BIT
                                                          | VK_COLOR_COMPONENT_B_BIT
                                                          | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilState;
    InitializeStruct( pipelineDepthStencilState );

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendState;
    InitializeStruct( pipelineColorBlendState );
    pipelineColorBlendState.attachmentCount = 1;
    pipelineColorBlendState.pAttachments    = &pipelineColorBlendAttachmentState;

    VkDynamicState dynamicStates[ 2 ] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipelineDynamicState;
    InitializeStruct( pipelineDynamicState );
    pipelineDynamicState.dynamicStateCount = GetArraySize( dynamicStates );
    pipelineDynamicState.pDynamicStates    = dynamicStates;

    VkGraphicsPipelineCreateInfo graphicsPipeline;
    InitializeStruct( graphicsPipeline );
    graphicsPipeline.stageCount          = GetArraySize( pipelineStages );
    graphicsPipeline.pStages             = pipelineStages;
    graphicsPipeline.pVertexInputState   = &pipelineVertexInputState;
    graphicsPipeline.pInputAssemblyState = &pipelineInputAssemblyState;
    graphicsPipeline.pViewportState      = &pipelineViewportState;
    graphicsPipeline.pRasterizationState = &pipelineRasterizationState;
    graphicsPipeline.pMultisampleState   = &pipelineMultisampleState;
    graphicsPipeline.pDepthStencilState  = &pipelineDepthStencilState;
    graphicsPipeline.pColorBlendState    = &pipelineColorBlendState;
    graphicsPipeline.pDynamicState       = &pipelineDynamicState;
    graphicsPipeline.layout              = hPipelineLayout;
    graphicsPipeline.renderPass          = pParams->pRenderPass;

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
    InitializeStruct( pipelineCacheCreateInfo );
    hPipelineCache.Recreate( pNode->hLogicalDevice, pipelineCacheCreateInfo );

    if ( false == hPipeline.Recreate( pNode->hLogicalDevice, hPipelineCache, graphicsPipeline ) ) {
        apemodevk::platform::DebugBreak( );
        return false;
    }

    return true;
}

void* apemode::vk::NuklearRenderer::DeviceUploadAtlas( InitParametersBase* init_params,
                                                       const void*         image,
                                                       uint32_t            width,
                                                       uint32_t            height ) {

    InitParameters* pParams       = reinterpret_cast< InitParameters* >( init_params );
    const uint8_t*    fontImgPixels = reinterpret_cast< const uint8_t* >( image );

    if ( !pParams || !fontImgPixels ) {
        return nullptr;
    }

    apemodevk::ImageDecoder                imageDecoder;
    apemodevk::ImageDecoder::DecodeOptions decodeOptions;
    decodeOptions.bGenerateMipMaps = false;

    auto fontSrcImg = imageDecoder.CreateSourceImage2D( fontImgPixels, VkExtent2D{width, height}, VK_FORMAT_R8G8B8A8_UNORM, decodeOptions );
    if ( !fontSrcImg ) {
        return nullptr;
    }

    apemodevk::ImageUploader                imageUploader;
    apemodevk::ImageUploader::UploadOptions uploadOptions;
    uploadOptions.bImgView = true;

    FontUploadedImg = eastl::move( imageUploader.UploadImage( pParams->pNode, *fontSrcImg, uploadOptions ) );
    return FontUploadedImg ? (void*) FontUploadedImg->hImg : nullptr;
}
