#pragma once

#include <apemode/vk/Buffer.Vulkan.h>
#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/DescriptorPool.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>

#include <apemode/platform/IAssetManager.h>
#include <apemode/platform/MathInc.h>

namespace apemode {
namespace vk {

struct DebugRenderer {
    struct PositionVertex {
        float Position[ 3 ];
    };

    struct SkyboxUBO {
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

    struct RenderParameters {
        float           Dims[ 2 ]  = {};             /* Required */
        float           Scale[ 2 ] = {};             /* Required */
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
        uint32_t        FrameIndex = 0;              /* Required */
        SkyboxUBO *     pFrameData = nullptr;        /* Required */
    };

    static uint32_t const kMaxFrameCount = 3;

    apemodevk::THandle< VkDescriptorSetLayout >      hDescSetLayout;
    apemodevk::THandle< VkPipelineLayout >           hPipelineLayout;
    apemodevk::THandle< VkPipelineCache >            hPipelineCache;
    apemodevk::THandle< VkPipeline >                 hPipeline;
    apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
    apemodevk::HostBufferPool                        BufferPools[ kMaxFrameCount ];
    apemodevk::DescriptorSetPool                     DescSetPools[ kMaxFrameCount ];

    bool RecreateResources( InitParameters *pInitParams );

    void Reset( uint32_t frameIndex );
    bool Render( RenderParameters *pRenderParams );
    void Flush( uint32_t frameIndex );
};

} // namespace vk
} // namespace apemode