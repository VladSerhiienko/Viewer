
#include <SceneRendererVk.h>
#include <SceneUploaderVk.h>

#include <Buffer.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <QueuePools.Vulkan.h>
#include <TOneTimeCmdBufferSubmit.Vulkan.h>

#include <AppState.h>
#include <ArrayUtils.h>
#include <MathInc.h>
#include <Scene.h>

namespace apemodevk {

using namespace apemodexm;

struct ObjectUBO {
    XMFLOAT4X4 WorldMatrix;
    XMFLOAT4X4 NormalMatrix;
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
    XMUINT4  Flags;
};

struct LightUBO {
    XMFLOAT4 LightDirection;
    XMFLOAT4 LightColor;
};

bool FillCombinedImgSamplerBinding( apemodevk::DescriptorSetBindingsBase::Binding* pBinding,
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
        }

        if ( pMissingImgView && pMissingSampler ) {
            pBinding->eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pBinding->ImageInfo.imageLayout = eImgLayout;
            pBinding->ImageInfo.imageView   = pMissingImgView;
            pBinding->ImageInfo.sampler     = pMissingSampler;
            return true;
        }
    }

    return false;
}
} // namespace apemodevk

bool apemode::vk::SceneRenderer::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {
    using namespace apemodevk;

    /* Scene was not provided.
     * Render parameters were not provided. */
    if ( nullptr == pScene || nullptr == pParamsBase ) {
        return false;
    }

    auto pSceneAsset = static_cast< const vk::SceneUploader::DeviceAsset* >( pScene->pDeviceAsset.get( ) );
    auto pParams     = static_cast< const RenderParameters* >( pParamsBase );

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
    auto& frame = Frames[ frameIndex ];

    VkDescriptorSet ppDescriptorSets[ 2 ] = {nullptr};
    uint32_t        pDynamicOffsets[ 4 ]  = {0};

    CameraUBO cameraData;
    cameraData.ViewMatrix    = pParams->ViewMatrix;
    cameraData.ProjMatrix    = pParams->ProjMatrix;
    cameraData.InvViewMatrix = pParams->InvViewMatrix;
    cameraData.InvProjMatrix = pParams->InvProjMatrix;

    LightUBO lightData;
    lightData.LightDirection = pParams->LightDirection;
    lightData.LightColor     = pParams->LightColor;

    auto cameraDataUploadBufferRange = frame.BufferPool.TSuballocate( cameraData );
    assert( VK_NULL_HANDLE != cameraDataUploadBufferRange.DescriptorBufferInfo.buffer );
    cameraDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( CameraUBO );

    auto lightDataUploadBufferRange = frame.BufferPool.TSuballocate( lightData );
    assert( VK_NULL_HANDLE != lightDataUploadBufferRange.DescriptorBufferInfo.buffer );
    lightDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( LightUBO );

    TDescriptorSetBindings< 4 > descriptorSetForPass;

    descriptorSetForPass.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 0 */
    descriptorSetForPass.pBinding[ 0 ].BufferInfo      = cameraDataUploadBufferRange.DescriptorBufferInfo;

    descriptorSetForPass.pBinding[ 1 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 1 */
    descriptorSetForPass.pBinding[ 1 ].BufferInfo      = lightDataUploadBufferRange.DescriptorBufferInfo;

    descriptorSetForPass.pBinding[ 2 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; /* 2 */
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.imageLayout = pParams->RadianceMap.eImgLayout;
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.imageView   = pParams->RadianceMap.pImgView;
    descriptorSetForPass.pBinding[ 2 ].ImageInfo.sampler     = pParams->RadianceMap.pSampler;

    descriptorSetForPass.pBinding[ 3 ].eDescriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; /* 3 */
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.imageLayout = pParams->IrradianceMap.eImgLayout;
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.imageView   = pParams->IrradianceMap.pImgView;
    descriptorSetForPass.pBinding[ 3 ].ImageInfo.sampler     = pParams->IrradianceMap.pSampler;

    ppDescriptorSets[ kDescriptorSetForPass ] =
        frame.DescriptorSetPools[ kDescriptorSetForPass ].GetDescriptorSet( &descriptorSetForPass );

    pDynamicOffsets[ 0 ] = cameraDataUploadBufferRange.DynamicOffset;
    pDynamicOffsets[ 1 ] = lightDataUploadBufferRange.DynamicOffset;

    for ( auto& node : pScene->Nodes ) {
        if ( node.MeshId >= pScene->Meshes.size( ) )
            continue;

        auto& mesh = pScene->Meshes[ node.MeshId ];

        auto pMeshAsset = (const vk::SceneUploader::MeshDeviceAsset*) mesh.pDeviceAsset.get( );
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

        const XMMATRIX rootMatrix   = XMLoadFloat4x4( &pParams->RootMatrix );
        const XMMATRIX worldMatrix  = pScene->BindPoseFrame.Transforms[ node.Id ].WorldMatrix * rootMatrix;
        const XMMATRIX normalMatrix = XMMatrixTranspose( XMMatrixInverse( nullptr, worldMatrix ) );

        XMStoreFloat4x4( &objectData.WorldMatrix, worldMatrix );
        XMStoreFloat4x4( &objectData.NormalMatrix, normalMatrix );

        auto pSubsetIt    = pScene->Subsets.data( ) + mesh.BaseSubset;
        auto pSubsetItEnd = pSubsetIt + mesh.SubsetCount;

        for ( ; pSubsetIt != pSubsetItEnd; ++pSubsetIt ) {
            const SceneMaterial*                          pMaterial      = nullptr;
            const vk::SceneUploader::MaterialDeviceAsset* pMaterialAsset = nullptr;

            if ( pSubsetIt->MaterialId != uint32_t( -1 ) ) {
                assert( pSubsetIt->MaterialId < pScene->Materials.size( ) );
                pMaterial      = &pScene->Materials[ pSubsetIt->MaterialId ];
                pMaterialAsset = static_cast< vk::SceneUploader::MaterialDeviceAsset const* >( pMaterial->pDeviceAsset.get( ) );
            } else {
                pMaterial      = &pSceneAsset->MissingMaterial;
                pMaterialAsset = &pSceneAsset->MissingMaterialAsset;
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
            materialData.MetallicRoughnessFactor.z = 1;
            materialData.MetallicRoughnessFactor.w = static_cast< float >( pParams->RadianceMap.MipLevels );
            materialData.Flags.x                   = flags;

            auto objectDataUploadBufferRange = frame.BufferPool.TSuballocate( objectData );
            assert( VK_NULL_HANDLE != objectDataUploadBufferRange.DescriptorBufferInfo.buffer );
            objectDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( ObjectUBO );

            auto materialDataUploadBufferRange = frame.BufferPool.TSuballocate( materialData );
            assert( VK_NULL_HANDLE != materialDataUploadBufferRange.DescriptorBufferInfo.buffer );
            materialDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( MaterialUBO );

            TDescriptorSetBindings< 8 > descriptorSetForObject( eTDescriptorSetNoInit );

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

            ppDescriptorSets[ kDescriptorSetForObj ] =
                frame.DescriptorSetPools[ kDescriptorSetForObj ].GetDescriptorSet( &descriptorSetForObject );

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

template < uint32_t TShaderStageCount >
struct TGraphicsPipelineCreateInfoComposite {

    VkPipelineShaderStageCreateInfo        Stages[ TShaderStageCount ];
    VkPipelineVertexInputStateCreateInfo   PipelineVertexInputStateCreateInfo;
    VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo;
    VkPipelineTessellationStateCreateInfo  PipelineTessellationStateCreateInfo;
    VkPipelineViewportStateCreateInfo      PipelineViewportStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   PipelineMultisampleStateCreateInfo;
    VkPipelineDepthStencilStateCreateInfo  PipelineDepthStencilStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    PipelineColorBlendStateCreateInfo;
    VkPipelineDynamicStateCreateInfo       PipelineDynamicStateCreateInfo;
    VkGraphicsPipelineCreateInfo           GraphicsPipelineCreateInfo;

    TGraphicsPipelineCreateInfoComposite( ) {
        using namespace apemodevk;

        InitializeStruct( Stages );
        InitializeStruct( PipelineVertexInputStateCreateInfo );
        InitializeStruct( PipelineInputAssemblyStateCreateInfo );
        InitializeStruct( PipelineTessellationStateCreateInfo );
        InitializeStruct( PipelineViewportStateCreateInfo );
        InitializeStruct( PipelineRasterizationStateCreateInfo );
        InitializeStruct( PipelineMultisampleStateCreateInfo );
        InitializeStruct( PipelineDepthStencilStateCreateInfo );
        InitializeStruct( PipelineColorBlendStateCreateInfo );
        InitializeStruct( PipelineDynamicStateCreateInfo );
        InitializeStruct( GraphicsPipelineCreateInfo );

        GraphicsPipelineCreateInfo.stageCount          = utils::GetArraySizeU( Stages );
        GraphicsPipelineCreateInfo.pStages             = &Stages[ 0 ];
        GraphicsPipelineCreateInfo.pVertexInputState   = &PipelineVertexInputStateCreateInfo;
        GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
        GraphicsPipelineCreateInfo.pTessellationState  = &PipelineTessellationStateCreateInfo;
        GraphicsPipelineCreateInfo.pViewportState      = &PipelineViewportStateCreateInfo;
        GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
        GraphicsPipelineCreateInfo.pMultisampleState   = &PipelineMultisampleStateCreateInfo;
        GraphicsPipelineCreateInfo.pDepthStencilState  = &PipelineDepthStencilStateCreateInfo;
        GraphicsPipelineCreateInfo.pColorBlendState    = &PipelineColorBlendStateCreateInfo;
        GraphicsPipelineCreateInfo.pDynamicState       = &PipelineDynamicStateCreateInfo;
    }
};

bool apemode::vk::SceneRenderer::Recreate( const RecreateParametersBase* pParamsBase ) {
    using namespace apemodevk;

    if ( nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (RecreateParameters*) pParamsBase;
    if ( nullptr == pParams->pNode )
        return false;

    pNode = pParams->pNode;

    THandle< VkShaderModule > hVertexShaderModule;
    THandle< VkShaderModule > hFragmentShaderModule;
    {
        auto compiledVertexShaderAsset = pParams->pAssetManager->Acquire( "shaders/.spv/Scene.vert.spv" );
        auto compiledVertexShader = compiledVertexShaderAsset->GetContentAsBinaryBuffer( );
        pParams->pAssetManager->Release( compiledVertexShaderAsset );
        if ( compiledVertexShader.empty( ) ) {
            apemodevk::platform::DebugBreak( );
            return false;
        }

        auto compiledFragmentShaderAsset = pParams->pAssetManager->Acquire( "shaders/.spv/Scene.frag.spv" );
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
        if ( !hDescriptorSetLayout.Recreate( *pNode, descriptorSetLayoutCreateInfos[ i ] ) ) {
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

    if ( false == hPipelineLayout.Recreate( *pNode, pipelineLayoutCreateInfo ) ) {
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

    vertexInputBindingDescription[ 0 ].stride    = sizeof( detail::DefaultVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (detail::DefaultVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (detail::DefaultVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (detail::DefaultVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (detail::DefaultVertex*) 0 )->texcoords );

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

    if ( false == hPipelineCache.Recreate( *pNode, pipelineCacheCreateInfo ) ) {
        return false;
    }

    if ( false == hPipeline.Recreate( *pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        return false;
    }

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        auto & frame = Frames[i];

        frame.BufferPool.Recreate( pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
        frame.DescriptorSetPools[ kDescriptorSetForPass ].Recreate( *pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForPass ] );
        frame.DescriptorSetPools[ kDescriptorSetForObj ].Recreate( *pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForObj ] );
    }

    return true;
}

bool apemode::vk::SceneRenderer::Reset( const Scene* pScene, uint32_t frameIndex ) {
    Frames[ frameIndex ].BufferPool.Reset( );
    return true;
}

bool apemode::vk::SceneRenderer::Flush( const Scene* pScene, uint32_t frameIndex ) {
    Frames[ frameIndex ].BufferPool.Flush( );
    return true;
}
