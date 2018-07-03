#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Image.Vulkan.h>

namespace apemodevk {
    struct HostBufferPool;

    struct LoadedImage {
        THandle< ImageComposite > hImg;
        THandle< VkImageView >    hImgView;
        VkImageCreateInfo         ImageCreateInfo;
        VkImageViewCreateInfo     ImgViewCreateInfo;
        VkImageLayout             eImgLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t                  QueueId       = kInvalidIndex;
        uint32_t                  QueueFamilyId = kInvalidIndex;

        ~LoadedImage( );
        void Destroy( );
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
            EImageFileFormat eFileFormat        = eImageFileFormat_Undefined;
            bool             bImgView           = false;
            bool             bAwaitLoading      = true;
            bool             bGenerateMipMaps   = false;
            size_t           StagingMemoryLimit = 64 * 1024 * 1024;
        };

        GraphicsDevice* pNode           = nullptr;
        HostBufferPool* pHostBufferPool = nullptr;

        ImageLoader() = default;
        ~ImageLoader();

        bool Recreate( GraphicsDevice* pNode );
        void Destroy( );

        apemodevk::unique_ptr< LoadedImage > LoadImageFromRawImgRGBA8( const uint8_t*     pImageBytes,
                                                                       uint16_t           imageWidth,
                                                                       uint16_t           imageHeight,
                                                                       LoadOptions const& loadOptions );

        apemodevk::unique_ptr< LoadedImage > LoadImageFromFileData( const uint8_t*     pFileContent,
                                                                    size_t             fileContentSize,
                                                                    LoadOptions const& loadOptions );
    };
}