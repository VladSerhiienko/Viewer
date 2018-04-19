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
            float pos[ 3 ];
        };

        struct FrameUniformBuffer {
            XMFLOAT4X4 worldMatrix;
            XMFLOAT4X4 viewMatrix;
            XMFLOAT4X4 projectionMatrix;
            XMFLOAT4   color;
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
            FrameUniformBuffer *pFrameData = nullptr;        /* Required */
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

        bool RecreateResources( InitParametersVk *initParams );

        void Reset( uint32_t FrameIndex );
        bool Render( RenderParametersVk *renderParams );
        void Flush( uint32_t FrameIndex );
    };
} // namespace apemode
