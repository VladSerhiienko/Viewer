#pragma once

#include <MathInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <Buffer.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>

namespace apemode {

    struct DebugRendererVk {
        struct PositionVertex {
            float Position[ 3 ];
        };

        struct SkyboxUBO {
            XMFLOAT4X4 WorldMatrix;
            XMFLOAT4X4 ViewMatrix;
            XMFLOAT4X4 ProjMatrix;
            XMFLOAT4   Color;
        };

        struct InitParametersVk {
            apemodevk::GraphicsDevice *pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler *pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required, swapchain img count typically */
        };

        struct RenderParametersVk {
            float               Dims[ 2 ]  = {};             /* Required */
            float               Scale[ 2 ] = {};             /* Required */
            VkCommandBuffer     pCmdBuffer = VK_NULL_HANDLE; /* Required */
            uint32_t            FrameIndex = 0;              /* Required */
            SkyboxUBO *pFrameData = nullptr;        /* Required */
        };

        static uint32_t const kMaxFrameCount = 3;

        apemodevk::TDescriptorSets< kMaxFrameCount >     DescSets;
        apemodevk::THandle< VkDescriptorSetLayout >      hDescSetLayout;
        apemodevk::THandle< VkPipelineLayout >           hPipelineLayout;
        apemodevk::THandle< VkPipelineCache >            hPipelineCache;
        apemodevk::THandle< VkPipeline >                 hPipeline;
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::HostBufferPool                        BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                     DescSetPools[ kMaxFrameCount ];

        bool RecreateResources( InitParametersVk *pInitParams );

        void Reset( uint32_t frameIndex );
        bool Render( RenderParametersVk *pRenderParams );
        void Flush( uint32_t frameIndex );
    };
} // namespace apemode
