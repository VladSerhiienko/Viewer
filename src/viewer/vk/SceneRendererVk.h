#pragma once

#include <BufferPools.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <SamplerManager.Vulkan.h>
#include <SceneRendererBase.h>
#include <SceneUploaderVk.h>

#include <MathInc.h>

namespace apemode {
struct Scene;

namespace vk {

class SceneRendererVk : public SceneRendererBase {
public:
    SceneRendererVk( )          = default;
    virtual ~SceneRendererVk( ) = default;

    struct RecreateParametersVk : RecreateParametersBase {
        apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
        apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
        VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
        VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
        uint32_t                   FrameCount      = 0;              /* Required */
    };

    bool Recreate( const RecreateParametersBase* pParams ) override;

    struct SceneRenderParametersVk : SceneRenderParametersBase {
        class EnvMap {
        public:
            VkSampler     pSampler;
            VkImageView   pImgView;
            VkImageLayout eImgLayout;
            uint32_t      MipLevels;
        };

        apemodevk::GraphicsDevice* pNode = nullptr;             /* Required */
        XMFLOAT2                   Dims;                        /* Required */
        XMFLOAT2                   Scale;                       /* Required */
        uint32_t                   FrameIndex = 0;              /* Required */
        VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
        XMFLOAT4X4                 RootMatrix;                  /* Required */
        XMFLOAT4X4                 ViewMatrix;                  /* Required */
        XMFLOAT4X4                 ProjMatrix;                  /* Required */
        XMFLOAT4X4                 InvViewMatrix;               /* Required */
        XMFLOAT4X4                 InvProjMatrix;               /* Required */
        EnvMap                     RadianceMap;                 /* Required */
        EnvMap                     IrradianceMap;               /* Required */
        XMFLOAT4                   LightDirection;              /* Required */
        XMFLOAT4                   LightColor;                  /* Required */
    };

    bool Reset( const Scene* pScene, uint32_t FrameIndex ) override;
    bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    bool Flush( const Scene* pScene, uint32_t FrameIndex ) override;

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

} // namespace vk
} // namespace apemode