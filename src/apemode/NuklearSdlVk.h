#pragma once

#include <NuklearSdlBase.h>

#include <Buffer.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <Image.Vulkan.h>
#include <NativeHandles.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

namespace apemode {

    class NuklearRendererSdlVk : public NuklearRendererSdlBase {
    public:
        /* User either provides a command buffer or a queue (with family id).
         * In case the command buffer is null, it will be allocated,
         * submitted to the queue and synchonized.
         */
        struct InitParametersVk : InitParametersBase {
            apemodevk::GraphicsDevice *pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler *pShaderCompiler = nullptr;        /* Required */
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
        static uint32_t const kMaxFrameCount = 3;

        apemodevk::GraphicsDevice *                     pNode       = nullptr;
        VkDescriptorPool                                pDescPool   = VK_NULL_HANDLE;
        VkRenderPass                                    pRenderPass = VK_NULL_HANDLE;
        VkCommandBuffer                                 pCmdBuffer  = VK_NULL_HANDLE;
        apemodevk::TDescriptorSets< 1 >                 DescSet;
        apemodevk::THandle< VkSampler >                 hFontSampler;
        apemodevk::THandle< VkDescriptorSetLayout >     hDescSetLayout;
        apemodevk::THandle< VkPipelineLayout >          hPipelineLayout;
        apemodevk::THandle< VkPipelineCache >           hPipelineCache;
        apemodevk::THandle< VkPipeline >                hPipeline;
        apemodevk::THandle< apemodevk::ImageComposite > hFontImg;
        apemodevk::THandle< VkImageView >               hFontImgView;

        apemodevk::THandle< apemodevk::BufferComposite > hUploadBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer[ kMaxFrameCount ];
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer[ kMaxFrameCount ];
        apemodevk::THandle< apemodevk::BufferComposite > hUniformBuffer[ kMaxFrameCount ];
        uint32_t                                         VertexBufferSize[ kMaxFrameCount ] = {0};
        uint32_t                                         IndexBufferSize[ kMaxFrameCount ]  = {0};

        std::vector< uint8_t > BufferContent;

    public:
        virtual ~NuklearRendererSdlVk( );
        void  DeviceDestroy( ) override;

        bool  Render( RenderParametersBase *pRenderParams ) override;   /* RenderParametersVk* */
        bool  DeviceCreate( InitParametersBase *pInitParams ) override; /* InitParametersVk* */
        void *DeviceUploadAtlas( InitParametersBase *pInitParams, /* InitParametersVk* */
                                 const void *        pImgData,
                                 uint32_t            imgWidth,
                                 uint32_t            imgHeight ) override;
    };

} // namespace apemode