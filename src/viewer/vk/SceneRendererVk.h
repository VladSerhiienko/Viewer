#pragma once

#include <viewer/vk/SceneUploaderVk.h>
#include <viewer/SceneRendererBase.h>

#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/DescriptorPool.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>
#include <apemode/vk_ext/SamplerManager.Vulkan.h>

#include <apemode/platform/IAssetManager.h>
#include <apemode/platform/MathInc.h>

namespace apemode {
struct Scene;

namespace vk {

class SceneRenderer : public SceneRendererBase {
public:
    SceneRenderer( )          = default;
    virtual ~SceneRenderer( ) = default;

    struct RecreateParameters : RecreateParametersBase {
        apemodevk::GraphicsDevice*        pNode         = nullptr;        /* Required. */
        apemode::platform::IAssetManager* pAssetManager = nullptr;        /* Required. */
        VkDescriptorPool                  pDescPool     = VK_NULL_HANDLE; /* Required. */
        VkRenderPass                      pRenderPass   = VK_NULL_HANDLE; /* Required. */
        uint32_t                          FrameCount    = 0;              /* Required. */
    };

    bool Recreate( const RecreateParametersBase* pParams ) override;

    struct RenderParameters : SceneRenderParametersBase {
        class EnvMap {
        public:
            VkSampler     pSampler   = VK_NULL_HANDLE;           /* Required. */
            VkImageView   pImgView   = VK_NULL_HANDLE;           /* Required. */
            VkImageLayout eImgLayout = VK_IMAGE_LAYOUT_MAX_ENUM; /* Required. */
            uint32_t      MipLevels  = 0;                        /* Required. */
        };

        apemodevk::GraphicsDevice*     pNode = nullptr;             /* Required. */
        XMFLOAT2                       Dims;                        /* Required. */
        XMFLOAT2                       Scale;                       /* Required. */
        uint32_t                       FrameIndex = 0;              /* Required. */
        VkCommandBuffer                pCmdBuffer = VK_NULL_HANDLE; /* Required. */
        XMFLOAT4X4                     RootMatrix;                  /* Required. */
        XMFLOAT4X4                     ViewMatrix;                  /* Required. */
        XMFLOAT4X4                     ProjMatrix;                  /* Required. */
        XMFLOAT4X4                     InvViewMatrix;               /* Required. */
        XMFLOAT4X4                     InvProjMatrix;               /* Required. */
        EnvMap                         RadianceMap;                 /* Required. */
        EnvMap                         IrradianceMap;               /* Required. */
        XMFLOAT4                       LightDirection;              /* Required. */
        XMFLOAT4                       LightColor;                  /* Required. */
        const SceneNodeTransformFrame* pTransformFrame = nullptr;   /* Ok (BindPose). */
    };

    bool Reset( const Scene* pScene, uint32_t FrameIndex ) override;
    bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    bool Flush( const Scene* pScene, uint32_t FrameIndex ) override;

    static constexpr uint32_t kDescriptorSetCountForStatic  = 2;
    static constexpr uint32_t kDescriptorSetCountForSkinned = 3;

    static constexpr uint32_t kDescriptorSetForPass       = 0;
    static constexpr uint32_t kDescriptorSetForObj        = 1;
    static constexpr uint32_t kDescriptorSetForSkinnedObj = 2;
    // static constexpr uint32_t kDescriptorSetCountForStatic  = 2;
    // static constexpr uint32_t kDescriptorSetCountForSkinned = 3;
    static constexpr uint32_t kDescriptorSetCount = 3; // = max( skinned 3, static 2 )

    static constexpr uint32_t kPipelineLayoutCount      = 2; // = skinned + static
    static constexpr uint32_t kPipelineLayoutForStatic  = 0;
    static constexpr uint32_t kPipelineLayoutForSkinned = 1;

    // TODO: Set as pecialization constant.
    // static constexpr uint32_t kBoneCount = 64;
    static const uint32_t kMaxBoneCount = 128;

    struct Frame {
        apemodevk::HostBufferPool    BufferPool;
        apemodevk::DescriptorSetPool DescriptorSetPools[ kDescriptorSetCount ];
    };

    struct PipelineComposite {
        enum FlagBits {
            /* Vertex */
            kFlag_VertexType_Static         = 1 << 0,
            kFlag_VertexType_StaticSkinned4 = 1 << 1,
            kFlag_VertexType_StaticSkinned8 = 1 << 2,
            kFlag_VertexType_Packed         = 1 << 3,
            kFlag_VertexType_PackedSkinned  = 1 << 4,
            /* Blend */
            kFlag_BlendType_Disabled = 0,
            kFlag_BlendType_Add      = 1 << 5,
        };

        using Flags = eastl::underlying_type< FlagBits >::type;
        using Map   = apemodevk::vector_map< PipelineComposite::Flags, PipelineComposite >;

        Flags                                 eFlags;
        apemodevk::THandle< VkPipelineCache > hPipelineCache;
        apemodevk::THandle< VkPipeline >      hPipeline;
        VkPipelineLayout                      pPipelineLayout;

        PipelineComposite( );
        PipelineComposite( PipelineComposite&& other );
        PipelineComposite& operator=( PipelineComposite&& other );
    };

    bool RenderScene( const Scene*                            pScene,
                      const RenderParameters*                 pParams,
                      PipelineComposite&                      pipeline,
                      const apemode::SceneNodeTransformFrame* pTransformFrame,
                      const vk::SceneUploader::DeviceAsset*   pSceneAsset );

    apemodevk::GraphicsDevice*                       pNode = nullptr;
    apemodevk::vector< Frame >                       Frames;
    PipelineComposite::Map                           PipelineComposites;
    apemodevk::THandle< VkPipelineLayout >           hPipelineLayouts[ kPipelineLayoutCount ];
    apemodevk::THandle< VkDescriptorSetLayout >      hDescriptorSetLayouts[ kDescriptorSetCount ];
    apemodevk::vector_multimap< uint32_t, uint32_t > SortedNodeIds;
    apemodevk::vector< XMFLOAT4X4 >                  BoneOffsetMatrices;
    apemodevk::vector< XMFLOAT4X4 >                  BoneNormalMatrices;
};

} // namespace vk
} // namespace apemode
