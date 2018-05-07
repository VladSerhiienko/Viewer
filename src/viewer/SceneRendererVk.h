#pragma once

#include <MathInc.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <SceneRendererBase.h>
#include <ImageLoader.Vulkan.h>
#include <SamplerManager.Vulkan.h>

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
            apemodevk::ImageLoader*    pImgLoader      = nullptr;        /* Required */
            apemodevk::SamplerManager* pSamplerManager = nullptr;        /* Required */
        };

        SceneRendererVk() {}
        virtual ~SceneRendererVk() {}

        virtual bool Recreate( const RecreateParametersBase* pParams ) override;
        virtual bool UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {
            class EnvMap {
            public:
                VkSampler     pSampler;
                VkImageView   pImgView;
                VkImageLayout eImgLayout;
            };

            apemodevk::GraphicsDevice* pNode = nullptr;             /* Required */
            XMFLOAT2                   Dims;                        /* Required */
            XMFLOAT2                   Scale;                       /* Required */
            uint32_t                   FrameIndex = 0;              /* Required */
            VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
            XMFLOAT4X4                 ViewMatrix;                  /* Required */
            XMFLOAT4X4                 ProjMatrix;                  /* Required */
            EnvMap                     RadianceMap;                 /* Required */
            EnvMap                     IrradianceMap;               /* Required */
        };

        virtual bool Reset( const Scene* pScene, uint32_t FrameIndex ) override;
        virtual bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
        virtual bool Flush( const Scene* pScene, uint32_t FrameIndex ) override;

        static constexpr uint32_t kMaxFrameCount        = 3;
        static constexpr uint32_t kDescriptorSetCount   = 2;
        static constexpr uint32_t kDescriptorSetForPass = 0;
        static constexpr uint32_t kDescriptorSetForObj  = 1;

        apemodevk::GraphicsDevice*                  pNode = nullptr;
        apemodevk::THandle< VkDescriptorSetLayout > hDescriptorSetLayouts[ kDescriptorSetCount ];
        apemodevk::THandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::THandle< VkPipelineCache >       hPipelineCache;
        apemodevk::THandle< VkPipeline >            hPipeline;
        apemodevk::HostBufferPool                   BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                DescriptorSetPools[ kMaxFrameCount ][ kDescriptorSetCount ];
    };
}