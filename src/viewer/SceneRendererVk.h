#pragma once

#include <MathInc.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <SceneRendererBase.h>

#ifndef kMaxFrameCount
#define kMaxFrameCount 3
#endif

namespace apemode {
    struct Scene;

    class SceneRendererVk : public SceneRendererBase {
    public:
        struct RecreateParametersVk : RecreateParametersBase {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        SceneRendererVk() {}
        virtual ~SceneRendererVk() {}

        virtual bool Recreate( const RecreateParametersBase* pParams ) override;
        virtual bool UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {
            apemodevk::GraphicsDevice* pNode = nullptr;             /* Required */
            XMFLOAT2                   dims;                        /* Required */
            XMFLOAT2                   scale;                       /* Required */
            uint32_t                   FrameIndex = 0;              /* Required */
            VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
            XMFLOAT4X4                 ViewMatrix;                  /* Required */
            XMFLOAT4X4                 ProjMatrix;                  /* Required */
        };

        virtual bool Reset( const Scene* pScene, uint32_t FrameIndex ) override;
        virtual bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
        virtual bool Flush( const Scene* pScene, uint32_t FrameIndex ) override;

        apemodevk::GraphicsDevice*                              pNode = nullptr;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDescriptorSets< kMaxFrameCount >            DescSets;
        apemodevk::HostBufferPool                               BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                            DescSetPools[ kMaxFrameCount ];
    };
}