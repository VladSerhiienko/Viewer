#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <Image.Vulkan.h>

namespace apemodevk {

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
        virtual uint32_t        GetMipLevels( ) const                          = 0;
        virtual uint32_t        GetFaces( ) const                              = 0;
    };

    class ImageDecoder {
    public:
        struct DecodeOptions {
            enum EImageFileFormat {
                eImageFileFormat_Undefined = 0,
                eImageFileFormat_DDS,
                eImageFileFormat_KTX,
                eImageFileFormat_PNG,
                // TODO PVR
            };

            EImageFileFormat eFileFormat      = eImageFileFormat_Undefined;
            bool             bGenerateMipMaps = false;
        };

        unique_ptr< ISourceImage > CreateSourceImage2D( const uint8_t*       pImgBytes,
                                                        VkExtent2D           imgExtent,
                                                        VkFormat             eImgFmt,
                                                        const DecodeOptions& decodeOptions );

        unique_ptr< ISourceImage > DecodeSourceImageFromData( const uint8_t*       pFileContent,
                                                              size_t               fileContentSize,
                                                              const DecodeOptions& decodeOptions );
    };

    struct LoadedImage {
        THandle< ImageComposite > hImg;
        THandle< VkImageView >    hImgView;
        VkImageCreateInfo         ImgCreateInfo;
        VkImageViewCreateInfo     ImgViewCreateInfo;
        VkImageLayout             eImgLayout    = VK_IMAGE_LAYOUT_UNDEFINED;

        ~LoadedImage( );
        void Destroy( );
    };

    class ImageLoader {
    public:
        struct LoadOptions {
            static constexpr size_t kStagingMemoryLimitHint_Optimal   = 0;
            static constexpr size_t kStagingMemoryLimitHint_16KB      = 16 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_64KB      = 64 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_2MB       = 2 * 1024 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_Unlimited = ~0ULL;

            size_t                  StagingMemoryLimitHint = kStagingMemoryLimitHint_Optimal;
            bool                    bImgView               = false;
            uint32_t                QueueFamilyId          = 0;
            VkImageLayout           eImgDstLayout          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkAccessFlagBits        eImgDstAccess          = VK_ACCESS_SHADER_READ_BIT;
            VkSharingMode           eImgSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
            VkImageTiling           eImgTiling             = VK_IMAGE_TILING_OPTIMAL;
            VkImageUsageFlags       eImgUsage              = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageAspectFlagBits   eImgAspect             = VK_IMAGE_ASPECT_COLOR_BIT;
            VkPipelineStageFlagBits eDstPipelineStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        };

        ImageLoader() = default;
        ~ImageLoader() = default;

        unique_ptr< LoadedImage > LoadImageFromSrc( GraphicsDevice*     pNode,
                                                    const ISourceImage& srcImg,
                                                    const LoadOptions&  loadOptions );
    };
}