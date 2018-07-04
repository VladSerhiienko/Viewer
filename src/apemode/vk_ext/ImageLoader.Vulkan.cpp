#include <BufferPools.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <MemoryManager.h>
#include <QueuePools.Vulkan.h>
#include <TOneTimeCmdBufferSubmit.Vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(disable:4309)
#include <gli/gli.hpp>
#include <gli/generate_mipmaps.hpp>
#pragma warning(default:4309)

namespace apemodevk {
    class SourceImage : public ISourceImage {
    public:
        SourceImage( gli::texture texture );
        ~SourceImage( ) override = default;
        VkImageViewType GetImageViewType( ) const override;
        VkImageType     GetImageType( ) const override;
        VkFormat        GetFormat( ) const override;
        VkDeviceSize    GetSize( uint32_t level ) const override;
        VkDeviceSize    GetSize( ) const override;
        const void*     GetData( uint32_t face, uint32_t level ) const override;
        VkExtent3D      GetExtent( uint32_t level ) const override;
        uint32_t        GetMipLevels( ) const override;
        uint32_t        GetFaces( ) const override;

    private:
        gli::texture Texture;
    };
} // namespace apemodevk

gli::texture DuplicateWithMipMaps( const gli::texture& originalTexture ) {
    apemodevk_memory_allocation_scope;

    if ( originalTexture.empty( ) ) {
        return gli::texture( );
    }

    const uint32_t maxExtent = std::max( std::max( originalTexture.extent( ).x, originalTexture.extent( ).y ), originalTexture.extent( ).z );
    const uint32_t maxLod    = static_cast< uint32_t >( std::floor( std::log2( maxExtent ) ) ) + 1;

    gli::texture duplicate( originalTexture.target( ),
                            originalTexture.format( ),
                            originalTexture.extent( ),
                            originalTexture.layers( ),
                            originalTexture.faces( ),
                            maxLod,
                            originalTexture.swizzles( ) );

    for ( gli::texture::size_type ll = 0; ll < duplicate.layers( ); ++ll )
        for ( gli::texture::size_type ff = 0; ff < duplicate.faces( ); ++ff )
            duplicate.copy( originalTexture, ll, ff, originalTexture.base_level( ), ll, ff, duplicate.base_level( ) );

    return duplicate;
}

gli::texture GenerateMipMaps( const gli::texture& texture ) {
    apemodevk_memory_allocation_scope;

    if ( texture.empty( ) ) {
        return gli::texture( );
    }

    switch ( texture.target( ) ) {
        case gli::TARGET_1D:
            return gli::generate_mipmaps( static_cast< const gli::texture1d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_1D_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture1d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_2D:
            return gli::generate_mipmaps( static_cast< const gli::texture2d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_2D_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture2d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_RECT:
            return gli::generate_mipmaps( static_cast< const gli::texture2d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_RECT_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture2d_array& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_3D:
            return gli::generate_mipmaps( static_cast< const gli::texture3d& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_CUBE:
            return gli::generate_mipmaps( static_cast< const gli::texture_cube& >( texture ), gli::FILTER_LINEAR );
        case gli::TARGET_CUBE_ARRAY:
            return gli::generate_mipmaps( static_cast< const gli::texture_cube_array& >( texture ), gli::FILTER_LINEAR );
    }

    assert( false );
    return gli::texture();
}

gli::texture LoadTexture( const uint8_t*                                           pFileContent,
                          size_t                                                   fileContentSize,
                          apemodevk::ImageDecoder::DecodeOptions::EImageFileFormat eFileFormat ) {
    apemodevk_memory_allocation_scope;

    gli::texture texture;
    switch ( eFileFormat ) {
        case apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_DDS:
        case apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_KTX: {
            assert( pFileContent && fileContentSize );
            texture = gli::load( (const char*) pFileContent, fileContentSize );
        } break;

        // case apemodevk::ImageLoader::eImageFileFormat_PNG:
        // case apemodevk::ImageLoader::eImageFileFormat_Undefined:
        default: {
            int imageWidth;
            int imageHeight;
            int componentsInFile;

            assert( pFileContent && fileContentSize );

            /* Load png file here from memory buffer */
            if ( stbi_uc* pImageBytes = stbi_load_from_memory( pFileContent, int( fileContentSize ), &imageWidth, &imageHeight, &componentsInFile, STBI_rgb_alpha ) ) {

                texture = gli::texture2d( gli::format::FORMAT_RGBA8_UNORM_PACK8,
                                          gli::extent2d( imageWidth, imageHeight ),
                                          1,
                                          gli::detail::get_format_info( gli::format::FORMAT_RGBA8_UNORM_PACK8 ).Swizzles );

                memcpy( texture.data( ), pImageBytes, imageWidth * imageHeight * 4 );
                stbi_image_free( pImageBytes );
            }
        } break;
    }

    return texture;
}

uint8_t* MapStagingBuffer( apemodevk::GraphicsDevice* pNode, apemodevk::THandle< apemodevk::BufferComposite >& hBuffer ) {
    using namespace apemodevk;

    if ( hBuffer.Handle.allocInfo.pMappedData ) {
        return reinterpret_cast< uint8_t * >( hBuffer.Handle.allocInfo.pMappedData );
    }

    uint8_t * pMapped = nullptr;
    if ( VK_SUCCESS == CheckedResult( vmaMapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation, (void **) ( &pMapped ) ) ) ) {
        return pMapped;
    }

    return nullptr;
}

void UnmapStagingBuffer( apemodevk::GraphicsDevice*                        pNode,
                         apemodevk::THandle< apemodevk::BufferComposite >& hBuffer ) {
    if ( !hBuffer.Handle.allocInfo.pMappedData ) {
        vmaUnmapMemory( pNode->hAllocator, hBuffer.Handle.pAllocation );
    }
}

apemodevk::unique_ptr< apemodevk::LoadedImage > LoadImageFromGLITexture(
    apemodevk::GraphicsDevice*                 pNode,
    const apemodevk::ISourceImage&             srcImg,
    const apemodevk::ImageLoader::LoadOptions& loadOptions ) {
    using namespace apemodevk;

    apemodevk_memory_allocation_scope;

    auto loadedImage = apemodevk::make_unique< LoadedImage >( );
    if ( !loadedImage ) {
        return nullptr;
    }

    InitializeStruct( loadedImage->ImageCreateInfo );
    InitializeStruct( loadedImage->ImgViewCreateInfo );

    loadedImage->ImageCreateInfo.format        = srcImg.GetFormat( );
    loadedImage->ImageCreateInfo.imageType     = srcImg.GetImageType( );
    loadedImage->ImageCreateInfo.extent        = srcImg.GetExtent( 0 );
    loadedImage->ImageCreateInfo.mipLevels     = srcImg.GetMipLevels( );
    loadedImage->ImageCreateInfo.arrayLayers   = srcImg.GetFaces( );
    loadedImage->ImageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    loadedImage->ImageCreateInfo.tiling        = loadOptions.eImgTiling;
    loadedImage->ImageCreateInfo.usage         = loadOptions.eImgUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    loadedImage->ImageCreateInfo.sharingMode   = loadOptions.eImgSharingMode;
    loadedImage->ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    loadedImage->ImgViewCreateInfo.flags                           = 0;
    loadedImage->ImgViewCreateInfo.format                          = srcImg.GetFormat( );
    loadedImage->ImgViewCreateInfo.viewType                        = srcImg.GetImageViewType( );
    loadedImage->ImgViewCreateInfo.subresourceRange.aspectMask     = loadOptions.eImgAspect;
    loadedImage->ImgViewCreateInfo.subresourceRange.levelCount     = srcImg.GetMipLevels( );
    loadedImage->ImgViewCreateInfo.subresourceRange.layerCount     = srcImg.GetFaces( );
    loadedImage->ImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    loadedImage->ImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;

    switch ( loadedImage->ImgViewCreateInfo.viewType ) {
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: {
            loadedImage->ImageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        } break;
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY: {
            loadedImage->ImageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        } break;

        default:
            break;
    }

    VmaAllocationCreateInfo imgAllocationCreateInfo;
    InitializeStruct( imgAllocationCreateInfo );
    imgAllocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    imgAllocationCreateInfo.flags = 0;

    if ( false == loadedImage->hImg.Recreate( pNode->hAllocator, loadedImage->ImageCreateInfo, imgAllocationCreateInfo ) ) {
        return nullptr;
    }

    if ( loadOptions.bImgView ) {
        loadedImage->ImgViewCreateInfo.image = loadedImage->hImg.Handle.pImg;
        if ( false == loadedImage->hImgView.Recreate( *pNode, loadedImage->ImgViewCreateInfo ) ) {
            return nullptr;
        }
    }

    size_t const maxFaceSize       = srcImg.GetSize( 0 );
    size_t const entireTextureSize = srcImg.GetSize( );
    size_t const stagingMemorySize = std::max( maxFaceSize, std::min( loadOptions.StagingMemoryLimitHint, entireTextureSize ) );

    THandle< BufferComposite > hStagingBuffer;

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.size  = stagingMemorySize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocationCreateInfo;
    InitializeStruct( stagingAllocationCreateInfo );
    stagingAllocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if ( false == hStagingBuffer.Recreate( pNode->hAllocator, bufferCreateInfo, stagingAllocationCreateInfo ) ) {
        return nullptr;
    }

    uint8_t* pMappedStagingMemory = MapStagingBuffer( pNode, hStagingBuffer );
    if ( nullptr == pMappedStagingMemory ) {
        return nullptr;
    }

    const OneTimeCmdBufferSubmitResult imgWriteBarrierResult =
    apemodevk::TOneTimeCmdBufferSubmit( pNode, loadOptions.QueueFamilyId, true, [&]( VkCommandBuffer pCmdBuffer ) {
        VkImageMemoryBarrier writeImageMemoryBarrier;
        InitializeStruct( writeImageMemoryBarrier );

        writeImageMemoryBarrier.image                       = loadedImage->hImg.Handle.pImg;
        writeImageMemoryBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        writeImageMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        writeImageMemoryBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        writeImageMemoryBarrier.subresourceRange.aspectMask = loadOptions.eImgAspect;
        writeImageMemoryBarrier.subresourceRange.levelCount = srcImg.GetMipLevels( );
        writeImageMemoryBarrier.subresourceRange.layerCount = srcImg.GetFaces( );
        writeImageMemoryBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        writeImageMemoryBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;

        vkCmdPipelineBarrier( pCmdBuffer,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              &writeImageMemoryBarrier );
        return true;
    } );

    if ( VK_SUCCESS != imgWriteBarrierResult.eResult ) {
        return nullptr;
    }

    uint32_t face     = 0;
    uint32_t mipLevel = 0;

    while ( mipLevel < srcImg.GetMipLevels( ) ) {
        std::vector< VkBufferImageCopy > bufferImageCopies;

        uint8_t* pMappedStagingMemoryHead = pMappedStagingMemory;
        size_t   stagingMemorySpaceLeft   = stagingMemorySize;

        for ( ; mipLevel < srcImg.GetMipLevels(); ++mipLevel ) {
            for ( ; face < srcImg.GetFaces( ); ++face ) {

                size_t const faceLevelDataSize = srcImg.GetSize( mipLevel );
                if ( stagingMemorySpaceLeft < faceLevelDataSize ) {
                    stagingMemorySpaceLeft = 0;
                    break;
                }

                const void* pFaceLevelData = srcImg.GetData( face, mipLevel );
                memcpy( pMappedStagingMemoryHead, pFaceLevelData, faceLevelDataSize );

                VkDeviceSize bufferOffset = static_cast< VkDeviceSize >( pMappedStagingMemoryHead - pMappedStagingMemory );
                pMappedStagingMemoryHead += faceLevelDataSize;
                stagingMemorySpaceLeft -= faceLevelDataSize;

                VkBufferImageCopy bufferImageCopy;
                InitializeStruct( bufferImageCopy );

                bufferImageCopy.imageSubresource.aspectMask     = loadOptions.eImgAspect;
                bufferImageCopy.imageSubresource.layerCount     = 1;
                bufferImageCopy.imageSubresource.baseArrayLayer = face;
                bufferImageCopy.imageSubresource.mipLevel       = mipLevel;
                bufferImageCopy.imageExtent                     = srcImg.GetExtent( mipLevel );
                bufferImageCopy.bufferOffset                    = bufferOffset;
                bufferImageCopy.bufferImageHeight               = 0; /* Tightly packed according to the imageExtent */
                bufferImageCopy.bufferRowLength                 = 0; /* Tightly packed according to the imageExtent */

                bufferImageCopies.emplace_back( bufferImageCopy );
            }

            if ( !stagingMemorySpaceLeft ) {
                break;
            }

            face = 0;
        }

        if ( !bufferImageCopies.empty( ) ) {

            const OneTimeCmdBufferSubmitResult imgCopyResult =
            apemodevk::TOneTimeCmdBufferSubmit( pNode, loadOptions.QueueFamilyId, true, [&]( VkCommandBuffer pCmdBuffer ) {
                vkCmdCopyBufferToImage( pCmdBuffer,
                                        hStagingBuffer.Handle.pBuffer,
                                        loadedImage->hImg.Handle.pImg,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        static_cast< uint32_t >( bufferImageCopies.size( ) ),
                                        bufferImageCopies.data( ) );
                return true;
            } );

            if ( VK_SUCCESS != imgCopyResult.eResult ) {
                return nullptr;
            }
        }
    }

    UnmapStagingBuffer( pNode, hStagingBuffer );

    const OneTimeCmdBufferSubmitResult imgReadBarrierResult =
    apemodevk::TOneTimeCmdBufferSubmit( pNode, loadOptions.QueueFamilyId, true, [&]( VkCommandBuffer pCmdBuffer ) {

        VkImageMemoryBarrier readImgMemoryBarrier;
        InitializeStruct( readImgMemoryBarrier );

        readImgMemoryBarrier.image                       = loadedImage->hImg.Handle.pImg;
        readImgMemoryBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        readImgMemoryBarrier.dstAccessMask               = loadOptions.eImgDstAccess;
        readImgMemoryBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        readImgMemoryBarrier.newLayout                   = loadOptions.eImgDstLayout;
        readImgMemoryBarrier.subresourceRange.aspectMask = loadOptions.eImgAspect;
        readImgMemoryBarrier.subresourceRange.levelCount = srcImg.GetMipLevels( );
        readImgMemoryBarrier.subresourceRange.layerCount = srcImg.GetFaces( );
        readImgMemoryBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        readImgMemoryBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;

        loadedImage->eImgLayout = readImgMemoryBarrier.newLayout;

        vkCmdPipelineBarrier( pCmdBuffer,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              loadOptions.eDstPipelineStage,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              &readImgMemoryBarrier );

        return true;
    } );

    if ( VK_SUCCESS != imgReadBarrierResult.eResult ) {
        return nullptr;
    }

    return std::move( loadedImage );
}

apemodevk::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromSrc( GraphicsDevice*     pNode,
                                                                                          const ISourceImage& srcImg,
                                                                                          LoadOptions const&  loadOptions ) {
    return LoadImageFromGLITexture( pNode, srcImg, loadOptions );
}

VkFormat ToImgFormat( const gli::format eTextureFormat ) {
    return static_cast< VkFormat >( eTextureFormat );
}

gli::format ToGLIFormat( VkFormat eImgFmt ) {
    return static_cast< gli::format >( eImgFmt );
}

VkImageType ToImgType( const gli::target eTextureTarget ) {
    switch ( eTextureTarget ) {
    case gli::TARGET_1D:            return VK_IMAGE_TYPE_1D;
    case gli::TARGET_1D_ARRAY:      return VK_IMAGE_TYPE_1D;
    case gli::TARGET_2D:            return VK_IMAGE_TYPE_2D;
    case gli::TARGET_2D_ARRAY:      return VK_IMAGE_TYPE_2D;
    case gli::TARGET_3D:            return VK_IMAGE_TYPE_3D;
    case gli::TARGET_RECT:          return VK_IMAGE_TYPE_2D;
    case gli::TARGET_RECT_ARRAY:    return VK_IMAGE_TYPE_2D;
    case gli::TARGET_CUBE:          return VK_IMAGE_TYPE_2D;
    case gli::TARGET_CUBE_ARRAY:    return VK_IMAGE_TYPE_2D;
    default:                        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

VkImageViewType ToImgViewType( const gli::target eTextureTarget ) {
    switch ( eTextureTarget ) {
    case gli::TARGET_1D:            return VK_IMAGE_VIEW_TYPE_1D;
    case gli::TARGET_1D_ARRAY:      return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case gli::TARGET_2D:            return VK_IMAGE_VIEW_TYPE_2D;
    case gli::TARGET_2D_ARRAY:      return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case gli::TARGET_3D:            return VK_IMAGE_VIEW_TYPE_3D;
    case gli::TARGET_RECT:          return VK_IMAGE_VIEW_TYPE_2D;
    case gli::TARGET_RECT_ARRAY:    return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case gli::TARGET_CUBE:          return VK_IMAGE_VIEW_TYPE_CUBE;
    case gli::TARGET_CUBE_ARRAY:    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:                        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

apemodevk::SourceImage::SourceImage( gli::texture texture ) : Texture( std::move( texture ) ) {
}

VkImageViewType apemodevk::SourceImage::GetImageViewType( ) const {
    return ToImgViewType( Texture.target( ) );
}

VkImageType apemodevk::SourceImage::GetImageType( ) const {
    return ToImgType( Texture.target( ) );
}

VkFormat apemodevk::SourceImage::GetFormat( ) const {
    return ToImgFormat( Texture.format( ) );
}

VkDeviceSize apemodevk::SourceImage::GetSize( uint32_t level ) const {
    return Texture.size( level );
}

VkDeviceSize apemodevk::SourceImage::GetSize( ) const {
    return Texture.size( );
}

const void* apemodevk::SourceImage::GetData( uint32_t face, uint32_t level ) const {
    return Texture.data( 0, face, level );
}

VkExtent3D apemodevk::SourceImage::GetExtent( uint32_t level ) const {
    const auto extent = Texture.extent( level );

    VkExtent3D extent3D;
    extent3D.width  = static_cast< uint32_t >( extent.x );
    extent3D.height = static_cast< uint32_t >( extent.y );
    extent3D.depth  = static_cast< uint32_t >( extent.z );
    return extent3D;
}

uint32_t apemodevk::SourceImage::GetMipLevels( ) const {
    return static_cast< uint32_t >( Texture.levels( ) );
}

uint32_t apemodevk::SourceImage::GetFaces( ) const {
    return static_cast< uint32_t >( Texture.faces( ) );
}

apemodevk::unique_ptr< apemodevk::ISourceImage > apemodevk::ImageDecoder::CreateSourceImage2D(
    const uint8_t* pImageBytes, VkExtent2D imageExtent, VkFormat eImgFmt, const DecodeOptions& decodeOptions ) {
    apemodevk_memory_allocation_scope;

    if ( !pImageBytes || !imageExtent.width || !imageExtent.height ) {
        return nullptr;
    }

    gli::format             gliFmt     = ToGLIFormat( eImgFmt );
    gli::detail::formatInfo gliFmtInfo = gli::detail::get_format_info( gliFmt );
    gli::extent2d           gliExtent  = gli::extent2d( imageExtent.width, imageExtent.height );
    gli::texture            texture    = gli::texture2d( gliFmt, gliExtent, 1, gliFmtInfo.Swizzles );

    memcpy( texture.data( ), pImageBytes, imageExtent.width * imageExtent.height * 4 );

    /* Check if the user needs mipmaps.
     * Note, that DDS and KTX files can contain mipmaps. */
    if ( !texture.empty( ) && decodeOptions.bGenerateMipMaps && ( 1 == texture.levels( ) ) ) {
        /* Cannot generate mipmaps the data is compressed. */
        if ( !gli::is_compressed( texture.format( ) ) ) {
            const gli::texture dumplicateWithMipMaps = DuplicateWithMipMaps( texture );
            texture = GenerateMipMaps( dumplicateWithMipMaps );
        }
    }

    if ( texture.empty( ) ) {
        return nullptr;
    }

    auto sourceImage = make_unique< SourceImage >( std::move( texture ) );
    return unique_ptr< ISourceImage >( sourceImage.release( ) );
}

apemodevk::unique_ptr< apemodevk::ISourceImage > apemodevk::ImageDecoder::DecodeSourceImageFromData(
    const uint8_t* pFileContent, size_t fileContentSize, const DecodeOptions& decodeOptions ) {
    apemodevk_memory_allocation_scope;

    if ( !pFileContent || !fileContentSize ) {
        return nullptr;
    }

    gli::texture texture = LoadTexture( pFileContent, fileContentSize, decodeOptions.eFileFormat );

    /* Check if the user needs mipmaps.
     * Note, that DDS and KTX files can contain mipmaps. */
    if ( !texture.empty( ) && decodeOptions.bGenerateMipMaps && ( 1 == texture.levels( ) ) ) {
        /* Cannot generate mipmaps the data is compressed. */
        if ( !gli::is_compressed( texture.format( ) ) ) {
            const gli::texture dumplicateWithMipMaps = DuplicateWithMipMaps( texture );
            texture = GenerateMipMaps( dumplicateWithMipMaps );
        }
    }

    if ( texture.empty( ) ) {
        return nullptr;
    }

    auto sourceImage = make_unique< SourceImage >( std::move( texture ) );
    return unique_ptr< ISourceImage >( sourceImage.release( ) );
}

apemodevk::LoadedImage::~LoadedImage( ) {
    Destroy( );
}

void apemodevk::LoadedImage::Destroy( ) {
    apemodevk_memory_allocation_scope;

    hImgView.Destroy( );
    hImg.Destroy( );

    eImgLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}