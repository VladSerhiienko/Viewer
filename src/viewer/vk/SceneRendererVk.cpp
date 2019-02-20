
#include "SceneRendererVk.h"

#include <viewer/vk/SceneUploaderVk.h>
#include <viewer/Scene.h>

#include <apemode/vk/Buffer.Vulkan.h>
#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/QueuePools.Vulkan.h>
#include <apemode/vk/TOneTimeCmdBufferSubmit.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>

#include <apemode/platform/AppState.h>
#include <apemode/platform/ArrayUtils.h>
#include <apemode/platform/MathInc.h>

namespace apemodevk {

using namespace apemodexm;

struct ObjectUBO {
    XMFLOAT4X4 WorldMatrix;
    XMFLOAT4X4 NormalMatrix;
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

    const apemode::SceneNodeTransformFrame* pTransformFrame =
        pParams->pTransformFrame ? pParams->pTransformFrame : &pScene->GetBindPoseTransformFrame( );

    SortedNodeIds.clear( );
    SortedNodeIds.reserve( pScene->Nodes.size( ) );

    for ( const SceneNode& node : pScene->Nodes ) {
        if ( node.MeshId == uint32_t( -1 ) ) {
            continue;
        }

        const SceneMesh& mesh = pScene->Meshes[ node.MeshId ];

        switch ( mesh.eVertexType ) {
            case apemode::detail::eVertexType_Default:
                SortedNodeIds.insert( eastl::make_pair< uint32_t, uint32_t >(
                    uint32_t( PipelineComposite::kFlag_VertexType_Default | PipelineComposite::kFlag_BlendType_Disabled ),
                    uint32_t( node.Id ) ) );
                break;
            case apemode::detail::eVertexType_Skinned:
                SortedNodeIds.insert( eastl::make_pair< uint32_t, uint32_t >(
                    uint32_t( PipelineComposite::kFlag_VertexType_Skinned | PipelineComposite::kFlag_BlendType_Disabled ),
                    uint32_t( node.Id ) ) );
                break;
            case apemode::detail::eVertexType_FatSkinned:
                SortedNodeIds.insert( eastl::make_pair< uint32_t, uint32_t >(
                    uint32_t( PipelineComposite::kFlag_VertexType_FatSkinned | PipelineComposite::kFlag_BlendType_Disabled ),
                    uint32_t( node.Id ) ) );
                break;
            default:
                break;
        }
    }

    for ( PipelineComposite::Flags ePipelineFlags :
          {// PipelineComposite::kFlag_VertexType_Packed | PipelineComposite::kFlag_BlendType_Disabled,
           // PipelineComposite::kFlag_VertexType_PackedSkinned | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_Default | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_Skinned | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_FatSkinned | PipelineComposite::kFlag_BlendType_Disabled
           
          } ) {
        auto pipelineCompositeIt = PipelineComposites.find( ePipelineFlags );
        if ( pipelineCompositeIt != PipelineComposites.end( ) ) {
            if ( !RenderScene( pScene, pParams, pipelineCompositeIt->second, pTransformFrame, pSceneAsset ) )
                return false;
        }
    }

    return true;
}

bool apemode::vk::SceneRenderer::RenderScene( const Scene*                            pScene,
                                              const RenderParameters*                 pParams,
                                              PipelineComposite&                      pipeline,
                                              const apemode::SceneNodeTransformFrame* pTransformFrame,
                                              const vk::SceneUploader::DeviceAsset*   pSceneAsset ) {
    using namespace apemodevk;

    const PipelineComposite::FlagBits eBits = PipelineComposite::FlagBits(pipeline.eFlags);

    auto nodeRange = SortedNodeIds.equal_range( eBits );
    size_t nodeCount = size_t( eastl::distance( nodeRange.first, nodeRange.second ) );
    if ( !nodeCount )
        return true;

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.hPipeline );

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

    const uint32_t frameIndex = pParams->FrameIndex;
    auto& frame = Frames[ frameIndex ];

    enum DynamicOffset {
        kDynamicOffset_CameraUBO = 0,
        kDynamicOffset_LightUBO = 1,
        kDynamicOffset_ObjectUBO = 2,
        kDynamicOffset_MaterialUBO = 3,
        kDynamicOffset_BoneOffsetsUBO = 4,
        kDynamicOffset_BoneNormalsUBO = 5,
        kDynamicOffsetStaticCount = 4,
        kDynamicOffsetSkinnedCount = 6,
        kDynamicOffsetCount = 6,
    };

    VkDescriptorSet ppDescriptorSets[ kDescriptorSetCount ] = {nullptr};
    uint32_t        pDynamicOffsets[ kDynamicOffsetCount ]  = {0};

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

    pDynamicOffsets[ kDynamicOffset_CameraUBO ] = cameraDataUploadBufferRange.DynamicOffset;
    pDynamicOffsets[ kDynamicOffset_LightUBO ]  = lightDataUploadBufferRange.DynamicOffset;

    //
    // DescriptorSet Pass
    //

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

    for ( auto nodeIt = nodeRange.first; nodeIt != nodeRange.second; ++nodeIt ) {
        auto& node = pScene->Nodes[ nodeIt->second ];
        assert( node.MeshId != uint32_t( -1 ) );
        auto& mesh = pScene->Meshes[ node.MeshId ];

        if ( mesh.SkinId != uint32_t( -1 ) ) {
            auto& skin = pScene->Skins[ mesh.SkinId ];
            if ( skin.LinkIds.size( ) > kMaxBoneCount ) {
                apemodevk::platform::LogFmt( apemodevk::platform::Err,
                                             "The skin is too big, supported skeleton = %u, received = %u",
                                             uint32_t( kMaxBoneCount ),
                                             uint32_t( skin.LinkIds.size( ) ) );
                continue;
            }
        }

        auto pMeshAsset = (const vk::SceneUploader::MeshDeviceAsset*) mesh.pDeviceAsset.get( );
        assert( pMeshAsset );

        //
        // Calculate Object (Spatial)
        //

        ObjectUBO objectData;
        const XMMATRIX worldMatrix  = pTransformFrame->Transforms[ node.Id ].WorldMatrix;
        const XMMATRIX normalMatrix = XMMatrixTranspose( XMMatrixInverse( nullptr, worldMatrix ) );

        if ( mesh.SkinId != uint32_t( -1 ) ) {
            XMStoreFloat4x4( &objectData.WorldMatrix, XMMatrixIdentity() );
            XMStoreFloat4x4( &objectData.NormalMatrix, XMMatrixIdentity() );
        } else {
            XMStoreFloat4x4( &objectData.WorldMatrix, worldMatrix );
            XMStoreFloat4x4( &objectData.NormalMatrix, normalMatrix );
        }

        //
        // Upload Object (Spatial)
        //

        auto objectDataUploadBufferRange = frame.BufferPool.TSuballocate( objectData );
        assert( VK_NULL_HANDLE != objectDataUploadBufferRange.DescriptorBufferInfo.buffer );
        objectDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( ObjectUBO );

        pDynamicOffsets[ kDynamicOffset_ObjectUBO ] = objectDataUploadBufferRange.DynamicOffset;

        if ( mesh.SkinId != uint32_t( -1 ) ) {

            //
            // Calculate SkinnedObject
            //

            assert( HasFlagEq( pipeline.eFlags, PipelineComposite::kFlag_VertexType_FatSkinned ) ||
                    HasFlagEq( pipeline.eFlags, PipelineComposite::kFlag_VertexType_Skinned ) );

            auto& skin = pScene->Skins[ mesh.SkinId ];
            assert( skin.LinkIds.size( ) <= BoneOffsetMatrices.size( ) );
            assert( skin.LinkIds.size( ) <= BoneNormalMatrices.size( ) );

            pScene->UpdateSkinMatrices( skin,
                                        pTransformFrame,
                                        worldMatrix,
                                        BoneOffsetMatrices.data( ),
                                        BoneNormalMatrices.data( ),
                                        kMaxBoneCount );

            //
            // Upload SkinnedObject
            //

            const uint32_t boneOffsetsRange = uint32_t( sizeof( XMFLOAT4X4 ) * kMaxBoneCount );
            const uint32_t boneNormalsRange = uint32_t( sizeof( XMFLOAT4X4 ) * kMaxBoneCount );

            apemodevk::HostBufferPool::SuballocResult boneOffsetsUploadBufferRange;
            boneOffsetsUploadBufferRange = frame.BufferPool.Suballocate( BoneOffsetMatrices.data( ), boneOffsetsRange );
            assert( VK_NULL_HANDLE != boneOffsetsUploadBufferRange.DescriptorBufferInfo.buffer );
            boneOffsetsUploadBufferRange.DescriptorBufferInfo.range = boneOffsetsRange;

            apemodevk::HostBufferPool::SuballocResult boneNormalsUploadBufferRange;
            boneNormalsUploadBufferRange = frame.BufferPool.Suballocate( BoneNormalMatrices.data( ), boneNormalsRange );
            assert( VK_NULL_HANDLE != boneNormalsUploadBufferRange.DescriptorBufferInfo.buffer );
            boneNormalsUploadBufferRange.DescriptorBufferInfo.range = boneNormalsRange;

            pDynamicOffsets[ kDynamicOffset_BoneOffsetsUBO ] = boneOffsetsUploadBufferRange.DynamicOffset;
            pDynamicOffsets[ kDynamicOffset_BoneNormalsUBO ] = boneNormalsUploadBufferRange.DynamicOffset;

            //
            // DescriptorSet SkinnedObject
            //

            TDescriptorSetBindings< 2 > descriptorSetForSkinnedObj;
            descriptorSetForSkinnedObj.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 0 */
            descriptorSetForSkinnedObj.pBinding[ 0 ].BufferInfo      = boneOffsetsUploadBufferRange.DescriptorBufferInfo;

            descriptorSetForSkinnedObj.pBinding[ 1 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; /* 1 */
            descriptorSetForSkinnedObj.pBinding[ 1 ].BufferInfo      = boneNormalsUploadBufferRange.DescriptorBufferInfo;

            ppDescriptorSets[ kDescriptorSetForSkinnedObj ] =
                frame.DescriptorSetPools[ kDescriptorSetForSkinnedObj ].GetDescriptorSet( &descriptorSetForSkinnedObj );
        }

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

            // TODO: Fix the bug with missing material assets.
            if ( !pMaterialAsset ) {
                pMaterialAsset = &pSceneAsset->MissingMaterialAsset;
                // apemodevk::platform::LogFmt(apemodevk::platform::Err, "Skipping subset with missing material asset.");
                // continue;
            }

            //
            // MaterialUBO
            //

            uint32_t flags = 0;
            flags |= pMaterialAsset->hBaseColorImgView ? 1 << 0 : 0;
            flags |= pMaterialAsset->hNormalImgView ? 1 << 1 : 0;
            flags |= pMaterialAsset->hEmissiveImgView ? 1 << 2 : 0;
            flags |= pMaterialAsset->hMetallicRoughnessOcclusionImgView ? 1 << 3 : 0;
            flags |= pMaterialAsset->hMetallicRoughnessOcclusionImgView ? 1 << 4 : 0;
            flags |= 0; // pMaterialAsset->bHasOcclusionMap ? 1 << 5 : 0;

            MaterialUBO materialData;
            /*
            materialData.BaseColorFactor.x = 1;
            materialData.BaseColorFactor.y = 1;
            materialData.BaseColorFactor.z = 1;
            */
            //*
            materialData.BaseColorFactor.x         = 1; // pMaterial->BaseColorFactor.x;
            materialData.BaseColorFactor.y         = 1; // pMaterial->BaseColorFactor.y;
            materialData.BaseColorFactor.z         = 1; // pMaterial->BaseColorFactor.z;
            //*/
            materialData.BaseColorFactor.w         = pMaterial->BaseColorFactor.w;
            materialData.EmissiveFactor.x          = pMaterial->EmissiveFactor.x;
            materialData.EmissiveFactor.y          = pMaterial->EmissiveFactor.y;
            materialData.EmissiveFactor.z          = pMaterial->EmissiveFactor.z;
            materialData.MetallicRoughnessFactor.x = pMaterial->MetallicFactor;
            materialData.MetallicRoughnessFactor.y = pMaterial->RoughnessFactor;
            materialData.MetallicRoughnessFactor.z = 1;
            materialData.MetallicRoughnessFactor.w = static_cast< float >( pParams->RadianceMap.MipLevels );
            materialData.Flags.x                   = flags;

            //
            // Upload Object (Material)
            //

            auto materialDataUploadBufferRange = frame.BufferPool.TSuballocate( materialData );
            assert( VK_NULL_HANDLE != materialDataUploadBufferRange.DescriptorBufferInfo.buffer );
            materialDataUploadBufferRange.DescriptorBufferInfo.range = sizeof( MaterialUBO );

            pDynamicOffsets[ kDynamicOffset_MaterialUBO ] = materialDataUploadBufferRange.DynamicOffset;

            //
            // DescriptorSet Object
            //

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
                                                                    // pSceneAsset->MissingTextureOnes->hImgView,
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
                                                                    pMaterialAsset->hMetallicRoughnessOcclusionImgView,
                                                                    pMaterialAsset->pMetallicRoughnessOcclusionSampler,
                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    pSceneAsset->MissingTextureZeros->hImgView,
                                                                    pSceneAsset->pMissingSampler );

//            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 6;
//            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
//                                                                    pMaterialAsset->hRoughnessImgView,
//                                                                    pMaterialAsset->pRoughnessSampler,
//                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                                                                    pSceneAsset->MissingTextureZeros->hImgView,
//                                                                    pSceneAsset->pMissingSampler );
//
//            descriptorSetForObject.pBinding[ objectSetBindingCount ].DstBinding = 7;
//            objectSetBindingCount += FillCombinedImgSamplerBinding( &descriptorSetForObject.pBinding[ objectSetBindingCount ],
//                                                                    pMaterialAsset->hOcclusionImgView,
//                                                                    pMaterialAsset->pOcclusionSampler,
//                                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//                                                                    pSceneAsset->MissingTextureZeros->hImgView,
//                                                                    pSceneAsset->pMissingSampler );

            descriptorSetForObject.BindingCount = objectSetBindingCount;

            ppDescriptorSets[ kDescriptorSetForObj ] =
                frame.DescriptorSetPools[ kDescriptorSetForObj ].GetDescriptorSet( &descriptorSetForObject );

            vkCmdBindDescriptorSets(
                pParams->pCmdBuffer,             /* Cmd */
                VK_PIPELINE_BIND_POINT_GRAPHICS, /* BindPoint */
                pipeline.pPipelineLayout,        /* PipelineLayout */
                0,                               /* FirstSet */
                mesh.SkinId != uint32_t( -1 ) ? kDescriptorSetCountForSkinned : kDescriptorSetCountForStatic, /* SetCount */
                ppDescriptorSets,                                                                             /* Sets */
                mesh.SkinId != uint32_t( -1 ) ? kDynamicOffsetSkinnedCount : kDynamicOffsetStaticCount, /* DymamicOffsetCount */
                pDynamicOffsets );                                                                      /* DymamicOffsets */

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
                                  pMeshAsset->eIndexType );                 /* UInt16/Uint32 */

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

template < uint32_t TMaxShaderStages                      = 5,
           uint32_t TMaxPipelineColorBlendAttachmentState = 16,
           uint32_t TMaxDynamicStates                     = 16,
           uint32_t TMaxVertexInputAttributeDescriptions  = 16,
           uint32_t TMaxVertexInputBindingDescriptions    = 16 >
struct TGraphicsPipelineCreateInfoComposite {
    VkGraphicsPipelineCreateInfo           Pipeline;
    VkPipelineShaderStageCreateInfo        PipelineShaderStages[ TMaxShaderStages ];
    VkPipelineVertexInputStateCreateInfo   PipelineVertexInputState;
    VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyState;
    VkPipelineTessellationStateCreateInfo  PipelineTessellationState;
    VkPipelineViewportStateCreateInfo      PipelineViewportState;
    VkPipelineRasterizationStateCreateInfo PipelineRasterizationState;
    VkPipelineMultisampleStateCreateInfo   PipelineMultisampleState;
    VkPipelineDepthStencilStateCreateInfo  PipelineDepthStencilState;
    VkPipelineColorBlendAttachmentState    PipelineColorBlendAttachmentStates[ TMaxPipelineColorBlendAttachmentState ];
    VkPipelineColorBlendStateCreateInfo    PipelineColorBlendState;
    VkPipelineDynamicStateCreateInfo       PipelineDynamicState;
    VkDynamicState                         eEnableDynamicStates[ TMaxDynamicStates ];
    VkVertexInputAttributeDescription      VertexInputAttributeDescriptions[ TMaxVertexInputAttributeDescriptions ];
    VkVertexInputBindingDescription        VertexInputBindingDescriptions[ TMaxVertexInputBindingDescriptions ];

    TGraphicsPipelineCreateInfoComposite( ) {
        using namespace apemodevk;

        InitializeStruct( PipelineShaderStages );
        InitializeStruct( PipelineVertexInputState);
        InitializeStruct( PipelineInputAssemblyState);
        InitializeStruct( PipelineTessellationState);
        InitializeStruct( PipelineViewportState);
        InitializeStruct( PipelineRasterizationState);
        InitializeStruct( PipelineMultisampleState);
        InitializeStruct( PipelineDepthStencilState);
        InitializeStruct( PipelineColorBlendState);
        InitializeStruct( PipelineDynamicState);
        InitializeStruct( Pipeline);

        InitializeStruct( PipelineColorBlendAttachmentStates);
        InitializeStruct( VertexInputAttributeDescriptions);
        InitializeStruct( VertexInputBindingDescriptions);

        Pipeline.pVertexInputState   = &PipelineVertexInputState;
        Pipeline.pInputAssemblyState = &PipelineInputAssemblyState;
        Pipeline.pTessellationState  = &PipelineTessellationState;
        Pipeline.pViewportState      = &PipelineViewportState;
        Pipeline.pRasterizationState = &PipelineRasterizationState;
        Pipeline.pMultisampleState   = &PipelineMultisampleState;
        Pipeline.pDepthStencilState  = &PipelineDepthStencilState;
        Pipeline.pColorBlendState    = &PipelineColorBlendState;
        Pipeline.pDynamicState       = &PipelineDynamicState;

        Pipeline.pStages    = PipelineShaderStages;
        Pipeline.stageCount = 0;

        PipelineVertexInputState.pVertexBindingDescriptions      = VertexInputBindingDescriptions;
        PipelineVertexInputState.pVertexAttributeDescriptions    = VertexInputAttributeDescriptions;
        PipelineVertexInputState.vertexBindingDescriptionCount   = 0;
        PipelineVertexInputState.vertexAttributeDescriptionCount = 0;

        PipelineDynamicState.pDynamicStates    = eEnableDynamicStates;
        PipelineDynamicState.dynamicStateCount = 0;

        PipelineColorBlendState.pAttachments    = PipelineColorBlendAttachmentStates;
        PipelineColorBlendState.attachmentCount = 0;
    }
};

namespace {

template < typename TVertex >
void TSetPipelineVertexInputStateCreateInfo( VkPipelineVertexInputStateCreateInfo* pPipelineVertexInputState,
                                             VkVertexInputBindingDescription*      pVertexBindingDescriptions,
                                             uint32_t                              maxVertexBindingDescriptions,
                                             VkVertexInputAttributeDescription*    pVertexInputAttributeDescriptions,
                                             uint32_t                              maxVertexInputAttributeDescriptions ) {
    // static_assert( false, "Unknown vertex type." );
    assert( false );
}

template <>
void TSetPipelineVertexInputStateCreateInfo< apemode::detail::DefaultVertex >(
    VkPipelineVertexInputStateCreateInfo* pPipelineVertexInputState,
    VkVertexInputBindingDescription*      pVertexBindingDescriptions,
    uint32_t                              maxVertexBindingDescriptions,
    VkVertexInputAttributeDescription*    pVertexInputAttributeDescriptions,
    uint32_t                              maxVertexInputAttributeDescriptions ) {
    using V = apemode::detail::DefaultVertex;

    pVertexBindingDescriptions[ 0 ].binding   = 0;
    pVertexBindingDescriptions[ 0 ].stride    = sizeof( V );
    pVertexBindingDescriptions[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    pPipelineVertexInputState->vertexBindingDescriptionCount   = 1;
    
    const uint32_t binding = pVertexBindingDescriptions[ 0 ].binding;

    uint8_t location = 0;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->position );

    ++location;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32G32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->texcoords );

    ++location;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->qtangent );

    ++location;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32_UINT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->indexColorRGB );

    ++location;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->colorAlpha );

    ++location;
    pPipelineVertexInputState->vertexAttributeDescriptionCount = location;
}

template <>
void TSetPipelineVertexInputStateCreateInfo< apemode::detail::SkinnedVertex >(
    VkPipelineVertexInputStateCreateInfo* pPipelineVertexInputState,
    VkVertexInputBindingDescription*      pVertexBindingDescriptions,
    uint32_t                              maxVertexBindingDescriptions,
    VkVertexInputAttributeDescription*    pVertexInputAttributeDescriptions,
    uint32_t                              maxVertexInputAttributeDescriptions ) {
    
    TSetPipelineVertexInputStateCreateInfo< apemode::detail::DefaultVertex >(
        pPipelineVertexInputState,
        pVertexBindingDescriptions,
        maxVertexBindingDescriptions,
        pVertexInputAttributeDescriptions,
        maxVertexInputAttributeDescriptions);
    
    using V = apemode::detail::SkinnedVertex;
    pVertexBindingDescriptions[ 0 ].stride = sizeof( V );
    
    const uint32_t binding = pVertexBindingDescriptions[ 0 ].binding;
    
    uint8_t location = pPipelineVertexInputState->vertexAttributeDescriptionCount;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->boneIndicesWeights );

    ++location;
    pPipelineVertexInputState->vertexAttributeDescriptionCount = location;
}

template <>
void TSetPipelineVertexInputStateCreateInfo< apemode::detail::FatSkinnedVertex >(
    VkPipelineVertexInputStateCreateInfo* pPipelineVertexInputState,
    VkVertexInputBindingDescription*      pVertexBindingDescriptions,
    uint32_t                              maxVertexBindingDescriptions,
    VkVertexInputAttributeDescription*    pVertexInputAttributeDescriptions,
    uint32_t                              maxVertexInputAttributeDescriptions ) {
    
    TSetPipelineVertexInputStateCreateInfo< apemode::detail::SkinnedVertex >(
        pPipelineVertexInputState,
        pVertexBindingDescriptions,
        maxVertexBindingDescriptions,
        pVertexInputAttributeDescriptions,
        maxVertexInputAttributeDescriptions);
    
    using V = apemode::detail::FatSkinnedVertex;
    pVertexBindingDescriptions[ 0 ].stride = sizeof( V );
    
    const uint32_t binding = pVertexBindingDescriptions[ 0 ].binding;
    
    uint8_t location = pPipelineVertexInputState->vertexAttributeDescriptionCount;
    pVertexInputAttributeDescriptions[ location ].location = location;
    pVertexInputAttributeDescriptions[ location ].binding  = binding;
    pVertexInputAttributeDescriptions[ location ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    pVertexInputAttributeDescriptions[ location ].offset   = ( size_t )( &( (V*) 0 )->extraBoneIndicesWeights );

    ++location;
    pPipelineVertexInputState->vertexAttributeDescriptionCount = location;
}

apemode::vector< uint8_t > GetPrecompiledShader( const apemode::platform::IAssetManager* pAssetManager,
                                                 const char*                             pszPrecompiledShaderAsset ) {
    auto compiledVertexShaderAsset = pAssetManager->Acquire( pszPrecompiledShaderAsset );
    assert( compiledVertexShaderAsset );
    if ( !compiledVertexShaderAsset ) {
        apemodevk::platform::DebugBreak( );
        return {};
    }

    auto compiledVertexShader = compiledVertexShaderAsset->GetContentAsBinaryBuffer( );
    assert( compiledVertexShader.size( ) );
    pAssetManager->Release( compiledVertexShaderAsset );
    if ( compiledVertexShader.empty( ) ) {
        apemodevk::platform::DebugBreak( );
        return {};
    }

    return eastl::move( compiledVertexShader );
}

apemodevk::THandle< VkShaderModule > CompileShader( apemodevk::GraphicsDevice*              pNode,
                                                    const apemode::platform::IAssetManager* pAssetManager,
                                                    const char*                             pszPrecompiledShaderAsset ) {
    apemodevk::THandle< VkShaderModule > shaderModule;

    auto precompiledShader = GetPrecompiledShader( pAssetManager, pszPrecompiledShaderAsset );
    if ( !precompiledShader.empty( ) ) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo;
        apemodevk::InitializeStruct( shaderModuleCreateInfo );
        shaderModuleCreateInfo.pCode = reinterpret_cast< const uint32_t* >( precompiledShader.data( ) );
        shaderModuleCreateInfo.codeSize = precompiledShader.size( );
        shaderModule.Recreate( pNode->hLogicalDevice, shaderModuleCreateInfo );
    }

    return eastl::move( shaderModule );
}

} // namespace

bool apemode::vk::SceneRenderer::Recreate( const RecreateParametersBase* pParamsBase ) {
    using namespace apemodevk;

    if ( nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (RecreateParameters*) pParamsBase;
    if ( nullptr == pParams->pNode )
        return false;

    pNode = pParams->pNode;

    BoneOffsetMatrices.resize( kMaxBoneCount );
    BoneNormalMatrices.resize( kMaxBoneCount );

    //
    // Initialize all the configurations that are supported.
    // TODO: Add blending configurations.
    //

    PipelineComposites.reserve( 4 );
    for ( PipelineComposite::Flags ePipelineFlags :
          {// TODO: MoltenVK struggles to use packed format.
           // PipelineComposite::kFlag_VertexType_Packed | PipelineComposite::kFlag_BlendType_Disabled,
           // PipelineComposite::kFlag_VertexType_PackedSkinned | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_Default | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_Skinned | PipelineComposite::kFlag_BlendType_Disabled,
           PipelineComposite::kFlag_VertexType_FatSkinned | PipelineComposite::kFlag_BlendType_Disabled} ) {
        PipelineComposites[ ePipelineFlags ].eFlags = ePipelineFlags;
    }

    //
    // Initialize all the shaders that are needed for these configurations.
    // TODO: Remove skinning shader if it is not needed.
    //

    THandle< VkShaderModule > hVertexShaderModule = CompileShader( pNode, pParams->pAssetManager, "shaders/Viewer.cso.d/UScene.vert.spv" );
    THandle< VkShaderModule > hSkinnedVertexShaderModule = CompileShader( pNode, pParams->pAssetManager, "shaders/Viewer.cso.d/UScene.vert-defs-SKINNING=1.spv" );
    THandle< VkShaderModule > hSkinned8VertexShaderModule = CompileShader( pNode, pParams->pAssetManager, "shaders/Viewer.cso.d/UScene.vert-defs-SKINNING8=1.spv" );
    THandle< VkShaderModule > hFragmentShaderModule = CompileShader( pNode, pParams->pAssetManager, "shaders/Viewer.cso.d/UScene.frag.spv" );

    if ( hVertexShaderModule.IsNull( ) ||
         hSkinnedVertexShaderModule.IsNull( ) ||
         hSkinned8VertexShaderModule.IsNull( ) ||
         hFragmentShaderModule.IsNull( ) ) {
        assert( false );
        return false;
    }

    VkSpecializationInfo specializationInfo;
    InitializeStruct( specializationInfo );

    VkSpecializationMapEntry boneCountEntry;
    InitializeStruct( boneCountEntry );
    boneCountEntry.constantID = 0;
    boneCountEntry.offset     = 0;
    boneCountEntry.size       = sizeof( int );

    const int maxBoneCount = int( kMaxBoneCount );

    specializationInfo.pMapEntries   = &boneCountEntry;
    specializationInfo.mapEntryCount = 1;
    specializationInfo.pData         = &maxBoneCount;
    specializationInfo.dataSize      = sizeof( maxBoneCount );

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

    //
    // Set 3 (SkinnedObject)
    //
    // layout( std140, set = 3, binding = 0 ) uniform SkinnedObjectUBO;

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindingsForSkinnedObj[ 2 ];
    InitializeStruct( descriptorSetLayoutBindingsForSkinnedObj );

    descriptorSetLayoutBindingsForSkinnedObj[ 0 ].binding         = 0;
    descriptorSetLayoutBindingsForSkinnedObj[ 0 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForSkinnedObj[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForSkinnedObj[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    descriptorSetLayoutBindingsForSkinnedObj[ 1 ].binding         = 1;
    descriptorSetLayoutBindingsForSkinnedObj[ 1 ].descriptorCount = 1;
    descriptorSetLayoutBindingsForSkinnedObj[ 1 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBindingsForSkinnedObj[ 1 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    //
    // DescriptorSetLayouts
    //

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfos[ kDescriptorSetCount ];
    InitializeStruct( descriptorSetLayoutCreateInfos );

    descriptorSetLayoutCreateInfos[ kDescriptorSetForPass ].bindingCount = apemode::GetArraySize( descriptorSetLayoutBindingsForPass );
    descriptorSetLayoutCreateInfos[ kDescriptorSetForPass ].pBindings    = descriptorSetLayoutBindingsForPass;
    descriptorSetLayoutCreateInfos[ kDescriptorSetForObj ].bindingCount = apemode::GetArraySize( descriptorSetLayoutBindingsForObj );
    descriptorSetLayoutCreateInfos[ kDescriptorSetForObj ].pBindings    = descriptorSetLayoutBindingsForObj;
    descriptorSetLayoutCreateInfos[ kDescriptorSetForSkinnedObj ].bindingCount = apemode::GetArraySize( descriptorSetLayoutBindingsForSkinnedObj );
    descriptorSetLayoutCreateInfos[ kDescriptorSetForSkinnedObj ].pBindings    = descriptorSetLayoutBindingsForSkinnedObj;

    VkDescriptorSetLayout ppDescriptorSetLayouts[ kDescriptorSetCount ] = {nullptr};
    for ( uint32_t i = 0; i < kDescriptorSetCount; ++i ) {
        if ( !hDescriptorSetLayouts[ i ].Recreate( *pNode, descriptorSetLayoutCreateInfos[ i ] ) ) {
            return false;
        }

        // []
        // kDescriptorSetForPass
        // kDescriptorSetForObj
        // kDescriptorSetForSkinnedObj
        ppDescriptorSetLayouts[ i ] = hDescriptorSetLayouts[ i ];
    }

    //
    // PipelineLayouts
    //

    VkPipelineLayoutCreateInfo staticPipelineLayoutCreateInfo;
    InitializeStruct( staticPipelineLayoutCreateInfo );
    staticPipelineLayoutCreateInfo.setLayoutCount = kDescriptorSetCountForStatic;
    staticPipelineLayoutCreateInfo.pSetLayouts    = ppDescriptorSetLayouts;

    VkPipelineLayoutCreateInfo skinnedPipelineLayoutCreateInfo;
    InitializeStruct( skinnedPipelineLayoutCreateInfo );
    skinnedPipelineLayoutCreateInfo.setLayoutCount = kDescriptorSetCountForSkinned;
    skinnedPipelineLayoutCreateInfo.pSetLayouts    = ppDescriptorSetLayouts;

    if ( !hPipelineLayouts[ kPipelineLayoutForStatic ].Recreate( *pNode, staticPipelineLayoutCreateInfo ) ||
         !hPipelineLayouts[ kPipelineLayoutForSkinned ].Recreate( *pNode, skinnedPipelineLayoutCreateInfo ) ) {
        assert( false );
        return false;
    }

    for ( auto& pipeline : PipelineComposites ) {
        if ( HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_Default ) ) {
            assert( !pipeline.second.pPipelineLayout );
            pipeline.second.pPipelineLayout = hPipelineLayouts[ kPipelineLayoutForStatic ];
        }
        if ( HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_Skinned ) ||
             HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_FatSkinned ) ) {
            assert( !pipeline.second.pPipelineLayout );
            pipeline.second.pPipelineLayout = hPipelineLayouts[ kPipelineLayoutForSkinned ];
        }
    }

    //
    // PipelineCaches
    //

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
    InitializeStruct( pipelineCacheCreateInfo );

    for ( auto& pipeline : PipelineComposites ) {
        assert( pipeline.second.hPipelineCache.IsNull( ) );
        if ( false == pipeline.second.hPipelineCache.Recreate( *pNode, pipelineCacheCreateInfo ) ) {
            return false;
        }
    }

    //
    // Pipelines
    //

    TGraphicsPipelineCreateInfoComposite< 2 > composite;

    composite.Pipeline.renderPass                                = pParams->pRenderPass;
    composite.PipelineInputAssemblyState.topology                = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    composite.PipelineRasterizationState.cullMode                = VK_CULL_MODE_BACK_BIT;
    composite.PipelineRasterizationState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
    composite.PipelineRasterizationState.polygonMode             = VK_POLYGON_MODE_FILL;
    composite.PipelineRasterizationState.depthClampEnable        = VK_FALSE;
    composite.PipelineRasterizationState.rasterizerDiscardEnable = VK_FALSE;
    composite.PipelineRasterizationState.depthBiasEnable         = VK_FALSE;
    composite.PipelineRasterizationState.lineWidth               = 1.0f;
    composite.PipelineDepthStencilState.depthTestEnable          = VK_TRUE;
    composite.PipelineDepthStencilState.depthWriteEnable         = VK_TRUE;
    composite.PipelineDepthStencilState.depthCompareOp           = VK_COMPARE_OP_LESS_OR_EQUAL;
    composite.PipelineDepthStencilState.depthBoundsTestEnable    = VK_FALSE;
    composite.PipelineDepthStencilState.stencilTestEnable        = VK_FALSE;
    composite.PipelineDepthStencilState.back.failOp              = VK_STENCIL_OP_KEEP;
    composite.PipelineDepthStencilState.back.passOp              = VK_STENCIL_OP_KEEP;
    composite.PipelineDepthStencilState.back.compareOp           = VK_COMPARE_OP_ALWAYS;
    composite.PipelineDepthStencilState.front.failOp             = VK_STENCIL_OP_KEEP;
    composite.PipelineDepthStencilState.front.passOp             = VK_STENCIL_OP_KEEP;
    composite.PipelineDepthStencilState.front.compareOp          = VK_COMPARE_OP_ALWAYS;
    composite.PipelineMultisampleState.pSampleMask               = NULL;
    composite.PipelineMultisampleState.rasterizationSamples      = VK_SAMPLE_COUNT_1_BIT;
    composite.PipelineViewportState.viewportCount                = 1;
    composite.PipelineViewportState.scissorCount                 = 1;

    composite.PipelineShaderStages[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    composite.PipelineShaderStages[ 0 ].pName  = "main";
    composite.PipelineShaderStages[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    composite.PipelineShaderStages[ 1 ].module = hFragmentShaderModule;
    composite.PipelineShaderStages[ 1 ].pName  = "main";
    composite.Pipeline.stageCount              = 2;

    const VkColorComponentFlags eColorComponentFlags  = VK_COLOR_COMPONENT_R_BIT
                                                      | VK_COLOR_COMPONENT_G_BIT
                                                      | VK_COLOR_COMPONENT_B_BIT
                                                      | VK_COLOR_COMPONENT_A_BIT;

    composite.PipelineColorBlendAttachmentStates[ 0 ].colorWriteMask = eColorComponentFlags;
    composite.PipelineColorBlendAttachmentStates[ 0 ].blendEnable    = VK_FALSE;
    composite.PipelineColorBlendState.attachmentCount                = 1;

    composite.eEnableDynamicStates[ 0 ]              = VK_DYNAMIC_STATE_SCISSOR;
    composite.eEnableDynamicStates[ 1 ]              = VK_DYNAMIC_STATE_VIEWPORT;
    composite.PipelineDynamicState.dynamicStateCount = 2;

    //
    for ( auto& pipeline : PipelineComposites ) {
        if ( HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_Default ) ) {
            composite.PipelineShaderStages[ 0 ].module              = hVertexShaderModule;
            composite.PipelineShaderStages[ 0 ].pSpecializationInfo = nullptr;
            composite.Pipeline.layout                               = pipeline.second.pPipelineLayout;

            TSetPipelineVertexInputStateCreateInfo< apemode::detail::DefaultVertex >(
                &composite.PipelineVertexInputState,
                composite.VertexInputBindingDescriptions,
                GetArraySize( composite.VertexInputBindingDescriptions ),
                composite.VertexInputAttributeDescriptions,
                GetArraySize( composite.VertexInputAttributeDescriptions ) );

            assert( pipeline.second.hPipeline.IsNull( ) );
            if ( !pipeline.second.hPipeline.Recreate( *pNode, pipeline.second.hPipelineCache, composite.Pipeline ) ) {
                assert( false );
                return false;
            }
        }

        if ( HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_Skinned ) ) {
            composite.PipelineShaderStages[ 0 ].module              = hSkinnedVertexShaderModule;
            composite.PipelineShaderStages[ 0 ].pSpecializationInfo = &specializationInfo;
            composite.Pipeline.layout                               = hPipelineLayouts[ kPipelineLayoutForSkinned ];

            TSetPipelineVertexInputStateCreateInfo< apemode::detail::SkinnedVertex >(
                &composite.PipelineVertexInputState,
                composite.VertexInputBindingDescriptions,
                GetArraySize( composite.VertexInputBindingDescriptions ),
                composite.VertexInputAttributeDescriptions,
                GetArraySize( composite.VertexInputAttributeDescriptions ) );

            assert( pipeline.second.hPipeline.IsNull( ) );
            if ( !pipeline.second.hPipeline.Recreate( *pNode, pipeline.second.hPipelineCache, composite.Pipeline ) ) {
                assert( false );
                return false;
            }
        }

        if ( HasFlagEq( pipeline.second.eFlags, PipelineComposite::kFlag_VertexType_FatSkinned ) ) {
            composite.PipelineShaderStages[ 0 ].module              = hSkinned8VertexShaderModule;
            composite.PipelineShaderStages[ 0 ].pSpecializationInfo = &specializationInfo;
            composite.Pipeline.layout                               = hPipelineLayouts[ kPipelineLayoutForSkinned ];

            TSetPipelineVertexInputStateCreateInfo< apemode::detail::FatSkinnedVertex >(
                &composite.PipelineVertexInputState,
                composite.VertexInputBindingDescriptions,
                GetArraySize( composite.VertexInputBindingDescriptions ),
                composite.VertexInputAttributeDescriptions,
                GetArraySize( composite.VertexInputAttributeDescriptions ) );

            assert( pipeline.second.hPipeline.IsNull( ) );
            if ( !pipeline.second.hPipeline.Recreate( *pNode, pipeline.second.hPipelineCache, composite.Pipeline ) ) {
                assert( false );
                return false;
            }
        }
    }

    Frames.resize( pParams->FrameCount );
    for ( auto & frame : Frames ) {
        frame.BufferPool.Recreate( pNode, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );

        if ( !frame.DescriptorSetPools[ kDescriptorSetForPass ].Recreate(
                 *pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForPass ] ) ) {
            return false;
        }

        if ( !frame.DescriptorSetPools[ kDescriptorSetForObj ].Recreate(
                 *pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForObj ] ) ) {
            return false;
        }

        if ( !frame.DescriptorSetPools[ kDescriptorSetForSkinnedObj ].Recreate(
                 *pNode, pParams->pDescPool, hDescriptorSetLayouts[ kDescriptorSetForSkinnedObj ] ) ) {
            return false;
        }
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

apemode::vk::SceneRenderer::PipelineComposite::PipelineComposite( )
    : eFlags( 0 ), hPipelineCache( ), hPipeline( ), pPipelineLayout( nullptr ) {
}

apemode::vk::SceneRenderer::PipelineComposite::PipelineComposite( PipelineComposite&& o )
    : eFlags( o.eFlags )
    , hPipelineCache( eastl::move( o.hPipelineCache ) )
    , hPipeline( eastl::move( o.hPipeline ) )
    , pPipelineLayout( o.pPipelineLayout ) {
}

apemode::vk::SceneRenderer::PipelineComposite&
apemode::vk::SceneRenderer::PipelineComposite::operator=( PipelineComposite&& o ) {
    eFlags          = o.eFlags;
    hPipelineCache  = eastl::move( o.hPipelineCache );
    hPipeline       = eastl::move( o.hPipeline );
    pPipelineLayout = o.pPipelineLayout;
    return *this;
}
