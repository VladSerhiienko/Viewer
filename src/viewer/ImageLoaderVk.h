#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;
    struct HostBufferPool;

    struct LoadedImage {
        apemodevk::TDispatchableHandle< VkImage >        hImg;
        apemodevk::TDispatchableHandle< VkImageView >    hImgView;
        apemodevk::TDispatchableHandle< VkDeviceMemory > hImgMemory;
        VkImageCreateInfo                                imageCreateInfo;
        VkImageViewCreateInfo                            imageViewCreateInfo;
        VkImageLayout                                    eImgLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t                                         queueId       = 0;
        uint32_t                                         queueFamilyId = 0;
    };

    class ImageLoader {
    public:
        enum EImageFileFormat {
            eImageFileFormat_DDS,
            eImageFileFormat_KTX,
            eImageFileFormat_PNG,
            // TODO PVR
        };

        GraphicsDevice* pNode           = nullptr;
        HostBufferPool* pHostBufferPool = nullptr;

        bool Recreate( GraphicsDevice* pNode, HostBufferPool* pHostBufferPool = nullptr );
        std::unique_ptr< LoadedImage > LoadImageFromData( const std::vector< uint8_t >& InFileContent,
                                                          EImageFileFormat              eFileFormat,
                                                          bool                          bImgView,
                                                          bool                          bAwaitLoading );
    };
}