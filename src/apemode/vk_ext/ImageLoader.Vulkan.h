#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Image.Vulkan.h>

namespace apemodevk {
    struct HostBufferPool;

    class ISourceImage {
    public:
        virtual ~ISourceImage( )                                               = default;
        virtual VkImageViewType GetImageViewType( ) const                      = 0;
        virtual VkImageType     GetImageType( ) const                          = 0;
        virtual VkFormat        GetFormat( ) const                             = 0;
        virtual const void*     GetData( uint32_t face, uint32_t level ) const = 0;
        virtual VkExtent3D      GetExtent( uint32_t level ) const              = 0;
        virtual VkDeviceSize    GetSize( uint32_t level ) const                = 0;
        virtual VkDeviceSize    GetSize( ) const                               = 0;
    };


    class ImageDecoder {
    public:
        struct DecodeOptions {
            enum EImageFileFormat {
                eImageFileFormat_Undefined,
                eImageFileFormat_DDS,
                eImageFileFormat_KTX,
                eImageFileFormat_PNG,
                // TODO PVR
            };

            EImageFileFormat eFileFormat      = eImageFileFormat_Undefined;
            bool             bGenerateMipMaps = false;
        };

        unique_ptr< ISourceImage > CreateSourceImage2D( const uint8_t*       pImageBytes,
                                                        VkExtent2D           imageExtent,
                                                        VkFormat             eImgFmt,
                                                        const DecodeOptions& decodeOptions );

        unique_ptr< ISourceImage > DecodeSourceImageFromData( const uint8_t*       pFileContent,
                                                              size_t               fileContentSize,
                                                              const DecodeOptions& decodeOptions );
    };

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

        struct LoadOptions {
            static constexpr size_t kStagingMemoryLimitHint_OptimalMemoryUsage = 0;
            static constexpr size_t kStagingMemoryLimitHint_16KB               = 16 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_64KB               = 64 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_2MB                = 2 * 1024 * 1024;

            EImageFileFormat eFileFormat            = eImageFileFormat_Undefined;
            bool             bImgView               = false;
            bool             bAwaitLoading          = true;
            bool             bGenerateMipMaps       = false;
            size_t           StagingMemoryLimitHint = kStagingMemoryLimitHint_OptimalMemoryUsage;
        };

        GraphicsDevice* pNode = nullptr;

        ImageLoader() = default;
        ~ImageLoader();

        bool Recreate( GraphicsDevice* pNode );
        void Destroy( );

        unique_ptr< LoadedImage > LoadImageFromRawImgRGBA8( const uint8_t*     pImageBytes,
                                                            uint16_t           imageWidth,
                                                            uint16_t           imageHeight,
                                                            LoadOptions const& loadOptions );

        unique_ptr< LoadedImage > LoadImageFromFileData( const uint8_t*     pFileContent,
                                                         size_t             fileContentSize,
                                                         LoadOptions const& loadOptions );
    };
}