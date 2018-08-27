#pragma once

#include <NuklearSdlBase.h>

#include <Buffer.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <Image.Vulkan.h>
#include <SamplerManager.Vulkan.h>
#include <NativeHandles.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemode {
namespace vk {

class NuklearRenderer : public NuklearRendererSdlBase {
public:
    /* User either provides a command buffer or a queue (with family id).
     * In case the command buffer is null, it will be allocated,
     * submitted to the queue and synchonized.
     */
    struct InitParametersVk : InitParametersBase {
        apemodevk::GraphicsDevice *pNode           = nullptr;        /* Required */
        apemodevk::SamplerManager *pSamplerManager = nullptr;        /* Required */
        apemodeos::IAssetManager * pAssetManager   = nullptr;        /* Required */
        VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
        VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
        VkCommandBuffer            pCmdBuffer      = VK_NULL_HANDLE; /* Optional (for uploading font img) */
        VkQueue                    pQueue          = VK_NULL_HANDLE; /* Optional (for uploading font img) */
        uint32_t                   QueueFamilyId   = 0;              /* Optional (for uploading font img) */
        uint32_t                   FrameCount      = 0;              /* Required, swapchain img count typically */
    };

    struct RenderParametersVk : RenderParametersBase {
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
        uint32_t        FrameIndex = 0;              /* Required */
    };

public:
    apemodevk::GraphicsDevice *                       pNode       = nullptr;
    VkDescriptorPool                                  pDescPool   = VK_NULL_HANDLE;
    VkRenderPass                                      pRenderPass = VK_NULL_HANDLE;
    VkCommandBuffer                                   pCmdBuffer  = VK_NULL_HANDLE;
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

    // apemodevk::THandle< apemodevk::BufferComposite > hUploadBuffer;
    // apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer[ kMaxFrameCount ];
    // apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer[ kMaxFrameCount ];
    // apemodevk::THandle< apemodevk::BufferComposite > hUniformBuffer[ kMaxFrameCount ];
    // uint32_t                                         VertexBufferSize[ kMaxFrameCount ] = {0};
    // uint32_t                                         IndexBufferSize[ kMaxFrameCount ]  = {0};
    // apemodevk::DescriptorSetPool                     DescSetPools[ kMaxFrameCount ];

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