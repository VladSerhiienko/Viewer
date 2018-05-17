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
            eImageFileFormat_Undefined,
            eImageFileFormat_DDS,
            eImageFileFormat_KTX,
            eImageFileFormat_PNG,
            // TODO PVR
        };

        struct LoadOptions {
            EImageFileFormat eFileFormat      = eImageFileFormat_Undefined;
            bool             bImgView         = false;
            bool             bAwaitLoading    = true;
            bool             bGenerateMipMaps = false;
        };

        GraphicsDevice* pNode           = nullptr;
        HostBufferPool* pHostBufferPool = nullptr;

        bool Recreate( GraphicsDevice* pNode, HostBufferPool* pHostBufferPool = nullptr );

        std::unique_ptr< LoadedImage > LoadImageFromData( const uint8_t*     pFileContent,
                                                          size_t             fileContentSize,
                                                          LoadOptions const& loadOptions );
    };
}