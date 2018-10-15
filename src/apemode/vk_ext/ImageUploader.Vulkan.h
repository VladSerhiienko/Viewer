#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>
#include <apemode/vk/Image.Vulkan.h>

namespace apemodevk {

    /** @brief ISourceImage interface is used by ImageLoader for creating textures out of it. */
    class APEMODEVK_API ISourceImage {
    public:
        virtual ~ISourceImage( )                                               = default;
        virtual VkImageViewType GetImageViewType( ) const                      = 0; /** @return Image view type. */
        virtual VkImageType     GetImageType( ) const                          = 0; /** @return Image type. */
        virtual VkFormat        GetFormat( ) const                             = 0; /** @return Image format. */
        virtual const void*     GetData( uint32_t face, uint32_t level ) const = 0; /** @return A pointer to a image face buffer at the given mip level. */
        virtual VkExtent3D      GetExtent( uint32_t level ) const              = 0; /** @return Image extent at the given mip level. */
        virtual VkDeviceSize    GetSize( uint32_t level ) const                = 0; /** @return The byte size of the image face at the given mip level. */
        virtual VkDeviceSize    GetSize( ) const                               = 0; /** @return The byte size of the image (all faces and mip levels). */
        virtual uint32_t        GetMipLevels( ) const                          = 0; /** @return The number of mip levels in the image. */
        virtual uint32_t        GetFaces( ) const                              = 0; /** @return The number of faces in the image. */
    };

    /** @brief ImageDecoder class for decoding image file bufferes to ISourceImage instances. */
    class APEMODEVK_API ImageDecoder {
    public:
        struct DecodeOptions {
            enum EImageFileFormat {
                eImageFileFormat_Autodetect = 0,
                eImageFileFormat_DDS,
                eImageFileFormat_KTX,
                eImageFileFormat_PNG,
                // TODO PVR
            };

            EImageFileFormat eFileFormat      = eImageFileFormat_Autodetect;
            bool             bGenerateMipMaps = false;
        };

        /**
         * @brief Creates a new ISourceImage instance out of the provided 2D image data.
         * @param pImgBytes The image buffer.
         * @param imgExtent The image width and height.
         * @param eImgFmt The format of the image buffer.
         * @param decodeOptions The options to apply (for example, enable mip map generation).
         * @return The ISourceImage instance.
         */
        unique_ptr< ISourceImage > CreateSourceImage2D( const uint8_t*       pImgBytes,
                                                        VkExtent2D           imgExtent,
                                                        VkFormat             eImgFmt,
                                                        const DecodeOptions& decodeOptions );

        /**
         * @brief Creates a new ISourceImage instance from the image file buffer.
         * @param pFileContent The image file buffer.
         * @param fileContentSize The size of image file buffer.
         * @param decodeOptions The options to apply (for example, enable mip map generation or specify file extention).
         * @return The ISourceImage instance.
         */
        unique_ptr< ISourceImage > DecodeSourceImageFromData( const uint8_t*       pFileContent,
                                                              size_t               fileContentSize,
                                                              const DecodeOptions& decodeOptions );
    };

    /** @brief LoadedImage struct contains a VkImage with the optional VkImageView, and their descriptors. */
    struct APEMODEVK_API UploadedImage {
        THandle< ImageComposite > hImg;              /**! @brief The image. */
        THandle< VkImageView >    hImgView;          /**! @brief The image view. */
        VkImageCreateInfo         ImgCreateInfo;     /**! @brief The image descriptor. */
        VkImageViewCreateInfo     ImgViewCreateInfo; /**! @brief The image view descriptor. */
        VkImageLayout             eImgLayout;        /**! @brief The image layout. */
        VkAccessFlags             eImgAccess;        /**! @brief The image access bits.*/
        VkPipelineStageFlags      ePipelineStage;    /**! @brief The image pipeline stage.*/

        ~UploadedImage( );
        void Destroy( );
    };

    /** @brief ImageLoader class create GPU images according to ISourceImage instances. */
    class APEMODEVK_API ImageUploader {
    public:
        /** @brief LoadOptions contains properties to customize the usage and loading of GPU images. */
        struct UploadOptions {
            /** @brief Staging buffer is sufficient to store the entire source image. */
            static constexpr size_t kStagingMemoryLimitHint_Unlimited = ~0ULL;
            /** @brief Tries to use the staging buffer of the least size. */
            static constexpr size_t kStagingMemoryLimitHint_Optimal = 0;
            static constexpr size_t kStagingMemoryLimitHint_16KB = 16 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_64KB = 64 * 1024;
            static constexpr size_t kStagingMemoryLimitHint_2MB = 2 * 1024 * 1024;

            bool                    bImgView               = false;
            size_t                  StagingMemoryLimitHint = kStagingMemoryLimitHint_Optimal;
            uint32_t                QueueFamilyId          = 0;
            VkImageUsageFlags       eImgUsage              = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageTiling           eImgTiling             = VK_IMAGE_TILING_OPTIMAL;
            VkSharingMode           eImgSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
            VkImageAspectFlagBits   eImgAspect             = VK_IMAGE_ASPECT_COLOR_BIT;
            VkAccessFlagBits        eImgDstAccess          = VK_ACCESS_SHADER_READ_BIT;
            VkImageLayout           eImgDstLayout          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkPipelineStageFlagBits eDstPipelineStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        };

        ImageUploader() = default;
        ~ImageUploader() = default;

        unique_ptr< UploadedImage > UploadImage( GraphicsDevice*     pNode,
                                                 const ISourceImage& srcImg,
                                                 const UploadOptions&  loadOptions );
    };
}