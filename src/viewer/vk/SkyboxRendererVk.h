#pragma once

#include <apemode/vk/BufferPools.Vulkan.h>
#include <apemode/vk/DescriptorPool.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>

#include <apemode/platform/IAssetManager.h>
#include <apemode/platform/MathInc.h>

#ifndef kMaxFrameCount
#define kMaxFrameCount 3
#endif

namespace apemode {
namespace vk {
using namespace DirectX;

class Skybox {
public:
    VkSampler     pSampler;
    uint32_t      Dimension;
    uint32_t      LevelOfDetail;
    float         Exposure;
    VkImageView   pImgView;
    VkImageLayout eImgLayout;
};

class SkyboxRenderer {
public:
    struct RecreateParameters {
        apemodevk::GraphicsDevice*        pNode         = nullptr;        /* Required */
        apemode::platform::IAssetManager* pAssetManager = nullptr;        /* Required */
        VkDescriptorPool                  pDescPool     = VK_NULL_HANDLE; /* Required */
        VkRenderPass                      pRenderPass   = VK_NULL_HANDLE; /* Required */
        uint32_t                          FrameCount    = 0;              /* Required */
    };

    bool Recreate( RecreateParameters* pParams );

    struct RenderParameters {
        apemodevk::GraphicsDevice* pNode       = nullptr;        /* Required */
        VkCommandBuffer            pCmdBuffer  = VK_NULL_HANDLE; /* Required */
        float                      FieldOfView = 0;              /* Required */
        uint32_t                   FrameIndex  = 0;              /* Required */
        XMFLOAT2                   Dims;                         /* Required */
        XMFLOAT2                   Scale;                        /* Required */
        XMFLOAT4X4                 InvViewMatrix;                /* Required */
        XMFLOAT4X4                 InvProjMatrix;                /* Required */
        XMFLOAT4X4                 ProjBiasMatrix;               /* Required */
    };

    void Reset( uint32_t FrameIndex );
    bool Render( Skybox* pSkybox, RenderParameters* pParams );
    void Flush( uint32_t FrameIndex );

    apemodevk::GraphicsDevice*                   pNode = nullptr;
    apemodevk::THandle< VkDescriptorSetLayout >  hDescSetLayout;
    apemodevk::THandle< VkPipelineLayout >       hPipelineLayout;
    apemodevk::THandle< VkPipelineCache >        hPipelineCache;
    apemodevk::THandle< VkPipeline >             hPipeline;
    apemodevk::HostBufferPool                    BufferPools[ kMaxFrameCount ];
    apemodevk::DescriptorSetPool                 DescSetPools[ kMaxFrameCount ];
};
} // namespace vk
} // namespace apemode