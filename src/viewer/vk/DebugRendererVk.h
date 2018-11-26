#pragma once

#include <apemode/vk/Buffer.Vulkan.h>
#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/DescriptorPool.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>

#include <apemode/platform/IAssetManager.h>
#include <apemode/platform/MathInc.h>

namespace apemode {
struct Scene;
struct SceneNodeTransformFrame;

namespace vk {

struct DebugRenderer {
    struct PositionVertex {
        float Position[ 3 ];
    };

    struct DebugUBO {
        XMFLOAT4X4 WorldMatrix;
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjMatrix;
        XMFLOAT4   Color;
    };

    struct InitParameters {
        apemodevk::GraphicsDevice *       pNode         = nullptr;        /* Required */
        apemode::platform::IAssetManager *pAssetManager = nullptr;        /* Required */
        VkDescriptorPool                  pDescPool     = VK_NULL_HANDLE; /* Required */
        VkRenderPass                      pRenderPass   = VK_NULL_HANDLE; /* Required */
        uint32_t                          FrameCount    = 0;              /* Required, swapchain img count typically */
    };

    struct RenderCubeParameters {
        float           Dims[ 2 ]  = {};             /* Required */
        float           Scale[ 2 ] = {};             /* Required */
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
        uint32_t        FrameIndex = 0;              /* Required */
        DebugUBO *      pFrameData = nullptr;        /* Required */
        float           LineWidth  = 1.0f;
    };

    struct RenderSceneParameters {
        apemodevk::GraphicsDevice *    pNode = nullptr;             /* Required. */
        XMFLOAT2                       Dims;                        /* Required. */
        XMFLOAT2                       Scale;                       /* Required. */
        uint32_t                       FrameIndex = 0;              /* Required. */
        VkCommandBuffer                pCmdBuffer = VK_NULL_HANDLE; /* Required. */
        XMFLOAT4X4                     RootMatrix;                  /* Required. */
        XMFLOAT4X4                     ViewMatrix;                  /* Required. */
        XMFLOAT4X4                     ProjMatrix;                  /* Required. */
        XMFLOAT4X4                     InvViewMatrix;               /* Required. */
        XMFLOAT4X4                     InvProjMatrix;               /* Required. */
        const SceneNodeTransformFrame *pTransformFrame    = nullptr;
        XMFLOAT4                       SceneColorOverride = {0, 1, 1, 0.50f}; /* Ok. */
        float                          LineWidth          = 1.0f;
    };

    static uint32_t const kMaxFrameCount = 3;

    apemodevk::THandle< VkDescriptorSetLayout >      hDescSetLayout;
    apemodevk::THandle< VkPipelineLayout >           hPipelineLayout;
    apemodevk::THandle< VkPipelineCache >            hPipelineCache;
    apemodevk::THandle< VkPipeline >                 hPipeline;
    apemodevk::THandle< VkPipelineCache >            hLinePipelineCache;
    apemodevk::THandle< VkPipeline >                 hLinePipeline;
    apemodevk::HostBufferPool                        BufferPools[ kMaxFrameCount ];
    apemodevk::DescriptorSetPool                     DescSetPools[ kMaxFrameCount ];

    apemodevk::THandle< apemodevk::BufferComposite > hCubeBuffer;
    apemodevk::THandle< apemodevk::BufferComposite > hBoneBuffer;
    apemodevk::vector< XMFLOAT3 >                    LineStagingBuffer;

    bool RecreateResources( InitParameters *pInitParams );

    void Reset( uint32_t frameIndex );
    bool Render( const RenderCubeParameters *pRenderParams );
    bool Render( const Scene *pScene, const RenderSceneParameters *pRenderParams );
    void Flush( uint32_t frameIndex );
};

} // namespace vk
} // namespace apemode
