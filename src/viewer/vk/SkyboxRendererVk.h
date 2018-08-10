#pragma once

#include <MathInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ImageUploader.Vulkan.h>

#ifndef kMaxFrameCount
#define kMaxFrameCount 3
#endif

namespace apemodevk {
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
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        bool Recreate( RecreateParameters* pParams );

        struct RenderParameters {
            apemodevk::GraphicsDevice* pNode       = nullptr;       /* Required */
            VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
            float                      FieldOfView = 0;             /* Required */
            uint32_t                   FrameIndex  = 0;             /* Required */
            XMFLOAT2                   Dims;                        /* Required */
            XMFLOAT2                   Scale;                       /* Required */
            XMFLOAT4X4                 InvViewMatrix;               /* Required */
            XMFLOAT4X4                 InvProjMatrix;               /* Required */
            XMFLOAT4X4                 ProjBiasMatrix;              /* Required */
        };

        void Reset( uint32_t FrameIndex );
        bool Render( Skybox* pSkybox, RenderParameters* pParams );
        void Flush( uint32_t FrameIndex );

        apemodevk::GraphicsDevice*                   pNode = nullptr;
        apemodevk::THandle< VkDescriptorSetLayout >  hDescSetLayout;
        apemodevk::THandle< VkPipelineLayout >       hPipelineLayout;
        apemodevk::THandle< VkPipelineCache >        hPipelineCache;
        apemodevk::THandle< VkPipeline >             hPipeline;
        apemodevk::TDescriptorSets< kMaxFrameCount > DescSets;
        apemodevk::HostBufferPool                    BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                 DescSetPools[ kMaxFrameCount ];
    };
}