#pragma once

#include <viewer/NuklearRendererBase.h>

#include <Buffer.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <Image.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <SamplerManager.Vulkan.h>
#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemode {
namespace vk {

class NuklearRenderer : public NuklearRendererBase {
public:
    /* User either provides a command buffer or a queue (with family id).
     * In case the command buffer is null, it will be allocated,
     * submitted to the queue and synchonized.
     */
    struct InitParameters : InitParametersBase {
        apemodevk::GraphicsDevice *pNode           = nullptr;        /* Required */
        apemodevk::SamplerManager *pSamplerManager = nullptr;        /* Required */
        apemodeos::IAssetManager * pAssetManager   = nullptr;        /* Required */
        VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
        VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
        uint32_t                   FrameCount      = 0;              /* Required, swapchain img count typically */
    };

    struct RenderParameters : RenderParametersBase {
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
        uint32_t        FrameIndex = 0;              /* Required */
    };

public:
    apemodevk::GraphicsDevice *                       pNode = nullptr;
    VkSampler                                         hFontSampler;
    apemodevk::THandle< VkDescriptorSetLayout >       hDescSetLayout;
    apemodevk::THandle< VkPipelineLayout >            hPipelineLayout;
    apemodevk::THandle< VkPipelineCache >             hPipelineCache;
    apemodevk::THandle< VkPipeline >                  hPipeline;
    apemodevk::unique_ptr< apemodevk::UploadedImage > FontUploadedImg;

    static uint32_t const kMaxFrameCount = 3;
    struct {
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer;
        apemodevk::DescriptorSetPool                     DescSetPool;
    } Frames[ kMaxFrameCount ];

public:
    virtual ~NuklearRenderer( );
    void DeviceDestroy( ) override;

    bool  Render( RenderParametersBase *pRenderParams ) override;   /* RenderParametersVk* */
    bool  DeviceCreate( InitParametersBase *pInitParams ) override; /* InitParametersVk* */
    void *DeviceUploadAtlas( InitParametersBase *pInitParams,       /* InitParametersVk* */
                             const void *        pImgData,
                             uint32_t            imgWidth,
                             uint32_t            imgHeight ) override;
};

} // namespace vk
} // namespace apemode