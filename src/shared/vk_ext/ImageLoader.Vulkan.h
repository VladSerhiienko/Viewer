#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Image.Vulkan.h>

namespace apemodevk {
    class GraphicsDevice;
    struct HostBufferPool;

    struct LoadedImage {
        THandle< ImageComposite > hImg;
        THandle< VkImageView >    hImgView;
        VkImageCreateInfo         ImageCreateInfo;
        VkImageViewCreateInfo     ImgViewCreateInfo;
        VkImageLayout             eImgLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t                  QueueId       = kInvalidIndex;
        uint32_t                  QueueFamilyId = kInvalidIndex;
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
        std::unique_ptr< LoadedImage > LoadImageFromData( const std::vector< uint8_t >& fileContent,
                                                          EImageFileFormat              eFileFormat,
                                                          bool                          bImgView,
                                                          bool                          bAwaitLoading );
    };
}