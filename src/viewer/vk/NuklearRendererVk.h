#pragma once

#include <viewer/NuklearRendererBase.h>

#include <apemode/vk/Buffer.Vulkan.h>
#include <apemode/vk/DescriptorPool.Vulkan.h>
#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk/GraphicsManager.Vulkan.h>
#include <apemode/vk/Image.Vulkan.h>
#include <apemode/vk/NativeHandles.Vulkan.h>
#include <apemode/vk/TInfoStruct.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>
#include <apemode/vk_ext/SamplerManager.Vulkan.h>

namespace apemode {
namespace vk {

class NuklearRenderer : public NuklearRendererBase {
public:
    /* User either provides a command buffer or a queue (with family id).
     * In case the command buffer is null, it will be allocated,
     * submitted to the queue and synchonized.
     */
    struct InitParameters : InitParametersBase {
        apemodevk::GraphicsDevice *       pNode           = nullptr;        /* Required */
        apemodevk::SamplerManager *       pSamplerManager = nullptr;        /* Required */
        apemode::platform::IAssetManager *pAssetManager   = nullptr;        /* Required */
        VkDescriptorPool                  pDescPool       = VK_NULL_HANDLE; /* Required */
        VkRenderPass                      pRenderPass     = VK_NULL_HANDLE; /* Required */
        uint32_t                          FrameCount      = 0;              /* Required, swapchain img count typically */
    };

    struct RenderParameters : RenderParametersBase {
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
        uint32_t        FrameIndex = 0;              /* Required */
    };

    struct Frame {
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer;
        apemodevk::DescriptorSetPool                     DescSetPool;
    };

    apemodevk::GraphicsDevice *                       pNode = nullptr;
    VkSampler                                         hFontSampler;
    apemodevk::THandle< VkDescriptorSetLayout >       hDescSetLayout;
    apemodevk::THandle< VkPipelineLayout >            hPipelineLayout;
    apemodevk::THandle< VkPipelineCache >             hPipelineCache;
    apemodevk::THandle< VkPipeline >                  hPipeline;
    apemodevk::unique_ptr< apemodevk::UploadedImage > FontUploadedImg;
    apemodevk::vector< Frame >                        Frames;

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